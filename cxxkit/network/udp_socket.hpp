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
 * raw I/O. State machine (v1): kIdle → kBound → kClosed; connected mode (D45) adds a pin:
 * kIdle lazy-binds to kBound, then a synchronous connect flips kBound → kConnected (no
 * kConnecting — UDP connect has no handshake). A failed connect stays kBound (the binding
 * survives). @c disconnect_remote returns kConnected → kBound. A failed @c bind stays kIdle
 * (retryable, TCP failed-connect shape).
 *
 * Connected-mode contract (Qt alignment): while kConnected, @c send() delivers to the pinned
 * peer, @c send_to is fatal (a connected socket sends via @c send only), @c bound_port stays
 * legal, and only peer datagrams are delivered. Connection refusal is asynchronous on most
 * platforms — an ICMP port-unreachable for a datagram sent to a closed port surfaces later as
 * a @c kConnectionRefused error through the send-completion path, NOT from @c connect_to itself.
 * (A refusal arriving while receive is armed stops delivery silently — re-arm via
 * @c set_on_datagram; it does not fire @c on_error.)
 *
 * Unbound first use (send or receive arming): from kIdle the socket lazily binds an IPv4
 * ephemeral endpoint ("0.0.0.0":0) — Qt's "unbound socket may send" contract — and
 * auto-transitions kIdle → kBound on success. Sending to IPv6 destinations from an unbound
 * socket is unsupported in v1 (bind explicitly first).
 *
 * @c set_on_datagram delivers each datagram's copy on the loop thread (the backend's receive
 * buffer is valid only during its callback; this class hands the user a @c std::string copy,
 * so holding it past the callback is safe). While kConnected, only peer datagrams arrive.
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
     * The transport is created immediately — UDP has no lazy connect point, bind needs a
     * handle. Loop thread only.
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
     * Legal from kIdle (a lazy ephemeral bind happens first) and kBound. @p data is copied
     * before returning. Each send's callback is invoked on the loop thread with @c false on
     * failure — @c last_error() carries the mapped reason (@c kMessageTooLarge for oversized
     * datagrams). No in-flight limit; completions arrive in submission order on the uv backend
     * (libuv queues per-handle sends FIFO). The asio backend does not guarantee completion
     * ordering for concurrent async_send_to — treat per-send results as unordered under asio.
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
     * @brief Pins a default peer (connected-mode UDP, D45): kIdle lazily binds an ephemeral
     *        endpoint first, then the peer is attached — synchronous (no handshake).
     *
     * Legal from kIdle and kBound only (fatal from kConnected — use disconnect first; fatal
     * when closing/closed). On success the machine is kConnected: @c send() goes to @p ip : @p port,
     * only datagrams from it are delivered, and @c send_to becomes fatal until
     * @c disconnect_remote. On failure the machine stays kBound (the binding survives) and
     * @c last_error()/@c set_on_error carry the mapped reason. Loop thread only.
     */
    void connect_to(const std::string &ip, uint16_t port);

    /**
     * @brief Un-pins the default peer: kConnected → kBound. Legal from kConnected only (fatal
     *        otherwise); the local binding and the receive arming are kept. Loop thread only.
     */
    void disconnect_remote();

    /**
     * @brief Toggles SO_BROADCAST (IPv4 only — IPv6 has no broadcast).
     *
     * Legal from kIdle (the flag applies at the lazy/actual bind — a level socket option
     * governs subsequent sends) and kBound (applied immediately); kClosed returns false.
     * Returns true on success; false means the backend/platform rejected the option
     * (@c last_error()/@c set_on_error carry the reason — e.g. on an IPv6-destined socket).
     * NOTE: a broadcast send (e.g. to 255.255.255.255) may still fail at delivery time —
     * firewalls/sandbox namespaces routinely drop it; the send completion reports that.
     * Loop thread only.
     */
    bool set_broadcast(bool enable);

    /**
     * @brief Connected-mode send: delivers to the pinned peer. Fatal unless kConnected (Qt
     *        contract). Completion semantics mirror @ref send_to. Loop thread only.
     */
    void send(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done);

    /** @brief @c std::string overload of @ref send. Loop thread only. */
    void send(const std::string &data, std::function<void(bool ok)> on_done);

    /**
     * @brief Arms receive interest; every arriving datagram is delivered as a copy on the loop
     *        thread.
     *
     * Legal from kIdle (a lazy ephemeral IPv4 bind happens first — arming needs a live
     * handle; failure stays kIdle, re-arm allowed later) and kBound. Re-setting replaces the
     * callback (callback swap, backend re-arm shape). Loop thread only.
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
     *        and a human-readable message on bind/send failure (receive errors stop delivery
     *        silently; re-arm via set_on_datagram).
     *
     * The callback is invoked through a local copy (PIT-40): it may close() or even destroy the
     * socket. Re-setting replaces the previous callback. Loop thread only.
     */
    void set_on_error(std::function<void(SocketError error, const std::string &message)> on_error);

    /**
     * @brief Sets the state-change callback: invoked on the loop thread on every entry into
     *        kIdle / kBound / kConnected / kClosed.
     *
     * The callback is invoked through a local copy (PIT-40). Re-setting replaces the previous
     * callback. Loop thread only.
     */
    void set_on_state_change(std::function<void(SocketState state)> on_state_change);

    /** @brief Current machine state (kIdle / kBound / kConnected / kClosed). Loop thread only. */
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
