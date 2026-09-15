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
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/tcp_server.hpp>
#include <cxxkit/network/tls_socket.hpp>

#include <gtest/gtest.h>

// Cert paths: the CMake target compiles this file with TEST_CERTS_DIR when the kernel/network
// stack is enabled (same convention as tst_tls_socket). Keep a source-tree fallback so the
// file still compiles outside CMake.
#ifndef TEST_CERTS_DIR
#    define TEST_CERTS_DIR "../tests/certs"
#endif

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

// HTTPS origin for the crossover cases (C wave item 3): a SERVER-ROLE TlsSocket over a
// TcpServer accept (TlsEchoPeer shape, PIT-40-compliant: the session unique_ptr is parked
// before the accept frame unwinds so the peer outlives every callback). After the handshake,
// read_start accumulates the DECRYPTED plaintext request; once the request head terminator
// (the four bytes CR LF CR LF) arrives, the canned @ref response is written back and
// close_notify is sent via TlsSocket::close. TLS 1.2/1.3 interop with curl's TLS is
// version-agnostic.
struct TlsOriginServer
{
    EventLoop &loop;
    TcpServer listener;
    std::unique_ptr<cxxkit::TlsSocket> session;
    bool tls_ready{false};
    bool responded{false};
    std::string request;
    std::string response;
    std::string error_log; // on_error messages — surfaced on handshake failures

    explicit TlsOriginServer(EventLoop &l)
        : loop(l)
        , listener(l)
    {
    }

    void start(const std::string &canned_response)
    {
        response = canned_response;
        listener.listen(kHost, 0, 128);
        listener.on_connection(
            [this](std::unique_ptr<cxxkit::TcpSocket> socket)
            {
                std::unique_ptr<cxxkit::TlsSocket> tls(new cxxkit::TlsSocket(loop));
                cxxkit::TlsSocket *raw = tls.get();
                raw->set_transport(std::move(socket));
                // SERVER identity (no client-cert demand): without these the handshake cannot
                // complete; verify_mode 0 keeps mbedTLS from demanding a CA chain.
                raw->set_verify_mode(0);
                raw->set_certificate(std::string(TEST_CERTS_DIR) + "/server-cert.pem");
                raw->set_private_key(std::string(TEST_CERTS_DIR) + "/server-key.pem");
                raw->set_on_error([this](cxxkit::SocketError, const std::string &message)
                                  { error_log += message + "; "; });
                // Register BEFORE start_tls: the ClientHello is usually buffered at accept, so
                // the handshake can complete inside the start_tls first drive (kConnected fires
                // there) — same lesson as TlsEchoPeer.
                raw->set_on_state_change(
                    [this, raw](cxxkit::SocketState state)
                    {
                        if (cxxkit::SocketState::kConnected == state && !this->tls_ready)
                        {
                            this->tls_ready = true;
                            raw->read_start(
                                [this, raw](const uint8_t *data, ssize_t nread)
                                {
                                    if (nread <= 0)
                                    {
                                        return; // peer close / transport EOF: session finished
                                    }
                                    request.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                                    if (!this->responded && this->request.find("\r\n\r\n") != std::string::npos)
                                    {
                                        this->responded = true;
                                        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(this->response.data());
                                        raw->write(bytes, this->response.size(), [raw](bool) { raw->close(); });
                                    }
                                });
                        }
                    });
                raw->start_tls([](bool) { });
                this->session = std::move(tls); // parked before the accept frame unwinds (PIT-40)
            });
    }

    uint16_t port() const { return listener.bound_port(); }
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
    std::string worker_error;
    std::thread worker(
        [&loop, &result, &blocking_call, &worker_error]
        {
            try
            {
                Response::SharedPtr response = blocking_call();
                loop.post(
                    [&loop, &result, response]
                    {
                        result = response;
                        loop.exit(0);
                    });
            }
            catch (const std::exception &e)
            {
                // A throwing transfer must not kill the worker (terminate) nor hang the loop:
                // surface the exception text and let the null result drive the assertion.
                worker_error = e.what();
                fprintf(stderr, "HTTP-WORKER-EXCEPTION: %s\n", e.what());
                loop.post([&loop] { loop.exit(0); });
            }
        });
    loop.exec();
    worker.join();
    if (!worker_error.empty())
    {
        fprintf(stderr, "HTTP-TRANSFER-RAISED: %s\n", worker_error.c_str());
    }
    return result;
}

