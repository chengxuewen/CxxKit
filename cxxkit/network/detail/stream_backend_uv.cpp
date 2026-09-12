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

#include <cxxkit/network/detail/stream_backend_uv.hpp>

#include <cxxkit/kernel/uv/detail/uv_event_dispatcher.hpp>
#include <cxxkit/network/detail/address_helper.hpp>
#include <cxxkit/network/detail/stream_backend.hpp>

#include <cxxkit/3rdparty/libuv/uv.h>

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

UvStreamBackend::UvStreamBackend() = default;

UvStreamBackend::~UvStreamBackend()
{
    // I6: nothing should reach here with a live handle — pump_until_closed ran at the pimpl's dtor.
    CXXKIT_CHECK(mHandle == nullptr) << "UvStreamBackend: handle outlived the drain pump (teardown bug)";
}

bool UvStreamBackend::open(EventLoop &loop)
{
    mLoop = &loop;
    // The loop's dispatcher is an UvEventDispatcher in this sublibrary's world (static_cast is the
    // inject-once contract: EventLoop owns it exclusively and cxxkit::network made it).
    mUvLoop = &static_cast<UvEventDispatcher &>(loop.dispatcher()).loop();
    return true;
}

void UvStreamBackend::connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvStreamBackend::connect: open() was not called";
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: connect failure, not a programming error — backend reports plain failure,
        // the pimpl layer maps it onto the public error surface.
        // Inert status (maps to kUnknown) — preserves T2's invalid-address public error exactly.
        // NOT UV_EADDRNOTAVAIL: that would map to kAddrNotAvailable and change the T2 contract.
        mNativeStatus = -UV_EINVAL;
        if (on_done)
        {
            on_done(false);
        }
        return;
    }

    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(mUvLoop, handle);
    if (init_rc != 0)
    {
        delete handle;
        CXXKIT_FATAL() << "UvStreamBackend::connect: uv_tcp_init failed (" << init_rc << ")";
    }
    handle->data = this;
    mHandle = handle;
    mOnConnect = std::move(on_done);

    uv_connect_t *req = new uv_connect_t;
    req->data = this;
    const int rc = uv_tcp_connect(req,
                                  handle,
                                  reinterpret_cast<const sockaddr *>(&addr),
                                  &UvStreamBackend::on_connect_done);
    if (rc != 0)
    {
        // Synchronous failure: no connect_cb will come. The pimpl's failure path (on the false
        // callback it just got) runs begin_close(), whose uv_close tears the handle down — same
        // flow as the async-failure path. Keep mConnectReq unset so nothing references the freed
        // req; delete it here since the callback never fires for it.
        delete req;
        mNativeStatus = rc;
        std::function<void(bool ok)> cb = std::move(mOnConnect);
        mOnConnect = nullptr;
        if (cb)
        {
            cb(false);
        }
        return;
    }
    mConnectReq = req;
}

void UvStreamBackend::write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvStreamBackend::write: open() was not called";
    PendingWrite pending;
    pending.mData.assign(data, data + len);
    pending.mOnWritten = std::move(on_done);
    mPendingWrites.push_back(std::move(pending));
    submit_next_write(); // no-op when a write is already in flight (lws: queue, never burst)
}

void UvStreamBackend::read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvStreamBackend::read_start: open() was not called";
    if (mReadArmed)
    {
        mOnData = std::move(on_data); // re-arm replaces the callback (interest update, F7-① shape)
        return;
    }
    mOnData = std::move(on_data);
    const int rc = uv_read_start(reinterpret_cast<uv_stream_t *>(mHandle),
                                 &UvStreamBackend::on_alloc,
                                 &UvStreamBackend::on_read);
    CXXKIT_CHECK(rc == 0) << "UvStreamBackend::read_start: uv_read_start failed (" << rc << ")";
    mReadArmed = true; // read sub-interest armed — the memcached IN-bit equivalent
}

void UvStreamBackend::read_stop()
{
    if (!mReadArmed)
    {
        return; // never armed / already stopped: no-op
    }
    uv_read_stop(reinterpret_cast<uv_stream_t *>(mHandle));
    mReadArmed = false; // read sub-interest disarmed — interest mask updated per transition
}

void UvStreamBackend::close(std::function<void()> on_closed)
{
    mOnClosed = std::move(on_closed);
    begin_close();
}

bool UvStreamBackend::is_open() const
{
    return mHandle != nullptr && !mCloseRequested;
}

