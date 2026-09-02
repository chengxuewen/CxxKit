/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
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

#include <cxxkit/thread/repeating_task.hpp>
#include <cxxkit/thread/task_queue.hpp>
#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/units/timestamp.hpp>
#include <cxxkit/tools/optional.hpp>
#include <cxxkit/memory/memory.hpp>
#include <cxxkit/tools/clock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <optional>

CXXKIT_BEGIN_NAMESPACE

namespace
{
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::MockFunction;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::WithArg;

CXXKIT_CXX14_CONSTEXPR TimeDelta kTimeout = TimeDelta::millis(1000);

class MockTaskQueue : public TaskQueueBase
{
public:
    MockTaskQueue()
        : mCurrentSetter(this)
    {
    }
    MOCK_METHOD(void, destroy, (), (override));
    MOCK_METHOD(bool, cancel_task, (const Task *), (override));
    MOCK_METHOD(void, post_task, (const Task::SharedPtr &, const SourceLocation &), (override));
    MOCK_METHOD(void,
                post_delayed_task,
                (const Task::SharedPtr &, const TimeDelta &, const SourceLocation &),
                (override));

private:
    CurrentSetter mCurrentSetter;
};

class FakeTaskQueue : public TaskQueueBase
{
public:
    explicit FakeTaskQueue(SimulatedClock *clock)
        : mCurrentSetter(this)
        , mClock(clock)
    {
    }

    void destroy() override { }
    bool cancel_task(const Task *) override { return false; }

    void post_task(const Task::SharedPtr &task, const SourceLocation & /*location*/) override
    {
        mLastTask = std::move(task);
        mLastDelay = TimeDelta::Zero();
    }

    void post_delayed_task(const Task::SharedPtr &task,
                           const TimeDelta &delay,
                           const SourceLocation & /*location*/) override
    {
        mLastTask = std::move(task);
        mLastDelay = delay;
    }

    bool advanceTimeAndRunLastTask()
    {
        EXPECT_TRUE(mLastTask);
        EXPECT_TRUE(mLastDelay.is_finite());
        mClock->advance_time(mLastDelay);
        mLastDelay = TimeDelta::minus_infinity();
        auto task = std::move(mLastTask);
        std::move(task)->run();
        return mLastTask == nullptr;
    }

    bool isTaskQueued() { return !!mLastTask; }

    TimeDelta lastDelay() const
    {
        EXPECT_TRUE(mLastDelay.is_finite());
        return mLastDelay;
    }

    // Optional<TaskQueue::DelayPrecision> last_precision() const { return last_precision_; }

private:
    SimulatedClock *mClock;
    Task::SharedPtr mLastTask;
    CurrentSetter mCurrentSetter;
    TimeDelta mLastDelay = TimeDelta::minus_infinity();
    // Optional<TaskQueue::DelayPrecision> last_precision_;
};

} // namespace

TEST(RepeatingTaskTest, TaskIsStoppedOnStop)
{
    const TimeDelta kShortInterval = TimeDelta::millis(50);

    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    std::atomic_int counter(0);
    auto handle = RepeatingTaskHandle::start(
        &task_queue,
        [&]
        {
            counter++;
            return kShortInterval;
        },
        &clock);
    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::Zero());
    EXPECT_FALSE(task_queue.advanceTimeAndRunLastTask());
    EXPECT_EQ(counter.load(), 1);

    // The handle reposted at the short interval.
    EXPECT_EQ(task_queue.lastDelay(), kShortInterval);

    // stop the handle. This prevernts the counter from incrementing.
    handle.stop();
    EXPECT_TRUE(task_queue.advanceTimeAndRunLastTask());
    EXPECT_EQ(counter.load(), 1);
}

TEST(RepeatingTaskTest, CompensatesForLongRunTime)
{
    const TimeDelta kRepeatInterval = TimeDelta::millis(2);
    // Sleeping inside the task for longer than the repeat interval once, should
    // be compensated for by repeating the task faster to catch up.
    const TimeDelta kSleepDuration = TimeDelta::millis(20);

    std::atomic_int counter(0);
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    RepeatingTaskHandle::start(
        &task_queue,
        [&]
        {
            ++counter;
            // Task takes longer than the repeat duration.
            clock.advance_time(kSleepDuration);
            return kRepeatInterval;
        },
        &clock);

    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::Zero());
    EXPECT_FALSE(task_queue.advanceTimeAndRunLastTask());

    // Task is posted right away since it took longer to run then the repeat
    // interval.
    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::Zero());
    EXPECT_EQ(counter.load(), 1);
}

TEST(RepeatingTaskTest, CompensatesForShortRunTime)
{
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    std::atomic_int counter(0);
    RepeatingTaskHandle::start(
        &task_queue,
        [&]
        {
            // Simulate the task taking 100ms, which should be compensated for.
            counter++;
            clock.advance_time(TimeDelta::millis(100));
            return TimeDelta::millis(300);
        },
        &clock);

    // Expect instant post task.
    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::Zero());
    // Task should be retained by the handler since it is not cancelled.
    EXPECT_FALSE(task_queue.advanceTimeAndRunLastTask());
    // New delay should be 200ms since repeat delay was 300ms but task took 100ms.
    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::millis(200));
}

TEST(RepeatingTaskTest, TaskCanStopItself)
{
    std::atomic_int counter(0);
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    RepeatingTaskHandle handle = RepeatingTaskHandle::start(&task_queue,
                                                            [&]
                                                            {
                                                                ++counter;
                                                                handle.stop();
                                                                return TimeDelta::millis(2);
                                                            });
    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::Zero());
    // Task cancelled itself so wants to be released.
    EXPECT_TRUE(task_queue.advanceTimeAndRunLastTask());
    EXPECT_EQ(counter.load(), 1);
}

