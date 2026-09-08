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

/// @file exp_tcp_echo.cpp
/// @brief Deterministic TcpServer + TcpSocket loopback echo demo (rc=0, needs CXXKIT_ENABLE_LIB_UV=ON).
///
/// Determinism contract (R-T3-3): the OS-assigned port number is runtime-random, so it goes to
/// stderr as a side channel — stdout carries only the fixed success line "tcp echo roundtrip ok".

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/uv/dispatcher_factory.hpp>
#include <cxxkit/uv/tcp_server.hpp>
#include <cxxkit/uv/tcp_socket.hpp>

#include <cstdio>
#include <memory>
#include <string>

int main()
{
    cxxkit::EventLoop loop(cxxkit::make_uv_dispatcher());
    cxxkit::TcpServer server(loop);

    std::string received;
    const std::string kPayload = "tcp echo roundtrip payload";
    std::unique_ptr<cxxkit::TcpSocket> server_side; // accepted socket outlives connection_cb
    std::unique_ptr<cxxkit::TcpSocket> client;

    server.on_connection(
        [&](std::unique_ptr<cxxkit::TcpSocket> socket)
        {
            // Store the accepted socket: the unique_ptr parameter dies at lambda return — its
            // destructor drain pump must not run inside connection_cb (I5).
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

    if (!server.listen("127.0.0.1", 0))
    {
        std::fprintf(stderr, "exp_tcp_echo: listen failed\n");
        return 1;
    }
    // Random per-run value: stderr only (R-T3-3 determinism contract).
    std::fprintf(stderr, "exp_tcp_echo: listening on 127.0.0.1:%u (ephemeral)\n", server.bound_port());

    client = std::unique_ptr<cxxkit::TcpSocket>(new cxxkit::TcpSocket(loop));
    client->connect("127.0.0.1",
                    server.bound_port(),
                    [&](bool ok)
                    {
                        if (!ok)
                        {
                            loop.exit(1);
                            return;
                        }
                        // Read interest is legal only from kConnected — arm inside the connect callback.
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

    const int rc = loop.exec();
    if (rc == 0 && received == kPayload)
    {
        std::printf("tcp echo roundtrip ok\n");
        return 0;
    }
    std::fprintf(stderr, "exp_tcp_echo: roundtrip incomplete\n");
    return 1;
}