bool UvStreamBackend::listen(const std::string &ip, uint16_t port, int backlog)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvStreamBackend::listen: open() was not called";
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        return false; // invalid address: a listen failure, not a programming error
    }

    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(mUvLoop, handle);
    if (init_rc != 0)
    {
        delete handle;
        return false;
    }
    handle->data = this;
    mHandle = handle; // from here on the close callback owns the free; failure paths close+pump it

    const int bind_rc = uv_tcp_bind(handle, reinterpret_cast<const sockaddr *>(&addr), 0);
    const int listen_rc = (bind_rc == 0) ? uv_listen(reinterpret_cast<uv_stream_t *>(handle),
                                                     backlog,
                                                     &UvStreamBackend::on_connection_cb)
                                         : bind_rc;
    if (listen_rc != 0)
    {
        // The handle IS registered with the loop (init did that) — the only clean teardown is
        // uv_close + pump until the callback freed it (TcpSocket dtor discipline), so a retry
        // starts from a clean slate and the loop never carries a zombie handle.
        // No-latch teardown (T2-verbatim): the failed listen is NOT a close lifecycle — a retry
        // must find a fresh, unlatched backend (the header documents "may be retried"). Routing
        // through begin_close would latch mCloseRequested and poison the retry (the pump would
        // refuse to close the next handle, leaking the listener at teardown).
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(mHandle)))
        {
            uv_close(reinterpret_cast<uv_handle_t *>(mHandle), &UvStreamBackend::on_closed);
        }
        for (int rounds = 0; rounds < 1000 && mHandle != nullptr; ++rounds)
        {
            mLoop->process_events(EventLoop::ProcessFlag::kAllEvents);
        }
        return false;
    }

    // R-T3-2: port 0 = OS-assigned; read the real port back via getsockname — race-free discovery
    // versus probing candidate ports from the outside.
    mBoundPort = port;
    sockaddr_storage bound;
    int bound_len = static_cast<int>(sizeof(bound));
    if (uv_tcp_getsockname(handle, reinterpret_cast<sockaddr *>(&bound), &bound_len) == 0)
    {
        if (bound.ss_family == AF_INET)
        {
            mBoundPort = ntohs(reinterpret_cast<const sockaddr_in *>(&bound)->sin_port);
        }
        else if (bound.ss_family == AF_INET6)
        {
            mBoundPort = ntohs(reinterpret_cast<const sockaddr_in6 *>(&bound)->sin6_port);
        }
    }
    return true;
}

uint16_t UvStreamBackend::bound_port() const
{
    return mBoundPort;
}

void UvStreamBackend::set_on_accept(std::function<void(std::unique_ptr<StreamBackend> client)> on_accept)
{
    mOnAccept = std::move(on_accept);
}

bool UvStreamBackend::adopt_native(void *native_handle, EventLoop &loop)
{
    CXXKIT_CHECK(native_handle != nullptr) << "UvStreamBackend::adopt_native: null handle";
    open(loop);
    // The only backend in this sublibrary's world is uv: the void* narrows to uv_tcp_t* here —
    // no uv types outside stream_backend_uv.{hpp,cpp} (D8).
    uv_tcp_t *taken = static_cast<uv_tcp_t *>(native_handle);

    // The handle was initialized on the server's loop engine — same engine by contract; a foreign
    // loop would corrupt uv's internal queues. Verify rather than trust (debugging aid, cheap).
    CXXKIT_CHECK(reinterpret_cast<void *>(taken->loop) == reinterpret_cast<void *>(mUvLoop))
        << "UvStreamBackend::adopt_native: handle belongs to a different uv loop";
    taken->data = this;
    mHandle = taken;
    return true;
}

bool UvStreamBackend::adopt_fd(int fd, EventLoop &loop)
{
    open(loop);
    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(mUvLoop, handle);
    if (init_rc != 0)
    {
        delete handle;
        CXXKIT_FATAL() << "UvStreamBackend::adopt_fd: uv_tcp_init failed (" << init_rc << ")";
    }
    handle->data = this; // set before open so the failure-path close callback sees us
    const int open_rc = uv_tcp_open(handle, static_cast<uv_os_sock_t>(fd));
    if (open_rc != 0)
    {
        // Not yet attached to the loop's activity: uv_close is still the safe detach (init registered
        // it), then the handle is unusable for adoption — fatal per the construction contract.
        uv_close(reinterpret_cast<uv_handle_t *>(handle), &UvStreamBackend::on_closed);
        CXXKIT_FATAL() << "UvStreamBackend::adopt_fd: uv_tcp_open failed (" << open_rc << ") for fd " << fd;
    }
    mHandle = handle;
    return true;
}

