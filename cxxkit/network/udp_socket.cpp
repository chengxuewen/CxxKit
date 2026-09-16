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
#include <cxxkit/network/detail/udp_socket_p.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/udp_socket.hpp>

#include <cxxkit/tools/checks.hpp>

#include <string>
#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

UdpSocketPrivate::UdpSocketPrivate(UdpSocket *p, EventLoop &loop)
    : mP(p)
    , mLoop(loop)
    , mBackend(network::detail::make_dgram_backend())
{
    mBackend->open(loop);                       // binds the backend to this loop's engine
    mLoopThreadId = std::this_thread::get_id(); // construction is loop-thread only; pin the check id
}

UdpSocketPrivate::UdpSocketPrivate(UdpSocket *p, std::unique_ptr<network::detail::DgramBackend> backend)
    : mP(p)
    , mLoop(backend->loop())
    , mBackend(std::move(backend))
{
    mLoopThreadId = std::this_thread::get_id(); // construction is loop-thread only; pin the check id
}

UdpSocketPrivate::~UdpSocketPrivate()
{
    // I6: nothing should reach here with a live handle — the public destructor closes and pumps.
    CXXKIT_CHECK(mCloseRequested || mState == SocketState::kIdle)
        << "UdpSocketPrivate: backend outlived the drain pump (teardown bug)";
}

void UdpSocketPrivate::check_loop_thread(const char *api) const
{
    // I1: always-on, not debug-only (TcpSocket shape). One thread-id compare per entry.
    if (std::this_thread::get_id() != mLoopThreadId)
    {
        CXXKIT_FATAL() << "UdpSocket::" << api << " called from a non-loop thread (loop thread "
                       << std::hash<std::thread::id>()(mLoopThreadId) << ", caller thread "
                       << std::hash<std::thread::id>()(std::this_thread::get_id())
                       << "). Serialize with EventLoop::post() from other threads instead.";
    }
}

void UdpSocketPrivate::set_state(SocketState state)
{
    mState = state;
    if (mOnStateChange)
    {
        // PIT-40: the callback may close() or even destroy the socket — invoke a local copy.
        std::function<void(SocketState)> cb = mOnStateChange;
        cb(state);
    }
}

void UdpSocketPrivate::report_error(SocketError error, const std::string &message)
{
    mLastError = error;
    if (mOnError)
    {
        // PIT-40: same discipline — the callback may tear the socket down mid-invoke.
        std::function<void(SocketError, const std::string &)> cb = mOnError;
        cb(error, message);
    }
}

bool UdpSocketPrivate::lazy_bind_for_send()
{
    // Unbound send = implicit ephemeral bind (controller ruling on plan Step 4.1 case 4 —
    // Qt's unbound-send contract). Failure leaves the machine kIdle; the caller maps the error.
    return mBackend->bind("0.0.0.0", 0);
}

void UdpSocketPrivate::send_done(UdpSocketPrivate *d, bool ok)
{
    // Backend completion trampoline: send failure maps onto the public error surface here.
    // Close-requested sends fire false via begin_close's fanout (in-flight uv requests complete
    // UV_ECANCELED after close and also flow through here with false) — anything reaching this
    // trampoline belongs to a live machine.
    if (!ok)
    {
        const int status = d->mBackend->native_status();
        d->report_error(network::detail::map_transport_error(status),
                        "UdpSocket: send failed (native status " + std::to_string(status) + ")");
    }
    if (!d->mPendingSendDones.empty())
    {
        // FIFO: the backend fires completions strictly in submission order per socket — pop
        // the oldest. PIT-40: invoke through a local copy (the callback may close()/destroy).
        std::function<void(bool ok)> cb = std::move(d->mPendingSendDones.front());
        d->mPendingSendDones.pop_front();
        if (cb)
        {
            cb(ok);
        }
    }
}

void UdpSocketPrivate::datagram_event(UdpSocketPrivate *d,
                                      const std::string &data,
                                      const std::string &ip,
                                      uint16_t port)
{
    // Backend trampoline: the backend hands us copies already (its buffer dies with the
    // callback); we deliver the user's copy — holding data past this call is safe.
    if (d->mOnDatagram)
    {
        // PIT-40: invoke through a local copy — the callback may close()/destroy the socket.
        std::function<void(const std::string &, const std::string &, uint16_t)> cb = d->mOnDatagram;
        cb(data, ip, port);
    }
}

