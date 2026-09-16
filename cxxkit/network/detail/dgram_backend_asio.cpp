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

#include <cxxkit/network/detail/dgram_backend_asio.hpp>

#include <cxxkit/network/detail/address_helper.hpp>
#include <cxxkit/network/detail/dgram_backend.hpp>

#include <cxxkit/3rdparty/asio/asio.hpp>

#include <cxxkit/tools/checks.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

namespace
{
/// Largest IP datagram any host must accept (IPv4 min-reassembly requirement); the uv backend
/// allocates the same value. One member buffer is sized to this once.
constexpr size_t kMaxDgramSize = 65536;
} // namespace

/// The per-loop io_context slot — the SAME shape stream_backend_asio.cpp uses (one engine per
/// EventLoop world; the slot lives on the loop via Object::set_user_data, key = this TU-local
/// address). Duplicating the 10-line struct beats cross-TU coupling: each backend family keeps
/// its own slot and they never share engine state.
struct LoopIo : cxxkit::Object::UserData
{
    asio::io_context io;
};

const char k_loop_io_key = 0;

/// Backend-owned asio cell: references into the loop's io slot plus the optional socket.
/// mAlive is the pump-closure liveness bridge (stream backend verbatim): every posted closure
/// holds a shared_ptr copy and checks the flag before touching the backend — destruction flips
/// it false, so no tick touches freed state and the re-post cycle ends by itself.
struct AsioDgramBackend::Native
{
    asio::io_context *io{nullptr};                 /// the loop's shared io slot (owned by the EventLoop)
    std::unique_ptr<asio::ip::udp::socket> socket; /// created in bind(), released in begin_close()
    std::shared_ptr<std::atomic<bool>> alive;      // pump closures' liveness bridge
    bool pumping{false};                           /// re-entrancy guard: asio callbacks can re-enter pump paths
    bool pending_tick{false};                      /// an immediate tick is in the loop's post queue
    int timer_id{-1};                              /// armed repeating cadence timer (-1 = none armed)
    asio::ip::udp::endpoint sender_scratch;        /// receiver's sender-endpoint scratch cell (asio
    bool connected{false};                         /// a default peer is pinned (connect())
    asio::ip::udp::endpoint peer;                  /// the pinned default peer
                                                   /// fills it during completion; must outlive the
                                                   /// op — a stack local would dangle, F1)

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

AsioDgramBackend::AsioDgramBackend()
    : mN(nullptr)
{
    // Native is created in open(): the pump is bound to a loop we do not have before open().
}

AsioDgramBackend::~AsioDgramBackend()
{
    // Same discipline as the stream backend: nothing should reach here with a live socket —
    // the pimpl's dtor pumps until the close lifecycle ran.
    CXXKIT_CHECK(mN == nullptr || !mN->socket) << "AsioDgramBackend: socket outlived the drain pump (teardown bug)";
    if (mN)
    {
        // Kill the pump cycle FIRST: any queued tick closure observes alive == false and no-ops;
        // the cadence timer stops here (timer_id lives in the cell being destroyed).
        mN->alive->store(false);
        this->stop_cadence_timer();
        mN.reset();
    }
}

bool AsioDgramBackend::open(EventLoop &loop)
{
    mLoop = &loop;
    if (!mN)
    {
        mN.reset(new Native);
    }
    mN->io = &loop_io_slot(loop)->io; // all endpoints of this loop pump on ONE engine
    // NO tick posted here: a never-bound backend must not carry posted work (stream backend
    // parity — the pump arms with the FIRST async op).
    return true;
}

bool AsioDgramBackend::bind(const std::string &ip, uint16_t port)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioDgramBackend::bind: open() was not called";
    // Double-bind guard (uv-side MEDIUM parity): a second socket over a live one would orphan
    // the first — latched via the cell pointer.
    CXXKIT_CHECK(mN->socket == nullptr) << "AsioDgramBackend::bind: already bound (double bind)";
    if (mCloseRequested)
    {
        return false; // closed backends do not re-bind (lifecycle latched, uv shape)
    }
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: bind failure, not a programming error — backend reports plain failure,
        // the pimpl layer maps it onto the public error surface (uv -UV_EINVAL parity).
        mNativeStatus = -EINVAL;
        return false;
    }

    // Build the endpoint from the filled sockaddr (single fill_sockaddr parse path): asio has no
    // public sockaddr ctor, so memcpy into a fresh endpoint's storage and resize to the family's
    // actual length (stream backend connect/listen verbatim).
    const sockaddr &sa = reinterpret_cast<const sockaddr &>(addr);
    const asio::ip::udp protocol = (sa.sa_family == AF_INET6) ? asio::ip::udp::v6() : asio::ip::udp::v4();
    asio::ip::udp::endpoint endpoint(protocol, 0);
    const socklen_t sa_len = (sa.sa_family == AF_INET6) ? static_cast<socklen_t>(sizeof(sockaddr_in6))
                                                        : static_cast<socklen_t>(sizeof(sockaddr_in));
    std::memcpy(endpoint.data(), &addr, sa_len);
    endpoint.resize(sa_len);

    std::error_code ec;
    mN->socket.reset(new asio::ip::udp::socket(*mN->io));
    // No SO_REUSEADDR here: uv_udp_bind does not set it (parity reference), and on Linux two
    // SO_REUSEADDR UDP sockets may BOTH bind the same port successfully — which would break the
    // public bind-conflict contract (false + kAddressInUse). open+bind in two steps so a bind
    // failure leaves a retryable unbound socket (stream listen shape — synchronous destruction
    // is the clean teardown: no async op was ever registered on the virgin socket).
    mN->socket->open(protocol, ec);
    if (!ec)
    {
        mN->socket->bind(endpoint, ec);
    }
    if (ec)
    {
        mN->socket.reset();                            // retryable unbound state (no latch — uv listen-failure parity)
        mNativeStatus = -static_cast<int>(ec.value()); // errno-style (uv parity)
        return false;
    }

    mBoundPort = port;
    // R-T3-2: port 0 = OS-assigned; read the real port back — race-free discovery.
    if (port == 0)
    {
        const asio::ip::udp::endpoint local = mN->socket->local_endpoint(ec);
        if (!ec)
        {
            mBoundPort = local.port();
        }
    }
    return true;
}