void UvStreamBackend::pump_until_closed()
{
    // ~TcpSocket / ~TcpServer / listen-failure teardown drain discipline, moved here (I6): close
    // (idempotent — a no-op if already requested), then pump NOWAIT rounds until the uv close
    // callback has actually run so no uv state (handle, in-flight reqs, callbacks) survives us.
    // The pump is bounded by loop.exit + wake: process_events rounds drain close callbacks.
    if (mLoop == nullptr && mHandle == nullptr)
    {
        return; // never opened (kIdle socket): no loop to pump, nothing to drain
    }
    if (mHandle != nullptr && !mCloseRequested)
    {
        begin_close();
    }
    // Run NOWAIT rounds until the handle cell is freed (the close callback clears mHandle). One
    // round typically suffices; the loop cap guards a pathological no-progress case.
    for (int rounds = 0; rounds < 1000 && mHandle != nullptr; ++rounds)
    {
        mLoop->process_events(EventLoop::ProcessFlag::kAllEvents);
    }
}

void UvStreamBackend::submit_next_write()
{
    if (mWriteReq != nullptr || mPendingWrites.empty())
    {
        return; // one uv_write in flight (lws), or nothing to send
    }
    PendingWrite pending = std::move(mPendingWrites.front());
    mPendingWrites.pop_front();

    // Storage must outlive the uv_write_t: park the bytes in mInFlight and point the uv_buf at it.
    mInFlight = std::move(pending.mData);
    uv_buf_t write_buf = uv_buf_init(reinterpret_cast<char *>(mInFlight.data()), mInFlight.size());

    uv_write_t *req = new uv_write_t;
    req->data = this;
    const int rc = uv_write(req,
                            reinterpret_cast<uv_stream_t *>(mHandle),
                            &write_buf,
                            1,
                            &UvStreamBackend::on_write_done);
    if (rc != 0)
    {
        // Synchronous failure: no write_cb will come. Deliver false inline and keep draining.
        delete req;
        mInFlight.clear();
        if (pending.mOnWritten)
        {
            pending.mOnWritten(false);
        }
        submit_next_write();
        return;
    }
    mWriteReq = req;
    mOnWrittenCurrent = std::move(pending.mOnWritten); // handed back in on_write_done
}

void UvStreamBackend::begin_close()
{
    // F8-② idempotence: uv_close is asynchronous; a second close on the same handle is
    // use-after-free. mCloseRequested latches — every later close() is a no-op.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;

    // Pending writes are discarded — every queued callback gets its false completion now. Swap the
    // whole deque out FIRST (EventLoop::take_post_queue shape): an on_written(false) callback that
    // re-enters write() would push into a live deque we are draining.
    // The in-flight uv_write (if any) completes in on_write_done with UV_ECANCELED -> false.
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
    // Pending connect completes false as well (discard semantics, symmetric with writes).
    if (mOnConnect)
    {
        std::function<void(bool ok)> cb = std::move(mOnConnect);
        mOnConnect = nullptr;
        cb(false);
    }
    if (mHandle == nullptr)
    {
        // Never initialized (kIdle shape): nothing to close at the uv level — synchronous done.
        if (mOnClosed)
        {
            std::function<void()> cb = mOnClosed;
            mOnClosed = nullptr;
            cb();
        }
        return;
    }
    if (mReadArmed)
    {
        uv_read_stop(reinterpret_cast<uv_stream_t *>(mHandle));
        mReadArmed = false;
    }
    mOnData = nullptr;
    if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(mHandle)))
    {
        uv_close(reinterpret_cast<uv_handle_t *>(mHandle), &UvStreamBackend::on_closed);
    }
}

// ----- uv trampolines (handle->data routes back here; Node tcp_wrap pattern) -----

void UvStreamBackend::on_connect_done(uv_connect_t *req, int status)
{
    UvStreamBackend *backend = static_cast<UvStreamBackend *>(req->data);
    delete req; // connect req is one-shot: freed immediately, not at socket close
    if (backend->mConnectReq == req)
    {
        backend->mConnectReq = nullptr;
    }
    backend->mNativeStatus = status;
    const bool ok = (status == 0) && !backend->mCloseRequested;
    if (backend->mOnConnect)
    {
        std::function<void(bool ok)> cb = std::move(backend->mOnConnect);
        backend->mOnConnect = nullptr;
        cb(ok);
    }
}

void UvStreamBackend::on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    UvStreamBackend *backend = static_cast<UvStreamBackend *>(handle->data);
    // beast flat_buffer shape: one contiguous block, resized to the suggested chunk, handed out whole.
    if (backend->mReadBuf.size() < suggested_size)
    {
        backend->mReadBuf.resize(suggested_size);
    }
    buf->base = reinterpret_cast<char *>(backend->mReadBuf.data());
    buf->len = backend->mReadBuf.size();
}