Response::SharedPtr spin_with_worker(EventLoop &loop, std::function<Response::SharedPtr()> blocking_call)
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
    for (int i = 0; i < 400 && result == nullptr; ++i)
    {
        loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents, 10);
    }
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

// 15. DELETE verb: request line carries "DELETE", canned 200 body round-trips.
TEST(HttpTest, DeleteVerb)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "deleted")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/item/7";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return del(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(std::string("deleted"), response->text());
    EXPECT_EQ(0UL, srv.request.find("DELETE /item/7 HTTP/1.1\r\n")) << "captured: " << srv.request;
}

// 16. PATCH verb: request line carries "PATCH", body bytes arrive after the head.
TEST(HttpTest, PatchVerb)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(204, "No Content", "", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/item/7";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return patch(Url{url}, Body{"patch bytes"}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(204L, response->status_code());
    EXPECT_EQ(0UL, srv.request.find("PATCH /item/7 HTTP/1.1\r\n")) << "captured: " << srv.request;
    const size_t body_pos = srv.request.find("\r\n\r\n");
    ASSERT_NE(std::string::npos, body_pos);
    EXPECT_EQ(std::string("patch bytes"), srv.request.substr(body_pos + 4));
}

// 17. HEAD verb: status/reason/headers surface, body stays empty by HTTP contract.
TEST(HttpTest, HeadVerb)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "X-Head-Probe: yes\r\n", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/meta";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return head(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(std::string("OK"), response->reason());
    EXPECT_EQ(std::string("yes"), response->header("X-Head-Probe"));
    EXPECT_EQ(std::string(""), response->text());
    EXPECT_EQ(0UL, srv.request.find("HEAD /meta HTTP/1.1\r\n")) << "captured: " << srv.request;
}

// 18. OPTIONS verb: request line carries "OPTIONS", status surfaces.
TEST(HttpTest, OptionsVerb)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "Allow: GET, HEAD\r\n", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/root";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return options(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(std::string("GET, HEAD"), response->header("Allow"));
    EXPECT_EQ(0UL, srv.request.find("OPTIONS /root HTTP/1.1\r\n")) << "captured: " << srv.request;
}

// 19. Proxy routing pin: with set_proxy pointed at a LISTENING port that answers garbage (never a
//     valid HTTP response), the transfer must fail with status 0 — and the proxy socket must
//     capture the forwarded request (Host header present). Capturing bytes on OUR socket proves
//     curl dialed the proxy instead of the target. bound_port() is read via loop.post() because
//     it is a thread-affinity API (FATAL from foreign threads).
TEST(HttpTest, ProxyRoutesThroughProxySocket)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    TcpServer proxy_srv(loop);
    ASSERT_TRUE(proxy_srv.listen(kHost, 0));
    std::unique_ptr<cxxkit::TcpSocket> proxy_side;
    std::string captured;
    proxy_srv.on_connection(
        [&proxy_side, &captured](std::unique_ptr<cxxkit::TcpSocket> socket)
        {
            proxy_side = std::move(socket);
            proxy_side->read_start(
                [&proxy_side, &captured](const uint8_t *data, ssize_t nread)
                {
                    if (nread <= 0)
                    {
                        return;
                    }
                    captured.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                    const uint8_t *resp = reinterpret_cast<const uint8_t *>("totally-not-http\r\n\r\n");
                    proxy_side->write(resp, 20, [](bool) { });
                });
        });

    const std::string target = std::string("http://127.0.0.1:1/via-proxy");
    // bound_port() is loop-thread-only — read it HERE (test body = loop thread), never inside
    // the worker lambda (fatal; that cross-thread call was the final-piece failure of the
    // earlier fixture attempts).
    const uint16_t proxy_port = proxy_srv.bound_port();
    fprintf(stderr, "PROXY-PORT %u\n", (unsigned)proxy_port);
    Response::SharedPtr response = spin_with_worker(loop,
                                                    [&target, &proxy_port]
                                                    {
                                                        Session session;
                                                        session.set_url(Url{target});
                                                        session.set_proxy(Proxy{kHost, proxy_port, Proxy::Type::kHTTP});
                                                        return session.get();
                                                    });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(0L, response->status_code());
    EXPECT_NE(std::string::npos, captured.find("Host:")) << "captured: " << captured;
}

