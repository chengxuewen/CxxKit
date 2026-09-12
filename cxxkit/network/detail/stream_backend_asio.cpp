/***
Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#include <cxxkit/network/detail/stream_backend_asio.hpp>

#include <cxxkit/network/detail/address_helper.hpp>
#include <cxxkit/network/detail/stream_backend.hpp>

#include <cxxkit/3rdparty/asio/asio.hpp>

#include <cxxkit/tools/checks.hpp>

#include <atomic>
#include <memory>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/// The per-loop io_context slot (Chromium SupportsUserData shape): EVERY backend bound to the
/// same EventLoop shares ONE io_context, so all endpoints of a loop world are ordered against a
/// single engine (uv parity: one uv_loop per EventLoop — cross-endpoint callback ordering, e.g.
/// accept-before-connect-done, is deterministic). The loop owns the slot (Object::set_user_data);
/// backends only reference it. Key = this TU-local address. See Native (below) for the
/// pump-closure liveness bridge.
struct LoopIo : cxxkit::Object::UserData
{
    asio::io_context io; // backends hold a raw reference; the loop outlives them (pump_until_closed
}; // closes every endpoint before the loop's user-data teardown matters)

const char k_loop_io_key = 0;

/// Backend-owned asio cell: references into the loop's io slot plus the optional endpoint
/// objects. A separate heap struct keeps the header asio-free (D8). mAlive is the pump-closure
/// liveness bridge: every posted pump closure holds a shared_ptr copy and checks the flag before
/// touching the backend — backend destruction flips it false, so no tick touches freed state and
/// the re-post cycle ends by itself (EMBED ruling: clean loop exit, no infinite cycle).
struct AsioStreamBackend::Native
{
    asio::io_context *io{nullptr};                     /// the loop's shared io slot (owned by the EventLoop)
    std::unique_ptr<asio::ip::tcp::socket> socket;     // client face (also adopt_native/adopt_fd)
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor; // server face
    std::shared_ptr<std::atomic<bool>> alive;          // pump closures' liveness bridge
    bool pumping{false};                               /// re-entrancy guard: asio callbacks can re-enter pump paths
    bool pending_tick{false};                          /// an immediate tick is in the loop's post queue
    int timer_id{-1};                                  /// armed repeating cadence timer (0 = none armed)

    Native()
        : alive(new std::atomic<bool>(true))
    {
    }
};

/// Fetches (creating once) the loop's shared io_context slot.
static LoopIo *loop_io_slot(EventLoop &loop)
{
    LoopIo *slot = static_cast<LoopIo *>(loop.user_data(&k_loop_io_key));
    if (slot == nullptr)
    {
        slot = new LoopIo;
        loop.set_user_data(&k_loop_io_key, std::unique_ptr<cxxkit::Object::UserData>(slot));
    }
    return slot;
}

AsioStreamBackend::AsioStreamBackend()
    : mN(nullptr)
{
    // Native is created in open(): the pump is bound to a loop we do not have before open().
    // adopt_native/adopt_fd route through open() as well.
}

AsioStreamBackend::~AsioStreamBackend()
{
    // I6: nothing should reach here with live endpoint objects — pump_until_closed ran at the
    // pimpl's dtor (or the listen-failure teardown drained them synchronously).
    CXXKIT_CHECK(mN == nullptr || (!mN->socket && !mN->acceptor))
        << "AsioStreamBackend: endpoint objects outlived the drain pump (teardown bug)";
    if (mN)
    {
        // Kill the pump cycle FIRST: any queued tick closure observes alive == false and no-ops;
        // the cadence timer stops here (its own body would also stop it, but timer_id lives in
        // the cell being destroyed).
        mN->alive->store(false);
        this->stop_cadence_timer();
        mN.reset();
    }
}

bool AsioStreamBackend::open(EventLoop &loop)
{
    mLoop = &loop;
    if (!mN)
    {
        mN.reset(new Native);
    }
    mN->io = &loop_io_slot(loop)->io; // all endpoints of this loop pump on ONE engine
    // NO tick posted here: an adopted/never-armed backend must not carry posted work, or every
    // EventLoop::process_events(flags, ms) quiet window returns true instantly on the pending
    // task (uv parity: uv_tcp_open posts nothing; the pump arms with the FIRST async op).
    return true;
}

void AsioStreamBackend::ensure_pump()
{
    CXXKIT_CHECK(mN != nullptr) << "AsioStreamBackend::ensure_pump: native cell missing";
    if (mN->pending_tick)
    {
        return; // a tick is already queued: post() coalescing by flag, no duplicate rounds
    }
    mN->pending_tick = true;
    // The weak_ptr is the liveness bridge: a tick sitting in the post queue at
    // backend-destruction time sees a dead flag and stops its timer without touching freed
    // state — the cycle ends cleanly (EMBED ruling).
    std::weak_ptr<std::atomic<bool>> alive_weak = mN->alive;
    mLoop->post(
        [this, alive_weak]()
        {
            std::shared_ptr<std::atomic<bool>> alive = alive_weak.lock();
            if (!alive || !alive->load())
            {
                // Backend destroyed: cycle ends here. Touch NOTHING through this — mLoop/mN are
                // members of the freed backend; the repeating cadence timer's own body sees the
                // dead flag and stops itself (pump_tick's dead branch). ASAN-proven UAF if we
                // dereference here (destroy-after-close flows).
                return;
            }
            mN->pending_tick = false;
            this->pump_tick();
        });
}

void AsioStreamBackend::pump_tick()
{
    if (!mN || mN->pumping)
    {
        return; // backend destroyed mid-cycle or re-entrant poll from an asio callback
    }
    // Poll UNCONDITIONALLY (has_work checked only for the cadence decision below): a just-closed
    // endpoint still has cancelled completion handlers queued (CloseTwiceIdempotent contract —
    // every write gets its false); skipping poll on empty has_work would strand deliveries.
    mN->pumping = true;
    const size_t handlers_ran = mN->io->poll(); // run every ready handler; never blocks
    mN->pumping = false;

    // Timer-gated cadence (T4 fix wave — the naive while-has_work re-post spun an idle loop at
    // 100% CPU and made every quiet process_events(ms) window return instantly):
    // - real asio activity -> immediate re-post (burst latency stays sub-tick);
    // - quiet but endpoints alive -> 1ms repeating timer carries the cadence
    //   (TaskQueueThread precedent, C14/PIT-26);
    // - idle -> drain-to-quiet (in-round posted completions, e.g. begin_close's on_closed), stop
    //   the timer, let the cycle die.
    if (!this->has_work())
    {
        for (int rounds = 0; rounds < 1000 && mN->io->poll() > 0; ++rounds)
        {
            // drain completions posted during the last round (close-callback delivery guarantee)
        }
        this->stop_cadence_timer();
        return;
    }
    if (handlers_ran > 0)
    {
        this->ensure_pump(); // burst: next round immediately
        return;
    }
    if (mN->timer_id <= 0)
    {
        // Quiet window: arm the 1ms cadence (repeat stays on the engine; the timer body checks
        // the shared-alive bridge and re-runs pump_tick, so the cadence self-terminates when the
        // backend goes idle or dies).
        std::weak_ptr<std::atomic<bool>> alive_weak = mN->alive;
        mN->timer_id = mLoop->start_timer(
            1,
            [this, alive_weak]()
            {
                std::shared_ptr<std::atomic<bool>> alive = alive_weak.lock();
                if (!alive || !alive->load())
                {
                    this->stop_cadence_timer(); // backend died with the timer armed: stop firing
                    return;
                }
                this->pump_tick();
            },
            true);
    }
}

void AsioStreamBackend::stop_cadence_timer()
{
    if (mN != nullptr && mN->timer_id > 0)
    {
        mLoop->stop_timer(mN->timer_id);
        mN->timer_id = -1;
    }
}

bool AsioStreamBackend::has_work() const
{
    return mN != nullptr && (mN->socket != nullptr || mN->acceptor != nullptr);
}

void AsioStreamBackend::connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioStreamBackend::connect: open() was not called";
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: connect failure, not a programming error — backend reports plain
        // failure with an inert status (maps to kUnknown), preserving T2's public error exactly.
        mNativeStatus = EINVAL;
        if (on_done)
        {
            on_done(false);
        }
        return;
    }

    mN->socket.reset(new asio::ip::tcp::socket(*mN->io));
    // Build the endpoint from the filled sockaddr (single fill_sockaddr parse path): asio has no
    // public sockaddr ctor, so memcpy into a fresh endpoint's storage and resize to the family's
    // actual length.
    const sockaddr &sa = reinterpret_cast<const sockaddr &>(addr);
    const asio::ip::tcp protocol = (sa.sa_family == AF_INET6) ? asio::ip::tcp::v6() : asio::ip::tcp::v4();
    asio::ip::tcp::endpoint endpoint(protocol, 0);
    const socklen_t sa_len = (sa.sa_family == AF_INET6) ? static_cast<socklen_t>(sizeof(sockaddr_in6))
                                                        : static_cast<socklen_t>(sizeof(sockaddr_in));
    std::memcpy(endpoint.data(), &addr, sa_len);
    endpoint.resize(sa_len);
    this->ensure_pump();

    mOnConnect = std::move(on_done);
    // PIT-40 local-copy discipline: the handler may close()/destroy mid-flight — deliver through
    // a moved-out local. The backend outlives every handler (pump_until_closed drains before the
    // dtor completes; the shared-alive bridge stops pump ticks at destruction).
    mN->socket->async_connect(endpoint,
                              [this](const std::error_code &ec)
                              {
                                  mNativeStatus = ec ? -static_cast<int>(ec.value()) : 0; // errno-style (uv parity)
                                  const bool ok = !ec && !mCloseRequested;
                                  std::function<void(bool ok)> cb = std::move(mOnConnect);
                                  mOnConnect = nullptr;
                                  if (cb)
                                  {
                                      cb(ok);
                                  }
                                  if (!ok)
                                  {
                                      // Failed connect: the socket STAYS alive here (uv parity — uv_tcp_t survives until
                                      // the pimpl's begin_close tears it down); only a close-cancelled connect leaves the
                                      // release to begin_close (no-op there — already null). Retryability comes from the
                                      // failed-connect state contract, not from a handle-less backend.
                                      if (!mCloseRequested)
                                      {
                                          mN->socket.reset();
                                      }
                                  }
                              });
}

void AsioStreamBackend::write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioStreamBackend::write: open() was not called";
    CXXKIT_CHECK(mN->socket != nullptr) << "AsioStreamBackend::write: no socket (never connected?)";
    PendingWrite pending;
    pending.mData.assign(data, data + len);
    pending.mOnWritten = std::move(on_done);
    mPendingWrites.push_back(std::move(pending));
    this->submit_next_write(); // one async_write in flight (lws parity — see header note)
    this->ensure_pump();       // new work: prefer an immediate tick over the 1ms timer cadence
}

void AsioStreamBackend::submit_next_write()
{
    if (mInFlight.size() > 0 || mPendingWrites.empty())
    {
        return; // one write in flight, or nothing to send
    }
    PendingWrite pending = std::move(mPendingWrites.front());
    mPendingWrites.pop_front();

    // Storage must outlive the operation: park the bytes in mInFlight and point asio's buffer at it.
    mInFlight = std::move(pending.mData);
    std::function<void(bool ok)> on_written = std::move(pending.mOnWritten); // C11: no init-capture
    mN->socket->async_write_some(asio::buffer(mInFlight),
                                 [this, on_written](const std::error_code &ec, size_t /*bytes*/) mutable
                                 {
                                     mNativeStatus = ec ? -static_cast<int>(ec.value()) : 0; // errno-style (uv parity)
                                     mInFlight.clear(); // the completion is the only trigger for the next write (lws)
                                     if (on_written)
                                     {
                                         on_written(!ec);
                                     }
                                     if (!ec && !mCloseRequested)
                                     {
                                         this->submit_next_write();
                                     }
                                     // On error the pimpl tears the session down (mirrors uv); queued writes false-fan-out
                                     // in begin_close.
                                 });
}

