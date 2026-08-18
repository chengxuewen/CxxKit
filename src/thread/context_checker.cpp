/***********************************************************************************************************************
**
** Library: cxxkit
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

#include "cxxkit/thread/context_checker.hpp"
#include "cxxkit/thread/platform_thread.hpp"
#include "cxxkit/thread/task_queue.hpp"

#include <mutex>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{

} // namespace detail

class ContextCheckerPrivate
{
    CXXKIT_DEFINE_PPTR(ContextChecker)
    CXXKIT_DECLARE_PUBLIC(ContextChecker)
    CXXKIT_DISABLE_COPY_MOVE(ContextCheckerPrivate)
public:
    using InitialState = ContextChecker::InitialState;

    ContextCheckerPrivate(ContextChecker *p, bool attached, TaskQueueBase *attachedTaskQueue);
    virtual ~ContextCheckerPrivate();

#if CXXKIT_DCHECK_IS_ON
    mutable std::mutex mMutex;
    // These are mutable so that IsCurrent can set them.
    mutable bool mAttached CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex);
    mutable PlatformThread::Id mValidThreadId CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex) = 0;
    mutable const TaskQueueBase *mValidQueue CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex) = nullptr;
#endif
};

ContextCheckerPrivate::ContextCheckerPrivate(ContextChecker *p, bool attached, TaskQueueBase *attachedTaskQueue)
    : mPPtr(p)
#if CXXKIT_DCHECK_IS_ON
    , mAttached(attached)
    , mValidThreadId(PlatformThread::currentThreadId())
    , mValidQueue(attachedTaskQueue ? attachedTaskQueue : TaskQueueBase::current())
#endif
{
}

ContextCheckerPrivate::~ContextCheckerPrivate()
{
}

ContextChecker::ContextChecker(InitialState initialState)
    : mDPtr(new ContextCheckerPrivate(this, (bool)initialState, nullptr))
{
}

ContextChecker::ContextChecker(TaskQueueBase *attachedTaskQueue)
    : mDPtr(new ContextCheckerPrivate(this, nullptr != attachedTaskQueue, attachedTaskQueue))
{
}

ContextChecker::~ContextChecker()
{
}

std::string ContextChecker::expectationToString(const ContextChecker *checker)
{
#if CXXKIT_DCHECK_IS_ON
    auto d = checker->dFunc();
    const TaskQueueBase *const currentQueue = TaskQueueBase::current();
    const auto currentThread = PlatformThread::currentThreadId();
    std::lock_guard<std::mutex> lock(d->mMutex);
    if (!d->mAttached)
    {
        return "Checker currently not attached.";
    }

    // The format of the string is meant to compliment the one we have inside of FatalLog() (cxxkit_checks.cpp).
    // Example:
    //
    // # Expected: TaskQueue: 0x0 SysQ: 0x7fff69541330 Thread: 0x11dcf6dc0
    // # Actual:   TaskQueue: 0x7fa8f0604190 SysQ: 0x7fa8f0604a30 Thread: 0x700006f1a000
    // TaskQueue doesn't match

    char msgbuf[CXXKIT_LINE_MAX] = {0};
    std::snprintf(msgbuf,
                  CXXKIT_LINE_MAX,
                  "# Expected: TaskQueue: %p Thread: %" CXXKIT_SIZE_FORMAT "\n"
                  "# Actual:   TaskQueue: %p Thread: %" CXXKIT_SIZE_FORMAT "\n",
                  d->mValidQueue,
                  d->mValidThreadId,
                  currentQueue,
                  currentThread);
    std::stringstream message;
    message << msgbuf;
    if ((d->mValidQueue || currentQueue) && d->mValidQueue != currentQueue)
    {
        message << "TaskQueue doesn't match\n";
    }
    else if (d->mValidThreadId != currentThread)
    {
        message << "Threads don't match\n";
    }
    return message.str();
#else
    return "";
#endif
}

bool ContextChecker::isCurrent() const
{
#if CXXKIT_DCHECK_IS_ON
    CXXKIT_D(const ContextChecker);
    const TaskQueueBase *const currentQueue = TaskQueueBase::current();
    const auto currentThread = PlatformThread::currentThreadId();
    std::lock_guard<std::mutex> lock(d->mMutex);
    if (!d->mAttached)
    {
        // Previously detached.
        d->mAttached = true;
        d->mValidThreadId = currentThread;
        d->mValidQueue = currentQueue;
        return true;
    }
    if (d->mValidQueue)
    {
        return d->mValidQueue == currentQueue;
    }
    return d->mValidThreadId == currentThread;
#endif
    return true;
}

void ContextChecker::detach()
{
#if CXXKIT_DCHECK_IS_ON
    CXXKIT_D(ContextChecker);
    std::lock_guard<std::mutex> lock(d->mMutex);
    d->mAttached = false;
// We don't need to touch the other members here, they will be reset on the next call to isCurrent().
#endif
}

CXXKIT_END_NAMESPACE