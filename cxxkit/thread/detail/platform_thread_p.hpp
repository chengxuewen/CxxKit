/***********************************************************************************************************************
**
** Library: cxxkit
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

#pragma once

#include "cxxkit/thread/reference_counter.hpp"
#include "cxxkit/thread/platform_thread.hpp"
#include "cxxkit/tools/logging.hpp"

CXXKIT_BEGIN_NAMESPACE

class PlatformThreadData
{
    mutable ReferenceCounter mRefCounter;

    ~PlatformThreadData()
    {
        if (CXXKIT_UNLIKELY(mRefCounter.loadAcquire() != 0))
        {
            CXXKIT_FATAL("Attempting to call destruct while ref count is not 0.");
        }

        PlatformThreadData::clearCurrent();
        thread.store(nullptr);
    }

public:
    PlatformThreadData(int initialRefCount = 1)
        : mRefCounter(initialRefCount)
    {
        // fprintf(stderr, "PlatformThreadData %p created\n", this);
    }

    static PlatformThreadData *current(PlatformThreadPrivate *thread);
    static PlatformThreadData *current(bool createIfNecessary = true); // impl
    static PlatformThreadData *current(PlatformThread *thread);
    static void clearCurrent(); // impl

    void ref()
    {
        mRefCounter.ref();
        CXXKIT_ASSERT(mRefCounter.loadAcquire() != 0);
    }
    void deref()
    {
        if (CXXKIT_UNLIKELY(mRefCounter.loadAcquire() == 0))
        {
            CXXKIT_FATAL("Attempting to call deref while ref count is 0.");
        }
        if (!mRefCounter.deref())
        {
            // fprintf(stderr, "PlatformThreadData %p delete\n", this);
            delete this;
        }
    }

    bool canWait{true};
    bool quitNow{false};
    bool isAdopted{false};

    int loopLevel{0};
    int scopeLevel{0};

    std::vector<void *> tls;
    std::atomic<PlatformThread::Id> threadId{0};
    std::atomic<PlatformThread *> thread{nullptr};
};

class CXXKIT_CORE_API PlatformThreadPrivate
{
public:
    using ThreadMutex = PlatformThread::ThreadMutex;
    using Priority = PlatformThread::Priority;

    PlatformThreadPrivate(PlatformThread *p, PlatformThreadData *data = nullptr);
    virtual ~PlatformThreadPrivate();

    static void setTerminationEnabled(bool enabled = true) { PlatformThread::setTerminationEnabled(enabled); }
    static PlatformThreadPrivate *get(PlatformThread *p) { return p->dFunc(); }
    static PlatformThread *get(PlatformThreadPrivate *d) { return d->pFunc(); }

    void setPriority(Priority priority); // impl
    bool start(Priority priority);       // impl
    Status terminate();                  // impl

    void onFinished() { mPPtr->onFinished(); }
    void onStarted() { mPPtr->onStarted(); }
    void run() { mPPtr->run(); }

    mutable ThreadMutex mMutex;
    mutable ThreadMutex::Condition mDoneCondition;

    int mReturnCode{-1};
    std::atomic<bool> mExited{false};
    std::atomic<bool> mRunning{false};
    std::atomic<bool> mFinished{false};
    std::atomic<bool> mInFinish{false}; //when in finish
    std::atomic<bool> mInterruptionRequested{false};

    std::string mName;
    uint_t mStackSize{0};
    bool mTerminatePending{false};
    bool mTerminationEnabled{false};
    PlatformThreadData *mData{nullptr};
    Priority mPriority{Priority::kInherit};

#ifdef CXXKIT_OS_WIN
    HANDLE mThreadHandle{nullptr};
#else
    pthread_t mThreadHandle{0};
#endif

protected:
    CXXKIT_DEFINE_PPTR(PlatformThread)
    CXXKIT_DECLARE_PUBLIC(PlatformThread)
    CXXKIT_DISABLE_COPY_MOVE(PlatformThreadPrivate)
};

class AdoptedPlatformThread : public PlatformThread
{
    CXXKIT_DECLARE_PRIVATE(PlatformThread)
public:
    explicit AdoptedPlatformThread(PlatformThreadData *data = nullptr)
        : PlatformThread(new PlatformThreadPrivate(this, data))
    {
        // thread should be running and not finished for the lifetime of the application
        this->dFunc()->mRunning = true;
        this->dFunc()->mFinished = false;
        this->init();
    }
    ~AdoptedPlatformThread() override { CXXKIT_TRACE("~AdoptedPlatformThread = %p\n", (void*)this); }

    void init(); // impl

protected:
    void run() override
    {
        // this function should never be called
        CXXKIT_FATAL("AdoptedPlatformThread::run(): Internal error, this implementation should never be called.");
    }
};

CXXKIT_END_NAMESPACE