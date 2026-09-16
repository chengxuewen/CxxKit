/***********************************************************************************************************************
**
** Library: CxxKit
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

#pragma once

#include <cxxkit/network/udp_socket.hpp>

#include <cxxkit/base/macros.hpp>
#include <cxxkit/kernel/event_loop.hpp> // defines the kernel guard that socket_error.hpp gates on
#include <cxxkit/network/detail/dgram_backend.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>

#include <functional>
#include <memory>
#include <string>
#include <thread>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/** @brief Private implementation of @ref UdpSocket (CXXKIT_DEFINE_DPTR pimpl partner, flat namespace). */
class UdpSocketPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(UdpSocketPrivate)

public:
    explicit UdpSocketPrivate(UdpSocket *p, EventLoop &loop);
    /// Takes over a pre-built backend (set_backend injection path: the test/backend moves in whole).
    explicit UdpSocketPrivate(UdpSocket *p, std::unique_ptr<network::detail::DgramBackend> backend);
    ~UdpSocketPrivate();

    // The machine states are the public SocketState (socket_state.hpp). UDP lifecycle (v1):
    // kIdle → kBound → kClosed; a failed bind stays kIdle (handle-less, retryable — TCP
    // failed-connect shape). No kConnecting/kConnected in v1 (connected-UDP is v1.5).

    /** @brief I1 fatal: every public entry is loop-thread only (TcpSocket shape, always-on). */
    void check_loop_thread(const char *api) const;

    /** @brief Sets @p state and notifies mOnStateChange (local-copy invoke, PIT-40). */
    void set_state(SocketState state);

    /** @brief Records @p error as mLastError and notifies mOnError (local-copy invoke, PIT-40). */
    void report_error(SocketError error, const std::string &message);

    /** @brief Shared teardown: stop recv via the backend, latch the close, kClosed. Idempotent. */
    void begin_close();

    /**
     * @brief Lazy ephemeral bind for unbound sends (controller ruling on plan Step 4.1 case 4):
     *        first send_to from kIdle binds "0.0.0.0":0 implicitly. @return false on bind failure
     *        (error reported by the caller — the machine stays kIdle).
     */
    bool lazy_bind_for_send();

    // Backend completion trampolines: state-machine reactions live HERE (pimpl side).
    static void send_done(UdpSocketPrivate *d, bool ok);
    static void datagram_event(UdpSocketPrivate *d, const std::string &data, const std::string &ip, uint16_t port);

    UdpSocket *mP{nullptr};
    EventLoop &mLoop;
    std::unique_ptr<network::detail::DgramBackend> mBackend;

    SocketState mState{SocketState::kIdle};
    bool mCloseRequested{false}; /// F8-② idempotence guard (pimpl-owned state-machine latch)
    std::function<void(bool ok)> mOnSendDone;
    std::function<void(const std::string &data, const std::string &ip, uint16_t port)> mOnDatagram;
    std::function<void(SocketError, const std::string &)> mOnError; /// invoked via local copy (PIT-40)
    std::function<void(SocketState)> mOnStateChange;                /// invoked via local copy (PIT-40)
    SocketError mLastError{SocketError::kNone};                     /// last mapped failure, kNone until first error

    std::thread::id mLoopThreadId; /// captured at construction from the loop's dispatcher
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
