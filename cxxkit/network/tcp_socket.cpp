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

#include <cxxkit/network/detail/address_helper.hpp>
#include <cxxkit/network/detail/error_mapping.hpp>
#include <cxxkit/network/detail/tcp_socket_p.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/tcp_socket.hpp>

#include <cxxkit/tools/checks.hpp>

#include <cstring>
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

TcpSocketPrivate::TcpSocketPrivate(TcpSocket *p, EventLoop &loop)
    : mP(p)
    , mLoop(loop)
{
    // The loop's dispatcher is an UvEventDispatcher in this sublibrary's world (static_cast is the
    // inject-once contract: EventLoop owns it exclusively and cxxkit::network made it).
    mDispatcher = static_cast<UvEventDispatcher *>(&loop.dispatcher());
    mLoopThreadId = std::this_thread::get_id(); // construction is loop-thread only; pin the check id
}

TcpSocketPrivate::~TcpSocketPrivate()
{
    // I6: nothing should reach here with a live handle — the public destructor closes and pumps.
    CXXKIT_CHECK(mHandle == nullptr) << "TcpSocketPrivate: handle outlived the drain pump (teardown bug)";
}

void TcpSocketPrivate::check_loop_thread(const char *api) const
{
    // I1: always-on, not debug-only (R-B2-5 shape). One thread-id compare per entry.
    if (std::this_thread::get_id() != mLoopThreadId)
    {
        CXXKIT_FATAL() << "TcpSocket::" << api << " called from a non-loop thread (loop thread "
                       << std::hash<std::thread::id>()(mLoopThreadId) << ", caller thread "
                       << std::hash<std::thread::id>()(std::this_thread::get_id())
                       << "). Serialize with EventLoop::post() from other threads instead.";
    }
}

TcpSocket::TcpSocket(EventLoop &loop)
    : mDPtr(new TcpSocketPrivate(this, loop))
{
}

TcpSocket::~TcpSocket()
{
    CXXKIT_D(TcpSocket);
    // I6: close (idempotent — a no-op if already requested), then pump the loop until the uv close
    // callback has actually run so no uv state (handle, in-flight reqs, callbacks) survives us.
    // The pump is bounded by loop.exit + wake: process_events rounds drain close callbacks.
    if (d->mHandle != nullptr || d->mState == SocketState::kConnecting)
    {
        this->close();
        // Run NOWAIT rounds until the close callback ran (it flips state to kClosed or kIdle —
        // a failed in-flight connect resets to kIdle). One round typically suffices; the loop cap
        // guards a pathological no-progress case.
        for (int rounds = 0; rounds < 1000 && d->mState != SocketState::kClosed && d->mState != SocketState::kIdle;
             ++rounds)
        {
            d->mLoop.process_events(EventLoop::ProcessFlag::kAllEvents);
        }
    }
}

std::unique_ptr<TcpSocket> TcpSocket::adopt_fd(EventLoop &loop, int fd)
{
    std::unique_ptr<TcpSocket> socket(new TcpSocket(loop));
    TcpSocketPrivate *d = socket->mDPtr.get();
    d->check_loop_thread("adopt_fd");

    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(&d->mDispatcher->loop(), handle);
    if (init_rc != 0)
    {
        delete handle;
        CXXKIT_FATAL() << "TcpSocket::adopt_fd: uv_tcp_init failed (" << init_rc << ")";
    }
    handle->data = d; // set before open so the failure-path close callback sees us
    const int open_rc = uv_tcp_open(handle, static_cast<uv_os_sock_t>(fd));
    if (open_rc != 0)
    {
        // Not yet attached to the loop's activity: uv_close is still the safe detach (init registered
        // it), then the handle is unusable for adoption — fatal per the construction contract.
        uv_close(reinterpret_cast<uv_handle_t *>(handle), &TcpSocketPrivate::on_closed);
        CXXKIT_FATAL() << "TcpSocket::adopt_fd: uv_tcp_open failed (" << open_rc << ") for fd " << fd;
    }
    d->attach_connected_handle(handle);
    return socket;
}

std::unique_ptr<TcpSocket> TcpSocket::adopt_native(EventLoop &loop, void *native_handle)
{
    std::unique_ptr<TcpSocket> socket(new TcpSocket(loop));
    TcpSocketPrivate *d = socket->mDPtr.get();
    d->check_loop_thread("adopt_native");
    CXXKIT_CHECK(native_handle != nullptr) << "TcpSocket::adopt_native: null handle";
    // The only backend in this sublibrary's world is uv: the void* narrows to uv_tcp_t* here in
    // the .cpp — the public header keeps no uv types (D8).
    uv_tcp_t *taken = static_cast<uv_tcp_t *>(native_handle);

    // The handle was initialized on the server's loop engine — same engine by contract; a foreign
    // loop would corrupt uv's internal queues. Verify rather than trust (debugging aid, cheap).
    CXXKIT_CHECK(reinterpret_cast<void *>(taken->loop) == reinterpret_cast<void *>(&d->mDispatcher->loop()))
        << "TcpSocket::adopt_native: handle belongs to a different uv loop";
    d->attach_connected_handle(taken);
    return socket;
}

