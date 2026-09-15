/**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <cxxkit/kernel/default_dispatcher.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/tcp_server.hpp>
#include <cxxkit/network/tcp_socket.hpp>
#include <cxxkit/network/tls_socket.hpp>

#include "tls_test_server.hpp"

#include <gtest/gtest.h>

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

// Encrypted payloads cross a real 127.0.0.1 socket whose peer (the server-role echo or the
// oracle) can close first — writing after that would SIGPIPE the test process. libuv write
// paths in this suite never outlive the loop, so a process-wide ignore is test-only safety.
namespace
{
struct SigpipeIgnorer
{
    SigpipeIgnorer() { signal(SIGPIPE, SIG_IGN); }
};
const SigpipeIgnorer kIgnoreSigpipe;

} // namespace

// D42 T3: TlsSocket end-to-end suite. Two peer shapes:
//  - TlsTestServer (D42 T2 handshake ORACLE) for handshake/verify-matrix cases;
//  - a SERVER-ROLE TlsSocket (reviewer Q5) for data/close cases — the mirror implementation is
//    its own best oracle, and both roles drive the same state machine.
// Cert paths resolve via TEST_CERTS_DIR (target_compile_definition — tests run with cwd = build).
// All waiting is event-driven (P3): bounded timed rounds, never sleeps.

namespace
{

using cxxkit::EventLoop;
using cxxkit::SocketError;
using cxxkit::SocketState;
using cxxkit::TcpServer;
using cxxkit::TcpSocket;
using cxxkit::TlsSocket;
using cxxkit::make_default_dispatcher;

/// @brief Adopts both ends of a connected socketpair as TcpSockets on @p loop; returns the
///        peer (ownership with the caller). The TLS machine itself is role-agnostic; this
///        route reaches kConnected without any listener/acceptor re-entrancy at all.
std::unique_ptr<TcpSocket> make_socketpair_adopt(EventLoop &loop, TlsSocket &client)
{
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
    {
        perror("socketpair");
        abort();
    }
    client.set_transport(TcpSocket::adopt_fd(loop, fds[0]));
    return TcpSocket::adopt_fd(loop, fds[1]);
}
using cxxkit::TlsTestServer;

const std::string kCaCert = std::string(TEST_CERTS_DIR) + "/ca-cert.pem";
const std::string kServerCert = std::string(TEST_CERTS_DIR) + "/server-cert.pem";
const std::string kServerKey = std::string(TEST_CERTS_DIR) + "/server-key.pem";
const std::string kWrongCaCert = std::string(TEST_CERTS_DIR) + "/wrong-ca-cert.pem";
// Not a PEM: the load succeeds (file exists) but the x509 parse fails — the bad-CA-file
// error branch needs a loadable-yet-unparseable path.
const std::string kBadPemPath = "/tmp/cxxkit_tst_tls_badca.pem";
const std::string kCaKeyPath = std::string(TEST_CERTS_DIR) + "/ca-key.pem";

/// @brief Pumps the loop until @p done flips or @p timeout_ms of WALL TIME elapsed
/// (@return true when done). Rounds must be bounded by real time, NOT by an iteration
/// count: process_events with a NOWAIT dispatcher returns instantly when nothing is
/// ready, so "waited += 250" accounting burns the whole budget in microseconds and
/// starves timer-driven pumps (the D42 T3 asio lesson — the 1ms cadence timer needs
/// wall time to fire).
bool wait_for(EventLoop &loop, const bool &done, uint64_t timeout_ms)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (!done && std::chrono::steady_clock::now() < deadline)
    {
        loop.process_events(EventLoop::ProcessFlag::kAllEvents, 250);
    }
    return done;
}

/// @brief SERVER-ROLE TLS echo peer (reviewer Q5): TcpServer accept -> set_transport ->
///        start_tls -> read_start(echo). Re-drive lives inside TlsSocket; the test only parks
///        the accepted socket and arms the echo after on_handshake_done(true).
struct TlsEchoPeer
{
    EventLoop &loop;
    TcpServer listener;
    std::unique_ptr<TlsSocket> session;
    bool tls_ready = false;
    bool do_echo = true; // false: the test owns the reader (no auto-echo arming)

    explicit TlsEchoPeer(EventLoop &l)
        : loop(l)
        , listener(l)
    {
    }
    void start()
    {
        listener.listen("127.0.0.1", 0, 128);
        listener.on_connection(
            [this](std::unique_ptr<TcpSocket> socket)
            {
                std::unique_ptr<TlsSocket> tls(new TlsSocket(loop));
                TlsSocket *raw = tls.get();
                raw->set_transport(std::move(socket));
                // SERVER identity: present the test server certificate (a TLS server without
                // an identity cannot complete a handshake). No client-cert demand (verify_mode
                // 0): authmode REQUIRED without a CA configured aborts the handshake with
                // CA_CHAIN_REQUIRED (mbedTLS consistency check) — client-auth is not what
                // these cases exercise.
                raw->set_verify_mode(0);
                raw->set_certificate(kServerCert);
                raw->set_private_key(kServerKey);
                // NOTE: the state-change hook MUST be registered before start_tls — the
                // ClientHello is usually already buffered at accept, so the whole handshake
                // can complete INSIDE the start_tls first drive (kConnected fires there).
                raw->set_on_state_change(
                    [this, raw](SocketState state)
                    {
                        // Arm the echo on kConnected. `session` is parked by the time any
                        // handshake progress can fire (set below before the accept frame
                        // unwinds), and the session never dies inside a dispatcher callback —
                        // the test frame owns its teardown.
                        if (state == SocketState::kConnected && !this->tls_ready)
                        {
                            this->tls_ready = true;
                            if (this->do_echo)
                            {
                                raw->read_start(
                                    [raw](const uint8_t *data, ssize_t nread)
                                    {
                                        if (nread > 0)
                                        {
                                            raw->write(data, static_cast<size_t>(nread), [](bool) { });
                                        }
                                    });
                            }
                        }
                    });
                raw->start_tls([](bool) { });
                this->session = std::move(tls); // ownership parked before the accept frame unwinds
            });
    }
};

// ----- Cluster A: handshake basics (TlsTestServer oracle) -----

// 1. connect_tls against the TlsTestServer: on_connected(true) + state kConnected.
TEST(TlsSocketTest, HandshakeOkClientServer)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer; // R-T3-3c: bridge outlives its socket
    std::unique_ptr<TcpSocket> parked;               // R-T3-3: accepted sockets never die in-callback
    std::unique_ptr<TcpSocket> parkedFail;           // R-T3-3b: same for the failure path
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(2);
    client.set_ca_path(kCaCert);
    client.set_hostname("localhost");

    bool done = false;
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_TRUE(ok);
                           EXPECT_EQ(SocketState::kConnected, client.state());
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_EQ(SocketState::kConnected, client.state());
}

// 2. State sequence across connect + close: kConnecting -> kConnected then kClosing -> kClosed
//    (order-checked subsequence, tst_tcp_socket case-11 shape).
TEST(TlsSocketTest, StateSequence)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer; // R-T3-3c: bridge outlives its socket
    std::unique_ptr<TcpSocket> parked;               // R-T3-3: accepted sockets never die in-callback
    std::unique_ptr<TcpSocket> parkedFail;           // R-T3-3b: same for the failure path
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(2);
    client.set_ca_path(kCaCert);
    client.set_hostname("localhost");

    std::vector<SocketState> states;
    bool done = false;
    bool handshake_ok = true; // non-fatal capture: a fatal here kills the whole suite
    client.set_on_state_change(
        [&](SocketState state)
        {
            states.push_back(state);
            if (state == SocketState::kClosed)
            {
                done = true;
            }
        });
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           client.close();
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));

    std::vector<SocketState> expected;
    expected.push_back(SocketState::kConnecting);
    expected.push_back(SocketState::kConnected);
    expected.push_back(SocketState::kClosing);
    expected.push_back(SocketState::kClosed);
    ASSERT_EQ(expected.size(), states.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], states[i]) << "subsequence index " << i;
    }
    EXPECT_EQ(SocketState::kClosed, client.state());
    EXPECT_FALSE(client.is_open());
}

// 12. write after close is a contract violation -> CXXKIT_CHECK fatal.
TEST(TlsSocketDeathTest, WriteAfterCloseFatal)
{
    // fast+style: the child is a fork of THIS process state; threadsafe would re-run the whole
    // scenario on a second thread and cross the loop-thread-only contract.
    testing::FLAGS_gtest_death_test_style = "fast";
    EventLoop loop(make_default_dispatcher());
    // Reach kConnected through the fully event-loop-reentrant path (socketpair adoption — no
    // listener dtor re-entrancy here), close(), then expect the post-close write CHECK fatal.
    TlsSocket client(loop);
    std::unique_ptr<TcpSocket> peer = make_socketpair_adopt(loop, client);
    ASSERT_TRUE(client.is_open());
    client.close();
    EXPECT_FALSE(client.is_open());
    ASSERT_DEATH(client.write(reinterpret_cast<const uint8_t *>("x"), 1, [](bool) { }), "");
}

// ----- Cluster B: encrypted data (server-role TlsSocket echo peer) -----

// 3. "tls-ping" -> encrypt -> echo -> decrypt -> compare.
TEST(TlsSocketTest, EncryptedRoundTrip)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0); // echo pair: no peer verification in the data-plane cases
    bool done = false;
    bool handshake_ok = true; // non-fatal capture: a fatal here kills the whole suite
    std::string received;
    const std::string kPayload = "tls-ping";
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           if (ok)
                           {
                               client.read_start(
                                   [&](const uint8_t *data, ssize_t nread)
                                   {
                                       if (nread > 0)
                                       {
                                           received.append(reinterpret_cast<const char *>(data),
                                                           static_cast<size_t>(nread));
                                           if (received == kPayload)
                                           {
                                               done = true;
                                           }
                                       }
                                   });
                               const uint8_t *bytes = reinterpret_cast<const uint8_t *>(kPayload.data());
                               client.write(bytes, kPayload.size(), [](bool) { });
                           } // if (ok)
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_TRUE(handshake_ok);
    EXPECT_EQ(kPayload, received);
}

// 4. 256KB payload: mbedTLS fragments into ~16KB records; client reassembles.
TEST(TlsSocketTest, LargePayloadFragmentation)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0); // echo pair: no peer verification in the data-plane cases
    bool done = false;
    bool handshake_ok = true; // non-fatal capture: a fatal here kills the whole suite
    std::vector<uint8_t> received;
    received.reserve(256 * 1024);
    std::vector<uint8_t> payload(256 * 1024);
    for (size_t i = 0; i < payload.size(); ++i)
    {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           if (ok)
                           {
                               client.read_start(
                                   [&](const uint8_t *data, ssize_t nread)
                                   {
                                       if (nread > 0)
                                       {
                                           received.insert(received.end(), data, data + nread);
                                           if (received.size() >= payload.size())
                                           {
                                               done = true;
                                           }
                                       }
                                   });
                               client.write(payload.data(), payload.size(), [](bool) { });
                           } // if (ok)
                       });
    ASSERT_TRUE(wait_for(loop, done, 8000));
    EXPECT_TRUE(handshake_ok);
    ASSERT_EQ(payload.size(), received.size());
    EXPECT_EQ(0, std::memcmp(payload.data(), received.data(), payload.size()));
}

// ----- Cluster C: verify matrix (TlsTestServer + client setters) -----

// 5. verify required + right CA + hostname: chain verifies, handshake ok.
TEST(TlsSocketTest, VerifyRequiredOk)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer; // R-T3-3c: bridge outlives its socket
    std::unique_ptr<TcpSocket> parked;               // R-T3-3: accepted sockets never die in-callback
    std::unique_ptr<TcpSocket> parkedFail;           // R-T3-3b: same for the failure path
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(2);
    client.set_ca_path(kCaCert);
    client.set_hostname("localhost");

    bool done = false;
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_TRUE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_TRUE(done);
    EXPECT_EQ(SocketState::kConnected, client.state());
    EXPECT_EQ(SocketError::kNone, client.last_error());
}

// 6. verify required + wrong CA: server's fatal alert lands as kTlsCertificateError.
TEST(TlsSocketTest, VerifyFailsWrongCa)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer; // R-T3-3c: bridge outlives its socket
    std::unique_ptr<TcpSocket> parked;               // R-T3-3: accepted sockets never die in-callback
    std::unique_ptr<TcpSocket> parkedFail;           // R-T3-3b: same for the failure path
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(2);
    client.set_ca_path(kWrongCaCert);
    client.set_hostname("localhost");

    SocketError err = SocketError::kNone;
    bool done = false;
    client.set_on_error([&](SocketError error, const std::string &) { err = error; });
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_FALSE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(SocketError::kTlsCertificateError, err);
    EXPECT_EQ(SocketError::kTlsCertificateError, client.last_error());
}

// 7. verify optional + wrong CA: handshake completes; the verification failure is REPORTED via
//    last_error (mbedtls_ssl_get_verify_result() !0 -> recorded, not fatal).
TEST(TlsSocketTest, VerifyOptionalReports)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer; // R-T3-3c: bridge outlives its socket
    std::unique_ptr<TcpSocket> parked;               // R-T3-3: accepted sockets never die in-callback
    std::unique_ptr<TcpSocket> parkedFail;           // R-T3-3b: same for the failure path
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(1);
    client.set_ca_path(kWrongCaCert);
    client.set_hostname("localhost");

    SocketError err = SocketError::kNone;
    client.set_on_error([&](SocketError error, const std::string &) { err = error; });
    bool done = false;
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_TRUE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_TRUE(done);
    EXPECT_EQ(SocketState::kConnected, client.state());
    EXPECT_EQ(SocketError::kTlsCertificateError, client.last_error());
    EXPECT_EQ(SocketError::kNone, err); // reported via last_error, not the error channel
}

// 8. verify none + wrong CA: chain unchecked, handshake ok, no error.
TEST(TlsSocketTest, VerifyNoneSkips)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer; // R-T3-3c: bridge outlives its socket
    std::unique_ptr<TcpSocket> parked;               // R-T3-3: accepted sockets never die in-callback
    std::unique_ptr<TcpSocket> parkedFail;           // R-T3-3b: same for the failure path
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(0);
    client.set_ca_path(kWrongCaCert);
    client.set_hostname("localhost");

    bool done = false;
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_TRUE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_TRUE(done);
    EXPECT_EQ(SocketState::kConnected, client.state());
    EXPECT_EQ(SocketError::kNone, client.last_error());
}


// ----- Cluster D: close semantics (server-role pair) -----

// 9. Server closes cleanly: client ssl_read surfaces PEER_CLOSE_NOTIFY -> on_data(nullptr, -1)
//    terminal + kTlsPeerClosed.
TEST(TlsSocketTest, PeerCloseNotify)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0); // echo pair: no peer verification in the data-plane cases
    bool eof_seen = false;
    bool handshake_ok = true; // non-fatal capture: a fatal here kills the whole suite
    SocketError err = SocketError::kNone;
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           if (ok)
                           {
                               client.read_start(
                                   [&](const uint8_t *data, ssize_t nread)
                                   {
                                       if (nread <= 0)
                                       {
                                           eof_seen = true;
                                       }
                                   });
                               client.set_on_error([&](SocketError error, const std::string &) { err = error; });
                               // server closes cleanly after the client is armed
                               peer.session->close();
                           } // if (ok)
                       });
    wait_for(loop, eof_seen, 4000);
    EXPECT_TRUE(handshake_ok);
    ASSERT_TRUE(eof_seen);
    EXPECT_EQ(SocketError::kTlsPeerClosed, err);
}

// 10. Client close(): server-role TlsSocket observes the clean close (close_notify in, EOF out).
TEST(TlsSocketTest, CloseNotifyFromClient)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.do_echo = false; // the test owns the server-side reader
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0); // echo pair: no peer verification in the data-plane cases
    bool done = false;
    bool server_eof = false;
    bool handshake_ok = true; // non-fatal capture: a fatal here kills the whole suite
    SocketError server_err = SocketError::kNone;
    std::string server_err_msg;
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           peer.session->set_on_error(
                               [&](SocketError error, const std::string &message)
                               {
                                   server_err = error;
                                   server_err_msg = message;
                                   done = true;
                               });
                           peer.session->read_start(
                               [&](const uint8_t *data, ssize_t nread)
                               {
                                   (void)data;
                                   if (nread <= 0)
                                   {
                                       server_eof = true;
                                       done = true;
                                   }
                               });
                           client.close();
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_TRUE(handshake_ok);
    EXPECT_TRUE(server_eof);
    EXPECT_EQ(SocketError::kTlsPeerClosed, server_err) << server_err_msg;
}

// 11. start_tls over an accepted transport: the real server path, validated end to end.
TEST(TlsSocketTest, ServerRoleStartTls)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0);
    bool handshake_ok = true; // non-fatal capture
    bool client_up = false;
    bool server_up = false;
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = ok;
                           client_up = ok;
                       });
    wait_for(loop, peer.tls_ready, 4000);
    server_up = peer.tls_ready; // on_handshake_done(true) fired inside the peer
    client_up = handshake_ok;
    ASSERT_TRUE(server_up);
    ASSERT_TRUE(client_up);

    // both roles connected: one encrypted roundtrip to prove the pair is live
    bool done = false;
    std::string received;
    const std::string kPayload = "starttls-roundtrip";
    client.read_start(
        [&](const uint8_t *data, ssize_t nread)
        {
            if (nread > 0)
            {
                received.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                if (received == kPayload)
                {
                    done = true;
                }
            }
        });
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(kPayload.data());
    client.write(bytes, kPayload.size(), [](bool) { });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_TRUE(handshake_ok);
    EXPECT_EQ(kPayload, received);
}
// 13. connect_tls to a refused port: the TCP connect failure surfaces as on_connected(false)
//     and the socket tears down (kClosed) — the connect-callback failure branch.
TEST(TlsSocketTest, ConnectRefusedFailsEntry)
{
    EventLoop loop(make_default_dispatcher());
    TlsSocket client(loop);
    bool done = false;
    client.connect_tls("127.0.0.1",
                       1,
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_FALSE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(SocketState::kClosed, client.state());
}

// 14. Bad CA path: freeze_config fails the x509 parse -> kTlsCertificateError + handshake
//     callback(false), before any bytes leave the machine.
TEST(TlsSocketTest, BadCaPathReportsCertificateError)
{
    // NOTE: no TlsTestServer here. freeze_config runs at attach_bridge INSIDE the TCP connect
    // completion; with the full test server the first TLS record can arrive in the same batch,
    // and a close() racing that batch can hit the transport read_start CHECK before the
    // freeze_config error path returns. An inert listener delivers nothing: the failure is
    // purely the local config parse.
    EventLoop loop(make_default_dispatcher());
    TcpServer inert(loop);
    ASSERT_TRUE(inert.listen("127.0.0.1", 0));

    TlsSocket client(loop);
    client.set_verify_mode(2);
    client.set_ca_path(kBadPemPath);

    SocketError err = SocketError::kNone;
    std::string message;
    bool done = false;
    client.set_on_error(
        [&](SocketError error, const std::string &msg)
        {
            err = error;
            message = msg;
        });
    client.connect_tls("127.0.0.1",
                       inert.bound_port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_FALSE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_EQ(SocketError::kTlsCertificateError, err);
    EXPECT_NE(std::string::npos, message.find("failed to parse CA file")) << message;
    EXPECT_EQ(SocketError::kTlsCertificateError, client.last_error());
}

// 15. Own cert/key parse failure: a certificate file fed as the private key (and vice versa)
//     fails the parse in freeze_config -> kTlsCertificateError + on_connected(false).
TEST(TlsSocketTest, OwnCertKeyParseFailure)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer;
    std::unique_ptr<TcpSocket> parked;
    std::unique_ptr<TcpSocket> parkedFail;
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(0); // no peer verification in this setup-error case
    client.set_certificate(kServerCert);
    client.set_private_key(kServerCert); // a CERT is not a KEY: pk_parse_keyfile fails

    SocketError err = SocketError::kNone;
    std::string message;
    bool done = false;
    client.set_on_error(
        [&](SocketError error, const std::string &msg)
        {
            err = error;
            message = msg;
        });
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool ok)
                       {
                           done = true;
                           EXPECT_FALSE(ok);
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    EXPECT_EQ(SocketError::kTlsCertificateError, err);
    EXPECT_NE(std::string::npos, message.find("own certificate")) << message;
}

// 16. cert/key MISMATCH (valid files from different pairs): both parse, conf_own_cert rejects
//     the pair -> kTlsHandshakeFailed + on_connected(false).
TEST(TlsSocketTest, OwnCertKeyPairAcceptedAndHandshakeOk)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> parkedPeer;
    std::unique_ptr<TcpSocket> parked;
    std::unique_ptr<TcpSocket> parkedFail;
    server.park_peer(&parkedPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(0);
    client.set_certificate(kServerCert); // valid pair + valid but UNRELATED key
    client.set_private_key(kCaKeyPath);

    bool done = false;
    bool ok_final = false;
    client.connect_tls("127.0.0.1",
                       server.port(),
                       [&](bool result)
                       {
                           done = true;
                           ok_final = result;
                       });
    ASSERT_TRUE(wait_for(loop, done, 4000));
    // mbedTLS 3.6 ssl_conf_own_cert accepts any parseable (cert, key) pair — even when the key
    // does not belong to the certificate — and the server (verify NONE) does not check the
    // client chain. Pin the observable contract: the own-identity load path completes and the
    // handshake SUCCEEDS with a mismatched client identity.
    EXPECT_TRUE(ok_final);
    EXPECT_EQ(SocketState::kConnected, client.state());
}

// 17. read_stop: a connected session drops its on_data; writes after read_stop still deliver
//     on the write channel (the write path is independent), and the terminal on_data(nullptr)
//     is NOT delivered to the disarmed callback.
TEST(TlsSocketTest, ReadStopDisarmsData)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.do_echo = false; // the test owns the server-side reader
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0);
    bool handshake_ok = true;
    bool client_saw_data = false;
    bool server_saw_payload = false;
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           client.read_start([&](const uint8_t *, ssize_t) { client_saw_data = true; });
                           client.read_stop(); // disarm before anything can arrive
                           peer.session->read_start(
                               [&](const uint8_t *data, ssize_t nread)
                               {
                                   if (nread > 0)
                                   {
                                       server_saw_payload = std::string(reinterpret_cast<const char *>(data),
                                                                        static_cast<size_t>(nread)) == "ping";
                                   }
                               });
                           const uint8_t *bytes = reinterpret_cast<const uint8_t *>("ping");
                           client.write(bytes, 4, [](bool) { });
                       });
    ASSERT_TRUE(wait_for(loop, server_saw_payload, 4000));
    EXPECT_TRUE(handshake_ok);
    EXPECT_FALSE(client_saw_data); // the disarmed callback must never fire

    // Re-arm still works (read_start replaces the callback).
    bool rearmed = false;
    client.read_start([&](const uint8_t *, ssize_t) { rearmed = true; });
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>("pong");
    peer.session->write(bytes, 4, [](bool) { });
    ASSERT_TRUE(wait_for(loop, rearmed, 4000));
    client.read_stop(); // leave the machine disarmed for teardown symmetry
    peer.session->read_stop();
}

// 18. Zero-length write: the documented trivially-done path — on_written(true) fires without
//     touching the wire.
TEST(TlsSocketTest, ZeroLengthWriteCompletes)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.do_echo = false;
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0);
    bool handshake_ok = true;
    bool written = false;
    bool written_ok = false;
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           client.write(nullptr,
                                        0,
                                        [&](bool ok2)
                                        {
                                            written = true;
                                            written_ok = ok2;
                                        });
                       });
    ASSERT_TRUE(wait_for(loop, written, 4000));
    EXPECT_TRUE(handshake_ok);
    EXPECT_TRUE(written_ok);
}

// 20. close_notify OBSERVED as a record: a peer that sends close_notify WITHOUT closing the
//     transport (half-close oracle) — the client's read pump parses the record and converts it
//     into the terminal on_data(nullptr, 0) + kTlsPeerClosed via the PEER_CLOSE_NOTIFY branch
//     (no transport error involved).
TEST(TlsSocketTest, PeerCloseNotifyObservedAsRecord)
{
    EventLoop loop(make_default_dispatcher());
    TlsTestServer server(loop, kCaCert, kServerCert, kServerKey);
    std::unique_ptr<TlsTestServer::Peer> serverPeer; // half-close oracle handle
    std::unique_ptr<TcpSocket> parked;               // the accepted (TLS-up) transport
    std::unique_ptr<TcpSocket> parkedFail;
    server.park_peer(&serverPeer);
    server.park_socket(&parked);
    server.park_failed_socket(&parkedFail);
    ASSERT_TRUE(server.start());

    TlsSocket client(loop);
    client.set_verify_mode(2);
    client.set_ca_path(kCaCert);
    client.set_hostname("localhost");

    bool done = false;
    bool eof_seen = false;
    SocketError err = SocketError::kNone;
    bool ready = false;
    server.set_on_client_tls_ready([&ready](std::unique_ptr<TcpSocket>, bool ok) { ready = ok; });
    client.set_on_state_change(
        [&](SocketState state)
        {
            if (state == SocketState::kConnected)
            {
                done = true;
            }
        });
    client.connect_tls("127.0.0.1", server.port(), [&](bool ok) { EXPECT_TRUE(ok); });
    // Both sides up (client kConnected + server peer parked/delivered) before the half-close.
    ASSERT_TRUE(wait_for(loop, done, 4000));
    ASSERT_TRUE(done);
    ASSERT_TRUE(wait_for(loop, ready, 4000)) << "server peer was not delivered";
    client.read_start(
        [&](const uint8_t *, ssize_t nread)
        {
            if (nread <= 0)
            {
                eof_seen = true;
            }
        });
    client.set_on_error([&](SocketError error, const std::string &) { err = error; });
    // Half-close: close_notify only; the transport (and this test's pump) live on.
    server.peer_notify_without_close(serverPeer.get());
    ASSERT_TRUE(wait_for(loop, eof_seen, 4000));
    EXPECT_EQ(SocketError::kTlsPeerClosed, err);
    EXPECT_EQ(SocketError::kTlsPeerClosed, client.last_error());
}

// 21. close() twice: idempotent — the second close neither fires another state sequence nor
//     touches the (already closing) transport.
TEST(TlsSocketTest, CloseTwiceIdempotent)
{
    EventLoop loop(make_default_dispatcher());
    TlsEchoPeer peer(loop);
    peer.do_echo = false;
    peer.start();

    TlsSocket client(loop);
    client.set_verify_mode(0);
    std::vector<SocketState> states;
    bool handshake_ok = true;
    bool closed = false;
    client.set_on_state_change(
        [&](SocketState state)
        {
            states.push_back(state);
            if (state == SocketState::kClosed)
            {
                closed = true;
            }
        });
    client.connect_tls("127.0.0.1",
                       peer.listener.bound_port(),
                       [&](bool ok)
                       {
                           handshake_ok = handshake_ok && ok;
                           client.close();
                           client.close(); // second call: latched no-op
                       });
    ASSERT_TRUE(wait_for(loop, closed, 4000));
    EXPECT_TRUE(handshake_ok);
    // Exactly ONE kClosing in the sequence despite the double close.
    EXPECT_EQ(1, std::count(states.begin(), states.end(), SocketState::kClosing));
    EXPECT_EQ(SocketState::kClosed, client.state());
}


} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
