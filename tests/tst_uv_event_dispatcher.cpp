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
#include <cxxkit/uv/uv_event_dispatcher.hpp>

#include <cxxkit/thread/semaphore.hpp>

#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{

using cxxkit::AbstractEventDispatcher;
using cxxkit::EventLoop;
using cxxkit::make_uv_dispatcher;

// Real-libuv engine tests (D11). The EventLoop shell drains its post queue at process_events entry; the uv
// engine contributes uv_run rounds, timers and the wake doorbell. All waiting is event-driven (P3): semaphores
// and loop->post/exit, never sleeps. The main test thread is the loop thread (factory pins it at construction).

// 1. post() from a worker thread; exec() on the loop thread drains and returns (cross-thread wake path).
TEST(UvEventDispatcherTest, PostCrossThreadDrains)
{
    EventLoop loop(make_uv_dispatcher());
    cxxkit::Semaphore posted;
    std::atomic<bool> ran(false);

    std::thread worker(
        [&]
        {
            loop.post(
                [&]
                {
                    ran.store(true);
                    loop.exit(0);
                });
            posted.release();
        });

    posted.acquire(); // the task is in the queue before exec starts spinning rounds
    EXPECT_EQ(0, loop.exec());
    worker.join();
    EXPECT_TRUE(ran.load());
}

// 2. Timer fires no earlier than its interval (I2 lower bound, generous for scheduler jitter); event-driven
// wait: exec blocks in UV_RUN_ONCE until the timer callback exits the loop.
TEST(UvEventDispatcherTest, TimerFiresWithLowerBound)
{
    EventLoop loop(make_uv_dispatcher());
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    loop.start_timer(
        50,
        [&]
        {
            EXPECT_GE(
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count(),
                45); // 5ms scheduler-grace band
            loop.exit(0);
        },
        false);
    EXPECT_EQ(0, loop.exec());
}

// 3. one-shot via shell wrap: repeat=false fires exactly once even though the engine arms uv_timer with
// repeat=interval (R-B2-4) — the wrapper stops the timer before running fn, the re-arm never fires again.
TEST(UvEventDispatcherTest, TimerOneShotViaShell)
{
    EventLoop loop(make_uv_dispatcher());
    std::atomic<int> fires(0);

    loop.start_timer(
        10,
        [&]
        {
            ++fires;
            // Give a would-be re-arm (10ms) every chance to misfire: keep the loop alive 3x longer.
            loop.start_timer(30, [&] { loop.exit(0); }, false);
        },
        false);
    EXPECT_EQ(0, loop.exec());
    EXPECT_EQ(1, fires.load());
}

// 4. Callback stop/restart legality (I3): inside a timer callback, stop self and start a new timer.
TEST(UvEventDispatcherTest, CallbackStopRestart)
{
    EventLoop loop(make_uv_dispatcher());
    std::atomic<int> firstFires(0);

    int firstId = loop.start_timer(10,
                                   [&]
                                   {
                                       ++firstFires;
                                       loop.stop_timer(firstId); // I3: stop self from within the callback
                                       loop.start_timer(10, [&] { loop.exit(7); }, false);
                                   });
    EXPECT_EQ(7, loop.exec());
    EXPECT_EQ(1, firstFires.load());
}

// 5. Wake-up storm (I4): N threads x M posts — all coalesced doorbells still drain the whole queue.
TEST(UvEventDispatcherTest, WakeUpStormNThreadsMTasks)
{
    EventLoop loop(make_uv_dispatcher());
    const int kThreads = 4;
    const int kPerThread = 25;
    const int kTotal = kThreads * kPerThread;
    std::atomic<int> executed(0);
    cxxkit::Semaphore start;
    cxxkit::Semaphore done; // starts at 0: the barrier must not pass before producers released

    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; ++t)
    {
        producers.emplace_back(
            [&]()
            {
                start.acquire();
                for (int i = 0; i < kPerThread; ++i)
                {
                    loop.post([&executed] { ++executed; });
                }
                done.release(); // barrier for the exit-check sentinel
            });
    }

    // Queue order (FIFO): [kick] [exit-check] ... storm tasks land behind while the loop thread is
    // parked inside the exit-check's barrier. The kick releases the producers; the exit-check parks
    // on done.acquire(kThreads) until every producer finished posting (producers never wait on the
    // loop, so no deadlock), then re-posts itself — the re-queued check runs strictly after the
    // whole 100-task storm has drained and exits the loop. A partial-drain regression would leave
    // exec blocked past the gtest timeout, not silently pass.
    loop.post([&] { start.release(kThreads); });
    std::function<void()> check_drained;
    check_drained = [&executed, &loop, kTotal, &check_drained, &done]
    {
        if (executed.load() >= kTotal)
        {
            loop.exit(0);
            return;
        }
        done.acquire(kThreads);
        loop.post(check_drained);
    };
    loop.post(check_drained);
    EXPECT_EQ(0, loop.exec());
    for (std::thread &t : producers)
    {
        t.join();
    }
    EXPECT_EQ(kTotal, executed.load());
}

