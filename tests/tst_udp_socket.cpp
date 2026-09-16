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
#include <cxxkit/kernel/default_dispatcher.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/udp_socket.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

// D44 T4: loopback UDP suite. The main test thread IS the loop thread (tst_tcp_socket shape):
// every socket call happens on the caller stack, all waiting is bounded process_events spinning.
// Receive interest is always armed BEFORE the first send (arm-then-send discipline — a datagram
// can never race an unarmed receiver).
//
// Zero-length datagrams are deliberately NOT covered: uv delivers nread == 0 as an idle
// keep-alive (not a datagram) and the platform behavior around empty datagrams differs
// (Linux delivers them, some stacks coalesce/drop) — deferred until a consumer needs it.

namespace
{

using cxxkit::EventLoop;
using cxxkit::UdpSocket;
using cxxkit::make_default_dispatcher;

/** @brief Bounded main-thread spin: pumps the loop until @p done or ~@p rounds × @p per_round_ms elapse. */
void spin_until(EventLoop &loop, const std::function<bool()> &done, int rounds = 200, uint64_t per_round_ms = 10)
{
    for (int i = 0; i < rounds && !done(); ++i)
    {
        loop.process_events(EventLoop::ProcessFlag::kAllEvents, per_round_ms);
    }
}

// 1. bind(127.0.0.1, 0) is synchronous: true, kBound, and the OS-assigned port reads back non-zero.
TEST(UdpSocketTest, BindEphemeralReadback)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket socket(loop);
    ASSERT_EQ(cxxkit::SocketState::kIdle, socket.state());

    EXPECT_TRUE(socket.bind("127.0.0.1", 0));
    EXPECT_EQ(cxxkit::SocketState::kBound, socket.state());
    EXPECT_NE(0u, socket.bound_port());
    EXPECT_EQ(cxxkit::SocketError::kNone, socket.last_error());
}

// 2. Binding a second socket to the exact same address:port fails: false, stays kIdle (retryable),
//    the failure maps onto kAddressInUse on both the callback and last_error surfaces.
TEST(UdpSocketTest, BindConflictAddressInUse)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket first(loop);
    ASSERT_TRUE(first.bind("127.0.0.1", 0));
    const uint16_t port = first.bound_port();
    ASSERT_NE(0u, port);

    UdpSocket second(loop);
    bool error_called = false;
    cxxkit::SocketError got = cxxkit::SocketError::kNone;
    second.set_on_error(
        [&](cxxkit::SocketError error, const std::string &)
        {
            error_called = true;
            got = error;
        });

    EXPECT_FALSE(second.bind("127.0.0.1", port));          // same addr:port, no SO_REUSEADDR -> EADDRINUSE
    EXPECT_EQ(cxxkit::SocketState::kIdle, second.state()); // failed bind stays kIdle (retryable)
    EXPECT_TRUE(error_called);
    EXPECT_EQ(cxxkit::SocketError::kAddressInUse, got);
    EXPECT_EQ(cxxkit::SocketError::kAddressInUse, second.last_error());
}

// 3. Explicit-bind roundtrip: B sends to A's bound port; A's on_datagram delivers the payload plus
//    the correct sender identity (loopback ip + B's bound port).
TEST(UdpSocketTest, SendReceiveRoundTripWithSenderIdentity)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket receiver(loop);
    UdpSocket sender(loop);
    ASSERT_TRUE(receiver.bind("127.0.0.1", 0));
    ASSERT_TRUE(sender.bind("127.0.0.1", 0));
    const uint16_t receiver_port = receiver.bound_port();
    const uint16_t sender_port = sender.bound_port();

    bool got = false;
    std::string data;
    std::string sender_ip;
    uint16_t sender_port_seen = 0;
    receiver.set_on_datagram(
        [&](const std::string &payload, const std::string &ip, uint16_t port)
        {
            data = payload;
            sender_ip = ip;
            sender_port_seen = port;
            got = true;
        });

    const std::string kPayload = "udp-roundtrip";
    sender.send_to(kPayload, "127.0.0.1", receiver_port, [](bool ok) { EXPECT_TRUE(ok); });
    spin_until(loop, [&] { return got; });

    ASSERT_TRUE(got);
    EXPECT_EQ(kPayload, data);
    EXPECT_EQ(std::string("127.0.0.1"), sender_ip);
    EXPECT_EQ(sender_port, sender_port_seen);
}