UdpSocket::UdpSocket(EventLoop &loop)
    : mDPtr(new UdpSocketPrivate(this, loop))
{
}

UdpSocket::~UdpSocket()
{
    CXXKIT_D(UdpSocket);
    // I6: close (idempotent — a no-op if already requested), then pump the loop until the close
    // callback has actually run so no transport state (handle, in-flight sends) survives us.
    // All user callbacks are cleared BEFORE the backend teardown (PIT-40: nothing user-visible
    // can fire during the drain) — including every queued send completion (begin_close would
    // fan them out, but a never-submitted pending edge must not fire into a dying socket).
    d->mPendingSendDones.clear();
    d->mOnDatagram = nullptr;
    d->mOnError = nullptr;
    d->mOnStateChange = nullptr;
    if (d->mState != SocketState::kClosed && d->mState != SocketState::kIdle)
    {
        d->begin_close();
    }
    d->mBackend->close();
    // Drain: the dgram backend has no pump_until_closed on its interface — the uv close
    // callback frees the handle, so pump the loop until native_handle() reports released.
    for (int rounds = 0; rounds < 1000 && d->mBackend->native_handle() != nullptr; ++rounds)
    {
        d->mLoop.process_events(EventLoop::ProcessFlag::kAllEvents);
    }
}

bool UdpSocket::bind(const std::string &ip, uint16_t port)
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("bind");
    CXXKIT_CHECK(d->mCloseRequested == false) << "UdpSocket::bind: socket is closing/closed";
    CXXKIT_CHECK(d->mState == SocketState::kIdle)
        << "UdpSocket::bind: state must be kIdle (got " << static_cast<int>(d->mState) << ")";

    if (!d->mBackend->bind(ip, port))
    {
        // Failed bind stays kIdle (handle-less, retryable — TCP failed-connect shape): the
        // backend released its handle on the failure path; map and surface the reason.
        const int status = d->mBackend->native_status();
        d->report_error(network::detail::map_transport_error(status),
                        "UdpSocket: bind failed (native status " + std::to_string(status) + ")");
        return false;
    }
    d->set_state(SocketState::kBound);
    return true;
}

uint16_t UdpSocket::bound_port() const
{
    CXXKIT_D(const UdpSocket);
    d->check_loop_thread("bound_port");
    CXXKIT_CHECK(d->mState == SocketState::kBound)
        << "UdpSocket::bound_port: state must be kBound (got " << static_cast<int>(d->mState) << ")";
    return d->mBackend->bound_port();
}

void UdpSocket::send_to(const uint8_t *data,
                        size_t len,
                        const std::string &ip,
                        uint16_t port,
                        std::function<void(bool ok)> on_done)
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("send_to");
    CXXKIT_CHECK(on_done != nullptr) << "UdpSocket::send_to requires a done callback";
    CXXKIT_CHECK(data != nullptr || len == 0) << "UdpSocket::send_to: null data with non-zero length";
    if (d->mCloseRequested || d->mState == SocketState::kClosed)
    {
        on_done(false); // closed socket: immediate discard completion, documented semantics
        return;
    }
    CXXKIT_CHECK(d->mState == SocketState::kIdle || d->mState == SocketState::kBound)
        << "UdpSocket::send_to: state must be kIdle or kBound (got " << static_cast<int>(d->mState) << ")";

    if (d->mState == SocketState::kIdle)
    {
        // Lazy ephemeral bind (controller ruling: unbound send is legal, Qt contract).
        if (!d->lazy_bind_for_send())
        {
            const int status = d->mBackend->native_status();
            d->report_error(network::detail::map_transport_error(status),
                            "UdpSocket: implicit bind before send failed (native status " + std::to_string(status) +
                                ")");
            on_done(false);
            return;
        }
        d->set_state(SocketState::kBound); // auto-transition kIdle → kBound
    }

    // FIFO completion queue (TlsSocket mPendingWrites precedent): each send pushes its done-
    // callback; the backend fires completions strictly in submission order per socket (uv
    // preserves FIFO), so send_done pops front-first. No in-flight limit (uv supports
    // concurrent uv_udp_send requests natively).
    d->mPendingSendDones.push_back(std::move(on_done));
    d->mBackend->send_to(data, len, ip, port, [d](bool ok) { UdpSocketPrivate::send_done(d, ok); });
}

