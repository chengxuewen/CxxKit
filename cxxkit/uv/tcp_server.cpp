/***
Library: CxxKit
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

#include <cxxkit/uv/detail/tcp_server_p.hpp>
#include <cxxkit/uv/tcp_server.hpp>

#include <cxxkit/tools/checks.hpp>
#include <cxxkit/uv/tcp_socket.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <memory>
#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

TcpServerPrivate::TcpServerPrivate(TcpServer *p, EventLoop &loop)
    : mP(p)
    , mLoop(loop)
{
    mDispatcher = static_cast<UvEventDispatcher *>(&loop.dispatcher());
    mLoopThreadId = std::this_thread::get_id();
}

TcpServerPrivate::~TcpServerPrivate()
{
    // I6: nothing should reach here with a live handle — the public destructor closes and pumps.
    CXXKIT_CHECK(mHandle == nullptr) << "TcpServerPrivate: handle outlived the drain pump (teardown bug)";
}

void TcpServerPrivate::check_loop_thread(const char *api) const
{
    if (std::this_thread::get_id() != mLoopThreadId)
    {
        CXXKIT_FATAL() << "TcpServer::" << api << " called from a non-loop thread (loop thread "
                       << std::hash<std::thread::id>()(mLoopThreadId) << ", caller thread "
                       << std::hash<std::thread::id>()(std::this_thread::get_id())
                       << "). Serialize with EventLoop::post() from other threads instead.";
    }
}

TcpServer::TcpServer(EventLoop &loop)
    : mDPtr(new TcpServerPrivate(this, loop))
{
}

TcpServer::~TcpServer()
{
    CXXKIT_D(TcpServer);
    // I6: close (idempotent), then pump NOWAIT rounds until the uv close callback ran — the same
    // drain discipline as TcpSocket's destructor. Accepted sockets are NOT touched (I6 boundary).
    if (d->mHandle != nullptr && !d->mCloseRequested && !uv_is_closing(reinterpret_cast<uv_handle_t *>(d->mHandle)))
    {
        d->mCloseRequested = true;
        uv_close(reinterpret_cast<uv_handle_t *>(d->mHandle), &TcpServerPrivate::on_closed);
    }
    for (int rounds = 0; rounds < 1000 && d->mHandle != nullptr; ++rounds)
    {
        d->mLoop.process_events(EventLoop::ProcessFlag::kAllEvents);
    }
}

bool TcpServer::listen(const std::string &ip, uint16_t port, int backlog)
{
    CXXKIT_D(TcpServer);
    d->check_loop_thread("listen");
    CXXKIT_CHECK(!d->mCloseRequested) << "TcpServer::listen: server is closing/closed";
    if (d->mListening)
    {
        return false; // already listening — an explicit retry contract, not a fatal
    }

    sockaddr_in addr;
    const int addr_rc = uv_ip4_addr(ip.c_str(), port, &addr);
    if (addr_rc != 0)
    {
        return false; // invalid address: a listen failure, not a programming error
    }

    uv_tcp_t *handle = new uv_tcp_t;
    const int init_rc = uv_tcp_init(&d->mDispatcher->loop(), handle);
    if (init_rc != 0)
    {
        delete handle;
        return false;
    }
    handle->data = d;
    d->mHandle = handle; // from here on on_closed owns the free; failure paths below close+pump it

    const int bind_rc = uv_tcp_bind(handle, reinterpret_cast<const sockaddr *>(&addr), 0);
    const int listen_rc = (bind_rc == 0) ? uv_listen(reinterpret_cast<uv_stream_t *>(handle),
                                                     backlog,
                                                     &TcpServerPrivate::on_connection_cb)
                                         : bind_rc;
    if (listen_rc != 0)
    {
        // The handle IS registered with the loop (init did that) — the only clean teardown is
        // uv_close + pump until the callback freed it (TcpSocket dtor discipline), so a retry
        // starts from a clean slate and the loop never carries a zombie handle.
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(d->mHandle)))
        {
            uv_close(reinterpret_cast<uv_handle_t *>(d->mHandle), &TcpServerPrivate::on_closed);
        }
        for (int rounds = 0; rounds < 1000 && d->mHandle != nullptr; ++rounds)
        {
            d->mLoop.process_events(EventLoop::ProcessFlag::kAllEvents);
        }
        return false;
    }
    d->mListening = true;

    // R-T3-2: port 0 = OS-assigned; read the real port back via getsockname — race-free discovery
    // versus probing candidate ports from the outside.
    sockaddr_storage bound;
    int bound_len = static_cast<int>(sizeof(bound));
    if (uv_tcp_getsockname(handle, reinterpret_cast<sockaddr *>(&bound), &bound_len) == 0 && bound.ss_family == AF_INET)
    {
        d->mBoundPort = ntohs(reinterpret_cast<const sockaddr_in *>(&bound)->sin_port);
    }
    else
    {
        d->mBoundPort = port;
    }
    return true;
}

uint16_t TcpServer::bound_port() const
{
    CXXKIT_D(const TcpServer);
    d->check_loop_thread("bound_port");
    return d->mBoundPort;
}

void TcpServer::on_connection(std::function<void(std::unique_ptr<TcpSocket>)> fn)
{
    CXXKIT_D(TcpServer);
    d->check_loop_thread("on_connection");
    d->mOnConnection = std::move(fn);
}

void TcpServerPrivate::on_connection_cb(uv_stream_t *server, int status)
{
    TcpServerPrivate *d = static_cast<TcpServerPrivate *>(server->data);
    if (status < 0)
    {
        return; // listen-side error (EMFILE & co): no backlog to drain, no sockets to produce
    }
    // memcached accept-batch discipline: drain everything the kernel has queued for this wake-up.
    while (true)
    {
        uv_tcp_t *client = new uv_tcp_t;
        if (uv_tcp_init(server->loop, client) != 0)
        {
            delete client; // init never registered it with the loop
            return;        // cannot init: stop draining
        }
        if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client)) != 0)
        {
            // Backlog empty (UV_EAGAIN) or broken peer: close the bare handle, no socket produced.
            uv_close(reinterpret_cast<uv_handle_t *>(client), &TcpServerPrivate::on_discard_closed);
            return;
        }
        if (!d->mOnConnection)
        {
            // No consumer installed: accept-and-discard keeps the backlog from filling silently.
            uv_close(reinterpret_cast<uv_handle_t *>(client), &TcpServerPrivate::on_discard_closed);
            continue;
        }
        // R-T3-1: the accepted handle is handed over — its uv_close (and lifetime) belongs to the
        // TcpSocket from here on; the server never touches it again.
        d->mOnConnection(TcpSocket::adopt_uv_tcp(d->mLoop, client));
    }
}

void TcpServerPrivate::on_discard_closed(uv_handle_t *handle)
{
    delete reinterpret_cast<uv_tcp_t *>(handle); // bare client cell: no pimpl behind it, just free
}

void TcpServerPrivate::on_closed(uv_handle_t *handle)
{
    TcpServerPrivate *d = static_cast<TcpServerPrivate *>(handle->data);
    d->mHandle = nullptr;
    d->mListening = false;
    delete handle;
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
