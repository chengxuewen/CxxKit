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

#include <cxxkit/network/network_global.hpp>

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>

// Detail-layer forward declaration for the server accept bridge (adopt_backend). A forward
// declaration keeps the private header out of the public surface; the type is abstract and only
// ever passed through a unique_ptr, so no definition is needed here.
namespace cxxkit::network::detail
{
class StreamBackend;
} // namespace cxxkit::network::detail

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class TcpSocketPrivate;

/**
 * @brief TCP stream over a @c cxxkit network event loop — memcached-style state machine.
 *
 * The transport is a pluggable backend (network/detail/stream_backend.hpp): this class owns the
 * state machine and the callback surface; the backend owns the native handle and the raw I/O.
 * Read and write run duplex on a connected socket (the memcached kReading/kWriting bits collapse
 * into "connected + read armed / write queued" sub-interests rather than distinct machine states).
 *
 * Write backpressure (lws discipline): at most one transport write is in flight. Further @c write
 * calls queue and are submitted strictly from the previous write's completion callback, preserving
 * order. @c on_written(false) is the discard signal.
 *
 * Threading (I1): every method is loop-thread only — violations are fatal. Cross-thread producers
 * must serialize through @c EventLoop::post() explicitly; this class deliberately does not do it
 * for them (stricter than the porting-scan allowance, clearer contract).
 *
 * Lifecycle (I3/I6): @c close() is idempotent (F8-②) — a second call is a no-op because the
 * transport close is asynchronous and double-closing the same handle is use-after-free. Pending
 * writes at close time are completed with @c on_written(false) (discard semantics). The destructor
 * closes if needed and pumps the loop until the close callback has run, so no transport state
 * outlives the object (ASAN-clean).
 */
class CXXKIT_NETWORK_API TcpSocket
{
public:
    /**
     * @brief Creates a socket bound to @p loop 's uv engine.
     *
     * The uv handle is initialized lazily at connect/adopt time. Loop thread only.
     */
    explicit TcpSocket(EventLoop &loop);
    /// Server-accept bridge ctor: takes over a whole client backend (detail-layer only).
    TcpSocket(EventLoop &loop, std::unique_ptr<network::detail::StreamBackend> backend);

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
     * Legal from kConnected. @c nread > 0 delivers @c (data, nread); @c nread == 0 is EAGAIN/no-data (interest stays armed)
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
     * the transport close; state becomes kClosing then kClosed when the close callback runs. Loop thread only.
     */
    void close();

    /**
     * @brief Sets the error callback: invoked on the loop thread with a mapped @ref SocketError
     *        and a human-readable message when a connect failure or read error occurs.
     *
     * The callback is invoked through a local copy (PIT-40): it may close() or even destroy the
     * socket. Re-setting replaces the previous callback. Loop thread only.
     */
    void set_on_error(std::function<void(SocketError error, const std::string &message)> on_error);

    /**
     * @brief Sets the state-change callback: invoked on the loop thread on every entry into
     *        kIdle / kConnecting / kConnected / kClosing / kClosed. kIdle recurs after a failed
     *        connect (handle-less, retryable) and IS reported; it is only absent before the first
     *        user-visible transition.
     *
     * The callback is invoked through a local copy (PIT-40). Re-setting replaces the previous
     * callback. Loop thread only.
     */
    void set_on_state_change(std::function<void(SocketState state)> on_state_change);

    /** @brief Current machine state (kIdle..kClosed). Loop thread only. */
    SocketState state() const;

    /** @brief Last mapped error, kNone until the first failure. Loop thread only. */
    SocketError last_error() const;

    /** @brief True while the socket is not closing/closed (kIdle..kConnected). */
    bool is_open() const;

    /**
     * @brief Adopts an already-connected fd (e.g. an accepted socket) into a TcpSocket (R-T2-2).
     *
     * directly. The fd is owned by the native handle from here on — do not close it externally. For
     * phase-2 test rigging (socketpair peers) and TcpServer accept (T3, F10 preview). Loop thread only.
     */
    static std::unique_ptr<TcpSocket> adopt_fd(EventLoop &loop, int fd);

    static std::unique_ptr<TcpSocket> adopt_native(EventLoop &loop, void *native_handle);

    /**
     * @brief Takes over a whole backend produced by another backend's accept path (R-T3-1).
     *
     * TcpServer's accept path only: the server's backend creates a client backend over the
     * accepted native handle and hands it here WHOLE — exactly one backend ever owns a given
     * native handle. Enters kConnected directly; the backend's loop must be @p loop.
     * Loop thread only. @note detail-layer escape hatch: exposed for the server bridge; callers
     * outside cxxkit::network should treat the signature as unstable.
     */
    static std::unique_ptr<TcpSocket> adopt_backend(EventLoop &loop,
                                                    std::unique_ptr<network::detail::StreamBackend> backend);

private:
    CXXKIT_DECLARE_PRIVATE(TcpSocket)
    CXXKIT_DEFINE_DPTR(TcpSocket)
    CXXKIT_DISABLE_COPY_MOVE(TcpSocket)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