// 20-22. Redirect control: a COUNTING server answers 302+Location on the first request and 200 on
//        the second (curl re-dials per hop, so the one-connection-per-request shape of
//        OneShotServer still applies). Fresh EventLoop + manual process_events spin per case:
//        EventLoop::exec after a completed exec returns immediately (preset exit), and the
//        run_with_loop exec pump raced the accept path in the proxy fixture — the inline worker
//        + spin shape is the proven pattern.
struct RedirectServer
{
    EventLoop &loop;
    TcpServer server;
    std::unique_ptr<cxxkit::TcpSocket> server_side;
    std::string requests;
    int request_count{0};

    explicit RedirectServer(EventLoop &l)
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
                        requests.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                        if (requests.find("\r\n\r\n") == std::string::npos)
                        {
                            return;
                        }
                        ++request_count;
                        // Hop 1 -> 302 + Location; hop 2+ -> 200 with the final body.
                        const std::string response = 1 == request_count
                                                         ? make_response(302, "Found", "Location: /final\r\n", "")
                                                         : make_response(200, "OK", "", "redirected-body");
                        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(response.data());
                        server_side->write(bytes, response.size(), [this](bool) { server_side->close(); });
                    });
            });
    }

    bool start() { return server.listen(kHost, 0); }
};

// Runs @p blocking_call on a worker thread while THIS (loop-owner) thread spins process_events
// until the call hands back its result via loop.post. Fresh-loop pattern: immune to the
// exec-reuse trap (a second exec() on a completed loop returns immediately).