uint16_t AsioDgramBackend::bound_port() const
{
    return mBoundPort;
}

void AsioDgramBackend::send_to(const uint8_t *data,
                               size_t len,
                               const std::string &ip,
                               uint16_t port,
                               std::function<void(bool ok)> on_done)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioDgramBackend::send_to: open() was not called";
    if (mCloseRequested || mN->socket == nullptr)
    {
        mNativeStatus = -ESHUTDOWN; // uv -UV_ESHUTDOWN parity (errno ESHUTDOWN exists on Linux)
        if (on_done)
        {
            on_done(false);
        }
        return;
    }

    // Resolve the destination BEFORE moving the callback away (uv fills its cell first too).
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: send failure, not a programming error. Inline false — no handler will
        // come for an op asio never took. Inert status (maps to kUnknown), uv invalid-address shape.
        mNativeStatus = -EINVAL;
        if (on_done)
        {
            on_done(false);
        }
        return;
    }
    const sockaddr &sa = reinterpret_cast<const sockaddr &>(addr);
    const asio::ip::udp protocol = (sa.sa_family == AF_INET6) ? asio::ip::udp::v6() : asio::ip::udp::v4();
    asio::ip::udp::endpoint endpoint(protocol, 0);
    const socklen_t sa_len = (sa.sa_family == AF_INET6) ? static_cast<socklen_t>(sizeof(sockaddr_in6))
                                                        : static_cast<socklen_t>(sizeof(sockaddr_in));
    std::memcpy(endpoint.data(), &addr, sa_len);
    endpoint.resize(sa_len);
    // D45 connected mode: an explicit-destination send from a connected socket overrides the
    // pin at the socket level for THIS datagram (sendto(2) semantics), then the pin resumes —
    // the public layer fatals this case, so the backend branch is a defensive passthrough.
    const bool connected = mConnected && mN != nullptr && mN->connected;
    if (connected && endpoint != mN->peer)
    {
        endpoint = mN->peer;
    }

    // One async op per call, no queue (uv native multi-in-flight parity): the handler owns the
    // copied bytes through a shared_ptr cell — PIT-40: nothing on our side is referenced after
    // this function returns except asio-owned state; the completion only touches the cell.
    std::shared_ptr<std::vector<uint8_t>> payload(new std::vector<uint8_t>(data, data + len));
    std::function<void(bool ok)> cb = std::move(on_done);
    // Connected: send WITHOUT an address (the pinned peer routes it — sendto(2) with any name
    // on a connected socket is rejected EINVAL by the Linux kernel, uv null-addr parity).
    if (connected)
    {
        mN->socket->async_send(asio::buffer(*payload),
                               [this, payload, cb](const std::error_code &ec, size_t /*bytes*/) mutable
                               { this->on_send_done(ec, std::move(cb)); });
    }
    else
    {
        mN->socket->async_send_to(asio::buffer(*payload),
                                  endpoint,
                                  [this, payload, cb](const std::error_code &ec, size_t /*bytes*/) mutable
                                  { this->on_send_done(ec, std::move(cb)); });
    }
    this->ensure_pump(); // new work: prefer an immediate tick over the 1ms timer cadence
}

