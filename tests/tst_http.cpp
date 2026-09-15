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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

/// @file tst_http.cpp
/// @brief Characterization tests for the cxxkit::http wrapper (cpr backend) against a loopback
///        canned-response TCP server — no external network.
///
/// Threading (D31/PIT-40 compliant): UvEventDispatcher::process_events is FATAL from a non-loop
/// thread (always-on affinity check), so the event loop must own ONE thread. The main test thread
/// runs loop.exec(); the BLOCKING cpr transfer runs on a worker thread; the worker hands the
/// response back via loop.post() + loop.exit(). Everything observable (captured request, response)
/// is quiescent when exec() returns — no locks needed at assert time.
///
/// Characterization findings pinned here (2026-09-14):
/// - Transport failures (timeout, connection refused) yield status_code() == 0 per the wrapper's
///   documented convention (cpr maps curl errors to status 0 in its Response).
/// - async_get/async_put/async_post/async_download run on std::async worker threads (not the
///   cxxkit ThreadPool — its start() returns void, PIT-54); .get() joins.
/// - cpr joins Payload pairs url-encoded as k=v&k2=v2 in the request body.
/// - env http_proxy/https_proxy break loopback requests (curl CONNECT → 502); tests run with
///   proxy environment stripped (CI default).

#include <cxxkit/base/global.hpp>

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/default_dispatcher.hpp>
#include <cxxkit/network/http.hpp>
#include <cxxkit/network/tcp_server.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{

using cxxkit::EventLoop;
using cxxkit::TcpServer;
using namespace cxxkit::http;

const char *const kHost = "127.0.0.1";

// Respond-once loopback server: accepts the first connection, accumulates request bytes until the
// request head terminator ("\r\n\r\n") arrives, then writes the canned response and closes. All
// callbacks run on the loop thread (loop.exec() runs in the test); the captured request is read
// after the loop has exited, so no cross-thread access remains.
struct OneShotServer
{
    EventLoop &loop;
    TcpServer server;
    std::unique_ptr<cxxkit::TcpSocket> server_side; // accepted socket must outlive on_connection (I5)
    std::string request;
    std::string response;

    explicit OneShotServer(EventLoop &l)
        : loop(l)
        , server(l)
    {
        server.on_connection(
            [this](std::unique_ptr<cxxkit::TcpSocket> socket)
            {
                server_side = std::move(socket);
                server_side->read_start(
                    [this](const uint8_t *data, ssize_t nread)
                    {
                        if (nread <= 0)
                        {
                            return;
                        }
                        request.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                        if (request.find("\r\n\r\n") != std::string::npos)
                        {
                            // Full request head captured -> respond and close. Writes from inside the
                            // read callback are legal (same thread); close waits for the drain pump.
                            const uint8_t *bytes = reinterpret_cast<const uint8_t *>(response.data());
                            server_side->write(bytes, response.size(), [this](bool) { server_side->close(); });
                        }
                    });
            });
    }

    /** @brief Listens on an ephemeral loopback port and arms @p raw_response. */
    bool start(const std::string &raw_response)
    {
        response = raw_response;
        return server.listen(kHost, 0) && server.bound_port() != 0;
    }
};

// "Nohup" server: accepts but never answers — drives the client timeout path.
struct SilentServer
{
    EventLoop &loop;
    TcpServer server;
    std::unique_ptr<cxxkit::TcpSocket> server_side;

    explicit SilentServer(EventLoop &l)
        : loop(l)
        , server(l)
    {
        server.on_connection(
            [this](std::unique_ptr<cxxkit::TcpSocket> socket)
            {
                server_side = std::move(socket);
                // deliberately NO read_start, NO write: the request goes unanswered
            });
    }

    bool start() { return server.listen(kHost, 0) && server.bound_port() != 0; }
};

// Runs the blocking HTTP call on a worker thread while the MAIN thread drives loop.exec(); the
// worker posts the response into the loop and exits it. Returns the response once exec() is done.
// exec() gets a deadline via the caller's session Timeout where relevant; the loop itself is
// bounded because every request path here either answers, fails fast, or times out (<=1s).
Response::SharedPtr run_with_loop(EventLoop &loop, std::function<Response::SharedPtr()> blocking_call)
{
    Response::SharedPtr result;
    std::thread worker(
        [&loop, &result, &blocking_call]
        {
            Response::SharedPtr response = blocking_call();
            loop.post(
                [&loop, &result, response]
                {
                    result = response;
                    loop.exit(0);
                });
        });
    loop.exec();
    worker.join();
    return result;
}