void TcpSocketPrivate::attach_connected_handle(uv_tcp_t *handle)
{
    handle->data = this;
    mHandle = handle;
    set_state(SocketState::kConnected); // adopted handle is already connected
}

void TcpSocketPrivate::set_state(SocketState state)
{
    mState = state;
    if (mOnStateChange)
    {
        // PIT-40: the callback may close() or even destroy the socket — invoke a local copy.
        std::function<void(SocketState)> cb = mOnStateChange;
        cb(state);
    }
}

void TcpSocketPrivate::report_error(SocketError error, const std::string &message)
{
    mLastError = error;
    if (mOnError)
    {
        // PIT-40: same discipline — the callback may tear the socket down mid-invoke.
        std::function<void(SocketError, const std::string &)> cb = mOnError;
        cb(error, message);
    }
}

void TcpSocket::connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("connect");
    CXXKIT_CHECK(on_connected != nullptr) << "TcpSocket::connect requires a connected callback";
    CXXKIT_CHECK(d->mState == SocketState::kIdle)
        << "TcpSocket::connect: state must be kIdle (got " << static_cast<int>(d->mState) << ")";
    CXXKIT_CHECK(d->mCloseRequested == false) << "TcpSocket::connect: socket is closing/closed";

    sockaddr_storage addr;
    if (!cxxkit::network::detail::fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: connect failure, not a programming error (symmetric with TcpServer::listen).
        // State stays kIdle (never left it); the error surface reports an unmappable failure as kUnknown.
        d->report_error(SocketError::kUnknown,
                        "TcpSocket::connect: invalid address \"" + ip + "\" (neither IPv4 nor IPv6)");
        std::function<void(bool ok)> cb = std::move(on_connected);
        cb(false);
        return;
    }

    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(&d->mDispatcher->loop(), handle);
    if (init_rc != 0)
    {
        delete handle;
        CXXKIT_FATAL() << "TcpSocket::connect: uv_tcp_init failed (" << init_rc << ")";
    }
    handle->data = d;
    d->mHandle = handle;
    d->set_state(SocketState::kConnecting);
    d->mOnConnect = std::move(on_connected);

    uv_connect_t *req = new uv_connect_t;
    req->data = d;
    const int rc = uv_tcp_connect(req,
                                  handle,
                                  reinterpret_cast<const sockaddr *>(&addr),
                                  &TcpSocketPrivate::on_connect_done);
    if (rc != 0)
    {
        // Synchronous failure: no connect_cb will come. Deliver false inline, then tear the handle
        // down. The teardown lands in kClosing→kClosed via set_state — but a failed connect leaves
        // the machine back at kIdle (handle-less, retryable), mirroring the async path in
        // on_connect_done; there the async close callback reports kClosed, here the socket never
        // left the connect attempt, so reset to kIdle after the error report.
        delete req;
        d->report_error(network::detail::map_uv_error(rc),
                        "TcpSocket::connect: uv_tcp_connect failed (" + std::to_string(rc) + ")");
        std::function<void(bool ok)> cb = std::move(d->mOnConnect);
        d->mOnConnect = nullptr;
        d->begin_close();
        d->set_state(SocketState::kIdle);
        cb(false);
        return;
    }
    d->mConnectReq = req;
}

void TcpSocket::write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("write");
    CXXKIT_CHECK(on_written != nullptr) << "TcpSocket::write requires a written callback";
    CXXKIT_CHECK(data != nullptr || len == 0) << "TcpSocket::write: null data with non-zero length";
    if (d->mCloseRequested || d->mState == SocketState::kClosing || d->mState == SocketState::kClosed)
    {
        on_written(false); // closed socket: immediate discard completion, documented semantics
        return;
    }
    CXXKIT_CHECK(d->mState == SocketState::kConnected)
        << "TcpSocket::write: state must be kConnected (got " << static_cast<int>(d->mState) << ")";

    TcpSocketPrivate::PendingWrite pending;
    pending.mData.assign(data, data + len);
    pending.mOnWritten = std::move(on_written);
    d->mPendingWrites.push_back(std::move(pending));
    d->submit_next_write(); // no-op when a write is already in flight (lws: queue, never burst)
}