void AsioStreamBackend::read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioStreamBackend::read_start: open() was not called";
    CXXKIT_CHECK(mN->socket != nullptr) << "AsioStreamBackend::read_start: no socket";
    if (mOnData)
    {
        mOnData = std::move(on_data); // re-arm replaces the callback (interest update, F7-① shape)
        return;
    }
    mOnData = std::move(on_data);
    this->arm_read();
    this->ensure_pump(); // new work: prefer an immediate tick over the 1ms timer cadence
}

void AsioStreamBackend::arm_read()
{
    if (mReadBuf.size() < 4096)
    {
        mReadBuf.resize(4096); // beast flat_buffer shape: one contiguous reusable block
    }
    mN->socket->async_read_some(
        asio::buffer(mReadBuf),
        [this](const std::error_code &ec, size_t bytes)
        {
            if (ec == asio::error::operation_aborted)
            {
                return; // close()/read_stop() cancel: no delivery, the close lifecycle owns the epilogue
            }
            if (ec)
            {
                // EOF or error: deliver the (nullptr, code) terminal event once, then go through
                // the close path. EOF normalizes onto the backend-neutral sentinel (uv parity:
                // UV_EOF -> kBackendEof; asio::error::eof -> the same sentinel).
                mNativeStatus = -static_cast<int>(ec.value()); // errno-style (uv parity)
                const ssize_t code = (ec == asio::error::eof) ? kBackendEof : static_cast<ssize_t>(mNativeStatus);
                std::function<void(const uint8_t *data, ssize_t nread)> cb = std::move(mOnData);
                mOnData = nullptr;
                if (cb)
                {
                    cb(nullptr, code);
                }
                this->begin_close();
                return;
            }
            if (bytes == 0)
            {
                this->arm_read(); // no data this round: re-arm (uv EAGAIN shape), interest stays armed
                return;
            }
            // F8-① copy discipline: the callback may close()/read_stop() mid-execution, which
            // would destroy the member std::function we are inside — invoke through a local copy.
            if (mOnData)
            {
                std::function<void(const uint8_t *data, ssize_t nread)> cb = mOnData;
                cb(mReadBuf.data(), static_cast<ssize_t>(bytes));
            }
            // Level-triggered: keep reading (uv_read_start parity) — unless the callback just
            // closed/stopped us: then the socket is gone and re-arming would null-deref.
            if (!mCloseRequested && mOnData && mN && mN->socket)
            {
                this->arm_read();
            }
        });
}