std::string make_response(long code,
                          const std::string &reason,
                          const std::string &extra_headers,
                          const std::string &body)
{
    return "HTTP/1.1 " + std::to_string(code) + " " + reason + "\r\n" + extra_headers +
           "Content-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
}

} // namespace

// 1. Canonical happy path: GET round-trip returns the canned status, body and reason verbatim.
TEST(HttpTest, GetRoundTrip)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "hello")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/path";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(std::string("hello"), response->text());
    EXPECT_EQ(std::string("OK"), response->reason());
}

// 2. Request shape: cpr sends a well-formed request line and Host header to the loopback server.
TEST(HttpTest, RequestLineAndHeaders)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "ok")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/path";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}, Parameters{{"x", "1"}}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(0UL, srv.request.find("GET /path?x=1 HTTP/1.1\r\n")) << "captured: " << srv.request;
    EXPECT_NE(std::string::npos, srv.request.find("Host:")) << "captured: " << srv.request;
}

// 3. Status pass-through: 404 and 500 surfaces map to status_code with bodies intact.
TEST(HttpTest, StatusCodes)
{
    {
        EventLoop loop(cxxkit::make_default_dispatcher());
        OneShotServer not_found(loop);
        ASSERT_TRUE(not_found.start(make_response(404, "Not Found", "", "gone")));
        const std::string url = std::string(kHost) + ":" + std::to_string(not_found.server.bound_port()) + "/missing";
        Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}); });
        ASSERT_NE(nullptr, response.get());
        EXPECT_EQ(404L, response->status_code());
        EXPECT_EQ(std::string("gone"), response->text());
    }
    {
        EventLoop loop(cxxkit::make_default_dispatcher());
        OneShotServer boom(loop);
        ASSERT_TRUE(boom.start(make_response(500, "Internal Server Error", "", "boom")));
        const std::string url = std::string(kHost) + ":" + std::to_string(boom.server.bound_port()) + "/broken";
        Response::SharedPtr response = run_with_loop(loop, [&url] { return post(Url{url}); });
        ASSERT_NE(nullptr, response.get());
        EXPECT_EQ(500L, response->status_code());
        EXPECT_EQ(std::string("boom"), response->text());
    }
}

// 4. Header lookup: single-key hit returns the value; a miss returns the empty string.
TEST(HttpTest, ResponseHeaders)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "X-Test-Header: cxxkit-value\r\n", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/path";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(std::string("cxxkit-value"), response->header("X-Test-Header"));
    EXPECT_EQ(std::string(""), response->header("X-Missing-Header"));
}