void TcpSocket::read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("read_start");
    CXXKIT_CHECK(on_data != nullptr) << "TcpSocket::read_start requires a data callback";
    CXXKIT_CHECK(d->mCloseRequested == false) << "TcpSocket::read_start: socket is closing/closed";
    CXXKIT_CHECK(d->mState == SocketState::kConnected)
        << "TcpSocket::read_start: state must be kConnected (got " << static_cast<int>(d->mState) << ")";

    if (d->mReadArmed)
    {
        d->mOnData = std::move(on_data); // re-arm replaces the callback (interest update, F7-① shape)
        return;
    }
    d->mOnData = std::move(on_data);
    const int rc = uv_read_start(reinterpret_cast<uv_stream_t *>(d->mHandle),
                                 &TcpSocketPrivate::on_alloc,
                                 &TcpSocketPrivate::on_read);
    CXXKIT_CHECK(rc == 0) << "TcpSocket::read_start: uv_read_start failed (" << rc << ")";
    d->mReadArmed = true; // read sub-interest armed — the memcached IN-bit equivalent
}

void TcpSocket::read_stop()
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("read_stop");
    if (!d->mReadArmed)
    {
        return; // never armed / already stopped: no-op
    }
    uv_read_stop(reinterpret_cast<uv_stream_t *>(d->mHandle));
    d->mReadArmed = false; // read sub-interest disarmed — interest mask updated per transition
}

void TcpSocket::close()
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("close");
    d->begin_close();
}

bool TcpSocket::is_open() const
{
    CXXKIT_D(const TcpSocket);
    return !d->mCloseRequested && d->mState != SocketState::kClosed;
}

void TcpSocket::set_on_error(std::function<void(SocketError, const std::string &)> on_error)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("set_on_error");
    d->mOnError = std::move(on_error);
}

void TcpSocket::set_on_state_change(std::function<void(SocketState)> on_state_change)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("set_on_state_change");
    d->mOnStateChange = std::move(on_state_change);
}

SocketState TcpSocket::state() const
{
    CXXKIT_D(const TcpSocket);
    d->check_loop_thread("state");
    return d->mState;
}

SocketError TcpSocket::last_error() const
{
    CXXKIT_D(const TcpSocket);
    d->check_loop_thread("last_error");
    return d->mLastError;
}

void TcpSocketPrivate::begin_close()
{
    // F8-② idempotence: uv_close is asynchronous; a second close on the same handle is
    // use-after-free. mCloseRequested is the latch — every later close() is a no-op.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;
    if (mState == SocketState::kClosed || mHandle == nullptr)
    {
        set_state(SocketState::kClosed); // never initialized (kIdle) — nothing to close
        return;
    }
    set_state(SocketState::kClosing);

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
    if (mReadArmed)
    {
        uv_read_stop(reinterpret_cast<uv_stream_t *>(mHandle));
        mReadArmed = false;
    }
    mOnData = nullptr;
    if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(mHandle)))
    {
        uv_close(reinterpret_cast<uv_handle_t *>(mHandle), &TcpSocketPrivate::on_closed);
    }
}