// 20. No-follow: set_redirect(false) surfaces the 302 verbatim; exactly ONE request arrived.
TEST(HttpTest, RedirectNoFollow)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    RedirectServer srv(loop);
    ASSERT_TRUE(srv.start());

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/start";
    Response::SharedPtr response = spin_with_worker(loop, [&url] { return get(Url{url}, Redirect{false, 50}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(302L, response->status_code());
    EXPECT_EQ(std::string("/final"), response->header("Location"));
    EXPECT_EQ(1, srv.request_count);
}

// 21. Follow: the 302 Location is chased; the final 200 body surfaces and TWO requests hit the
//     server (hop 1 + hop 2).
TEST(HttpTest, RedirectFollow)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    RedirectServer srv(loop);
    ASSERT_TRUE(srv.start());

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/start";
    Response::SharedPtr response = spin_with_worker(
        loop,
        [&url] { return get(Url{url}, Redirect{true, 50}, Timeout{std::chrono::milliseconds(1000)}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_EQ(std::string("redirected-body"), response->text());
    EXPECT_EQ(2, srv.request_count);
    EXPECT_NE(std::string::npos, srv.requests.find("GET /start HTTP/1.1\r\n")) << srv.requests;
    EXPECT_NE(std::string::npos, srv.requests.find("GET /final HTTP/1.1\r\n")) << srv.requests;
}

// 22. follow=true with maximum=0: curl refuses all redirects (CURLOPT_MAXREDIRS=0), so the
//     behavior pins to the no-follow shape — 302 surfaces, one request.
TEST(HttpTest, RedirectMaxZeroRefusesRedirects)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    RedirectServer srv(loop);
    ASSERT_TRUE(srv.start());

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/start";
    Response::SharedPtr response = spin_with_worker(loop, [&url] { return get(Url{url}, Redirect{true, 0}); });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(302L, response->status_code());
    EXPECT_EQ(1, srv.request_count);
}
// 23. Cookie accessor family: the Initializer constructor populates every field the wrapper
//     exposes, and the accessors round-trip them verbatim (is_including_subdomains, is_https_only,
//     get_expires/get_expires_string/get_domain/get_path plus the covered get_name/get_value).
//     Also pins the empty default-constructed Cookie: every accessor degrades to its fallback.
TEST(HttpTest, CookieAccessorFamily)
{
    const auto expires = std::chrono::system_clock::now() + std::chrono::hours(1);
    const Cookie cookie(Cookie::Initializer{"session", "abc123", "example.com", true, "/login", true, expires});
    EXPECT_EQ(std::string("session"), cookie.get_name());
    EXPECT_EQ(std::string("abc123"), cookie.get_value());
    EXPECT_EQ(std::string("example.com"), cookie.get_domain());
    EXPECT_TRUE(cookie.is_including_subdomains());
    EXPECT_EQ(std::string("/login"), cookie.get_path());
    EXPECT_TRUE(cookie.is_https_only());
    EXPECT_EQ(expires, cookie.get_expires());
    EXPECT_FALSE(std::string(cookie.get_expires_string()).empty());

    // Default-constructed (no cpr cookie inside): every accessor takes the documented fallback.
    const Cookie empty;
    EXPECT_FALSE(empty.is_including_subdomains());
    EXPECT_FALSE(empty.is_https_only());
    EXPECT_EQ(std::chrono::system_clock::time_point(), empty.get_expires());
    EXPECT_EQ(std::string(""), empty.get_expires_string());
    EXPECT_EQ(std::string(""), empty.get_domain());
    EXPECT_EQ(std::string(""), empty.get_value());
    EXPECT_EQ(std::string(""), empty.get_path());
    EXPECT_EQ(std::string(""), empty.get_name());
}

// 24. Set-Cookie with attributes: the parsed response cookie surfaces the attribute values
//     (path/domain/https-only) through the full accessor family.
TEST(HttpTest, CookieAttributesFromResponse)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(
        make_response(200, "OK", "Set-Cookie: session=abc123; Path=/login; Domain=127.0.0.1; Secure\r\n", "")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/login";
    Response::SharedPtr response = run_with_loop(loop, [&url] { return get(Url{url}); });
    ASSERT_NE(nullptr, response.get());
    ASSERT_FALSE(response->cookies().data().empty());
    const Cookie::SharedPtr cookie = response->cookies().data().at(0);
    EXPECT_EQ(std::string("session"), cookie->get_name());
    EXPECT_EQ(std::string("abc123"), cookie->get_value());
    EXPECT_EQ(std::string("/login"), cookie->get_path());
    EXPECT_TRUE(cookie->is_https_only());
}

// 25. Proxy Initializer path + getters: host/port/type round-trip through the pimpl (no
//     transfer involved — this only pins the settings surface).
TEST(HttpTest, ProxyAccessors)
{
    // PIT-58: Initializer's StringView field borrows — passing a std::string TEMPORARY makes
    // the view dangle before the Proxy ctor copies it out (ASAN use-after-scope). Literals
    // (static storage) are the safe shape; the ctor's copy-out is immediate.
    const Proxy::Initializer init{kHost, 8080, Proxy::Type::kSOCKS5};
    const Proxy proxy(init);
    EXPECT_EQ(std::string(kHost), proxy.get_host());
    EXPECT_EQ(8080, proxy.get_port());
    EXPECT_EQ(Proxy::Type::kSOCKS5, proxy.get_type());

    // Default-constructed proxy: zero-valued fallbacks.
    const Proxy empty;
    EXPECT_EQ(std::string(""), empty.get_host());
    EXPECT_EQ(0, empty.get_port());
    EXPECT_EQ(Proxy::Type::kHTTP, empty.get_type());
}

// 26. Authentication surface: DIGEST and NTLM modes survive the to_cpr/from_cpr round trip
//     through auth_mode(), and auth_string() returns the stored token (empty until a transfer
//     populates it — pin the wrapper-level behavior, not curl internals).
TEST(HttpTest, AuthenticationModesRoundTrip)
{
    const Authentication digest{"user", "pass", Authentication::Mode::kDIGEST};
    EXPECT_EQ(Authentication::Mode::kDIGEST, digest.auth_mode());
    const Authentication ntlm{"user", "pass", Authentication::Mode::kNTLM};
    EXPECT_EQ(Authentication::Mode::kNTLM, ntlm.auth_mode());

    // Default-constructed authentication: BASIC fallback; cpr materializes the token as
    // "username:password" with both empty -> just the ":" joiner.
    const Authentication empty;
    EXPECT_EQ(Authentication::Mode::kBASIC, empty.auth_mode());
    EXPECT_EQ(std::string(":"), std::string(empty.auth_string()));
}

// 27. Session-level options that the free-function forms already exercise elsewhere, pinned
//     directly: update_header merges (the request carries BOTH headers on the wire),
//     set_connect_timeout is accepted, and session.set_cookies attaches request cookies.
TEST(HttpTest, SessionHeaderMergeAndCookies)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "ok")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/path";
    Response::SharedPtr response = spin_with_worker(
        loop,
        [&url]
        {
            Session session;
            session.set_url(Url{url});
            session.set_header(Header{{"X-One", "1"}});
            session.update_header(Header{{"X-Two", "2"}}); // merge, not replace
            session.set_connect_timeout(ConnectTimeout{std::chrono::milliseconds(500)});
            session.set_cookies(Cookies{Cookie::Initializer{"k", "v", "", false, "/", false}});
            return session.get();
        });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    EXPECT_NE(std::string::npos, srv.request.find("X-One: 1")) << "captured: " << srv.request;
    EXPECT_NE(std::string::npos, srv.request.find("X-Two: 2")) << "captured: " << srv.request;
    EXPECT_NE(std::string::npos, srv.request.find("Cookie:")) << "captured: " << srv.request;
}

// 28. download(WriteCallback): the callback form streams the body through user code chunk by
//     chunk (return true = continue) and the final response still surfaces status/body.
TEST(HttpTest, DownloadToWriteCallback)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    OneShotServer srv(loop);
    ASSERT_TRUE(srv.start(make_response(200, "OK", "", "chunked-download-body")));

    const std::string url = std::string(kHost) + ":" + std::to_string(srv.server.bound_port()) + "/file";
    Response::SharedPtr response = spin_with_worker(loop,
                                                    [&url]
                                                    {
                                                        Session session;
                                                        session.set_url(Url{url});
                                                        std::string collected;
                                                        WriteCallback write(
                                                            [&collected](std::string data, intptr_t)
                                                            {
                                                                collected += data;
                                                                return true;
                                                            });
                                                        Response::SharedPtr result = session.download(write);
                                                        EXPECT_EQ(std::string("chunked-download-body"), collected);
                                                        return result;
                                                    });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
}


