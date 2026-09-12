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

#include <cxxkit/network/detail/tcp_server_p.hpp>
#include <cxxkit/network/tcp_server.hpp>

#include <memory>
#include <cxxkit/network/tcp_socket.hpp>

#include <cxxkit/tools/checks.hpp>

#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

TcpServerPrivate::TcpServerPrivate(TcpServer *p, EventLoop &loop)
    : mP(p)
    , mLoop(loop)
{
    mBackend->open(loop); // binds the backend to this loop's engine
    mLoopThreadId = std::this_thread::get_id();
}

// Forward declaration (tcp_server.cpp must not include backend headers — grep gate): the accepted
// client's void* native handle hands the native handle over to TcpSocket::adopt_native (R-T3-1).

TcpServerPrivate::~TcpServerPrivate()
{
    // I6: nothing should reach here with a live handle — the public destructor closes and pumps.
    CXXKIT_CHECK(!mBackend->is_open()) << "TcpServerPrivate: backend outlived the drain pump (teardown bug)";
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
    // I6: close (idempotent), then pump NOWAIT rounds until the close callback ran — the same
    // drain discipline as TcpSocket's destructor. The whole sequence (latch + backend close +
    // pump) lives in the backend's pump_until_closed. Accepted sockets are NOT touched (I6
    // boundary).
    d->mBackend->pump_until_closed();
}

bool TcpServer::listen(const std::string &ip, uint16_t port, int backlog)
{
    CXXKIT_D(TcpServer);
    d->check_loop_thread("listen");
    if (d->mListening)
    {
        return false; // already listening — an explicit retry contract, not a fatal
    }

    // Bind+listen, including the listen-failure teardown (close + pump — pump_until_closed's
    // third site) and the OS-port read-back (R-T3-2), live in the backend. Loop-failure semantics
    // preserved: false return, server stays unlistened and retryable.
    if (!d->mBackend->listen(ip, port, backlog))
    {
        return false;
    }
    d->mListening = true;
    d->mBoundPort = d->mBackend->bound_port();

    // R-T3-1: every accepted client backend is handed over WHOLE — its uv handle (and lifetime)
    // belongs to the client backend; the TcpSocketPrivate(backend) ctor takes it over with no
    // second backend ever wrapping the same native handle.
    d->mBackend->set_on_accept(
        [d](std::unique_ptr<network::detail::StreamBackend> client)
        {
            if (d->mOnConnection)
            {
                d->mOnConnection(TcpSocket::adopt_backend(d->mLoop, std::move(client)));
            }
            else
            {
                // No consumer installed: accept-and-discard keeps the backlog from filling silently
                // (T2 semantics). We are inside the uv connection callback — process_events must
                // NOT be re-entered (I5), so the close+drain is deferred: the backend is kept alive
                // by moving it into the posted closure, and the next loop round closes + drains it
                // before dropping it.
                // std::function requires copyable captures: the shared_ptr is the ownership bridge
                // (the posted closure is the last owner — the backend dies right after the drain).
                std::shared_ptr<network::detail::StreamBackend> doomed(std::move(client));
                d->mLoop.post(
                    [doomed]()
                    {
                        doomed->close(nullptr);
                        doomed->pump_until_closed();
                    });
            }
        });
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

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