TEST(RepeatingTaskTest, TaskCanStopItselfByReturningInfinity)
{
    std::atomic_int counter(0);
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    RepeatingTaskHandle handle = RepeatingTaskHandle::start(&task_queue,
                                                            [&]
                                                            {
                                                                ++counter;
                                                                return TimeDelta::plus_infinity();
                                                            });
    EXPECT_EQ(task_queue.lastDelay(), TimeDelta::Zero());
    // Task cancelled itself so wants to be released.
    EXPECT_TRUE(task_queue.advanceTimeAndRunLastTask());
    EXPECT_EQ(counter.load(), 1);
}

TEST(RepeatingTaskTest, ClockIntegration)
{
    Task::SharedPtr delayed_task;
    TimeDelta expected_delay = TimeDelta::Zero();
    SimulatedClock clock(Timestamp::Zero());

    NiceMock<MockTaskQueue> task_queue;
    ON_CALL(task_queue, post_delayed_task)
        .WillByDefault(
            [&](const Task::SharedPtr &task, TimeDelta delay, const SourceLocation &)
            {
                EXPECT_EQ(delay, expected_delay);
                delayed_task = std::move(task);
            });

    expected_delay = TimeDelta::millis(100);
    RepeatingTaskHandle handle = RepeatingTaskHandle::delayed_start(
        &task_queue,
        TimeDelta::millis(100),
        [&clock]()
        {
            EXPECT_EQ(Timestamp::millis(100), clock.current_time());
            // Simulate work happening for 10ms.
            clock.advance_time_milliseconds(10);
            return TimeDelta::millis(100);
        },
        &clock);

    clock.advance_time_milliseconds(100);
    Task::SharedPtr task_to_run = std::move(delayed_task);
    expected_delay = TimeDelta::millis(90);
    std::move(task_to_run)->run();
    EXPECT_NE(delayed_task, nullptr);
    handle.stop();
}

TEST(RepeatingTaskTest, CanBeStoppedAfterTaskQueueDeletedTheRepeatingTask)
{
    Task::SharedPtr repeating_task;

    MockTaskQueue task_queue;
    EXPECT_CALL(task_queue, post_delayed_task)
        .WillOnce(WithArg<0>([&](Task::SharedPtr task) { repeating_task = std::move(task); }));

    RepeatingTaskHandle handle = RepeatingTaskHandle::delayed_start(&task_queue,
                                                                    TimeDelta::millis(100),
                                                                    [] { return TimeDelta::millis(100); });

    // shutdown task queue: delete all pending tasks and run 'regular' task.
    repeating_task = nullptr;
    handle.stop();
}

#if 0
TEST(RepeatingTaskTest, DefaultPrecisionIsLow)
{
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    // Closure that repeats twice.
    MockFunction<TimeDelta()> closure;
    EXPECT_CALL(closure, Call()).WillOnce(Return(TimeDelta::millis(1))).WillOnce(Return(TimeDelta::plus_infinity()));
    RepeatingTaskHandle::start(&task_queue, closure.AsStdFunction());
    // Initial task is a PostTask().
    // EXPECT_FALSE(task_queue.last_precision().has_value());
    EXPECT_FALSE(task_queue.advanceTimeAndRunLastTask());
    // Repeated task is a delayed task with the default precision: low.
    EXPECT_TRUE(task_queue.last_precision().has_value());
    EXPECT_EQ(task_queue.last_precision().value(), TaskQueue::DelayPrecision::kLow);
    // No more tasks.
    EXPECT_TRUE(task_queue.advanceTimeAndRunLastTask());
}

TEST(RepeatingTaskTest, CanSpecifyToPostTasksWithLowPrecision)
{
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    // Closure that repeats twice.
    MockFunction<TimeDelta()> closure;
    EXPECT_CALL(closure, Call()).WillOnce(Return(TimeDelta::millis(1))).WillOnce(Return(TimeDelta::plus_infinity()));
    RepeatingTaskHandle::start(&task_queue, closure.AsStdFunction(), TaskQueue::DelayPrecision::kLow);
    // Initial task is a PostTask().
    EXPECT_FALSE(task_queue.last_precision().has_value());
    EXPECT_FALSE(task_queue.advanceTimeAndRunLastTask());
    // Repeated task is a delayed task with the specified precision.
    EXPECT_TRUE(task_queue.last_precision().has_value());
    EXPECT_EQ(task_queue.last_precision().value(), TaskQueue::DelayPrecision::kLow);
    // No more tasks.
    EXPECT_TRUE(task_queue.advanceTimeAndRunLastTask());
}

TEST(RepeatingTaskTest, CanSpecifyToPostTasksWithHighPrecision)
// {
    SimulatedClock clock(Timestamp::Zero());
    FakeTaskQueue task_queue(&clock);
    // Closure that repeats twice.
    MockFunction<TimeDelta()> closure;
    EXPECT_CALL(closure, Call()).WillOnce(Return(TimeDelta::millis(1))).WillOnce(Return(TimeDelta::plus_infinity()));
    RepeatingTaskHandle::start(&task_queue, closure.AsStdFunction(), TaskQueue::DelayPrecision::kHigh);
    // Initial task is a PostTask().
    EXPECT_FALSE(task_queue.last_precision().has_value());
    EXPECT_FALSE(task_queue.advanceTimeAndRunLastTask());
    // Repeated task is a delayed task with the specified precision.
    EXPECT_TRUE(task_queue.last_precision().has_value());
    EXPECT_EQ(task_queue.last_precision().value(), TaskQueue::DelayPrecision::kHigh);
    // No more tasks.
    EXPECT_TRUE(task_queue.advanceTimeAndRunLastTask());
}
#endif

CXXKIT_END_NAMESPACE