void UdpSocket::send_to(const std::string &data,
                        const std::string &ip,
                        uint16_t port,
                        std::function<void(bool ok)> on_done)
{
    this->send_to(reinterpret_cast<const uint8_t *>(data.data()), data.size(), ip, port, std::move(on_done));
}

void UdpSocket::set_on_datagram(
    std::function<void(const std::string &data, const std::string &sender_ip, uint16_t sender_port)> on_datagram)
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("set_on_datagram");
    CXXKIT_CHECK(d->mCloseRequested == false) << "UdpSocket::set_on_datagram: socket is closing/closed";
    CXXKIT_CHECK(d->mState == SocketState::kIdle || d->mState == SocketState::kBound)
        << "UdpSocket::set_on_datagram: state must be kIdle or kBound (got " << static_cast<int>(d->mState) << ")";

    // C1 (review wave 1): receive_start needs a live handle — arming from kIdle performs the
    // same lazy ephemeral bind the send path does. Failure stays kIdle (re-arm allowed later).
    if (d->mState == SocketState::kIdle)
    {
        if (!d->lazy_bind_for_send())
        {
            const int status = d->mBackend->native_status();
            d->report_error(network::detail::map_transport_error(status),
                            "UdpSocket: implicit bind before receive failed (native status " + std::to_string(status) +
                                ")");
            return;
        }
        d->set_state(SocketState::kBound); // auto-transition kIdle → kBound
    }

    d->mOnDatagram = std::move(on_datagram);
    // The pimpl owns the user callback; the backend gets a trampoline into it (PIT-40: the
    // lambda's copies die before anything on our side is referenced after the call). Re-setting
    // the user callback just swaps mOnDatagram — the trampoline picks it up (re-arm shape).
    d->mBackend->receive_start(
        [d](const uint8_t *data, size_t len, const std::string &ip, uint16_t port)
        {
            // Copy discipline: the backend's buffer is valid only during this callback — copy
            // before delivering (dgram_backend.hpp contract).
            d->datagram_event(d, std::string(reinterpret_cast<const char *>(data), len), ip, port);
        });
}

void UdpSocket::close()
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("close");
    d->begin_close();
}

void UdpSocket::set_backend(std::unique_ptr<network::detail::DgramBackend> backend)
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("set_backend");
    CXXKIT_CHECK(d->mState == SocketState::kIdle) << "UdpSocket::set_backend: state must be kIdle";
    CXXKIT_CHECK(backend != nullptr) << "UdpSocket::set_backend: null backend";
    CXXKIT_CHECK(&backend->loop() == &d->mLoop) << "UdpSocket::set_backend: backend is bound to a different loop";
    d->mBackend = std::move(backend);
}

void UdpSocket::set_on_error(std::function<void(SocketError, const std::string &)> on_error)
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("set_on_error");
    d->mOnError = std::move(on_error);
}

void UdpSocket::set_on_state_change(std::function<void(SocketState)> on_state_change)
{
    CXXKIT_D(UdpSocket);
    d->check_loop_thread("set_on_state_change");
    d->mOnStateChange = std::move(on_state_change);
}

SocketState UdpSocket::state() const
{
    CXXKIT_D(const UdpSocket);
    d->check_loop_thread("state");
    return d->mState;
}

SocketError UdpSocket::last_error() const
{
    CXXKIT_D(const UdpSocket);
    d->check_loop_thread("last_error");
    return d->mLastError;
}

void UdpSocketPrivate::begin_close()
{
    // F8-② idempotence: the transport close is asynchronous; a second close on the same handle
    // is use-after-free. mCloseRequested is the latch — every later close() is a no-op.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;

    // Pending sends complete false right here (discard semantics, symmetric with TcpSocket
    // writes) — covers the never-submitted queue edge too; in-flight uv requests additionally
    // flow through send_done with UV_ECANCELED → their callbacks were already fired here, and
    // the queue is empty by then (send_done pops, finds nothing, is inert). The datagram
    // callback is dropped with the socket.
    for (auto &cb : mPendingSendDones)
    {
        if (cb)
        {
            cb(false);
        }
    }
    mPendingSendDones.clear();
    mOnDatagram = nullptr;
    mBackend->close();
    set_state(SocketState::kClosed); // uv close drains in the backend; the machine is terminal now
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
