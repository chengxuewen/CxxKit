/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in without restriction, including without limitation
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

#include <cxxkit/uv/tcp_socket.hpp>

#include <cxxkit/base/macros.hpp>
#include <cxxkit/uv/uv_event_dispatcher.hpp>

#include <cxxkit/3rdparty/libuv/uv.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <thread>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/** @brief Private implementation of @ref TcpSocket (CXXKIT_DEFINE_DPTR pimpl partner, flat namespace). */
class TcpSocketPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(TcpSocketPrivate)

public:
    explicit TcpSocketPrivate(TcpSocket *p, EventLoop &loop);
    ~TcpSocketPrivate();

    /** @brief memcached-style machine states; kConnected carries duplex read/write sub-interests. */
    enum class State
    {
        kIdle,       /// constructed; no handle yet
        kConnecting, /// uv_tcp_connect in flight
        kConnected,  /// connected (or adopted fd); read arm / write queue are sub-interests
        kClosing,    /// uv_close requested; callbacks still draining (F8-② window)
        kClosed      /// close callback ran; nothing pending
    };

    /** @brief One queued transmission: the copied bytes + its completion callback (lws backpressure). */
    struct PendingWrite
    {
        std::vector<uint8_t> mData;
        std::function<void(bool ok)> mOnWritten;
    };

    // uv C callbacks (static trampolines) — handle->data routes back to the pimpl (Node tcp_wrap pattern).
    static void on_connect_done(uv_connect_t *req, int status);
    static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void on_write_done(uv_write_t *req, int status);
    static void on_closed(uv_handle_t *handle);

    /** @brief I1 fatal: every public entry is loop-thread only (R-B2-5 shape, always-on). */
    void check_loop_thread(const char *api) const;

    /**
     * @brief Shared pimpl assembly for the two adopt entries (R-T3-1): binds an already-connected
     * handle (fresh uv_tcp_open result, or a server-accepted uv_tcp_t) to this socket's state machine.
     * Takes ownership of @p handle on every path that does not abort.
     */
    void attach_connected_handle(uv_tcp_t *handle);

    /** @brief Submits the front of pending_writes (at most one uv_write in flight). */
    void submit_next_write();

    /** @brief Shared teardown: flush pending callbacks, stop reading, uv_close. Idempotent via mCloseRequested. */
    void begin_close();

    TcpSocket *mP{nullptr};
    EventLoop &mLoop;
    UvEventDispatcher *mDispatcher{nullptr};
    uv_tcp_t *mHandle{nullptr};         /// heap cell; freed in on_closed
    uv_connect_t *mConnectReq{nullptr}; /// heap cell; freed in on_connect_done
    uv_write_t *mWriteReq{nullptr};     /// in-flight write's req; freed in on_write_done
    uv_buf_t mWriteBuf{nullptr, 0};     /// in-flight write's buffer view (storage owned by mInFlight)
    std::vector<uint8_t> mInFlight;     /// storage for the in-flight uv_write (must outlive the req)

    State mState{State::kIdle};
    bool mCloseRequested{false};             /// F8-② idempotence guard
    bool mReadArmed{false};                  /// read sub-interest (uv_read_start/stop armed state)
    std::deque<PendingWrite> mPendingWrites; /// queued while a write is in flight (FIFO, lws)
    std::function<void(bool ok)> mOnConnect;
    std::function<void(bool ok)> mOnWrittenCurrent; /// in-flight write's callback; handed back in on_write_done
    std::function<void(const uint8_t *data, ssize_t nread)> mOnData;

    std::vector<uint8_t> mReadBuf; /// beast flat_buffer shape: one contiguous block + uv fills from the front
    std::thread::id mLoopThreadId; /// captured at construction from the loop's dispatcher
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