// 6. Nested process_events from a callback is fatal (I5/S6) — message mentions re-entered.
//
// The re-entrancy latch guards dispatcher callbacks (they run inside uv_run). A post-task callback runs
// in the SHELL's drain phase — before the dispatcher is entered — so it cannot trip the latch; the true
// nesting hazard is a TIMER callback (it runs inside uv_run with the latch set) calling process_events.
#    if GTEST_HAS_DEATH_TEST && !defined(CXXKIT_ANDROID)
TEST(UvEventDispatcherDeathTest, NestedProcessEventsAsserts)
{
    EventLoop loop(make_uv_dispatcher());
    loop.start_timer(
        10,
        [&]
        {
            // Runs inside uv_run (mInProcessEvents == true): re-entering through the shell
            // reaches the dispatcher with the latch set -> the I5 fatal.
            loop.process_events(EventLoop::ProcessFlag::kAllEvents);
        },
        false);
    // Matcher "" (any fatal death) per tst_checks/tst_event_loop convention: the fatal message goes
    // through spdlog's own FD, not the gtest-captured stdout, so a regex matcher cannot see it.
    EXPECT_EXIT(loop.exec(), ::testing::KilledBySignal(SIGABRT), "");
}
#    endif

// 7. Destructor with a pending timer and un-drained post (I6): clean teardown, no leak (ASAN tree watches).
TEST(UvEventDispatcherTest, DestructorCleansUpHandles)
{
    std::atomic<int> neverRun(0);
    {
        EventLoop loop(make_uv_dispatcher());
        loop.start_timer(1000, [&neverRun] { ++neverRun; });     // never fires
        loop.post([&neverRun] { ++neverRun; });                  // never drained
        loop.process_events(EventLoop::ProcessFlag::kAllEvents); // NOWAIT round: queue not empty → drains the post
    } // destruction with a live pending timer
    EXPECT_EQ(1, neverRun.load()); // post drained by the NOWAIT round above; timer freed without firing
}

// 8. B-phase acceptance (A2 left-open maximumTime window): process_events(flags, 100ms) with no events must
// return false in ~100ms against a real blocking driver, not hang or overshoot wildly.
TEST(UvEventDispatcherTest, ProcessEventsTimeoutWithRealDriver)
{
    EventLoop loop(make_uv_dispatcher());
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    const bool processed = loop.process_events(EventLoop::ProcessFlag::kAllEvents, 100);

    const long long elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_FALSE(processed);
    EXPECT_GE(elapsed_ms, 95);   // honored the timeout (5ms band)
    EXPECT_LE(elapsed_ms, 2000); // did not hang: generous ceiling, catches deadline-loss regressions
}

// Phase-2 socket notifier (uv_poll): real fds via socketpair(2). The dispatcher is reachable through the
// loop's dispatcher() accessor — register/unregister are dispatcher-level APIs (TcpSocket will consume them).
using SocketMask = AbstractEventDispatcher::SocketEventMask;

// 9. Write into a socketpair: the readable notification fires on the peer fd.
TEST(UvEventDispatcherTest, RegisterPollFiresOnReadable)
{
    std::unique_ptr<AbstractEventDispatcher> dispatcher = make_uv_dispatcher();
    cxxkit::UvEventDispatcher *uv = static_cast<cxxkit::UvEventDispatcher *>(dispatcher.get());
    EventLoop loop(std::move(dispatcher));
    int fds[2];
    ASSERT_EQ(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    cxxkit::Semaphore fired;
    uv->register_socket_notifier(
        fds[0],
        SocketMask::kRead,
        [&](SocketMask mask)
        {
            EXPECT_EQ(static_cast<int>(SocketMask::kRead),
                      static_cast<int>(mask) & static_cast<int>(SocketMask::kRead));
            char buf[16];
            const ssize_t n = ::read(fds[0], buf, sizeof(buf)); // drain: level-triggered re-arm check
            EXPECT_EQ(1, n);
            fired.release();
        });

    char byte = 'x';
    ASSERT_EQ(1, ::write(fds[1], &byte, 1)); // producer: peer fd becomes readable
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kWaitForMoreEvents, 1000));
    EXPECT_TRUE(fired.try_acquire());

    uv->unregister_socket_notifier(fds[0]);
    ::close(fds[0]);
    ::close(fds[1]);
}