void AsioDgramBackend::on_send_done(const std::error_code &ec, std::function<void(bool ok)> cb)
{
    mNativeStatus = ec ? -static_cast<int>(ec.value()) : 0; // errno-style (uv parity)
    if (cb)
    {
        // PIT-40: invoke through the moved-out local — the callback may close()/destroy the
        // backend mid-execution.
        cb(!ec);
    }
    // payload dies with the handler; no lws queue on udp (concurrent async_send_to is native
    // asio behavior).
}

bool AsioDgramBackend::connect(const std::string &ip, uint16_t port)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioDgramBackend::connect: open() was not called";
    if (mCloseRequested || mN == nullptr || mN->socket == nullptr)
    {
        mNativeStatus = -ESHUTDOWN;
        return false;
    }
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: connect failure, not a programming error (uv invalid-addr shape).
        mNativeStatus = -EINVAL;
        return false;
    }
    // Build the endpoint the same way bind() does (single fill_sockaddr parse path).
    const sockaddr &sa = reinterpret_cast<const sockaddr &>(addr);
    const asio::ip::udp protocol = (sa.sa_family == AF_INET6) ? asio::ip::udp::v6() : asio::ip::udp::v4();
    asio::ip::udp::endpoint endpoint(protocol, 0);
    const socklen_t sa_len = (sa.sa_family == AF_INET6) ? static_cast<socklen_t>(sizeof(sockaddr_in6))
                                                        : static_cast<socklen_t>(sizeof(sockaddr_in));
    std::memcpy(endpoint.data(), &addr, sa_len);
    endpoint.resize(sa_len);

    // Synchronous on a udp socket (connect(2) with no handshake). Error-style codes only.
    std::error_code ec;
    mN->socket->connect(endpoint, ec);
    if (ec)
    {
        mNativeStatus = -static_cast<int>(ec.value());
        return false;
    }
    mN->peer = endpoint;
    mN->connected = true;
    mConnected = true;
    // asio's sync_connect aborts the outstanding async_receive_from (operation_aborted fires
    // on the next pump) — re-arm receive interest so a connected socket keeps delivering
    // (uv parity: uv_udp_connect does not disturb recv interest).
    if (mRecvArmed)
    {
        mRecvArmed = false; // the aborted handler already dropped the re-arm guard
        this->arm_receive();
        this->ensure_pump();
    }
    return true;
}