void UvStreamBackend::on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    UvStreamBackend *backend = static_cast<UvStreamBackend *>(stream->data);
    if (nread == 0)
    {
        return; // EAGAIN/no-data: no delivery, keep the interest armed
    }
    if (nread < 0)
    {
        // EOF or error: deliver the (nullptr, code) terminal event once, then go through the
        // close path. EOF is normalized onto the backend-neutral sentinel (uv: UV_EOF enum);
        // error mapping and the mOnError surface stay in the pimpl (Task 2 split).
        backend->mNativeStatus = static_cast<int>(nread);
        const ssize_t code = (nread == UV_EOF) ? kBackendEof : nread;
        std::function<void(const uint8_t *data, ssize_t nread)> cb = backend->mOnData;
        backend->mOnData = nullptr;
        if (cb)
        {
            cb(nullptr, code);
        }
        backend->begin_close();
        return;
    }
    // F8-① copy discipline: the callback may close()/read_stop() mid-execution, which would destroy
    // the member std::function we are inside — invoke through a local copy instead.
    if (backend->mOnData)
    {
        std::function<void(const uint8_t *data, ssize_t nread)> cb = backend->mOnData;
        cb(reinterpret_cast<const uint8_t *>(buf->base), nread);
    }
}

void UvStreamBackend::on_write_done(uv_write_t *req, int status)
{
    UvStreamBackend *backend = static_cast<UvStreamBackend *>(req->data);
    backend->mNativeStatus = status;
    std::function<void(bool ok)> cb = std::move(backend->mOnWrittenCurrent);
    backend->mOnWrittenCurrent = nullptr;
    delete req;
    if (backend->mWriteReq == req)
    {
        backend->mWriteReq = nullptr;
    }
    backend->mInFlight.clear();
    if (cb)
    {
        cb(status == 0);
    }
    // lws discipline: the completion of one write is the only trigger for the next.
    backend->submit_next_write();
}

void UvStreamBackend::on_closed(uv_handle_t *handle)
{
    UvStreamBackend *backend = static_cast<UvStreamBackend *>(handle->data);
    backend->mHandle = nullptr;
    if (backend->mOnClosed)
    {
        std::function<void()> cb = backend->mOnClosed;
        backend->mOnClosed = nullptr;
        cb();
    }
    // The allocation is uv_tcp_t (248B); the callback parameter type is uv_handle_t (96B). Deleting
    // through the base-typed pointer is a new-delete-type-mismatch (ASAN) — cast back to the real
    // allocation type (both client handles and adopted/fd handles are new uv_tcp_t).
    delete reinterpret_cast<uv_tcp_t *>(handle);
}

void UvStreamBackend::on_connection_cb(uv_stream_t *server, int status)
{
    UvStreamBackend *backend = static_cast<UvStreamBackend *>(server->data);
    if (status < 0)
    {
        return; // listen-side error (EMFILE & co): no backlog to drain, no sockets to produce
    }
    // memcached accept-batch discipline: drain everything the kernel has queued for this wake-up.
    while (true)
    {
        uv_tcp_t *client = new uv_tcp_t;
        if (uv_tcp_init(server->loop, client) != 0)
        {
            delete client; // init never registered it with the loop
            return;        // cannot init: stop draining
        }
        if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client)) != 0)
        {
            // Backlog empty (UV_EAGAIN) or broken peer: close the bare handle, no socket produced.
            uv_close(reinterpret_cast<uv_handle_t *>(client), &UvStreamBackend::on_discard_closed);
            return;
        }
        if (!backend->mOnAccept)
        {
            // No consumer installed: accept-and-discard keeps the backlog from filling silently.
            uv_close(reinterpret_cast<uv_handle_t *>(client), &UvStreamBackend::on_discard_closed);
            continue;
        }
        // R-T3-1: the accepted handle is handed over — its uv_close (and lifetime) belongs to the
        // client backend from here on; the server never touches it again.
        std::unique_ptr<UvStreamBackend> client_backend(new UvStreamBackend);
        client_backend->adopt_native(client, *backend->mLoop);
        backend->mOnAccept(std::unique_ptr<StreamBackend>(client_backend.release()));
    }
}

void UvStreamBackend::on_discard_closed(uv_handle_t *handle)
{
    delete reinterpret_cast<uv_tcp_t *>(handle); // bare client cell: no pimpl behind it, just free
}

std::unique_ptr<StreamBackend> make_stream_backend()
{
    return std::unique_ptr<StreamBackend>(new UvStreamBackend);
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