void AsioStreamBackend::read_stop()
{
    if (!mOnData)
    {
        return; // never armed / already stopped: no-op
    }
    mOnData = nullptr;
    // Disarm the pending async_read_some: cancel pushes operation_aborted into its handler,
    // which delivers nothing (the aborted branch returns before any mOnData touch).
    if (mN && mN->socket)
    {
        std::error_code ec;
        mN->socket->cancel(ec); // error ignored: a raced close invalidates the cancel — fine
    }
}

void AsioStreamBackend::close(std::function<void()> on_closed)
{
    mOnClosed = std::move(on_closed);
    this->begin_close();
}

bool AsioStreamBackend::is_open() const
{
    return mN != nullptr && !mCloseRequested && (mN->socket != nullptr || mN->acceptor != nullptr);
}

void AsioStreamBackend::begin_close()
{
    // F8-② idempotence: a second close must not double-fire mOnClosed or re-tear the endpoints.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;

    // Pending writes false-fan-out now (swap-drain shape, uv parity): an on_written(false) that
    // re-enters write() pushes into a live deque we already swapped out — safe. A cancelled
    // in-flight write delivers through its own handler with ec set (false) — exactly once total.
    std::deque<PendingWrite> discards;
    mPendingWrites.swap(discards);
    while (!discards.empty())
    {
        PendingWrite pending = std::move(discards.front());
        discards.pop_front();
        if (pending.mOnWritten)
        {
            pending.mOnWritten(false);
        }
    }
    // Pending connect completes false as well (discard semantics, symmetric with writes): the
    // cancel below makes its handler run with operation_aborted, whose ok gate delivers false
    // during the next pump drain — exactly once.
    if (mN && mN->socket)
    {
        std::error_code ec;
        mN->socket->cancel(ec); // error ignored: teardown path
    }

    if (mN == nullptr || (mN->socket == nullptr && mN->acceptor == nullptr))
    {
        // Never initialized (kIdle shape): nothing to close at the asio level — synchronous done.
        if (mOnClosed)
        {
            std::function<void()> cb = mOnClosed;
            mOnClosed = nullptr;
            cb();
        }
        return;
    }

    mOnData = nullptr;
    // Release the endpoint objects: their destructors close the descriptors synchronously. The
    // cancelled ops' handlers (already queued) only touch backend members — never the destroyed
    // objects (aborted/closed gates early-return). mNativeStatus keeps the last completed op's
    // status for the pimpl's error surface.
    mN->socket.reset();
    mN->acceptor.reset();
    if (mOnClosed)
    {
        // ASYNC completion (uv uv_close parity): the close callback lands on a LATER pump tick,
        // never synchronously inside the caller's frame — the pimpl's state machine depends on
        // that interleaving (failed-connect reset to kIdle happens before the close callback).
        // PIT-40 local copy: the callback may destroy us mid-flight.
        std::function<void()> cb = std::move(mOnClosed);
        mOnClosed = nullptr;
        asio::post(*mN->io, [cb]() { cb(); });
    }
}

