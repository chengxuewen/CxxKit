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

#include <cxxkit/thread/detail/platform_thread_p.hpp>
#include <cxxkit/tools/exception.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/tools/result.hpp>
#include <cxxkit/tools/checks.hpp>

#include <thread>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
class PlatformFutureThread : public PlatformThread
{
    std::future<void> mFuture;

public:
    explicit PlatformFutureThread(std::future<void> &&future)
        : mFuture(std::move(future))
    {
    }
    ~PlatformFutureThread() override { }

protected:
    void run() override { mFuture.get(); }
};
} // namespace detail

PlatformThreadData *PlatformThreadData::current(PlatformThreadPrivate *thread)
{
    CXXKIT_ASSERT(nullptr != thread);
    return thread->mData;
}

PlatformThreadData *PlatformThreadData::current(PlatformThread *thread)
{
    CXXKIT_ASSERT(nullptr != thread);
    return PlatformThreadPrivate::get(thread)->mData;
}

PlatformThreadPrivate::PlatformThreadPrivate(PlatformThread *p, PlatformThreadData *data)
    : mPPtr(p)
    , mData(data)
{
    if (!mData)
    {
        mData = new PlatformThreadData;
    }
}

PlatformThreadPrivate::~PlatformThreadPrivate()
{
    mData->deref();
}

PlatformThread::PlatformThread()
    : PlatformThread(new PlatformThreadPrivate(this))
{
}

PlatformThread::PlatformThread(PlatformThreadPrivate *d)
    : mDPtr(d)
{
    mDPtr->mData->thread.store(this);
    this->setName("PlatformThread", this);
}

PlatformThread::~PlatformThread()
{
    CXXKIT_D(PlatformThread);
    ThreadMutex::UniqueLock lock(d->mMutex);
    if (d->mInFinish)
    {
        lock.unlock();
        this->wait();
        lock.lock();
    }
    if (d->mRunning && !d->mFinished && !d->mData->isAdopted)
    {
        CXXKIT_FATAL("PlatformThread: Destroyed while thread is still running");
        d->mData->thread.store(nullptr);
    }
}

bool PlatformThread::isInterruptionRequested() const
{
    CXXKIT_D(const PlatformThread);
    // fast path: check that the flag is not set:
    if (!d->mInterruptionRequested.load(std::memory_order_relaxed))
    {
        return false;
    }
    // slow path: if the flag is set, take into account run status:
    ThreadMutex::Lock lock(d->mMutex);
    return d->mRunning && !d->mFinished && !d->mInFinish;
}

