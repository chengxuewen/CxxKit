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

#include <cxxkit/uv/detail/tcp_socket_p.hpp>
#include <cxxkit/uv/tcp_socket.hpp>

#include <cxxkit/tools/checks.hpp>

#include <cstring>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

TcpSocketPrivate::TcpSocketPrivate(TcpSocket *p, EventLoop &loop)
    : mP(p)
    , mLoop(loop)
{
    // The loop's dispatcher is an UvEventDispatcher in this sublibrary's world (static_cast is the
    // inject-once contract: EventLoop owns it exclusively and cxxkit::uv made it).
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
    if (d->mHandle != nullptr || d->mState == TcpSocketPrivate::State::kConnecting)
    {
        this->close();
        // Run NOWAIT rounds until the close callback ran (it flips state to kClosed). One round
        // typically suffices; the loop cap guards a pathological no-progress case.
        for (int rounds = 0; rounds < 1000 && d->mState != TcpSocketPrivate::State::kClosed; ++rounds)
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
    const int open_rc = uv_tcp_open(handle, static_cast<uv_os_sock_t>(fd));
    if (open_rc != 0)
    {
        // Not yet attached to the loop's activity: uv_close is still the safe detach (init registered
        // it), then the handle is unusable for adoption — fatal per the construction contract.
        uv_close(reinterpret_cast<uv_handle_t *>(handle), &TcpSocketPrivate::on_closed);
        CXXKIT_FATAL() << "TcpSocket::adopt_fd: uv_tcp_open failed (" << open_rc << ") for fd " << fd;
    }
    handle->data = d;
    d->mHandle = handle;
    d->mState = TcpSocketPrivate::State::kConnected; // adopted fd is already connected
    return socket;
}

void TcpSocket::connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("connect");
    CXXKIT_CHECK(on_connected != nullptr) << "TcpSocket::connect requires a connected callback";
    CXXKIT_CHECK(d->mState == TcpSocketPrivate::State::kIdle)
        << "TcpSocket::connect: state must be kIdle (got " << static_cast<int>(d->mState) << ")";
    CXXKIT_CHECK(d->mCloseRequested == false) << "TcpSocket::connect: socket is closing/closed";

    sockaddr_in addr;
    const int addr_rc = uv_ip4_addr(ip.c_str(), port, &addr);
    CXXKIT_CHECK(addr_rc == 0) << "TcpSocket::connect: invalid address " << ip << ":" << port << " (" << addr_rc << ")";

    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(&d->mDispatcher->loop(), handle);
    if (init_rc != 0)
    {
        delete handle;
        CXXKIT_FATAL() << "TcpSocket::connect: uv_tcp_init failed (" << init_rc << ")";
    }
    handle->data = d;
    d->mHandle = handle;
    d->mState = TcpSocketPrivate::State::kConnecting;
    d->mOnConnect = std::move(on_connected);

    uv_connect_t *req = new uv_connect_t;
    req->data = d;
    const int rc = uv_tcp_connect(req,
                                  handle,
                                  reinterpret_cast<const sockaddr *>(&addr),
                                  &TcpSocketPrivate::on_connect_done);
    if (rc != 0)
    {
        // Synchronous failure: no connect_cb will come. Deliver false inline, then tear the handle down.
        delete req;
        std::function<void(bool ok)> cb = std::move(d->mOnConnect);
        d->mOnConnect = nullptr;
        d->begin_close();
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
    if (d->mCloseRequested || d->mState == TcpSocketPrivate::State::kClosing ||
        d->mState == TcpSocketPrivate::State::kClosed)
    {
        on_written(false); // closed socket: immediate discard completion, documented semantics
        return;
    }
    CXXKIT_CHECK(d->mState == TcpSocketPrivate::State::kConnected)
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
    CXXKIT_CHECK(d->mState == TcpSocketPrivate::State::kConnected)
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
    return !d->mCloseRequested && d->mState != TcpSocketPrivate::State::kClosed;
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
    if (mState == State::kClosed || mHandle == nullptr)
    {
        mState = State::kClosed; // never initialized (kIdle) — nothing to close
        return;
    }
    mState = State::kClosing;

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
    if (d->mState == State::kConnecting)
    {
        d->mState = ok ? State::kConnected : State::kClosing;
    }
    if (d->mOnConnect)
    {
        std::function<void(bool ok)> cb = std::move(d->mOnConnect);
        d->mOnConnect = nullptr;
        cb(ok);
    }
    if (!ok && d->mState == State::kClosing && !d->mCloseRequested)
    {
        d->begin_close(); // failed connect without an explicit close: tear the handle down
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
        return; // EAGAIN (Nothing-Eelse): no delivery, keep the interest armed
    }
    if (nread < 0)
    {
        // EOF (UV_EOF) or error: deliver the (nullptr, nread) terminal event once, disarm, and map
        // onto the close path (error-state stickiness avoidance: everything after is a no-op).
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
    d->mState = State::kClosed;
    delete handle;
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