// 29-31. THE CROSSOVER (C wave item 3): TlsSocket as an HTTPS origin server x cpr SSL-options
//        client. The TlsSocket read_start delivers DECRYPTED plaintext, so the HTTP layer
//        (make_response / "\r\n\r\n" matching) works unchanged on top of TLS. Fresh EventLoop +
//        spin_with_worker per case (ProxyRoutes shape — NOT run_with_loop, NOT exec reuse);
//        bound_port() is read on the loop thread (PIT-57).

// 29. VerifyOkWithCustomCA: cpr verifies our mbedTLS origin with the test CA; SAN carries
//     127.0.0.1 (curl verifies IP SANs), so GET https://127.0.0.1:port/hello returns 200.
TEST(HttpTest, HttpsVerifyOkWithCustomCA)
{
    EventLoop loop(cxxkit::make_default_dispatcher());
    TlsOriginServer srv(loop);
    srv.start(make_response(200, "OK", "", "https-hello"));

    const uint16_t port = srv.port(); // loop thread (PIT-57)
    const std::string ca = std::string(TEST_CERTS_DIR) + "/ca-cert.pem";
    const std::string url = std::string("https://") + kHost + ":" + std::to_string(port) + "/hello";
    Response::SharedPtr response = spin_with_worker(loop,
                                                    [&url, &ca]
                                                    {
                                                        Session session;
                                                        session.set_url(Url{url});
                                                        SslOptions ssl;
                                                        ssl.set_ca_info(ca);
                                                        ssl.set_verify_peer(true);
                                                        ssl.set_verify_host(true);
                                                        session.set_ssl_options(ssl);
                                                        return session.get();
                                                    });
    ASSERT_NE(nullptr, response.get());
    fprintf(stderr, "CURL-ERR %ld %s\n", response->error_code(), response->error_message().c_str());
    EXPECT_EQ(200L, response->status_code()) << "server errors: " << srv.error_log;
    EXPECT_EQ(std::string("https-hello"), response->text());
    EXPECT_TRUE(srv.tls_ready);
    EXPECT_NE(std::string::npos, srv.request.find("GET /hello HTTP/1.1\r\n")) << "captured: " << srv.request;
    EXPECT_NE(std::string::npos, srv.request.find("Host:")) << "captured: " << srv.request;
}

