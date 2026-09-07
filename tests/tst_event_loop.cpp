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

#include <cxxkit/base/global.hpp>

#include <cxxkit/kernel/event_loop.hpp>

#include "fake_dispatcher.hpp"

#include <cxxkit/kernel/connect_queued.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{
using cxxkit::EventLoop;
using cxxkit::FakeDispatcher;

class EventLoopTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mDispatcher = std::make_unique<FakeDispatcher>();
        mDispatcherPtr = mDispatcher.get();
        mLoop = std::make_unique<EventLoop>(std::move(mDispatcher));
    }

    std::unique_ptr<FakeDispatcher> mDispatcher;
    FakeDispatcher *mDispatcherPtr{nullptr};
    std::unique_ptr<EventLoop> mLoop;
};

// 1. exec drains posted tasks until exit; returns the exit code.
TEST_F(EventLoopTest, ExecRunsUntilExitRetCode)
{
    bool ran = false;
    mLoop->post(
        [&]
        {
            ran = true;
            mLoop->exit(42);
        });
    EXPECT_EQ(42, mLoop->exec());
    EXPECT_TRUE(ran);
}

// 2. exit() before exec() → exec returns immediately with that code.
TEST_F(EventLoopTest, ExitBeforeExecReturnsImmediately)
{
    mLoop->exit(7);
    EXPECT_EQ(7, mLoop->exec());
}

// 3. Nested exec is rejected: -1 + warning; outer exec unaffected.
TEST_F(EventLoopTest, NestedExecReturnsMinusOne)
{
    bool outerRan = false;
    mLoop->post(
        [&]
        {
            EXPECT_EQ(-1, mLoop->exec());
            outerRan = true;
            mLoop->exit(5);
        });
    EXPECT_EQ(5, mLoop->exec());
    EXPECT_TRUE(outerRan);
}

// 4. post FIFO order preserved across one drain.
TEST_F(EventLoopTest, PostFifoDrainOrder)
{
    std::vector<int> order;
    mLoop->post([&order] { order.push_back(1); });
    mLoop->post([&order] { order.push_back(2); });
    mLoop->post([&order] { order.push_back(3); });
    EXPECT_TRUE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents));
    ASSERT_EQ(3u, order.size());
    EXPECT_EQ(1, order[0]);
    EXPECT_EQ(2, order[1]);
    EXPECT_EQ(3, order[2]);
}

// 5. start_timer hands out unique non-zero ids.
TEST_F(EventLoopTest, StartTimerAssignsUniqueIds)
{
    int idA = mLoop->start_timer(100, [] { });
    int idB = mLoop->start_timer(100, [] { });
    EXPECT_NE(0, idA);
    EXPECT_NE(0, idB);
    EXPECT_NE(idA, idB);
}

// 6. repeat=false timer: wrapper stops the timer before invoking fn (M3) — fires once.
TEST_F(EventLoopTest, StartTimerOneShotFiresOnce)
{
    int fires = 0;
    int id = mLoop->start_timer(100, [&fires] { ++fires; }, false);
    auto timerFn = mDispatcherPtr->take_timer_fn(id);
    ASSERT_TRUE(static_cast<bool>(timerFn));
    timerFn(); // first expiry: wrapper stops the timer, then runs fn
    EXPECT_EQ(1, fires);
    EXPECT_FALSE(mDispatcherPtr->has_timer(id)); // wrapper removed it from the dispatcher
    // A real driver never re-fires after stop_timer; re-invoking the stale captured
    // std::function here would bypass the double, not the shell — so we assert removal only.
}

// 7. stop_timer forwards the id to the dispatcher.
TEST_F(EventLoopTest, StopTimerForwardsId)
{
    int id = mLoop->start_timer(100, [] { });
    EXPECT_TRUE(mDispatcherPtr->has_timer(id));
    mLoop->stop_timer(id);
    EXPECT_FALSE(mDispatcherPtr->has_timer(id));
}

// 8. maximum_ms synthesis: timeout path returns false; pending events return true.
TEST_F(EventLoopTest, ProcessEventsTimeoutReturns)
{
    // No events: dispatcher gets kWaitForMoreEvents, times out → false.
    EXPECT_FALSE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents, 0));
    // Pending posted events: processed → true.
    mLoop->post([] { });
    EXPECT_TRUE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents, 0));
}