// 10. Interest mask respected: Write-only registration does NOT fire for readability; kRead|kWrite
// duplex registration fires with both bits on a readable-with-writable-buffer fd.
TEST(UvEventDispatcherTest, PollInterestMaskRespected)
{
    std::unique_ptr<AbstractEventDispatcher> dispatcher = make_uv_dispatcher();
    cxxkit::UvEventDispatcher *uv = static_cast<cxxkit::UvEventDispatcher *>(dispatcher.get());
    EventLoop loop(std::move(dispatcher));
    int fds[2];
    ASSERT_EQ(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    int readHits = 0; // not atomic: callback and asserts are both on the loop (= test) thread
    int writeHits = 0;
    int duplexHits = 0;
    SocketMask duplexSeen;

    // fds[0]: write-only. A fresh socketpair buffer is writable, so this fires — with kWrite ONLY.
    uv->register_socket_notifier(fds[0],
                                 SocketMask::kWrite,
                                 [&](SocketMask mask)
                                 {
                                     EXPECT_EQ(static_cast<int>(SocketMask::kWrite),
                                               static_cast<int>(mask)); // no kRead bit leaked
                                     ++writeHits;
                                 });
    // fds[1]: duplex. Fresh buffer: writable; nothing buffered to read yet -> first fire is kWrite only.
    uv->register_socket_notifier(
        fds[1],
        SocketMask::kRead | SocketMask::kWrite,
        [&](SocketMask mask)
        {
            ++duplexHits;
            duplexSeen = mask;
            if ((static_cast<int>(mask) & static_cast<int>(SocketMask::kRead)) != 0)
            {
                char buf[16];
                ::read(fds[1], buf, sizeof(buf)); // drain so the duplex fd stops re-firing readable
            }
        });

    char byte = 'y';
    ASSERT_EQ(1, ::write(fds[0], &byte, 1)); // -> fds[1] readable; fds[0] still never readable
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents, 500));

    EXPECT_EQ(0, readHits);   // write-only fd: no read notification ever (the whole point)
    EXPECT_GE(writeHits, 1);  // its writable buffer did fire — kWrite
    EXPECT_GE(duplexHits, 1); // duplex fd fired with kWrite (+kRead after the write)
    EXPECT_NE(0, static_cast<int>(duplexSeen) & static_cast<int>(SocketMask::kWrite));

    uv->unregister_socket_notifier(fds[0]);
    uv->unregister_socket_notifier(fds[1]);
    ::close(fds[0]);
    ::close(fds[1]);
}

// 11. Unregister stops delivery: a written byte after unregister produces no callback (fd still open).
TEST(UvEventDispatcherTest, PollUnregisterStopsDelivery)
{
    std::unique_ptr<AbstractEventDispatcher> dispatcher = make_uv_dispatcher();
    cxxkit::UvEventDispatcher *uv = static_cast<cxxkit::UvEventDispatcher *>(dispatcher.get());
    EventLoop loop(std::move(dispatcher));
    int fds[2];
    ASSERT_EQ(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    int hits = 0;
    uv->register_socket_notifier(fds[0], SocketMask::kRead, [&](SocketMask) { ++hits; });

    char byte = 'z';
    ASSERT_EQ(1, ::write(fds[1], &byte, 1));
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents, 500));
    ASSERT_GE(hits, 1); // pre-unregister delivery works

    char sink[16];
    ASSERT_EQ(1, ::read(fds[0], sink, sizeof(sink))); // drain so the fd is quiet again
    uv->unregister_socket_notifier(fds[0]);

    ASSERT_EQ(1, ::write(fds[1], &byte, 1)); // readable again — but nobody is listening
    EXPECT_FALSE(loop.process_events(EventLoop::ProcessFlag::kAllEvents, 100));
    EXPECT_EQ(1, hits); // no second delivery

    ::close(fds[0]);
    ::close(fds[1]);
}

