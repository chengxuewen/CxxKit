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

#include <cxxkit/thread/task_queue_thread.hpp>
#include <cxxkit/time/date_time.hpp>
#include <cxxkit/units/timestamp.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/thread/mutex.hpp>

#include <set>
#include <map>
#include <thread>
#include <algorithm>

CXXKIT_BEGIN_NAMESPACE

class TaskQueueThreadPrivate
{
    CXXKIT_DEFINE_PPTR(TaskQueueThread)
    CXXKIT_DECLARE_PUBLIC(TaskQueueThread)
    CXXKIT_DISABLE_COPY_MOVE(TaskQueueThreadPrivate)
public:
    struct PendingTask
    {
        Task::Id id;
        Task::SharedPtr task;
        struct Compare
        {
            bool operator()(const PendingTask &lhs, const PendingTask &rhs) const { return lhs.id < rhs.id; }
        };
    };
    using PendingTasksSet = std::set<PendingTask, PendingTask::Compare>;
    struct DelayedTask
    {
        Task::Id id;
        int64_t timestamp; // usecs
        Task::SharedPtr task;
        struct Compare
        {
            bool operator()(const DelayedTask &lhs, const DelayedTask &rhs) const
            {
                return std::tie(lhs.timestamp, lhs.id) < std::tie(rhs.timestamp, rhs.id);
            }
        };
    };
    using DelayedTasksSet = std::set<DelayedTask, DelayedTask::Compare>;

    explicit TaskQueueThreadPrivate(TaskQueueThread *p);
    ~TaskQueueThreadPrivate() = default;

    void init();

    std::thread mThread;
    RecursiveMutex mMutex;
    RecursiveMutex::Condition mTaskReadyCondition;
    bool mQuit CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex) = false;
    Task::Id mTaskIdCounter CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex) = 0;
    PendingTasksSet mPendingTasks CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex);
    DelayedTasksSet mDelayedTasks CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex);
};

TaskQueueThreadPrivate::TaskQueueThreadPrivate(TaskQueueThread *p)
    : mPPtr(p)
{
}

void TaskQueueThreadPrivate::init()
{
    Semaphore started;
    RecursiveMutex::UniqueLock lock(mMutex);
    mThread = std::thread(
        [this, &started]()
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThreadPrivate: thread started");
            TaskQueueThread::CurrentSetter current_setter(mPPtr);
            started.release();
            mPPtr->process_tasks();
            CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThreadPrivate: thread finished");
        });
    started.acquire();
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThreadPrivate: constructor done");
}

TaskQueueThread::TaskQueueThread()
    : mDPtr(new TaskQueueThreadPrivate(this))
{
    mDPtr->init();
}

TaskQueueThread::SharedPtr TaskQueueThread::make_shared()
{
    return SharedPtr(new TaskQueueThread, [](TaskQueueThread *thread) { thread->destroy(); });
}

TaskQueueThread::UniquePtr TaskQueueThread::make_unique()
{
    return UniquePtr(new TaskQueueThread);
}

TaskQueueThread::~TaskQueueThread()
{
}

void TaskQueueThread::destroy()
{
    CXXKIT_D(TaskQueueThread);
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::destroy()");
    CXXKIT_ASSERT(!this->is_current());
    {
        RecursiveMutex::Lock lock(d->mMutex);
        d->mQuit = true;
        CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::destroy() notify_all");
        d->mTaskReadyCondition.notify_all();
    }
    if (d->mThread.joinable())
    {
        d->mThread.join();
    }
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::destroy() delete");
    delete this;
}

bool TaskQueueThread::cancel_task(const Task *task)
{
    CXXKIT_D(TaskQueueThread);
    RecursiveMutex::Lock lock(d->mMutex);
    bool canceled = false;
    for (auto iter = d->mPendingTasks.begin(); iter != d->mPendingTasks.end();)
    {
        if (iter->task.get() == task)
        {
            iter = d->mPendingTasks.erase(iter);
            canceled = true;
        }
        else
        {
            ++iter;
        }
    }
    for (auto iter = d->mDelayedTasks.begin(); iter != d->mDelayedTasks.end();)
    {
        if (iter->task.get() == task)
        {
            iter = d->mDelayedTasks.erase(iter);
            canceled = true;
        }
        else
        {
            ++iter;
        }
    }
    return canceled;
}

void TaskQueueThread::post_task(const Task::SharedPtr &task, const SourceLocation &location)
{
    CXXKIT_D(TaskQueueThread);
    RecursiveMutex::Lock lock(d->mMutex);
    d->mPendingTasks.insert({++d->mTaskIdCounter, std::move(task)});
    d->mTaskReadyCondition.notify_one();
}

