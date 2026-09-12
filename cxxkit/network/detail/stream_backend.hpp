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

#include <cxxkit/kernel/event_loop.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/// Read-callback end-of-file sentinel: a backend delivers @c (nullptr, kBackendEof) once when the
/// peer closed the stream cleanly. Backends translate their native EOF (uv: UV_EOF enum) onto this.
constexpr ssize_t kBackendEof = -4095; // uv's UV_EOF value (enum, not a macro — vendored 1.49.2)

/**
 * @brief Backend-neutral stream transport interface (Task 3, D41): connect/write/read/accept/close
 *        plus the drain-pump destructor discipline.
 *
 * The pimpl layer (TcpSocketPrivate/TcpServerPrivate) owns the state machine, thread-affinity
 * checks, error mapping and the mOnError/mOnStateChange triggers; a StreamBackend does I/O only.
 * Contracts carried over verbatim from today's uv implementation:
 * - single in-flight write with FIFO queue (lws backpressure) is a backend concern;
 * - @c close is idempotent and async under uv — its synchronous false-fanout over queued writes
 *   and the pending connect is the backend's responsibility;
 * - @c pump_until_closed covers ALL THREE existing drain loops (~TcpSocket `rounds < 1000`,
 *   ~TcpServer `mHandle != nullptr`, listen-failure teardown) — no uv state outlives the backend.
 * - @c adopt_native takes a backend-owned native handle (uv_tcp_t* under uv); @c adopt_fd adopts
 *   a raw fd (uv_tcp_open shape; socket assign under asio).
 */
class StreamBackend
{
public:
    virtual ~StreamBackend() = default;

    // client face
    virtual bool open(EventLoop &loop) = 0;
    virtual void connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done) = 0;
    virtual void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done) = 0;
    virtual void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data) = 0;
    virtual void read_stop() = 0;
    virtual void close(std::function<void()> on_closed) = 0; // idempotent; async under uv
    virtual bool is_open() const = 0;

    // server face
    virtual bool listen(const std::string &ip, uint16_t port, int backlog) = 0;
    virtual uint16_t bound_port() const = 0;
    virtual void set_on_accept(std::function<void(std::unique_ptr<StreamBackend> client)> on_accept) = 0;

    /**
     * @brief The loop this backend is bound to (set by @c open / @c adopt_native / @c adopt_fd).
     */
    virtual EventLoop &loop() const = 0;

    // Last native failure status of the most recent completed operation (0 = none/success).
    // Backend-neutral granularity: uv uses the negative errno-style uv status, asio would use the
    // std::error_code value. The pimpl maps it onto SocketError (error_mapping.hpp).
    virtual int native_status() const = 0;

    // native handle view (void* — backend-owned type). The accept path hands backends WHOLE
    // (adopt_backend), so nothing outside the backend ever needs the raw handle; kept as a
    // debugging aid only.
    virtual void *native_handle() const = 0;


    // native handle adopt/release (void* — backend-owned type)
    virtual bool adopt_native(void *native_handle, EventLoop &loop) = 0;
    // adopt entries (Momus F2: fd adopt REQUIRED — public adopt_fd keeps tests green; grep gate depends on it)
    virtual bool adopt_fd(int fd, EventLoop &loop) = 0; // uv: init+uv_tcp_open; asio: socket.assign

    // dtor discipline: pump the loop until the native close callback ran
    // (covers ALL THREE existing pump loops: ~TcpSocket, ~TcpServer, listen-failure teardown)
    virtual void pump_until_closed() = 0;
};

/** @brief Compiled per backend selection (uv today; asio lands as another definition). */
std::unique_ptr<StreamBackend> make_stream_backend();

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