Status PlatformThread::requestInterruption()
{
    CXXKIT_D(PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    if (!d->mRunning || d->mFinished || d->mInFinish)
    {
        return "Thread is not running or finished";
    }
    d->mInterruptionRequested.store(true, std::memory_order_relaxed);
    return Status::ok;
}

std::string PlatformThread::name() const
{
    CXXKIT_D(const PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    return d->mName;
}

Status PlatformThread::setName(const StringView name, const void *obj)
{
    CXXKIT_D(PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    if (d->mRunning)
    {
        return "cannot set name while the thread is running";
    }
    d->mName = std::string(name);
    if (obj)
    {
        // The %p specifier typically produce at most 16 hex digits, possibly with a 0x prefix.
        // But format is implementation defined, so add some margin.
        char buf[30];
        std::snprintf(buf, sizeof(buf), " 0x%p", obj);
        d->mName += buf;
    }
    return Status::ok;
}

PlatformThread::Priority PlatformThread::priority() const
{
    CXXKIT_D(const PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    return d->mPriority;
}

Status PlatformThread::setPriority(Priority priority)
{
    if (priority == Priority::kInherit)
    {
        return "Argument cannot be InheritPriority";
    }
    CXXKIT_D(PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    if (!d->mRunning)
    {
        return "Cannot set priority, thread is not running";
    }
    d->setPriority(priority);
    return Status::ok;
}

uint_t PlatformThread::stackSize() const
{
    CXXKIT_D(const PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    return d->mStackSize;
}

Status PlatformThread::setStackSize(uint_t stackSize)
{
    CXXKIT_D(PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    if (d->mRunning)
    {
        return "cannot change stack size while the thread is running";
    }
    d->mStackSize = stackSize;
    return Status::ok;
}

bool PlatformThread::isFinished() const
{
    CXXKIT_D(const PlatformThread);
    return d->mFinished.load(std::memory_order_relaxed);
}

bool PlatformThread::isRunning() const
{
    CXXKIT_D(const PlatformThread);
    return d->mRunning.load(std::memory_order_relaxed) && !d->mInFinish.load(std::memory_order_relaxed);
}

bool PlatformThread::isAdopted() const
{
    CXXKIT_D(const PlatformThread);
    return d->mData->isAdopted;
}

// PlatformThread::Handle PlatformThread::threadHandle() const
// {
//     CXXKIT_D(const PlatformThread);
//     return d->mData->threadHandle.load();
// }

PlatformThread::Id PlatformThread::threadId() const
{
    CXXKIT_D(const PlatformThread);
    return d->mData->threadId.load();
}

int PlatformThread::retval() const
{
    CXXKIT_D(const PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    return d->mReturnCode;
}

Status PlatformThread::start(Priority priority)
{
    CXXKIT_D(PlatformThread);
    ThreadMutex::UniqueLock lock(d->mMutex);

    if (d->mInFinish)
    {
        d->mDoneCondition.wait(lock, [d]() { return d->mFinished.load(); });
    }
    if (d->mRunning)
    {
        return "PlatformThread::start: Thread already running";
    }

    d->mRunning = true;
    d->mFinished = false;
    d->mReturnCode = 0;
    d->mExited = false;
    d->mInterruptionRequested = false;
    d->mPriority = priority;
    if (!d->start(priority))
    {
        CXXKIT_WARNING("PlatformThread::start: Thread creation error");
        d->mRunning = false;
        d->mFinished = false;
        // d->mData->threadHandle.store(nullptr);
        return "PlatformThread::start: Thread creation error";
    }
    return Status::ok;
}

Status PlatformThread::terminate()
{
    CXXKIT_D(PlatformThread);
    ThreadMutex::Lock lock(d->mMutex);
    if (!this->isRunning())
    {
        return "Thread not running";
    }
    return d->terminate();
}

bool PlatformThread::wait(unsigned int msecs)
{
    CXXKIT_D(PlatformThread);
    if (this->threadId() == PlatformThread::currentThreadId())
    {
        CXXKIT_WARNING("PlatformThread::wait: Thread tried to wait on itself");
        return false;
    }

    ThreadMutex::UniqueLock lock(d->mMutex);
    if (d->mFinished || !d->mRunning)
    {
        return true;
    }

    while (d->mRunning)
    {
        if (kWaitForeverMSecs == msecs)
        {
            d->mDoneCondition.wait(lock);
        }
        else
        {
            if (std::cv_status::timeout == d->mDoneCondition.wait_for(lock, std::chrono::milliseconds(msecs)))
            {
                return false;
            }
        }
    }
    return true;
}

PlatformThread *PlatformThread::currentThread() noexcept
{
    return PlatformThreadData::current()->thread.load();
}

void PlatformThread::yield()
{
    std::this_thread::yield();
}

void PlatformThread::usleep(unsigned long usecs)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

void PlatformThread::msleep(unsigned long msecs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
}

void PlatformThread::sleep(unsigned long secs)
{
    std::this_thread::sleep_for(std::chrono::seconds(secs));
}

PlatformThread::UniquePtr PlatformThread::create(std::future<void> &&future)
{
    return UniquePtr(new detail::PlatformFutureThread(std::move(future)));
}

CXXKIT_END_NAMESPACE