bool AsioStreamBackend::listen(const std::string &ip, uint16_t port, int backlog)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioStreamBackend::listen: open() was not called";
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        return false; // invalid address: a listen failure, not a programming error
    }

    const sockaddr &sa = reinterpret_cast<const sockaddr &>(addr);
    const asio::ip::tcp protocol = (sa.sa_family == AF_INET6) ? asio::ip::tcp::v6() : asio::ip::tcp::v4();
    asio::ip::tcp::endpoint endpoint(protocol, 0);
    const socklen_t sa_len = (sa.sa_family == AF_INET6) ? static_cast<socklen_t>(sizeof(sockaddr_in6))
                                                        : static_cast<socklen_t>(sizeof(sockaddr_in));
    std::memcpy(endpoint.data(), &addr, sa_len);
    endpoint.resize(sa_len);

    mN->acceptor.reset(new asio::ip::tcp::acceptor(*mN->io));
    std::error_code ec;
    mN->acceptor->open(protocol, ec);
    if (!ec)
    {
        // uv parity: reuse-address by default (uv_tcp_bind sets SO_REUSEADDR).
        mN->acceptor->set_option(asio::socket_base::reuse_address(true), ec);
    }
    if (!ec)
    {
        mN->acceptor->bind(endpoint, ec);
    }
    if (!ec)
    {
        mN->acceptor->listen(backlog > 0 ? backlog : asio::socket_base::max_listen_connections, ec);
    }
    if (ec)
    {
        // Listen failure: reset to the retryable unlistened state (T2-verbatim no-latch: the
        // failed listen is NOT a close lifecycle). The acceptor never registered an async op —
        // synchronous destruction is the clean teardown, no pump needed (uv needed one because
        // uv_tcp_init registers the handle with the loop; asio's acceptor ctor does not).
        mN->acceptor.reset();
        mNativeStatus = -static_cast<int>(ec.value()); // errno-style (uv parity)
        return false;
    }

    this->ensure_pump();
    this->arm_accept();
    mBoundPort = port;
    // R-T3-2: port 0 = OS-assigned; read the real port back — race-free discovery.
    if (port == 0)
    {
        const asio::ip::tcp::endpoint local = mN->acceptor->local_endpoint(ec);
        if (!ec)
        {
            mBoundPort = local.port();
        }
    }
    return true;
}

