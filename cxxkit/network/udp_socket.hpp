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

// Detail-layer forward declaration for the backend injection bridge (set_backend). A forward
// declaration keeps the private header out of the public surface; the type is abstract and only
// ever passed through a unique_ptr, so no definition is needed here.
namespace cxxkit::network::detail
{
class DgramBackend;
} // namespace cxxkit::network::detail

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class UdpSocketPrivate;

/**
 * @brief Connectionless UDP socket over a @c cxxkit network event loop (D44).
 *
 * The transport is a pluggable datagram backend (network/detail/dgram_backend.hpp): this class
 * owns the state machine and the callback surface; the backend owns the native handle and the
 * raw I/O. State machine (v1): kIdle → kBound → kClosed — no kConnecting/kConnected (a
 * connected-mode UDP is v1.5). A failed @c bind stays kIdle (retryable, TCP failed-connect
 * shape).
 *
 * Unbound sends: sending from kIdle performs a lazy ephemeral bind ("0.0.0.0":0) first — Qt's
 * "unbound socket may send" contract — and auto-transitions kIdle → kBound on success.
 *
 * @c set_on_datagram delivers each datagram's copy on the loop thread (the backend's receive
 * buffer is valid only during its callback; this class hands the user a @c std::string copy,
 * so holding it past the callback is safe).
 *
 * Threading (I1): every method is loop-thread only — violations are fatal. Cross-thread
 * producers must serialize through @c EventLoop::post() explicitly.
 *
 * Lifecycle (F8-②): @c close() is idempotent — the transport close is asynchronous and
 * double-closing the same handle is use-after-free. The destructor closes if needed and pumps
 * the loop until the close callback has run, so no transport state outlives the object.
 */
class CXXKIT_NETWORK_API UdpSocket
{
public:
    /**
     * @brief Creates a socket bound to @p loop 's engine.
     *
     * The transport is created immediately (unlike TcpSocket, UDP has no lazy connect point —
     * bind needs a handle). Loop thread only.
     */
    explicit UdpSocket(EventLoop &loop);

    /** @brief Closes (if needed) and pumps the loop until the close callback ran (I6). Loop thread only. */
    ~UdpSocket();

    /**
     * @brief Binds to @p ip : @p port. Synchronous — UDP bind has no handshake.
     *
     * Legal from kIdle only. Pass @c port 0 to let the OS assign an ephemeral port; read it
     * back with @c bound_port(). Returns false on failure (address in use, invalid address,
     * ...) — the socket stays kIdle and @c last_error()/@c set_on_error carry the reason; the
     * socket may be retried. Loop thread only.
     */
    bool bind(const std::string &ip, uint16_t port);

    /**
     * @brief The port the socket is actually bound to.
     *
     * Fatal unless the socket is kBound (mirror of the connected-state-only API contract).
     * Loop thread only.
     */
    uint16_t bound_port() const;

    /**
     * @brief Sends @p len bytes to @p ip : @p port; @p on_done fires with the outcome.
     *
     * Legal from kIdle (a lazy ephemeral bind happens first) and kBound. @p data is copied
     * before returning. The callback is invoked on the loop thread with @c false on failure —
     * @c last_error() carries the mapped reason (@c kMessageTooLarge for oversized datagrams).
     * Re-setting replaces the previous callback (the previous one is dropped un-invoked).
     * Loop thread only.
     */
    void send_to(const uint8_t *data,
                 size_t len,
                 const std::string &ip,
                 uint16_t port,
                 std::function<void(bool ok)> on_done);

    /** @brief @c std::string overload of @ref send_to. Loop thread only. */
    void send_to(const std::string &data, const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done);

    /**
     * @brief Arms receive interest; every arriving datagram is delivered as a copy on the loop
     *        thread.
     *
     * Legal from kIdle and kBound. Re-setting replaces the callback (callback swap, backend
     * re-arm shape). Loop thread only.
     */
    void set_on_datagram(
        std::function<void(const std::string &data, const std::string &sender_ip, uint16_t sender_port)> on_datagram);

    /**
     * @brief Closes the socket. Idempotent (F8-②) — subsequent calls are no-ops.
     *
     * Pending send completions fire with @c false (discard semantics); the datagram callback is
     * dropped. State becomes kClosed immediately (the uv close itself drains in the backend).
     * Loop thread only.
     */
    void close();

    /**
     * @brief Injects a whole pre-built backend (TlsSocket @c set_transport shape: test
     *        injection + alternative transports).
     *
     * Legal from kIdle only, before any bind/send. The socket takes exclusive ownership of the
     * backend — exactly one owner per native handle. The backend's loop must be @p loop this
     * socket was constructed with (checked). Loop thread only. @note detail-layer escape hatch:
     * callers outside cxxkit::network should treat the signature as unstable.
     */
    void set_backend(std::unique_ptr<network::detail::DgramBackend> backend);

    /**
     * @brief Sets the error callback: invoked on the loop thread with a mapped @ref SocketError
     *        and a human-readable message on bind/send failure or a receive error.
     *
     * The callback is invoked through a local copy (PIT-40): it may close() or even destroy the
     * socket. Re-setting replaces the previous callback. Loop thread only.
     */
    void set_on_error(std::function<void(SocketError error, const std::string &message)> on_error);

    /**
     * @brief Sets the state-change callback: invoked on the loop thread on every entry into
     *        kIdle / kBound / kClosed.
     *
     * The callback is invoked through a local copy (PIT-40). Re-setting replaces the previous
     * callback. Loop thread only.
     */
    void set_on_state_change(std::function<void(SocketState state)> on_state_change);

    /** @brief Current machine state (kIdle / kBound / kClosed). Loop thread only. */
    SocketState state() const;

    /** @brief Last mapped error, kNone until the first failure. Loop thread only. */
    SocketError last_error() const;

private:
    CXXKIT_DECLARE_PRIVATE(UdpSocket)
    CXXKIT_DEFINE_DPTR(UdpSocket)
    CXXKIT_DISABLE_COPY_MOVE(UdpSocket)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
