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
#include <cxxkit/uv/tcp_socket.hpp>

#include <cstdint>
#include <functional>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class TcpServerPrivate;

/**
 * @brief TCP listener over a @c cxxkit::uv event loop — accepts connections into TcpSocket
 *        instances (phase-2 core IO, F10).
 *
 * Wraps one @c uv_tcp_t server handle (Node pipe/tcp server pattern): @c uv_tcp_bind +
 * @c uv_listen install the listener; each @c connection_cb drains the pending backlog with a
 * @c while (uv_accept(...) == 0) loop (memcached accept-batch discipline), initting a bare
 * client handle per round. Every accepted handle is handed to @c TcpSocket::adopt_uv_tcp
 * (R-T3-1) — ownership of the handle transfers to the TcpSocket, whose lifecycle is then the
 * consumer's alone. An accept that fails mid-drain closes the client handle it was holding and
 * produces no socket (no half-adopted state ever escapes).
 *
 * Accepted-socket ownership (I6 boundary): destroying the server closes the SERVER handle only;
 * already-accepted sockets are independent objects and keep working. The server does not track
 * them — once handed over via @c on_connection the server has no say in their lifetime.
 *
 * Threading (I1): every method is loop-thread only — violations are fatal. Cross-thread
 * producers serialize through @c EventLoop::post().
 *
 * Lifecycle (I6): the destructor closes the server handle (idempotent close, F8-② shape) and
 * pumps the loop until the close callback has run, so no uv state outlives the object.
 */
class CXXKIT_UV_API TcpServer
{
public:
    /**
     * @brief Creates a server bound to @p loop 's uv engine.
     *
     * The uv handle is initialized lazily at listen time. Loop thread only.
     */
    explicit TcpServer(EventLoop &loop);

    /** @brief Closes the server handle (if any) and pumps the loop until closed (I6). Loop thread only. */
    ~TcpServer();

    /**
     * @brief Binds to @p ip : @p port and starts listening with @p backlog pending connections.
     *
     * Pass @c port 0 to let the OS assign an ephemeral port; read it back with @c bound_port()
     * (uv_tcp_getsockname — R-T3-2, race-free versus probing ports from the outside). Returns
     * false on bind/listen failure (address in use, invalid address, ...) — the server stays
     * unlistened and may be retried. Fails (false) if already listening. Loop thread only.
     */
    bool listen(const std::string &ip, uint16_t port, int backlog = 128);

    /**
     * @brief The port the listener is actually bound to.
     *
     * Zero until a successful @c listen, or when listening on port 0 before the bind completed.
     * Loop thread only.
     */
    uint16_t bound_port() const;

    /**
     * @brief Sets the callback invoked once per accepted connection, on the loop thread.
     *
     * The @c unique_ptr hands over the accepted socket's ownership. Store it and dispose of it
     * OUTSIDE this callback (or post() a follow-up to drop it later): destroying it here would
     * drain the loop from inside a dispatcher callback and trip the I5 nested-iteration fatal.
     * Sockets still in the kernel backlog when no callback is set are accepted into closed
     * handles (discarded silently).
     * Loop thread only.
     */
    void on_connection(std::function<void(std::unique_ptr<TcpSocket>)> fn);

private:
    CXXKIT_DECLARE_PRIVATE(TcpServer)
    CXXKIT_DEFINE_DPTR(TcpServer)
    CXXKIT_DISABLE_COPY_MOVE(TcpServer)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