void AsioStreamBackend::arm_accept()
{
    // Fresh socket per accept, constructed against the shared io_context (accept-loop shape).
    std::unique_ptr<asio::ip::tcp::socket> client(new asio::ip::tcp::socket(*mN->io));
    asio::ip::tcp::socket &client_ref = *client;
    mN->acceptor->async_accept(client_ref,
                               [this, client = std::move(client)](const std::error_code &ec) mutable
                               {
                                   if (ec == asio::error::operation_aborted || mCloseRequested)
                                   {
                                       return; // teardown: the accept loop dies with the acceptor
                                   }
                                   if (!ec)
                                   {
                                       if (mOnAccept)
                                       {
                                           // R-T3-1: the accepted handle is handed over WHOLE — its lifetime belongs to
                                           // the client backend from here on (adopt_native takes the socket pointer;
                                           // the io_context is the loop's shared slot — identical on both sides).
                                           std::unique_ptr<AsioStreamBackend> client_backend(new AsioStreamBackend);
                                           client_backend->adopt_native(client.get(), *mLoop);
                                           client.release(); // ownership moved into the client backend
                                           mOnAccept(std::unique_ptr<StreamBackend>(client_backend.release()));
                                       }
                                       // else: no consumer installed — drop the socket (accept-and-discard parity).
                                   }
                                   if (!mCloseRequested && mN->acceptor)
                                   {
                                       this->arm_accept(); // continue the accept loop (level-triggered parity)
                                   }
                               });
}