void TaskQueueThread::post_delayed_task(const Task::SharedPtr &task,
                                      const TimeDelta &delay,
                                      const SourceLocation &location)
{
    CXXKIT_D(TaskQueueThread);
    RecursiveMutex::Lock lock(d->mMutex);
    const auto ts = DateTime::steady_time_u_secs() + delay.us();
    d->mDelayedTasks.insert({++d->mTaskIdCounter, ts, std::move(task)});
        d->mTaskReadyCondition.notify_one();
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread: post_delayed_task");
}

TaskQueueThread::NextTask TaskQueueThread::pop_next_task()
{
    CXXKIT_D(TaskQueueThread);
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::pop_next_task()");
        NextTask result;
    const int64_t tickUSecs = DateTime::steady_time_u_secs();
    RecursiveMutex::Lock lock(d->mMutex);
    if (d->mQuit)
    {
        result.finalTask = true;
        return result;
    }

    if (!d->mDelayedTasks.empty())
    {
        auto delayedTask = d->mDelayedTasks.begin();
        const auto delayedTaskTimestamp = delayedTask->timestamp;
        if (tickUSecs >= delayedTaskTimestamp)
        {
            if (!d->mPendingTasks.empty())
            {
                auto pendingTask = d->mPendingTasks.begin();
                if (pendingTask->id < delayedTask->id)
                {
                    result.runTask = std::move(pendingTask->task);
                    d->mPendingTasks.erase(pendingTask);
                    return result;
                }
            }

                        result.runTask = std::move(delayedTask->task);
            d->mDelayedTasks.erase(delayedTask);
            return result;
        }

        result.sleepTime = TimeDelta::Millis(DivideRoundUp(delayedTaskTimestamp - tickUSecs, 1'000));
    }

    if (!d->mPendingTasks.empty())
    {
        auto pendingTask = d->mPendingTasks.begin();
        result.runTask = std::move(pendingTask->task);
        d->mPendingTasks.erase(pendingTask);
    }

    if (!result.runTask && result.sleepTime.us() > 1000)
    {
        // Nothing due soon, and the empty-queue default sleepTime is PlusInfinity: waiting on it
        // would sleep a full second (the wait cap) even after a post/post_delayed_task notify that
        // arrived while this thread was already waiting with an earlier deadline (predicate wait
        // does not shorten a deadline). A 1ms short-poll bounds that window far inside any real
        // delay deadline (fixes intermittent TaskQueueThreadTest.PostDelayedTask 1s timeouts:
        // the 3ms task was being run 1s late). Delayed tasks with a precise sleepTime < 1ms keep
        // their exact wait.
        result.sleepTime = TimeDelta::Millis(1);
    }

    return result;
}

void TaskQueueThread::process_tasks()
{
    CXXKIT_D(TaskQueueThread);
    RecursiveMutex::UniqueLock lock(d->mMutex);
    lock.unlock();
    while (true)
    {
        CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::process_tasks() loop");
        const auto nextTask = this->pop_next_task();
        if (nextTask.finalTask)
        {
            break;
        }

        if (nextTask.runTask)
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                                 "TaskQueueThread::process_tasks() runTask:{}",
                                 utils::fmt::ptr(nextTask.runTask.get()));
            // process entry immediately then try again
            nextTask.runTask->run();
            // Attempt to run more tasks before going to sleep.
            continue;
        }

        lock.lock();
        CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(),
                             "TaskQueueThread::process_tasks() wait {} us",
                             nextTask.sleepTime.us());
                const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::microseconds(std::min(nextTask.sleepTime.us(), (int64_t)1000000LL));
        // Predicate wait: re-checks the queues under the lock after any wakeup. This is the
        // standard condition_variable idiom and closes a real lost-wakeup window: init() only
        // waits for the worker thread to *start* (started.release happens before process_tasks),
        // so a post/post_delayed_task notify_one racing ahead of this thread's first wait is
        // dropped; the thread then slept a full 1s on an empty-queue sleepTime (mDelayTasks
        // empty => default TimeDelta => min(...,1s) cap) and delayed tasks missed their window
        // (observed: TaskQueueThreadTest.PostDelayedTask 1s timeout, intermittent).
        d->mTaskReadyCondition.wait_until(lock, deadline, [d] {
            return d->mQuit || !d->mPendingTasks.empty() ||
                   (!d->mDelayedTasks.empty() &&
                    d->mDelayedTasks.begin()->timestamp <= DateTime::steady_time_u_secs());
        });
        lock.unlock();
    }
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::process_tasks() break loop");
    lock.lock();
    // Ensure remaining deleted tasks are destroyed with Current() set up to this task queue.
    d->mPendingTasks.clear();
    CXXKIT_LOGGING_TRACE(CXXKIT_TASK_QUEUE_LOGGER(), "TaskQueueThread::process_tasks() done");
}

CXXKIT_END_NAMESPACE