void TcpSocketPrivate::submit_next_write()
{
    if (mWriteReq != nullptr || mPendingWrites.empty())
    {
        return; // one uv_write in flight (lws), or nothing to send
    }
    PendingWrite pending = std::move(mPendingWrites.front());
    mPendingWrites.pop_front();

    // Storage must outlive the uv_write_t: park the bytes in mInFlight and point the uv_buf at it.
    mInFlight = std::move(pending.mData);
    mWriteBuf = uv_buf_init(reinterpret_cast<char *>(mInFlight.data()), mInFlight.size());

    uv_write_t *req = new uv_write_t;
    req->data = this;
    const int rc = uv_write(req,
                            reinterpret_cast<uv_stream_t *>(mHandle),
                            &mWriteBuf,
                            1,
                            &TcpSocketPrivate::on_write_done);
    if (rc != 0)
    {
        // Synchronous failure: no write_cb will come. Deliver false inline and keep draining.
        delete req;
        mInFlight.clear();
        mWriteBuf = uv_buf_init(nullptr, 0);
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

void TcpSocketPrivate::on_connect_done(uv_connect_t *req, int status)
{
    TcpSocketPrivate *d = static_cast<TcpSocketPrivate *>(req->data);
    delete req; // connect req is one-shot: freed immediately, not at socket close
    if (d->mConnectReq == req)
    {
        d->mConnectReq = nullptr;
    }
    const bool ok = (status == 0) && !d->mCloseRequested;
    if (ok)
    {
        if (d->mState == SocketState::kConnecting)
        {
            d->set_state(SocketState::kConnected);
        }
    }
    else
    {
        // Failed connect (or close-during-connect). The failed-connect state contract: the machine
        // returns to kIdle (handle-less, retryable) once the async teardown lands in kClosed. When
        // close was requested externally the machine stays on the kClosing→kClosed path instead.
        if (!d->mCloseRequested && status != 0)
        {
            d->report_error(network::detail::map_uv_error(status),
                            "TcpSocket: connect failed (uv status " + std::to_string(status) + ")");
        }
        if (d->mState == SocketState::kConnecting)
        {
            d->set_state(SocketState::kClosing);
        }
    }
    if (d->mOnConnect)
    {
        std::function<void(bool ok)> cb = std::move(d->mOnConnect);
        d->mOnConnect = nullptr;
        cb(ok);
    }
    if (!ok && d->mState == SocketState::kClosing && !d->mCloseRequested)
    {
        // Failed connect without an explicit close: tear the handle down, then reset the machine to
        // kIdle — the failed-connect state contract (handle-less, retryable; the T2 test pins
        // s.state() == kIdle after a refused connect). begin_close fires kClosing, and its async
        // close callback would fire kClosed next; the direct reset to kIdle here keeps the notified
        // subsequence clean: kConnecting, kClosing, kIdle. The close callback sees kIdle (already
        // terminal for the failed-connect path) and stays silent.
        d->begin_close();
        if (d->mState == SocketState::kClosing)
        {
            d->set_state(SocketState::kIdle);
        }
    }
}


void TcpSocketPrivate::on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    TcpSocketPrivate *d = static_cast<TcpSocketPrivate *>(handle->data);
    // beast flat_buffer shape: one contiguous block, resized to the suggested chunk, handed out whole.
    if (d->mReadBuf.size() < suggested_size)
    {
        d->mReadBuf.resize(suggested_size);
    }
    buf->base = reinterpret_cast<char *>(d->mReadBuf.data());
    buf->len = d->mReadBuf.size();
}

void TcpSocketPrivate::on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    TcpSocketPrivate *d = static_cast<TcpSocketPrivate *>(stream->data);
    if (nread == 0)
    {
        return; // EAGAIN/no-data: no delivery, keep the interest armed
    }
    if (nread < 0)
    {
        // EOF (UV_EOF) or error: deliver the (nullptr, nread) terminal event once, map the failure
        // onto the public error surface (EOF → kEof, else map_uv_error), disarm, and go through the
        // close path (error-state stickiness avoidance: everything after is a no-op).
        const SocketError error = (nread == UV_EOF) ? SocketError::kEof
                                                    : network::detail::map_uv_error(static_cast<int>(nread));
        d->report_error(error,
                        (error == SocketError::kEof)
                            ? "TcpSocket: end of file (peer closed)"
                            : "TcpSocket: read error (uv status " + std::to_string(nread) + ")");
        if (d->mOnData)
        {
            std::function<void(const uint8_t *data, ssize_t nread)> cb = std::move(d->mOnData);
            d->mOnData = nullptr;
            cb(nullptr, nread);
        }
        if (d->mReadArmed)
        {
            uv_read_stop(stream);
            d->mReadArmed = false;
        }
        d->begin_close();
        return;
    }
    // F8-① copy discipline: the callback may close()/read_stop() mid-execution, which would destroy
    // the member std::function we are inside — invoke through a local copy instead.
    if (d->mOnData)
    {
        std::function<void(const uint8_t *data, ssize_t nread)> cb = d->mOnData;
        cb(reinterpret_cast<const uint8_t *>(buf->base), nread);
    }
}

void TcpSocketPrivate::on_write_done(uv_write_t *req, int status)
{
    TcpSocketPrivate *d = static_cast<TcpSocketPrivate *>(req->data);
    std::function<void(bool ok)> cb = std::move(d->mOnWrittenCurrent);
    d->mOnWrittenCurrent = nullptr;
    delete req;
    if (d->mWriteReq == req)
    {
        d->mWriteReq = nullptr;
    }
    d->mInFlight.clear();
    d->mWriteBuf = uv_buf_init(nullptr, 0);
    if (cb)
    {
        cb(status == 0);
    }
    // lws discipline: the completion of one write is the only trigger for the next.
    d->submit_next_write();
}

void TcpSocketPrivate::on_closed(uv_handle_t *handle)
{
    TcpSocketPrivate *d = static_cast<TcpSocketPrivate *>(handle->data);
    d->mHandle = nullptr;
    if (d->mState != SocketState::kIdle)
    {
        // Failed-connect teardown resets the machine to kIdle before this callback runs (see
        // on_connect_done); do not overwrite it with kClosed. Normal closes report kClosed here.
        d->set_state(SocketState::kClosed);
    }
    // The allocation is uv_tcp_t (248B); the callback parameter type is uv_handle_t (96B). Deleting
    // through the base-typed pointer is a new-delete-type-mismatch (ASAN) — cast back to the real
    // allocation type (both client handles and adopted/fd handles are new uv_tcp_t).
    delete reinterpret_cast<uv_tcp_t *>(handle);
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
