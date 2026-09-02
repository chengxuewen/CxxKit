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
#include <cxxkit/functional/invocable.hpp>
#include <cxxkit/tools/logging.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
class RepeatingTaskClosure final
{
public:
    RepeatingTaskClosure(TaskQueueBase *taskQueue,
                         TimeDelta firstDelay,
                         UniqueFunction<TimeDelta()> closure,
                         Clock *clock,
                         const TaskQueueBase::SafetyFlag::SharedPtr &aliveFlag,
                         const SourceLocation &location);
    RepeatingTaskClosure(RepeatingTaskClosure &&) = default;
    RepeatingTaskClosure &operator=(RepeatingTaskClosure &&) = delete;
    ~RepeatingTaskClosure();

    void operator()() &&;

private:
    TaskQueueBase *const mTaskQueue;
    Clock *const mClock;
    const SourceLocation mLocation;
    UniqueFunction<TimeDelta()> mClosure;
    // This is always finite.
    Timestamp mNextRunTime CXXKIT_ATTRIBUTE_GUARDED_BY(mTaskQueue);
    TaskQueueBase::SafetyFlag::SharedPtr mAliveFlag CXXKIT_ATTRIBUTE_GUARDED_BY(mTaskQueue);
};

RepeatingTaskClosure::RepeatingTaskClosure(TaskQueueBase *taskQueue,
                                           TimeDelta firstDelay,
                                           UniqueFunction<TimeDelta()> closure,
                                           Clock *clock,
                                           const TaskQueueBase::SafetyFlag::SharedPtr &aliveFlag,
                                           const SourceLocation &location)
    : mTaskQueue(taskQueue)
    , mClock(clock)
    , mLocation(location)
    , mClosure(std::move(closure))
    , mNextRunTime(mClock->current_time() + firstDelay)
    , mAliveFlag(aliveFlag)
{
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                         "RepeatingTaskClosure::RepeatingTaskClosure() ctor:{}",
                         utils::fmt::ptr(this));
}

RepeatingTaskClosure::~RepeatingTaskClosure()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                         "RepeatingTaskClosure::~RepeatingTaskClosure() dtor:{}",
                         utils::fmt::ptr(this));
}

void RepeatingTaskClosure::operator()() &&
{
    // CXXKIT_DCHECK_RUN_ON(mTaskQueue);
    if (!mAliveFlag->is_alive())
    {
        CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                             "RepeatingTaskClosure::operator() not Alive:{}",
                             utils::fmt::ptr(this));
        return;
    }

    // detail::RepeatingTaskImplDTraceProbeRun();
    TimeDelta delay = mClosure();
    CXXKIT_DCHECK_GE(delay, TimeDelta::Zero());

    // A delay of +infinity means that the task should not be run again.
    // Alternatively, the closure might have stopped this task.
    if (delay.is_plus_infinity() || !mAliveFlag->is_alive())
    {
        CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                             "RepeatingTaskHandle::operator() not be run again {}",
                             utils::fmt::ptr(this));
        return;
    }

    TimeDelta lost_time = mClock->current_time() - mNextRunTime;
    mNextRunTime += delay;
    delay -= lost_time;
    delay = std::max(delay, TimeDelta::Zero());

    mTaskQueue->post_delayed_task(std::move(*this), delay, mLocation);
}
} // namespace detail

RepeatingTaskHandle::~RepeatingTaskHandle()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                         "RepeatingTaskHandle::RepeatingTaskHandle() dtor:{}",
                         utils::fmt::ptr(this));
}

RepeatingTaskHandle RepeatingTaskHandle::start(TaskQueueBase *taskQueue,
                                               UniqueFunction<TimeDelta()> closure,
                                               Clock *clock,
                                               const SourceLocation &location)
{
    auto aliveFlag = TaskQueueBase::SafetyFlag::create_detached();
    // detail::RepeatingTaskHandleDTraceProbeStart();
    auto function = detail::RepeatingTaskClosure(taskQueue,
                                                 TimeDelta::Zero(),
                                                 std::move(closure),
                                                 clock,
                                                 aliveFlag,
                                                 location);
    taskQueue->post_task(std::move(function), location);
    return RepeatingTaskHandle(std::move(aliveFlag));
}

// delayed_start is equivalent to Start except that the first invocation of the closure will be delayed
// by the given amount.
RepeatingTaskHandle RepeatingTaskHandle::delayed_start(TaskQueueBase *taskQueue,
                                                       TimeDelta firstDelay,
                                                       UniqueFunction<TimeDelta()> closure,
                                                       Clock *clock,
                                                       const SourceLocation &location)
{
    auto aliveFlag = TaskQueueBase::SafetyFlag::create_detached();
    // detail::RepeatingTaskHandleDTraceProbeDelayedStart();
    auto function = detail::RepeatingTaskClosure(taskQueue, firstDelay, std::move(closure), clock, aliveFlag, location);
    taskQueue->post_delayed_task(std::move(function), firstDelay, location);
    return RepeatingTaskHandle(std::move(aliveFlag));
}

void RepeatingTaskHandle::stop()
{
    if (mAliveFlag)
    {
        mAliveFlag->set_not_alive();
        mAliveFlag.reset();
    }
}

bool RepeatingTaskHandle::is_running() const
{
    return mAliveFlag != nullptr;
}

CXXKIT_END_NAMESPACE
