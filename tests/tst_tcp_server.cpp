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

#include <cxxkit/base/global.hpp>

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/uv/dispatcher_factory.hpp>
#include <cxxkit/uv/tcp_server.hpp>
#include <cxxkit/uv/tcp_socket.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{

using cxxkit::EventLoop;
using cxxkit::TcpServer;
using cxxkit::TcpSocket;
using cxxkit::make_uv_dispatcher;

// T3: real listener + real TCP connect over 127.0.0.1. Port discovery is race-free (R-T3-2):
// listen on port 0, read the OS-assigned port back via TcpServer::bound_port() (uv_tcp_getsockname),
// then connect to it. All waiting is event-driven (P3): bounded timed rounds, never sleeps.

// 1. Full loop: listen -> TcpSocket connect -> write -> server echo -> client receives the same bytes.
TEST(TcpServerTest, ListenAcceptEcho)
{
    EventLoop loop(make_uv_dispatcher());
    TcpServer server(loop);
    std::unique_ptr<TcpSocket> server_side; // keeps the accepted socket alive across the test
    server.on_connection(
        [&](std::unique_ptr<TcpSocket> socket)
        {
            // echo server: bounce every received chunk straight back. The unique_ptr parameter dies
            // at lambda return — store it or the TcpSocket destructor's drain pump re-enters the loop
            // from inside connection_cb (I5 violation).
            server_side = std::move(socket);
            server_side->read_start(
                [&](const uint8_t *data, ssize_t nread)
                {
                    if (nread > 0)
                    {
                        server_side->write(data, static_cast<size_t>(nread), [](bool) { });
                    }
                });
        });
    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    ASSERT_NE(0u, server.bound_port()) << "port 0 must surface the OS-assigned port";

    std::unique_ptr<TcpSocket> client;
    std::string received;
    const std::string kPayload = "tcp-server-echo-payload";

    client = std::unique_ptr<TcpSocket>(new TcpSocket(loop));
    client->connect("127.0.0.1",
                    server.bound_port(),
                    [&](bool ok)
                    {
                        ASSERT_TRUE(ok); // read interest is legal only from kConnected — arm inside the callback
                        client->read_start(
                            [&](const uint8_t *data, ssize_t nread)
                            {
                                if (nread > 0)
                                {
                                    received.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                                    if (received == kPayload)
                                    {
                                        loop.exit(0);
                                    }
                                }
                            });
                        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(kPayload.data());
                        client->write(bytes, kPayload.size(), [](bool) { });
                    });

    // Bounded event-driven wait (P3): connect round-trip + write/echo, 2s per round bounds a
    // regression instead of hanging.
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    EXPECT_EQ(kPayload, received);
}

// 2. Two simultaneous connections stay independent: each client gets its own echo, both complete.
TEST(TcpServerTest, AcceptDuringActiveConnections)
{
    EventLoop loop(make_uv_dispatcher());
    TcpServer server(loop);
    std::vector<std::unique_ptr<TcpSocket>> server_sides; // accepted sockets outlive the callbacks
    server.on_connection(
        [&](std::unique_ptr<TcpSocket> socket)
        {
            // store: the unique_ptr parameter dies at lambda return — its drain pump must not
            // run inside connection_cb (I5)
            server_sides.push_back(std::move(socket));
            TcpSocket *echo_socket = server_sides.back().get(); // THIS connection's accepted end
            echo_socket->read_start(
                [&, echo_socket](const uint8_t *data, ssize_t nread)
                {
                    if (nread > 0)
                    {
                        echo_socket->write(data, static_cast<size_t>(nread), [](bool) { });
                    }
                });
        });
    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    ASSERT_NE(0u, server.bound_port());

    std::vector<std::unique_ptr<TcpSocket>> clients;
    std::vector<std::string> received(2);
    const std::string kPayloads[2] = {"first-client-payload", "second-client-payload"};
    int echoes_done = 0;

    for (int i = 0; i < 2; ++i)
    {
        clients.push_back(std::unique_ptr<TcpSocket>(new TcpSocket(loop)));
        TcpSocket *client = clients.back().get();
        client->connect("127.0.0.1",
                        server.bound_port(),
                        [&, client, i](bool ok)
                        {
                            ASSERT_TRUE(ok); // arm read interest only once connected
                            client->read_start(
                                [&, i](const uint8_t *data, ssize_t nread)
                                {
                                    if (nread > 0)
                                    {
                                        received[i].append(reinterpret_cast<const char *>(data),
                                                           static_cast<size_t>(nread));
                                        if (received[i] == kPayloads[i] && ++echoes_done == 2)
                                        {
                                            loop.exit(0);
                                        }
                                    }
                                });
                            const uint8_t *bytes = reinterpret_cast<const uint8_t *>(kPayloads[i].data());
                            client->write(bytes, kPayloads[i].size(), [](bool) { });
                        });
    }

    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    EXPECT_EQ(kPayloads[0], received[0]);
    EXPECT_EQ(kPayloads[1], received[1]);
    EXPECT_EQ(2, echoes_done);
}

// 3. I6 boundary: destroying the server closes only the SERVER handle — an already-accepted
// socket keeps working (echo still flows) and tears itself down cleanly afterwards.
TEST(TcpServerTest, ServerDestructorWithLiveConnections)
{
    EventLoop loop(make_uv_dispatcher());
    std::unique_ptr<TcpSocket> accepted; // outlives the server below
    std::unique_ptr<TcpSocket> client;   // ditto — only the SERVER dies inside the block (peer
                                         // EOF would auto-close the accepted end otherwise)

    {
        TcpServer server(loop);
        server.on_connection(
            [&](std::unique_ptr<TcpSocket> socket)
            {
                accepted = std::move(socket); // handed over: server no longer involved
                accepted->read_start(
                    [&](const uint8_t *data, ssize_t nread)
                    {
                        if (nread > 0)
                        {
                            accepted->write(data, static_cast<size_t>(nread), [](bool) { });
                        }
                    });
            });
        ASSERT_TRUE(server.listen("127.0.0.1", 0));
        ASSERT_NE(0u, server.bound_port());

        client = std::unique_ptr<TcpSocket>(new TcpSocket(loop));
        client->connect("127.0.0.1",
                        server.bound_port(),
                        [&](bool ok)
                        {
                            ASSERT_TRUE(ok); // arm read interest only once connected
                            client->read_start(
                                [&](const uint8_t *, ssize_t nread)
                                {
                                    if (nread > 0)
                                    {
                                        loop.exit(0);
                                    }
                                });
                            const uint8_t byte = 'k';
                            client->write(&byte, 1, [](bool) { });
                        });
        loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
        loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
        ASSERT_NE(nullptr, accepted.get()) << "connection must be accepted before the server dies";
    } // server destroyed here: closes the listener, the accepted socket must survive

    // The accepted socket is still open and still echoes (server death did not touch it).
    ASSERT_TRUE(accepted->is_open());

    // Loop still usable after the server teardown pump (I6).
    loop.process_events(EventLoop::ProcessFlag::kAllEvents);
}

} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