void AsioDgramBackend::disconnect_remote()
{
    // Un-pin the peer; keep the handle and its binding. asio's datagram disconnect is a
    // zero-endpoint connect (POSIX disconnect(2) shape).
    if (mN != nullptr && mN->socket && !mCloseRequested)
    {
        std::error_code ec;
        mN->socket->connect(asio::ip::udp::endpoint(), ec);
    }
    if (mN != nullptr)
    {
        mN->connected = false;
    }
    mConnected = false;
}


void AsioDgramBackend::receive_start(
    std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> on_datagram)
{
    CXXKIT_CHECK(mLoop != nullptr) << "AsioDgramBackend::receive_start: open() was not called";
    if (mCloseRequested)
    {
        return;
    }
    mOnDatagram = std::move(on_datagram);
    if (mRecvArmed)
    {
        return; // already receiving — callback swap is the whole update (uv read_start re-arm shape)
    }
    this->arm_receive();
    this->ensure_pump(); // new work: prefer an immediate tick over the 1ms timer cadence
}

void AsioDgramBackend::arm_receive()
{
    if (mRecvBuf.size() < kMaxDgramSize)
    {
        mRecvBuf.resize(kMaxDgramSize); // one contiguous reusable block (uv alloc parity)
    }
    // The sender endpoint lives in the Native cell (F1): asio's async_receive_from holds
    // Endpoint& and writes it during completion — a stack local would dangle the moment this
    // function returns (use-after-scope; completions run on a later pump tick). The cell
    // outlives every op.
    mN->socket->async_receive_from(
        asio::buffer(mRecvBuf),
        mN->sender_scratch,
        [this](const std::error_code &ec, size_t bytes)
        {
            if (ec == asio::error::operation_aborted)
            {
                return; // close() cancel: no delivery, the close lifecycle owns the epilogue
            }
            if (ec)
            {
                // Recv error (EMSGSIZE / connected-ICMP ECONNREFUSED...): stop receiving (uv
                // nread<0 docs parity — the caller MUST stop), surface the status; the pimpl
                // maps it and the user's error path decides re-arm. No auto-delivery.
                mNativeStatus = -static_cast<int>(ec.value()); // errno-style (uv parity)
                mRecvArmed = false;
                mOnDatagram = nullptr; // dropping the callback stops deliveries; socket stays open
                return;
            }
            // D45 connected mode: the kernel filters to the pinned peer — a sender address can
            // only be the peer, so surface it directly (the op carries no source address).
            const bool connected = mConnected && mN != nullptr && mN->connected;
            if (bytes > 0 && mOnDatagram && mRecvArmed)
            {
                // PIT-40 copy discipline: the callback may close()/destroy the backend mid-
                // execution — invoke through a local copy, and hand out copies of the source
                // address strings built before the call (the scratch cell is ours, but the
                // strings must not re-read it after a re-arm).
                std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> cb =
                    mOnDatagram;
                const std::string sender_ip = connected ? mN->peer.address().to_string()
                                                        : mN->sender_scratch.address().to_string();
                const uint16_t sender_port = connected ? mN->peer.port() : mN->sender_scratch.port();
                cb(mRecvBuf.data(), bytes, sender_ip, sender_port);
            }
            // Level-triggered: keep receiving (uv recv_start parity) — zero-length datagrams
            // and spurious wakes re-arm too — unless close() just ran: then the socket is gone
            // and re-arming would null-deref.
            if (!mCloseRequested && mRecvArmed && mN->socket)
            {
                this->arm_receive();
            }
        });
    mRecvArmed = true;
}

