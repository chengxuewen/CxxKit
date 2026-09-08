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
#include <cxxkit/uv/tcp_socket.hpp>

#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{

using cxxkit::EventLoop;
using cxxkit::TcpSocket;
using cxxkit::make_uv_dispatcher;

// R-T2-1: both ends of a socketpair adopted into the SAME loop as two TcpSockets (uv_tcp_open) — the
// test rig needs no listener, no T3 TcpServer, no addresses. The main test thread is the loop thread.
// All waiting is event-driven (P3): timers/exit from callbacks, never sleeps.

/** @brief Makes a connected socketpair adopted into two TcpSockets on @p loop. */
struct SocketPair
{
    EventLoop &loop;
    std::unique_ptr<TcpSocket> a;
    std::unique_ptr<TcpSocket> b;
    int rawA;
    int rawB;

    explicit SocketPair(EventLoop &l)
        : loop(l)
    {
        int fds[2];
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
        {
            perror("socketpair");
            abort();
        }
        rawA = fds[0];
        rawB = fds[1];
        a = TcpSocket::adopt_fd(loop, rawA); // uv_tcp_open takes over the fd from here
        b = TcpSocket::adopt_fd(loop, rawB);
    }
};

// 1. Round trip: A writes, B's read_start delivers the same bytes (echo back for full-duplex proof).
TEST(TcpSocketTest, ConnectEchoRoundTrip)
{
    EventLoop loop(make_uv_dispatcher());
    SocketPair pair(loop);

    std::string received;
    const std::string kPayload = "ping-from-a";

    pair.a->read_start(
        [&](const uint8_t *data, ssize_t nread)
        {
            if (nread > 0)
            {
                received.append(reinterpret_cast<const char *>(data), static_cast<size_t>(nread));
                if (received == "pong-from-b")
                {
                    loop.exit(0);
                }
            }
        });
    pair.b->read_start(
        [&](const uint8_t *data, ssize_t nread)
        {
            if (nread > 0)
            {
                // echo back: B -> A
                pair.b->write(data, static_cast<size_t>(nread), [&](bool) { });
            }
        });

    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(kPayload.data());
    pair.a->write(bytes, kPayload.size(), [&](bool ok) { EXPECT_TRUE(ok); });

    // Bounded event-driven wait (P3): timed rounds until the pong lands; 2s per round bounds a
    // regression instead of hanging. A->B write, B echoes, A receives: two rounds suffice.
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    EXPECT_EQ("ping-from-a", received);
}

// 2. Backpressure (lws): writes issued while one is in flight queue FIFO; completions arrive in order.
TEST(TcpSocketTest, WriteBackpressureQueueing)
{
    EventLoop loop(make_uv_dispatcher());
    SocketPair pair(loop);

    std::vector<int> completed;

    // Queue three writes back-to-back BEFORE any loop round: at most the first can be in flight when
    // the rest are submitted — they must queue and complete strictly in submission order.
    for (int i = 0; i < 3; ++i)
    {
        const uint8_t byte = static_cast<uint8_t>('0' + i);
        pair.a->write(&byte,
                      1,
                      [&, i](bool ok)
                      {
                          EXPECT_TRUE(ok);
                          completed.push_back(i); // capture i BY VALUE: the loop var is dead by delivery
                      });
    }

    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 2000);
    // drain the peer so write completions can fire (socketpair buffers are big; completion is
    // kernel-ack, not peer-read) — then verify order.
    EXPECT_EQ(static_cast<size_t>(3), completed.size());
    for (size_t i = 0; i < completed.size() && i < 3; ++i)
    {
        EXPECT_EQ(static_cast<int>(i), completed[i]); // FIFO order preserved
    }
}

// 3. read_stop disarms the read interest: bytes written after read_stop deliver nothing.
TEST(TcpSocketTest, ReadStopRespectsMask)
{
    EventLoop loop(make_uv_dispatcher());
    SocketPair pair(loop);

    int hits = 0;
    pair.b->read_start(
        [&](const uint8_t *, ssize_t nread)
        {
            if (nread > 0)
            {
                ++hits;
            }
        });

    const uint8_t byte = 'x';
    pair.a->write(&byte, 1, [&](bool) { });
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 1000); // delivers once
    ASSERT_EQ(1, hits);

    pair.b->read_stop(); // interest mask: read bit off
    pair.a->write(&byte, 1, [&](bool) { });
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 200); // quiet: nothing delivers
    EXPECT_EQ(1, hits);                                           // still exactly one delivery
}

// 4. I3: close() from inside an on_data callback must not crash — the socket tears itself down safely.
TEST(TcpSocketTest, CloseInsideCallback)
{
    EventLoop loop(make_uv_dispatcher());
    SocketPair pair(loop);

    bool closed_inside = false;
    bool got_data = false;
    pair.b->read_start(
        [&](const uint8_t *, ssize_t nread)
        {
            if (nread > 0)
            {
                got_data = true;
                pair.b->close(); // I3: self-close mid-callback
                closed_inside = true;
            }
        });

    const uint8_t byte = 'c';
    pair.a->write(&byte, 1, [&](bool) { });
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 1000);
    EXPECT_TRUE(got_data);
    EXPECT_TRUE(closed_inside);
    EXPECT_FALSE(pair.b->is_open());
}

