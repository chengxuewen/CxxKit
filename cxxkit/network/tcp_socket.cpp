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

#include <cxxkit/network/detail/error_mapping.hpp>
#include <cxxkit/network/detail/tcp_socket_p.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/tcp_socket.hpp>

#include <cxxkit/tools/checks.hpp>

#include <string>
#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

TcpSocketPrivate::TcpSocketPrivate(TcpSocket *p, EventLoop &loop)
    : mP(p)
    , mLoop(loop)
{
    mBackend->open(loop);                       // binds the backend to this loop's engine
    mLoopThreadId = std::this_thread::get_id(); // construction is loop-thread only; pin the check id
}

TcpSocketPrivate::TcpSocketPrivate(TcpSocket *p, std::unique_ptr<network::detail::StreamBackend> backend)
    : mP(p)
    , mLoop(backend->loop())
    , mBackend(std::move(backend))
{
    mLoopThreadId = std::this_thread::get_id(); // construction is loop-thread only; pin the check id
}

TcpSocketPrivate::~TcpSocketPrivate()
{
    // I6: nothing should reach here with a live handle — the public destructor closes and pumps.
    // A never-opened backend (kIdle socket, connect never called) is legal teardown too: no
    // transport state exists, no close was ever needed.
    CXXKIT_CHECK(mCloseRequested || !mBackend->is_open())
        << "TcpSocketPrivate: backend outlived the drain pump (teardown bug)";
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

TcpSocket::TcpSocket(EventLoop &loop)
    : mDPtr(new TcpSocketPrivate(this, loop))
{
}

TcpSocket::TcpSocket(EventLoop &loop, std::unique_ptr<network::detail::StreamBackend> backend)
    : mDPtr(new TcpSocketPrivate(this, std::move(backend)))
{
}

TcpSocket::~TcpSocket()
{
    CXXKIT_D(TcpSocket);
    // I6: close (idempotent — a no-op if already requested), then pump the loop until the close
    // callback has actually run so no transport state (handle, in-flight reqs, callbacks) survives
    // us. The pump moved into the backend (pump_until_closed); the failed-connect kIdle stop lives
    // here: a failed in-flight connect resets the machine to kIdle (handle-less, retryable).
    if (d->mState != SocketState::kClosed && d->mState != SocketState::kIdle)
    {
        this->close();
    }
    d->mBackend->pump_until_closed();
}

std::unique_ptr<TcpSocket> TcpSocket::adopt_fd(EventLoop &loop, int fd)
{
    std::unique_ptr<TcpSocket> socket(new TcpSocket(loop));
    TcpSocketPrivate *d = socket->mDPtr.get();
    d->check_loop_thread("adopt_fd");

    if (!d->mBackend->adopt_fd(fd, loop))
    {
        CXXKIT_FATAL() << "TcpSocket::adopt_fd: backend refused fd " << fd;
    }
    d->set_state(SocketState::kConnected); // adopted handle is already connected
    return socket;
}

std::unique_ptr<TcpSocket> TcpSocket::adopt_native(EventLoop &loop, void *native_handle)
{
    std::unique_ptr<TcpSocket> socket(new TcpSocket(loop));
    TcpSocketPrivate *d = socket->mDPtr.get();
    d->check_loop_thread("adopt_native");

    if (!d->mBackend->adopt_native(native_handle, loop))
    {
        CXXKIT_FATAL() << "TcpSocket::adopt_native: backend refused the native handle";
    }
    d->set_state(SocketState::kConnected); // adopted handle is already connected
    return socket;
}

std::unique_ptr<TcpSocket> TcpSocket::adopt_backend(EventLoop &loop,
                                                    std::unique_ptr<network::detail::StreamBackend> backend)
{
    CXXKIT_CHECK(backend != nullptr) << "TcpSocket::adopt_backend: null backend";
    CXXKIT_CHECK(&backend->loop() == &loop) << "TcpSocket::adopt_backend: backend is bound to a different loop";
    std::unique_ptr<TcpSocket> socket(new TcpSocket(loop, std::move(backend)));
    socket->mDPtr->check_loop_thread("adopt_backend");
    socket->mDPtr->set_state(SocketState::kConnected); // adopted backend is already connected
    return socket;
}

void TcpSocket::connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("connect");
    CXXKIT_CHECK(on_connected != nullptr) << "TcpSocket::connect requires a connected callback";
    CXXKIT_CHECK(d->mState == SocketState::kIdle)
        << "TcpSocket::connect: state must be kIdle (got " << static_cast<int>(d->mState) << ")";
    CXXKIT_CHECK(d->mCloseRequested == false) << "TcpSocket::connect: socket is closing/closed";

    d->mOnConnect = std::move(on_connected);
    d->set_state(SocketState::kConnecting);
    d->mBackend->connect(ip, port, [d](bool ok) { TcpSocketPrivate::connect_done(d, ok); });
}