// 4. Lazy bind (T2 ruling): a kIdle socket may send — the send implicitly binds an IPv4 ephemeral
//    endpoint, auto-transitions kIdle -> kBound, and the datagram still arrives at the receiver.
TEST(UdpSocketTest, LazyBindUnboundSend)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket receiver(loop);
    UdpSocket sender(loop);
    ASSERT_TRUE(receiver.bind("127.0.0.1", 0));
    const uint16_t receiver_port = receiver.bound_port();
    ASSERT_EQ(cxxkit::SocketState::kIdle, sender.state()); // never bound

    bool got = false;
    std::string data;
    receiver.set_on_datagram(
        [&](const std::string &payload, const std::string &, uint16_t)
        {
            data = payload;
            got = true;
        });

    bool sent_ok = false;
    sender.send_to("lazy-bind-ping", "127.0.0.1", receiver_port, [&](bool ok) { sent_ok = ok; });
    // The lazy bind is synchronous inside send_to: the machine must already be kBound with a
    // real (non-zero) ephemeral port before any pump.
    EXPECT_EQ(cxxkit::SocketState::kBound, sender.state());
    EXPECT_NE(0u, sender.bound_port());

    spin_until(loop, [&] { return got && sent_ok; });
    EXPECT_TRUE(sent_ok);
    ASSERT_TRUE(got);
    EXPECT_EQ("lazy-bind-ping", data);
}

// 5. send_to on a closed socket is a discard, not a crash/fatal: the done callback fires with
//    false synchronously on the caller stack and the machine stays kClosed.
TEST(UdpSocketTest, SendAfterCloseDiscardsWithFalse)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket socket(loop);
    ASSERT_TRUE(socket.bind("127.0.0.1", 0));
    socket.close();
    ASSERT_EQ(cxxkit::SocketState::kClosed, socket.state());

    bool done_called = false;
    bool done_ok = true;
    socket.send_to("after-close",
                   "127.0.0.1",
                   1,
                   [&](bool ok)
                   {
                       done_called = true;
                       done_ok = ok;
                   });
    EXPECT_TRUE(done_called); // completed synchronously on the caller stack
    EXPECT_FALSE(done_ok);    // discard completion, documented semantics
    EXPECT_EQ(cxxkit::SocketState::kClosed, socket.state());
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 100); // quiet teardown, no abort
}

// 6. Destroying a bound, armed, traffic-carrying socket without close(): the destructor closes and
//    pumps until the close callback ran — no crash, no leak (ASAN watch), loop still usable.
TEST(UdpSocketTest, DestructorWithoutClose)
{
    EventLoop loop(make_default_dispatcher());
    {
        UdpSocket receiver(loop);
        UdpSocket sender(loop);
        ASSERT_TRUE(receiver.bind("127.0.0.1", 0));
        ASSERT_TRUE(sender.bind("127.0.0.1", 0));

        int hits = 0;
        receiver.set_on_datagram([&](const std::string &, const std::string &, uint16_t) { ++hits; });
        sender.send_to("pre-dtor", "127.0.0.1", receiver.bound_port(), [](bool) { });
        spin_until(loop, [&] { return hits > 0; });
        EXPECT_GE(hits, 1);
    } // both dtors here: close + pump happens inside ~UdpSocket
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 100); // loop still usable after
}

// 7. A near-page datagram (8192 bytes — loopback MTU is fine) survives the roundtrip byte-exact.
TEST(UdpSocketTest, LargeDatagramRoundTrip)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket receiver(loop);
    UdpSocket sender(loop);
    ASSERT_TRUE(receiver.bind("127.0.0.1", 0));
    ASSERT_TRUE(sender.bind("127.0.0.1", 0));

    std::vector<char> pattern(8192);
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        pattern[i] = static_cast<char>((i * 31 + 7) % 251);
    }
    const std::string kPayload(pattern.begin(), pattern.end());

    bool got = false;
    std::string data;
    receiver.set_on_datagram(
        [&](const std::string &payload, const std::string &, uint16_t)
        {
            data = payload;
            got = true;
        });

    sender.send_to(kPayload, "127.0.0.1", receiver.bound_port(), [](bool ok) { EXPECT_TRUE(ok); });
    spin_until(loop, [&] { return got; });

    ASSERT_TRUE(got);
    EXPECT_EQ(kPayload.size(), data.size());
    EXPECT_EQ(kPayload, data); // byte-exact: no truncation, no reassembly drift
}

// 8. Two-socket ping-pong: A sends to B; B's on_datagram replies to the sender identity it saw;
//    A's on_datagram receives the reply — both deliveries land.
TEST(UdpSocketTest, TwoSocketPingPong)
{
    EventLoop loop(make_default_dispatcher());
    UdpSocket a(loop);
    UdpSocket b(loop);
    ASSERT_TRUE(a.bind("127.0.0.1", 0));
    ASSERT_TRUE(b.bind("127.0.0.1", 0));

    bool b_got = false;
    bool a_got = false;
    std::string a_data;
    b.set_on_datagram(
        [&](const std::string &, const std::string &ip, uint16_t port)
        {
            b_got = true;
            b.send_to("pong", ip, port, [](bool ok) { EXPECT_TRUE(ok); }); // reply to the seen sender
        });
    a.set_on_datagram(
        [&](const std::string &payload, const std::string &, uint16_t)
        {
            a_data = payload;
            a_got = true;
        });

    a.send_to("ping", "127.0.0.1", b.bound_port(), [](bool ok) { EXPECT_TRUE(ok); });
    spin_until(loop, [&] { return b_got && a_got; });

    EXPECT_TRUE(b_got);
    ASSERT_TRUE(a_got);
    EXPECT_EQ("pong", a_data);
}

} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