// 5. POST payload encoding: cpr joins pairs url-encoded as k=v&k2=v2 in the request body.
TEST(HttpTest, PostWithPayload)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(201, "Created", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/submit";
    Response::SharedPtr response = run_with_loop(
        loop,
        [&url] { return post(Url{url}, Payload{{"key1", "value1"}, {"key2", "value2"}}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(201L, response->status_code());
    EXPECT_NE(std::string::npos, srv.request.find("key1=value1&key2=value2")) << "captured: " << srv.request;
}

// 6. PUT with a raw body: body bytes arrive verbatim after the request head.
TEST(HttpTest, PutWithBody)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(204, "No Content", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/resource";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return put(Url{url}, Body{"put body bytes"}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(204L, response->status_code());
    const size_t body_pos = srv.request.find("\r\n\r\n");
    ASSERT_NE(std::string::npos, body_pos);
    EXPECT_EQ(std::string("put body bytes"), srv.request.substr(body_pos + 4));
}

// 7. Header options: a custom request header actually reaches the wire.
TEST(HttpTest, CustomRequestHeader)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/path";
    Response::SharedPtr response = run_with_loop(
        loop,
        [&url] { return get(Url{url}, Header{{"X-Custom-Header", "custom-value"}}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_NE(std::string::npos, srv.request.find("X-Custom-Header: custom-value")) << "captured: " << srv.request;
}

// 8. Bearer token: set_bearer produces an "Authorization: Bearer <token>" request header.
TEST(HttpTest, BearerAuthHeader)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/secure";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}, Bearer{"token-123"}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_NE(std::string::npos, srv.request.find("Authorization: Bearer token-123")) << "captured: " << srv.request;
}

// 9. Basic auth: Authentication{kBASIC} produces an "Authorization: Basic <base64>" header.
TEST(HttpTest, BasicAuthHeader)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/secure";
    Response::SharedPtr response = run_with_loop(
        loop,
        [&url] { return get(Url{url}, Authentication{"user", "pass", Authentication::Mode::kBASIC}); });
    ASSERT_NE(nullptr, response.get());
    // base64("user:pass") = dXNlcjpwYXNz — pins curl's exact encoding of the credentials.
    EXPECT_NE(std::string::npos, srv.request.find("Authorization: Basic dXNlcjpwYXNz")) << "captured: " << srv.request;
}

// 10. Cookies: Set-Cookie from the canned response is parsed and surfaced via response.cookies().
TEST(HttpTest, CookiesRoundTrip)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "Set-Cookie: session=abc123; Path=/\r\n", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/login";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    ASSERT_FALSE(response->cookies().data().empty()) << "Set-Cookie must surface in response.cookies()";
    EXPECT_EQ(std::string("session"), response->cookies().data().at(0)->get_name());
    EXPECT_EQ(std::string("abc123"), response->cookies().data().at(0)->get_value());
}

// 11. Transport failure = 0 convention: unreachable port yields status_code 0, non-null response.
TEST(HttpTest, ConnectionRefusedMapsToZeroStatus)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    Response::SharedPtr response = run_with_loop(loop, [] { return get(Url{std::string("127.0.0.1:1/nothing")}); });
    ASSERT_NE(nullptr, response.get()) << "transport errors must still produce a Response object";
    EXPECT_EQ(0L, response->status_code());
    EXPECT_EQ(std::string(""), response->text());
}

// 12. Timeout: an accepted-but-never-answered request gives up within the deadline -> status 0.
//     Bounded: curl's Timeout(300ms) aborts the transfer, the worker posts and the loop exits.
TEST(HttpTest, TimeoutMapsToZeroStatus)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    SilentServer srv(loop);
    ASSERT_TRUE(srv.start());

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/slow";
    Response::SharedPtr response = run_with_loop(loop,
                                                 [&url]
                                                 { return get(Url{url}, Timeout{std::chrono::milliseconds(300)}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(0L, response->status_code());
}

// 13. Async download: async_download resolves via std::async with the canned body written to disk.
//     async_get/async_put/async_post share the same std::async path since the PIT-54 fix
TEST(HttpTest, AsyncDownload)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "async-file-body")));

    const std::string path = "/tmp/cxxkit_tst_http_async_download.bin";
    std::remove(path.c_str());
    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/file";
    Response::SharedPtr response = run_with_loop(loop, [&path, &url] { return async_download(path, Url{url}).get(); });
    std::remove(path.c_str());
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
}

// 13a. Async GET: resolves on a std::async worker; body asserted server-round-trip.
TEST(HttpTest, AsyncGet)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "async-get-body")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/async";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return async_get(Url{url}).get(); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(std::string("async-get-body"), response->text());
}

// 13b. Async POST: payload asserted server-side, proving the request body crossed the worker
//      thread boundary intact.
TEST(HttpTest, AsyncPostPayload)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(201, "Created", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/async-post";
    Response::SharedPtr response = run_with_loop(loop,
                                                 [&url]
                                                 { return async_post(Url{url}, Payload{{"k", "v1&v2"}}).get(); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(201L, response->status_code());
    EXPECT_NE(std::string::npos, srv.request.find("k=v1%26v2")) << "captured: " << srv.request;
}

// 13c. Async failure path: connection refused on the std::async worker still yields a Response
//      with status 0 (transport-failure convention) — no exception escapes .get().
TEST(HttpTest, AsyncConnectionRefused)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    Response::SharedPtr response = run_with_loop(loop,
                                                 []
                                                 { return async_get(Url{std::string("127.0.0.1:1/nothing")}).get(); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(0L, response->status_code());
}

// 14. Download: session.download(file) streams the canned body into the target file.
TEST(HttpTest, DownloadToOfstream)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "file-body-bytes")));

    const char *const kPath = "/tmp/cxxkit_tst_http_download.bin";
    std::remove(kPath);
    {
        std::ofstream file(kPath, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/file";
        Response::SharedPtr response = run_with_loop(loop, [&url, &file] { return download(file, Url{url}); });
        ASSERT_NE(nullptr, response.get());
        EXPECT_EQ(200L, response->status_code());
    }
    std::ifstream file(kPath, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_EQ(std::string("file-body-bytes"), content);
    std::remove(kPath);
}

// The machine's http_proxy/https_proxy environment makes curl route loopback requests through the
// proxy (CONNECT → 502), which would poison every case. Strip the family before gtest runs: the
// cpr/curl backend reads these at transfer time, so a one-time unset in main() is sufficient.
int main(int argc, char *argv[])
{
    const char *const kProxyVars[] =
        {"http_proxy", "https_proxy", "all_proxy", "HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY"};
    for (const char *const name : kProxyVars)
    {
        ::unsetenv(name);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
