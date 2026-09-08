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

} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
