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
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/**
 * @brief Backend-neutral datagram transport interface (D44): bind/send/receive/close
 *        for UDP and similar connectionless protocols.
 *
 * Mirrors the style of @ref StreamBackend but exposes datagram semantics: no read/write
 * pairing (send_to / receive_start are independent), and bind is synchronous (no async
 * handshake). Connected mode (D45): @c connect pins a default peer so @c send goes
 * without an address and receive filters to that peer — synchronous on both backends
 * (UDP connect has no handshake). Backends translate their native error status onto the
 * pimpl's SocketError via @c native_status.
 */
class DgramBackend
{
public:
    virtual ~DgramBackend() = default;

    /**
     * @brief Bind this backend to an event loop.
     * Must be called before any other method. Returns false on failure.
     */
    virtual bool open(EventLoop &loop) = 0;

    /**
     * @brief Bind to a local address/port. Synchronous — UDP bind has no async handshake.
     * @return false on failure (native_status carries the reason).
     */
    virtual bool bind(const std::string &ip, uint16_t port) = 0;

    /// Actual bound port (0 = ephemeral; read back after bind with port 0). TcpServer same pattern.
    virtual uint16_t bound_port() const = 0;

    /**
     * @brief Pin the default peer (@c connect(2) semantics on the datagram socket).
     *
     * Synchronous — connected UDP has no handshake. Subsequent @c send calls deliver to
     * this peer; receives filter to it. Does not change the local binding.
     * @return false on failure (native_status carries the reason).
     */
    virtual bool connect(const std::string &ip, uint16_t port) = 0;

    /// Un-pin the default peer (connected → unconnected; binding and buffered data kept).
    virtual void disconnect_remote() = 0;

    /**
     * @brief Toggle SO_BROADCAST (IPv4 only — IPv6 has no broadcast; backends report failure).
     *
     * Legal on a bound or never-bound handle (a level socket option — applies to subsequent
     * sends; the uv path resolves the descriptor via uv_fileno, which requires a created
     * handle). @return false when the platform/backend rejects the option (native_status
     * carries the reason).
     */
    virtual bool set_broadcast(bool enable) = 0;

    /**
     * @brief Send a datagram to the specified destination.
     * @param on_done called with true on success, false on failure (native_status carries reason).
     */
    virtual void send_to(const uint8_t *data,
                         size_t len,
                         const std::string &ip,
                         uint16_t port,
                         std::function<void(bool ok)> on_done) = 0;

    /// Start receiving datagrams. on_datagram delivers (data, len, src_ip, src_port).
    /// The data buffer is valid only during the callback; consumers must copy.
    virtual void receive_start(
        std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> on_datagram) = 0;

    /// Idempotent close; drain pending callbacks.
    virtual void close() = 0;

    /// The loop this backend is bound to (set by @c open).
    virtual EventLoop &loop() const = 0;

    /// Last native failure status of the most recent completed operation (0 = none/success).
    virtual int native_status() const = 0;

    /// Native handle view (void* — backend-owned type). Debugging aid only.
    virtual void *native_handle() const = 0;
};

/** @brief Compiled per backend selection (uv today; asio lands as another definition). */
std::unique_ptr<DgramBackend> make_dgram_backend();

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