void AsioDgramBackend::close()
{
    // F8-② idempotence: a second close must not re-tear the socket. mCloseRequested latches.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;
    mOnDatagram = nullptr;
    mRecvArmed = false;
    if (mN != nullptr && mN->socket)
    {
        std::error_code ec;
        mN->socket->cancel(ec); // aborts the outstanding ops (receive early-returns; sends
                                // deliver cb(false) — the pimpl tolerates late false, uv parity)
        // F2: the cancelled handlers fire on the NEXT poll and capture this — they write
        // mNativeStatus and invoke their callbacks, so they MUST run while the backend is alive.
        // Reset without draining = UAF write + phantom cb into freed state on a later pump tick
        // (uv parity: uv_close keeps the handle alive until on_closed, so the pimpl's drain pump
        // actually drives the cancellations; stream_backend_asio fixed the identical shape at
        // T3-review-F2 with drain-then-reset). restart() first per PIT-55 (scheduler auto-stop).
        mN->io->restart();
        for (int rounds = 0; rounds < 1000 && mN->io->poll() > 0; ++rounds)
        {
            // drain cancelled completions
        }
        mN->socket.reset(); // destructor closes the descriptor synchronously (stream parity)
    }
    // Connected state dies with the socket: a closed backend reports unconnected (no re-pin
    // is possible — the close lifecycle is latched).
    if (mN != nullptr)
    {
        mN->connected = false;
        mConnected = false;
    }
}

void *AsioDgramBackend::native_handle() const
{
    return (mN && mN->socket) ? static_cast<void *>(mN->socket.get()) : nullptr;
}

// ----- pump machinery (stream_backend_asio verbatim shape: ensure_pump / pump_tick / cadence) -----

void AsioDgramBackend::ensure_pump()
{
    CXXKIT_CHECK(mN != nullptr) << "AsioDgramBackend::ensure_pump: native cell missing";
    if (mN->pending_tick)
    {
        return; // a tick is already queued: post() coalescing by flag, no duplicate rounds
    }
    mN->pending_tick = true;
    // The weak_ptr is the liveness bridge: a tick sitting in the post queue at backend-destruction
    // time sees a dead flag and stops its timer without touching freed state (EMBED ruling).
    std::weak_ptr<std::atomic<bool>> alive_weak = mN->alive;
    mLoop->post(
        [this, alive_weak]()
        {
            std::shared_ptr<std::atomic<bool>> alive = alive_weak.lock();
            if (!alive || !alive->load())
            {
                // Backend destroyed: cycle ends here. Touch NOTHING through this (ASAN-proven
                // UAF if we dereference here — stream backend destroy-after-close flows).
                return;
            }
            mN->pending_tick = false;
            this->pump_tick();
        });
}

void AsioDgramBackend::pump_tick()
{
    if (!mN || mN->pumping)
    {
        return; // backend destroyed mid-cycle or re-entrant poll from an asio callback
    }
    // PIT-55: restart() before EVERY poll (asio scheduler auto-stop; no-op when not stopped —
    // the documented pattern, stream backend verbatim).
    mN->io->restart();
    mN->pumping = true;
    const size_t handlers_ran = mN->io->poll(); // run every ready handler; never blocks
    mN->pumping = false;

    // Timer-gated cadence (stream fix wave): burst -> immediate re-post; quiet-but-alive -> 1ms
    // repeating timer (C14/PIT-26); idle -> drain-to-quiet, stop the timer, let the cycle die.
    if (!this->has_work())
    {
        for (int rounds = 0; rounds < 1000 && mN->io->poll() > 0; ++rounds)
        {
            // drain completions posted during the last round
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

void AsioDgramBackend::stop_cadence_timer()
{
    if (mN != nullptr && mN->timer_id > 0)
    {
        mLoop->stop_timer(mN->timer_id);
        mN->timer_id = -1;
    }
}

bool AsioDgramBackend::has_work() const
{
    return mN != nullptr && mN->socket != nullptr; // udp has no acceptor concept
}

std::unique_ptr<DgramBackend> make_dgram_backend()
{
    return std::unique_ptr<DgramBackend>(new AsioDgramBackend);
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
