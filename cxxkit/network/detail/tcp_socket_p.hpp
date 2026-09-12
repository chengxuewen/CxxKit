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

#include <cxxkit/network/tcp_socket.hpp>

#include <cxxkit/base/macros.hpp>
#include <cxxkit/kernel/event_loop.hpp> // defines the kernel guard that socket_error.hpp gates on
#include <cxxkit/network/detail/stream_backend.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>

#include <functional>
#include <memory>
#include <string>
#include <thread>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/** @brief Private implementation of @ref TcpSocket (CXXKIT_DEFINE_DPTR pimpl partner, flat namespace). */
class TcpSocketPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(TcpSocketPrivate)

public:
    explicit TcpSocketPrivate(TcpSocket *p, EventLoop &loop);
    /// Takes over a pre-built backend (accept path: the server's client backend moves in whole —
    /// no second backend is ever created over the same native handle).
    explicit TcpSocketPrivate(TcpSocket *p, std::unique_ptr<network::detail::StreamBackend> backend);
    ~TcpSocketPrivate();

    // The machine states are the public SocketState (socket_state.hpp) — no private enum anymore.
    // Lifecycle: kIdle → kConnecting → kConnected → kClosing → kClosed; a failed connect returns
    // to kIdle (the handle-less constructed state; connect is retryable).

    /** @brief I1 fatal: every public entry is loop-thread only (R-B2-5 shape, always-on). */
    void check_loop_thread(const char *api) const;

    /** @brief Sets @p state and notifies mOnStateChange (local-copy invoke, PIT-40). */
    void set_state(SocketState state);

    /** @brief Records @p error as mLastError and notifies mOnError (local-copy invoke, PIT-40). */
    void report_error(SocketError error, const std::string &message);

    /** @brief Shared teardown: discard pending callbacks via the backend, latch the close. Idempotent. */
    void begin_close();

    // Backend completion trampolines: the state-machine reactions to transport events live HERE
    // (pimpl side) — the backend calls back with plain data, no state knowledge.
    static void connect_done(TcpSocketPrivate *d, bool ok);
    static void read_event(TcpSocketPrivate *d, const uint8_t *data, ssize_t nread);

    TcpSocket *mP{nullptr};
    EventLoop &mLoop;
    std::unique_ptr<network::detail::StreamBackend> mBackend{network::detail::make_stream_backend()};

    SocketState mState{SocketState::kIdle};
    bool mCloseRequested{false}; /// F8-② idempotence guard (pimpl-owned state-machine latch)
    std::function<void(bool ok)> mOnConnect;
    std::function<void(const uint8_t *data, ssize_t nread)> mOnData;
    std::function<void(SocketError, const std::string &)> mOnError; /// invoked via local copy (PIT-40)
    std::function<void(SocketState)> mOnStateChange;                /// invoked via local copy (PIT-40)
    SocketError mLastError{SocketError::kNone};                     /// last mapped failure, kNone until first error

    std::thread::id mLoopThreadId; /// captured at construction from the loop's dispatcher
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
