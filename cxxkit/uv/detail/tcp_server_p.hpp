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

#include <cxxkit/uv/tcp_server.hpp>

#include <cxxkit/base/macros.hpp>
#include <cxxkit/uv/uv_event_dispatcher.hpp>

#include <cxxkit/3rdparty/libuv/uv.h>

#include <cstdint>
#include <functional>
#include <thread>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/** @brief Private implementation of @ref TcpServer (CXXKIT_DEFINE_DPTR pimpl partner, flat namespace). */
class TcpServerPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(TcpServerPrivate)

public:
    explicit TcpServerPrivate(TcpServer *p, EventLoop &loop);
    ~TcpServerPrivate();

    /** @brief uv connection_cb trampoline: (server, status); accept-drains the backlog. */
    static void on_connection_cb(uv_stream_t *server, int status);
    /** @brief Server handle close callback: frees the heap cell, flags kClosed. */
    static void on_closed(uv_handle_t *handle);
    /** @brief Close callback for discarded bare client cells (accept-fail / no consumer). */
    static void on_discard_closed(uv_handle_t *handle);

    /** @brief I1 fatal: every public entry is loop-thread only. */
    void check_loop_thread(const char *api) const;

    TcpServer *mP{nullptr};
    EventLoop &mLoop;
    UvEventDispatcher *mDispatcher{nullptr};
    uv_tcp_t *mHandle{nullptr};  /// heap cell; freed in on_closed
    bool mCloseRequested{false}; /// F8-② idempotence latch for the server handle
    bool mListening{false};
    uint16_t mBoundPort{0};
    std::function<void(std::unique_ptr<TcpSocket>)> mOnConnection;
    std::thread::id mLoopThreadId;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