uint16_t AsioStreamBackend::bound_port() const
{
    return mBoundPort;
}

void AsioStreamBackend::set_on_accept(std::function<void(std::unique_ptr<StreamBackend> client)> on_accept)
{
    mOnAccept = std::move(on_accept);
}

bool AsioStreamBackend::adopt_native(void *native_handle, EventLoop &loop)
{
    CXXKIT_CHECK(native_handle != nullptr) << "AsioStreamBackend::adopt_native: null handle";
    this->open(loop);
    // The void* narrows to asio::ip::tcp::socket* here (the server accept path produced it) —
    // no asio types outside stream_backend_asio.{hpp,cpp} (D8). The socket was constructed
    // against the SERVER's io_context slot, which (per-loop slot) is the SAME context this
    // backend just bound via open() — same loop world, same engine (uv adopt checks its loop;
    // here the per-loop slot makes the check tautological-but-cheap, kept as a debugging aid).
    asio::ip::tcp::socket *taken = static_cast<asio::ip::tcp::socket *>(native_handle);
    CXXKIT_CHECK(&taken->get_executor().context() == static_cast<const void *>(mN->io))
        << "AsioStreamBackend::adopt_native: socket belongs to a different io_context";
    mN->socket.reset(taken);
    return true;
}

bool AsioStreamBackend::adopt_fd(int fd, EventLoop &loop)
{
    this->open(loop);
    // assign(protocol, fd): adopt an already-connected descriptor (uv_tcp_open shape). The fd's
    // peer family decides the protocol (v4 vs v6).
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(fd, reinterpret_cast<sockaddr *>(&addr), &addr_len) != 0)
    {
        CXXKIT_FATAL() << "AsioStreamBackend::adopt_fd: getpeername failed for fd " << fd;
    }
    const asio::ip::tcp protocol = (addr.ss_family == AF_INET6) ? asio::ip::tcp::v6() : asio::ip::tcp::v4();
    asio::ip::tcp::socket socket(*mN->io);
    std::error_code ec;
    socket.assign(protocol, fd, ec);
    if (ec)
    {
        CXXKIT_FATAL() << "AsioStreamBackend::adopt_fd: socket.assign failed (" << ec.message() << ") for fd " << fd;
    }
    mN->socket.reset(new asio::ip::tcp::socket(std::move(socket)));
    return true;
}

void *AsioStreamBackend::native_handle() const
{
    return (mN && mN->socket) ? static_cast<void *>(mN->socket.get()) : nullptr;
}

void AsioStreamBackend::pump_until_closed()
{
    // ~TcpSocket / ~TcpServer / listen-failure teardown drain discipline (I6): close (idempotent
    // — a no-op if already requested), then pump NOWAIT rounds until no endpoint object survives
    // and no handler remains pending. asio teardown is synchronous (endpoint destructors close
    // the descriptors), so this typically drains in one round; the cap guards a pathological
    // no-progress case.
    if (mLoop == nullptr && (mN == nullptr || (!mN->socket && !mN->acceptor)))
    {
        return; // never opened (kIdle socket): no loop to pump, nothing to drain
    }
    if (this->has_work() && !mCloseRequested)
    {
        this->begin_close();
    }
    // The cadence timer must not outlive the drain: pump_until_closed is the backend's last act.
    this->stop_cadence_timer();
    // Drain the cancelled ops' completion handlers (connect false-delivery, write aborts) so no
    // asio handler survives us. mOnData is already null (begin_close); mOnConnect delivers its
    // false inside the drained connect handler.
    if (mN)
    {
        for (int rounds = 0; rounds < 1000 && mN->io->poll() > 0; ++rounds)
        {
            // drain
        }
        mN->socket.reset();
        mN->acceptor.reset();
        // The pump cycle ends by itself: has_work() is now false, so no tick re-posts; any tick
        // already sitting in the EventLoop's post queue no-ops against the shared-alive flag
        // once the backend dies (dtor flips it).
    }
}

std::unique_ptr<StreamBackend> make_stream_backend()
{
    return std::unique_ptr<StreamBackend>(new AsioStreamBackend);
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