// 5. I6: destructor with an in-flight transfer must pump the loop clean — no uv state survives (ASAN watch).
TEST(TcpSocketTest, DestructorMidTransfer)
{
    EventLoop loop(make_uv_dispatcher());
    {
        SocketPair pair(loop);
        int reads = 0;
        pair.b->read_start([&](const uint8_t *, ssize_t) { ++reads; });
        const uint8_t byte = 'd';
        for (int i = 0; i < 4; ++i)
        {
            pair.a->write(&byte, 1, [&](bool) { });
        }
        loop.process_events(EventLoop::ProcessFlag::kAllEvents, 500); // mid-transfer...
    } // ...both sockets destroyed here: dtor closes + pumps until close callbacks ran
    loop.process_events(EventLoop::ProcessFlag::kAllEvents); // loop still usable after
}

// 6. Error path: peer closes -> on_data(nullptr, nread<=0) terminal event, socket auto-closes.
TEST(TcpSocketTest, ErrorPathRemoteClose)
{
    EventLoop loop(make_uv_dispatcher());
    SocketPair pair(loop);

    bool eof_seen = false;
    ssize_t eof_nread = 1;
    bool written_ok = false;
    pair.b->read_start(
        [&](const uint8_t *data, ssize_t nread)
        {
            if (data == nullptr && nread <= 0)
            {
                eof_seen = true;
                eof_nread = nread;
                loop.exit(0);
            }
        });

    const uint8_t byte = 'e';
    pair.a->write(&byte, 1, [&](bool ok) { written_ok = ok; });
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 300); // let the write land

    ::shutdown(pair.rawA, SHUT_WR); // peer EOF: B's read delivers UV_EOF (rawA's fd is B's peer... no:
                                    // rawA belongs to socket a; shutting a down EOFs the pair for b)
    loop.process_events(EventLoop::ProcessFlag::kWaitForMoreEvents, 2000);

    EXPECT_TRUE(written_ok);
    EXPECT_TRUE(eof_seen);
    EXPECT_LT(eof_nread, 0); // UV_EOF is a negative nread
    EXPECT_FALSE(pair.b->is_open());
}

// 7. F8-②: close() is idempotent; pending queued writes complete with false; second close is a no-op.
TEST(TcpSocketTest, CloseTwiceIdempotent)
{
    EventLoop loop(make_uv_dispatcher());
    SocketPair pair(loop);

    std::vector<bool> results;
    // Queue several writes without draining: later ones stay pending (backpressure) when close hits.
    for (int i = 0; i < 8; ++i)
    {
        std::vector<uint8_t> block(64 * 1024, static_cast<uint8_t>(i)); // 512KB total: forces queueing
        pair.a->write(block.data(), block.size(), [&](bool ok) { results.push_back(ok); });
    }

    pair.a->close(); // pending writes discarded -> on_written(false) each
    pair.a->close(); // F8-②: no-op, no double-uv_close, no crash
    pair.a->close(); // and again for good measure

    // The in-flight write (if any) completes false via UV_ECANCELED in on_write_done; the FIRST
    // write may legitimately complete true — it was already submitted to the kernel buffer before
    // close(). What F8-② guarantees: every write got a completion, and no queued write reports true.
    loop.process_events(EventLoop::ProcessFlag::kAllEvents, 1000);
    EXPECT_EQ(static_cast<size_t>(8), results.size()); // all eight completions delivered
    EXPECT_FALSE(std::all_of(results.begin(), results.end(), [](bool ok) { return ok; }));
    EXPECT_FALSE(pair.a->is_open());
}

// 8. I1: write from a non-loop thread is fatal (deliberately stricter than post-serialization).
#    if GTEST_HAS_DEATH_TEST && !defined(CXXKIT_ANDROID)
TEST(TcpSocketDeathTest, CrossThreadWriteRejected)
{
    // EXPECT_EXIT forks: the child builds the loop + sockets on ITS main thread (the loop thread),
    // then calls write() from a worker thread — the I1 fatal aborts the child; the parent observes.
    // Matcher "" (any fatal death) per tst_uv_event_dispatcher convention: the fatal message goes
    // through spdlog's own FD, not the gtest-captured stdout.
    EXPECT_EXIT(
        {
            EventLoop loop(make_uv_dispatcher());
            SocketPair pair(loop);
            std::thread offloop(
                [&]
                {
                    const uint8_t byte = '!';
                    pair.a->write(&byte, 1, [](bool) { }); // wrong thread -> CXXKIT_FATAL (I1)
                });
            offloop.join();
        },
        ::testing::KilledBySignal(SIGABRT),
        "");
}
#    endif

} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
