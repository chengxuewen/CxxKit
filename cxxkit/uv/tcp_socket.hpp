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

#include <cxxkit/uv/uv_global.hpp>

#include <cxxkit/kernel/event_loop.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class TcpSocketPrivate;

/**
 * @brief TCP stream over a @c cxxkit::uv event loop — memcached-style state machine (phase-2 core IO).
 *
 * Wraps one @c uv_tcp_t directly (Node tcp_wrap pattern): the uv callbacks trampoline back into this
 * class via @c handle->data, drive a state machine, and re-arm interest on every transition. The
 * memcached poll-mask discipline maps onto libuv's stream API as follows: read interest is the
 * @c uv_read_start/@c uv_read_stop pair (a level-triggered readable watch) and write interest is the
 * pending-@c uv_write chain (an implicit writable watch that fires exactly once per queued buffer).
 * There is no single mask to re-attach; each direction arms independently, which preserves the
 * discipline's intent — the loop never watches a direction the state machine did not ask for.
 *
 * States: @c kIdle → @c kConnecting → @c kConnected → @c kClosing → @c kClosed. Read and write run
 * duplex on a connected socket (the memcached kReading/kWriting bits collapse into "connected +
 * read_start armed / write queued" sub-interests rather than distinct machine states).
 *
 * Write backpressure (lws discipline): at most one @c uv_write is in flight. Further @c write calls
 * queue in @c pending_writes and are submitted strictly from the previous write's completion
 * callback, preserving order. @c on_written(false) is the discard signal.
 *
 * Threading (I1): every method is loop-thread only — violations are fatal. Cross-thread producers
 * must serialize through @c EventLoop::post() explicitly; this class deliberately does not do it
 * for them (stricter than the porting-scan allowance, clearer contract).
 *
 * Lifecycle (I3/I6): @c close() is idempotent (F8-②) — a second call is a no-op because
 * @c uv_close is asynchronous and double-closing the same handle is use-after-free. Pending writes
 * at close time are completed with @c on_written(false) (discard semantics). The destructor closes
 * if needed and pumps the loop until the close callback has run, so no uv state outlives the object
 * (ASAN-clean).
 */
class CXXKIT_UV_API TcpSocket
{
public:
    /**
     * @brief Creates a socket bound to @p loop 's uv engine.
     *
     * The uv handle is initialized lazily at connect/adopt time. Loop thread only.
     */
    explicit TcpSocket(EventLoop &loop);

    /** @brief Closes (if needed) and pumps the loop until the close callback ran (I6). Loop thread only. */
    ~TcpSocket();

    /**
     * @brief Starts a non-blocking connect; @p on_connected reports success on the loop thread.
     *
     * Legal from kIdle. The callback is one-shot: invoked once with @c true, or once with @c false
     * on connect failure / close-during-connect (discard semantics), then dropped.
     */
    void connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected);

    /**
     * @brief Queues @p len bytes for transmission; @p on_written fires when the bytes are written
     *        out (or dropped, with @c false).
     *
     * Legal from kConnected. Backpressure (lws): while a write is in flight, further writes queue
     * in FIFO order; each is submitted from its predecessor's completion callback. @p data is
     * copied before returning. Loop thread only — a cross-thread call is fatal (I1).
     */
    void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written);

    /**
     * @brief Arms read interest; @p on_data delivers received bytes on the loop thread.
     *
     * Legal from kConnected. @c nread > 0 delivers @c (data, nread); @c nread == 0 is Nothing-Eelse
     * (EAGAIN/EWOULDBLOCK) and is NOT delivered; a null pointer with @c nread <= 0 is EOF or error
     * (peer close included) — after which the socket is closed and @c read_start is disarmed. The
     * buffer is valid only during the callback. Loop thread only.
     */
    void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data);

    /** @brief Disarms read interest (the read half of the interest mask). Loop thread only. */
    void read_stop();

    /**
     * @brief Closes the socket. Idempotent (F8-②) — subsequent calls are no-ops.
     *
     * Pending writes are completed with @c on_written(false) (discard semantics); an in-flight
     * connect completes with @c on_connected(false). The uv handle dies asynchronously via
     * @c uv_close; state becomes kClosing then kClosed when the close callback runs. Loop thread only.
     */
    void close();

    /** @brief True while the socket is not closing/closed (kIdle..kConnected). */
    bool is_open() const;

    /**
     * @brief Adopts an already-connected fd (e.g. an accepted socket) into a TcpSocket (R-T2-2).
     *
     * @c uv_tcp_open attaches the fd to a fresh uv handle on the loop; the socket enters kConnected
     * directly. The fd is owned by the uv handle from here on — do not close it externally. For
     * phase-2 test rigging (socketpair peers) and TcpServer accept (T3, F10 preview). Loop thread only.
     */
    static std::unique_ptr<TcpSocket> adopt_fd(EventLoop &loop, int fd);

private:
    CXXKIT_DECLARE_PRIVATE(TcpSocket)
    CXXKIT_DEFINE_DPTR(TcpSocket)
    CXXKIT_DISABLE_COPY_MOVE(TcpSocket)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