// 9. process_events(flags) passes the dispatcher's return value through.
TEST_F(EventLoopTest, ProcessEventsReturnPassthrough)
{
    mDispatcherPtr->mOnProcessEvents = [](EventLoop::ProcessFlags) { return true; };
    EXPECT_TRUE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents));
    mDispatcherPtr->mOnProcessEvents = [](EventLoop::ProcessFlags) { return false; };
    EXPECT_FALSE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents));
}

// 10. exit() rings the dispatcher's bell (M4) — otherwise a blocked loop never wakes.
TEST_F(EventLoopTest, WakeUpOnExit)
{
    int before = mDispatcherPtr->mWakeUpCount.load();
    mLoop->exit(0);
    EXPECT_GE(mDispatcherPtr->mWakeUpCount.load(), before + 1);
}

// 12. exit(-1) before exec() is a real preset (not confusable with fresh state): exec returns
// -1 immediately without running a round.
TEST_F(EventLoopTest, ExitNegativeCodePresetReturns)
{
    bool ran = false;
    mDispatcherPtr->mOnProcessEvents = [&](EventLoop::ProcessFlags)
    {
        ran = true;
        return true;
    };
    mLoop->exit(-1);
    EXPECT_EQ(-1, mLoop->exec());
    EXPECT_FALSE(ran); // no round ran
}

// 13. connect_queued: emit does not run fn inline; process_events delivers on the loop
// thread with args passed by value.
TEST_F(EventLoopTest, ConnectQueuedDeliversOnLoopThread)
{
    cxxkit::Signal<int, std::string> sig;
    int deliveredInt = 0;
    std::string deliveredStr;
    cxxkit::signals::Connection conn = connect_queued(sig,
                                                      mLoop.get(),
                                                      [&](int v, const std::string &s)
                                                      {
                                                          deliveredInt = v;
                                                          deliveredStr = s;
                                                      });
    EXPECT_TRUE(conn.connected());

    sig(7, "seven"); // emit on the test thread — fn must NOT run yet
    EXPECT_EQ(0, deliveredInt);
    EXPECT_TRUE(deliveredStr.empty());
    EXPECT_GE(mDispatcherPtr->mWakeUpCount.load(), 1); // emit kicked the dispatcher

    mLoop->process_events(EventLoop::ProcessFlag::kAllEvents);
    EXPECT_EQ(7, deliveredInt);
    EXPECT_EQ("seven", deliveredStr);
}

// 14. connect_queued returns a live Connection; disconnect stops future deliveries.
TEST_F(EventLoopTest, ConnectQueuedReturnsConnectionForDisconnect)
{
    cxxkit::Signal<int> sig;
    int delivered = 0;
    cxxkit::signals::Connection conn = connect_queued(sig, mLoop.get(), [&delivered](int v) { delivered = v; });
    ASSERT_TRUE(conn.connected());
    conn.disconnect();
    EXPECT_FALSE(conn.connected());

    int wakeBefore = mDispatcherPtr->mWakeUpCount.load();
    sig(1); // no slot left: no post, no wake_up
    mLoop->process_events(EventLoop::ProcessFlag::kAllEvents);
    EXPECT_EQ(0, delivered);
    EXPECT_EQ(wakeBefore, mDispatcherPtr->mWakeUpCount.load());
}

// 15. connect_queued copies args at emit time; mutating the source afterwards does not
// change what the loop thread receives.
TEST_F(EventLoopTest, ConnectQueuedArgsCopiedNotReferenced)
{
    cxxkit::Signal<int> sig;
    int delivered = 0;
    connect_queued(sig, mLoop.get(), [&delivered](int v) { delivered = v; });

    int value = 1;
    sig(value);
    value = 2; // mutate the source after emit
    mLoop->process_events(EventLoop::ProcessFlag::kAllEvents);
    EXPECT_EQ(1, delivered);
}

// 11. null dispatcher is a fatal construction error.
TEST(EventLoopDeathTest, NullDispatcherChecks)
{
    EXPECT_DEATH(EventLoop loop(nullptr), "");
}
} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