void TcpSocketPrivate::connect_done(TcpSocketPrivate *d, bool ok)
{
    // Backend completion trampoline: the failed-connect state contract lives here in the pimpl.
    // The machine returns to kIdle (handle-less, retryable) once the async teardown lands; when
    // close was requested externally the machine stays on the kClosing→kClosed path instead.
    const bool closed = d->mCloseRequested;
    if (ok && d->mState == SocketState::kConnecting)
    {
        d->set_state(SocketState::kConnected);
    }
    else if (!closed)
    {
        d->report_error(network::detail::map_transport_error(d->mBackend->native_status()),
                        "TcpSocket: connect failed (native status " + std::to_string(d->mBackend->native_status()) +
                            ")");
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
        // Failed connect without an explicit close: tear the transport down, then reset the machine
        // to kIdle — the failed-connect state contract (handle-less, retryable; the T2 test pins
        // s.state() == kIdle after a refused connect). begin_close fires kClosing, and the async
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

    d->mBackend->write(data, len, std::move(on_written));
}

void TcpSocket::read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data)
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("read_start");
    CXXKIT_CHECK(on_data != nullptr) << "TcpSocket::read_start requires a data callback";
    CXXKIT_CHECK(d->mCloseRequested == false) << "TcpSocket::read_start: socket is closing/closed";
    CXXKIT_CHECK(d->mState == SocketState::kConnected)
        << "TcpSocket::read_start: state must be kConnected (got " << static_cast<int>(d->mState) << ")";

    d->mOnData = std::move(on_data); // re-arm replaces the callback (interest update, F7-① shape)
    d->mBackend->read_start([d](const uint8_t *data, ssize_t nread) { TcpSocketPrivate::read_event(d, data, nread); });
}

void TcpSocketPrivate::read_event(TcpSocketPrivate *d, const uint8_t *data, ssize_t nread)
{
    if (nread >= 0)
    {
        if (d->mOnData)
        {
            std::function<void(const uint8_t *data, ssize_t nread)> cb = d->mOnData;
            cb(data, nread);
        }
        return;
    }
    // EOF (kEof) or error: map the failure onto the public error surface (EOF → kEof, else
    // backend EOF sentinel), deliver the (nullptr, nread) terminal event once, then go through
    // path (error-state stickiness avoidance: everything after is a no-op).
    const SocketError error = (nread == network::detail::kBackendEof)
                                  ? SocketError::kEof
                                  : network::detail::map_transport_error(static_cast<int>(nread));
    d->report_error(error,
                    (error == SocketError::kEof)
                        ? "TcpSocket: end of file (peer closed)"
                        : "TcpSocket: read error (transport status " + std::to_string(nread) + ")");
    if (d->mOnData)
    {
        std::function<void(const uint8_t *data, ssize_t nread)> cb = std::move(d->mOnData);
        d->mOnData = nullptr;
        cb(nullptr, nread);
    }
    d->mBackend->read_stop(); // disarm first, mirroring the original order (stop, then teardown)
    d->begin_close();
}

void TcpSocket::read_stop()
{
    CXXKIT_D(TcpSocket);
    d->check_loop_thread("read_stop");
    d->mBackend->read_stop(); // never armed / already stopped: backend no-ops
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
    // F8-② idempotence: the transport close is asynchronous; a second close on the same handle is
    // use-after-free. mCloseRequested is the latch — every later close() is a no-op.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;
    if (mState == SocketState::kClosed || !mBackend->is_open())
    {
        set_state(SocketState::kClosed); // never initialized (kIdle) — nothing to close
        mBackend->close(nullptr);
        return;
    }
    set_state(SocketState::kClosing);

    // Pending connect completes false as well (discard semantics, symmetric with writes); queued
    // writes are fanned out with false by the backend's close (its lws queue, its fanout duty).
    if (mOnConnect)
    {
        std::function<void(bool ok)> cb = std::move(mOnConnect);
        mOnConnect = nullptr;
        cb(false);
    }
    mOnData = nullptr;
    mBackend->close(
        [this]()
        {
            if (mState != SocketState::kIdle)
            {
                // Failed-connect teardown resets the machine to kIdle before this callback
                // runs (see connect_done); do not overwrite it with kClosed. Normal closes
                // report kClosed here.
                set_state(SocketState::kClosed);
            }
        });
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
