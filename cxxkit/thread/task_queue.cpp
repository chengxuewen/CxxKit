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

#include <cxxkit/thread/semaphore.hpp>


#include <cxxkit/thread/context_checker.hpp>
#include <cxxkit/tools/scope_guard.hpp>
#include <cxxkit/thread/task_queue.hpp>
#include <cxxkit/units/timestamp.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/tools/utility.hpp>

#include <atomic>

CXXKIT_DEFINE_LOGGER_WITH_LEVEL("cxxkit::TaskQueue", CXXKIT_TASK_QUEUE_LOGGER, cxxkit::LogLevel::Warning)

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
static thread_local TaskQueueBase *currentTaskQueue = nullptr;
} // namespace detail

class TaskQueueBase::SafetyFlagPrivate
{
    CXXKIT_DEFINE_PPTR(SafetyFlag)
    CXXKIT_DECLARE_PUBLIC(SafetyFlag)
    CXXKIT_DISABLE_COPY_MOVE(SafetyFlagPrivate)
public:
    SafetyFlagPrivate(SafetyFlag *p, bool alive);
    SafetyFlagPrivate(SafetyFlag *p, bool alive, TaskQueueBase *taskQueue);
    ~SafetyFlagPrivate() { }

    std::atomic<bool> mAlive{true};
    CXXKIT_ATTRIBUTE_NO_UNIQUE_ADDRESS ContextChecker mContextChecker;
};

TaskQueueBase::SafetyFlagPrivate::SafetyFlagPrivate(SafetyFlag *p, bool alive)
    : mPPtr(p)
    , mAlive(alive)
{
}

TaskQueueBase::SafetyFlagPrivate::SafetyFlagPrivate(SafetyFlag *p, bool alive, TaskQueueBase *taskQueue)
    : mPPtr(p)
    , mAlive(alive)
    , mContextChecker(taskQueue)
{
}

TaskQueueBase::SafetyFlag::SafetyFlag(bool alive)
    : mDPtr(new SafetyFlagPrivate(this, alive))
{
}

TaskQueueBase::SafetyFlag::SafetyFlag(bool alive, Nonnull<TaskQueueBase *> attachedQueue)
    : mDPtr(new SafetyFlagPrivate(this, alive, attachedQueue))
{
}

TaskQueueBase::SafetyFlag::SharedPtr TaskQueueBase::SafetyFlag::create()
{
    return SharedPtr(new SafetyFlag(true));
}

TaskQueueBase::SafetyFlag::SharedPtr TaskQueueBase::SafetyFlag::create_detached()
{
    auto flag = SharedPtr(new SafetyFlag(true));
    flag->d_func()->mContextChecker.detach();
    return flag;
}

TaskQueueBase::SafetyFlag::SharedPtr TaskQueueBase::SafetyFlag::create_attached_to_task_queue(
    bool alive,
    Nonnull<TaskQueueBase *> attachedQueue)
{
    return SharedPtr(new SafetyFlag(alive, attachedQueue));
}

TaskQueueBase::SafetyFlag::SharedPtr TaskQueueBase::SafetyFlag::create_detached_inactive()
{
    auto flag = SharedPtr(new SafetyFlag(false));
    flag->d_func()->mContextChecker.detach();
    return flag;
}

TaskQueueBase::SafetyFlag::~SafetyFlag()
{
}

bool TaskQueueBase::SafetyFlag::is_alive() const
{
    CXXKIT_D(const SafetyFlag);
    return d->mAlive.load(std::memory_order_acquire);
}

void TaskQueueBase::SafetyFlag::set_not_alive()
{
    CXXKIT_D(SafetyFlag);
    d->mAlive.store(false, std::memory_order_release);
}
void TaskQueueBase::SafetyFlag::set_alive()
{
    CXXKIT_D(SafetyFlag);
    d->mAlive.store(true, std::memory_order_release);
}

Task::SharedPtr TaskQueueBase::create_safe_task(const SafetyFlag::SharedPtr &flag, Task *task, bool autoDelete)
{
    return Task::create(std::move(Task::UniqueFunc(
        [=]() mutable
        {
            if (flag->is_alive())
            {
                task->run();
            }
            if (autoDelete)
            {
                delete task;
            }
        })));
}

Task::SharedPtr TaskQueueBase::create_safe_task(const SafetyFlag::SharedPtr &flag, UniqueFunction<void() &&> function)
{
    auto movefunction = utils::make_move_wrapper(std::move(function));
    return Task::create(std::move(Task::UniqueFunc(
        [flag, movefunction]() mutable
        {
            if (flag->is_alive())
            {
                movefunction.move()();
            }
        })));
}

TaskQueueBase::CurrentSetter::CurrentSetter(TaskQueueBase *taskQueue)
    : mPrevious(TaskQueueBase::current())
{
    detail::currentTaskQueue = taskQueue;
}

TaskQueueBase::CurrentSetter::~CurrentSetter()
{
    detail::currentTaskQueue = mPrevious;
}

TaskQueueBase *TaskQueueBase::current()
{
    return detail::currentTaskQueue;
}

void TaskQueueBase::send_task(const Task::SharedPtr &task, const SourceLocation &location)
{
    if (this->is_current())
    {
        task->run();
        return;
    }

    Semaphore semaphore;
    auto cleanup = utils::make_scope_guard([&semaphore] { semaphore.release(); });
    this->post_task([task, cleanup = std::move(cleanup)] { task->run(); });
    if (!semaphore.try_acquire(1, TimeDelta::Seconds(10).ms()))
    {
        CXXKIT_WARNING("TaskQueueBase::send_task: timeout waiting 10s for task to complete");
        semaphore.acquire();
    }
}

CXXKIT_END_NAMESPACE