// 12. F7-①: re-registering the same fd updates interests in place (idempotent, not an error) — memcached-
// style per-transition re-arm. Write-only then Read-only on the same fd: only read readiness delivers.
TEST(UvEventDispatcherTest, PollReregisterSameFdUpdates)
{
    std::unique_ptr<AbstractEventDispatcher> dispatcher = make_uv_dispatcher();
    cxxkit::UvEventDispatcher *uv = static_cast<cxxkit::UvEventDispatcher *>(dispatcher.get());
    EventLoop loop(std::move(dispatcher));
    int fds[2];
    ASSERT_EQ(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    int writeHits = 0;
    int readHits = 0;
    auto write_cb = [&](SocketMask) { ++writeHits; };
    auto read_cb = [&](SocketMask mask)
    {
        EXPECT_EQ(static_cast<int>(SocketMask::kRead), static_cast<int>(mask));
        ++readHits;
        char buf[16];
        ::read(fds[0], buf, sizeof(buf));
    };

    uv->register_socket_notifier(fds[0], SocketMask::kWrite, write_cb);
    // Same fd, new interests + new callback (curl socket_cb re-attach shape):
    uv->register_socket_notifier(fds[0], SocketMask::kRead, read_cb);

    char byte = 'r';
    ASSERT_EQ(1, ::write(fds[1], &byte, 1));
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents, 500));

    EXPECT_EQ(1, readHits);  // the UPDATED registration delivered read readiness
    EXPECT_EQ(0, writeHits); // the stale write interest is gone (swapped, not accumulated)

    uv->unregister_socket_notifier(fds[0]);
    ::close(fds[0]);
    ::close(fds[1]);
}

// 13. F8-①: a callback that unregisters its own fd mid-flight must not tear its own execution — the
// engine copies the std::function before invoking, so the callback runs to completion.
TEST(UvEventDispatcherTest, PollUnregisterInsideCallback)
{
    std::unique_ptr<AbstractEventDispatcher> dispatcher = make_uv_dispatcher();
    cxxkit::UvEventDispatcher *uv = static_cast<cxxkit::UvEventDispatcher *>(dispatcher.get());
    EventLoop loop(std::move(dispatcher));
    int fds[2];
    ASSERT_EQ(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds));

    bool afterUnregister = false; // proves execution continued past the unregister call
    uv->register_socket_notifier(fds[0],
                                 SocketMask::kRead,
                                 [&](SocketMask)
                                 {
                                     uv->unregister_socket_notifier(fds[0]); // self-dismantle mid-callback (F8-①)
                                     afterUnregister = true; // still executing the copied function — no use-after-free
                                     char buf[16];
                                     const ssize_t n = ::read(fds[0], buf, sizeof(buf));
                                     EXPECT_EQ(1, n);
                                 });

    char byte = 's';
    ASSERT_EQ(1, ::write(fds[1], &byte, 1));
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents, 500));
    EXPECT_TRUE(afterUnregister);

    // The fd is unregistered now: another write delivers nothing.
    ASSERT_EQ(1, ::write(fds[1], &byte, 1));
    EXPECT_FALSE(loop.process_events(EventLoop::ProcessFlag::kAllEvents, 100));

    ::close(fds[0]);
    ::close(fds[1]);
}

// 14. I6 + poll handles: destruction with an ACTIVE poll registration (never unregistered) tears down
// cleanly — the destructor closes the poll handle without touching the still-open fd. ASAN tree watches.
TEST(UvEventDispatcherTest, DestructorCleansUpActivePolls)
{
    int fds[2];
    ASSERT_EQ(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    int hits = 0;
    {
        std::unique_ptr<AbstractEventDispatcher> dispatcher = make_uv_dispatcher();
        cxxkit::UvEventDispatcher *uv = static_cast<cxxkit::UvEventDispatcher *>(dispatcher.get());
        EventLoop loop(std::move(dispatcher));
        uv->register_socket_notifier(fds[0], SocketMask::kRead, [&](SocketMask) { ++hits; });
        loop.process_events(EventLoop::ProcessFlag::kAllEvents); // NOWAIT round, fd quiet: no delivery
    } // destruction with a live poll registration
    EXPECT_EQ(0, hits);
    ::close(fds[0]); // fd still valid after dispatcher teardown — the engine never closed it
    ::close(fds[1]);
}


} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