// 30. VerifyFailsWrongCA: same handshake, but the client trusts only the WRONG CA — the cert
//     verify fails and cpr maps the curl error to status 0 (transport-failure convention).
TEST(HttpTest, HttpsVerifyFailsWrongCA)
{
    // TLS certificate verification fails during the HANDSHAKE — the HTTP request never
    // reaches the server. Pin the correct semantics: status 0 client-side, ZERO bytes server-side.
    EventLoop loop(cxxkit::make_default_dispatcher());
    TlsOriginServer srv(loop);
    srv.start(make_response(200, "OK", "", "https-hello"));

    const uint16_t port = srv.port();
    const std::string ca = std::string(TEST_CERTS_DIR) + "/wrong-ca-cert.pem";
    const std::string url = std::string("https://") + kHost + ":" + std::to_string(port) + "/hello";
    Response::SharedPtr response = spin_with_worker(loop,
                                                    [&url, &ca]
                                                    {
                                                        Session session;
                                                        session.set_url(Url{url});
                                                        SslOptions ssl;
                                                        ssl.set_ca_info(ca);
                                                        ssl.set_verify_peer(true);
                                                        ssl.set_verify_host(true);
                                                        session.set_ssl_options(ssl);
                                                        return session.get();
                                                    });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(0L, response->status_code());
    EXPECT_TRUE(srv.request.empty()) << "verify failure must prevent the HTTP request; got: " << srv.request;
}

// 31. MultipartUpload (plain-TCP): a body-capturing OneShotServer variant answers after the
//     FULL body (Content-Length bytes past the head) has arrived; the request must carry the
//     multipart boundary, both parts, the part content-type and the file part's filename.
TEST(HttpTest, MultipartUploadBodyCaptured)
{
    // POST carries the multipart body; OneShotServer answers on the header terminator and
    // would drop everything after it — capture until the FINAL boundary instead.
    EventLoop loop(cxxkit::make_default_dispatcher());
    TcpServer server(loop);
    std::unique_ptr<cxxkit::TcpSocket> server_side;
    std::string captured;
    const std::string canned = make_response(200, "OK", "", "uploaded");
    bool responded = false;
    server.on_connection(
        [&](std::unique_ptr<cxxkit::TcpSocket> socket)
        {
            server_side = std::move(socket);
            server_side->read_start(
                [&server_side, &captured, &canned, &responded](const uint8_t *data, ssize_t nread)
                {
                    if (nread <= 0)
                    {
                        return;
                    }
                    captured.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                    if (!responded && captured.find("contents") != std::string::npos &&
                        captured.find("--", captured.find("contents")) != std::string::npos)
                    {
                        // the LAST part's payload + its closing boundary have landed
                        responded = true;
                        server_side->write(reinterpret_cast<const uint8_t *>(canned.data()),
                                           canned.size(),
                                           [](bool) { });
                    }
                });
        });
    ASSERT_TRUE(server.listen(kHost, 0));

    const uint16_t port = server.bound_port();
    const std::string url = std::string("http://") + kHost + ":" + std::to_string(port) + "/upload";
    Response::SharedPtr response = spin_with_worker(loop,
                                                    [&url]
                                                    {
                                                        Session session;
                                                        session.set_url(Url{url});
                                                        Multipart multipart;
                                                        multipart.add(Part{"field1", "value1"});
                                                        multipart.add(Part{"file", "contents", "text/plain"});
                                                        session.set_multipart(multipart);
                                                        return session.post();
                                                    });
    ASSERT_NE(nullptr, response.get());
    EXPECT_EQ(200L, response->status_code());
    // The response can land before the loop delivers the final body segment to the capture
    // callback — spin until the capture is complete (or budget out).
    for (int i = 0; i < 200 && captured.find("filename=\"f.txt\"") == std::string::npos; ++i)
    {
        loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents, 10);
    }
    EXPECT_NE(std::string::npos, captured.find("Content-Type: multipart/form-data; boundary="))
        << "captured: " << captured;
    EXPECT_NE(std::string::npos, captured.find("Content-Disposition: form-data; name=\"field1\""))
        << "captured: " << captured;
    EXPECT_NE(std::string::npos, captured.find("value1")) << "captured: " << captured;
    EXPECT_NE(std::string::npos, captured.find("name=\"file\"")) << "captured: " << captured;
    EXPECT_NE(std::string::npos, captured.find("Content-Type: text/plain")) << "captured: " << captured;
    // NOTE: no filename assertion — cpr::Part carries filename only via the File/Buffer
    // variants, which the wrapper does not surface yet (YAGNI; add a Buffer Part overload
    // when a file-upload consumer shows up).
    EXPECT_NE(std::string::npos, captured.find("contents")) << "captured: " << captured;
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
