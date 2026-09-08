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
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
***********************************************************************************************************************/

#include <cxxkit/base/global.hpp>

#include <cxxkit/kernel/event_loop.hpp>

#include "fake_dispatcher.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{
using cxxkit::EventLoop;
using cxxkit::FakeDispatcher;

class EventLoopExtTest : public ::testing::Test
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

// 1. Zero-interval one-shot timer bypasses engine registration (Qt singleShotImpl precedent):
// no dispatcher entry appears; fn runs on the next drain.
TEST_F(EventLoopExtTest, ZeroShotTimerBypassesEngineRegistration)
{
    bool ran = false;
    int registrationsBefore = mDispatcherPtr->timer_registration_count();
    int id = mLoop->start_timer(0, [&ran] { ran = true; }, false);
    EXPECT_NE(0, id);
    EXPECT_EQ(registrationsBefore, mDispatcherPtr->timer_registration_count()); // ghost id: no engine entry
    EXPECT_FALSE(mDispatcherPtr->has_timer(id));

    mLoop->stop_timer(id); // documented no-op on a ghost id — must not crash
    EXPECT_TRUE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents));
    EXPECT_TRUE(ran);
}

// 2. Zero-period repeating timer still registers through the engine: fires every round.
TEST_F(EventLoopExtTest, ZeroRepeatStillRegisters)
{
    int id = mLoop->start_timer(0, [] { }, true);
    EXPECT_NE(0, id);
    EXPECT_TRUE(mDispatcherPtr->has_timer(id));

    auto timerFn = mDispatcherPtr->take_timer_fn(id);
    ASSERT_TRUE(static_cast<bool>(timerFn));
    timerFn();
    EXPECT_TRUE(mDispatcherPtr->has_timer(id)); // repeating: the entry survives its own firing
}

// 3. A 0ms one-shot timer and post(fn) share the posted queue: FIFO order across one drain.
TEST_F(EventLoopExtTest, ZeroShotPostEquivalence)
{
    std::vector<int> order;
    mLoop->post([&order] { order.push_back(1); });
    mLoop->start_timer(0, [&order] { order.push_back(2); }, false);
    mLoop->post([&order] { order.push_back(3); });
    EXPECT_TRUE(mLoop->process_events(EventLoop::ProcessFlag::kAllEvents));
    ASSERT_EQ(3u, order.size());
    EXPECT_EQ(1, order[0]);
    EXPECT_EQ(2, order[1]);
    EXPECT_EQ(3, order[2]);
}
} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
