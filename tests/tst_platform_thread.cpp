/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright 2016 The WebRTC Project Authors.
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

#include <cxxkit/thread/platform_thread.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/tools/logging.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
enum
{
    kOneMinute = 60 * 1000,
    kFiveMinutes = 5 * kOneMinute
};

static inline int64_t timeSinceEpochMSecs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

class CurrentThread : public PlatformThread
{
public:
    PlatformThread::Id id{0};
    PlatformThread *thread{nullptr};
    std::atomic<bool> loopWait{false};

    CurrentThread() { }
    ~CurrentThread() override { loopWait.store(false); }

    void waitLoopWait()
    {
        while (!loopWait.load())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    void stopLoopWait() { loopWait.store(false); }

protected:
    void run() override
    {
        id = currentThreadId();
        thread = currentThread();
        loopWait.store(true);
        while (loopWait.load())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
};
class SimpleThread : public PlatformThread
{
public:
    std::mutex mutex;
    std::condition_variable cond;

    using PlatformThread::PlatformThread;
    SimpleThread() = default;

protected:
    void run()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond.notify_one();
    }
};
class TerminateThread : public SimpleThread
{
public:
    using SimpleThread::SimpleThread;
    TerminateThread() = default;

protected:
    void run()
    {
        this->setTerminationEnabled(false);
        {
            std::unique_lock<std::mutex> lock(mutex);
            cond.notify_one();
            cond.wait_for(lock, std::chrono::milliseconds(kFiveMinutes));
        }
        this->setTerminationEnabled(true);
        CXXKIT_FATAL("TerminateThread: test case hung");
    }
};
class SleepThread : public SimpleThread
{
public:
    enum SleepType
    {
        Second,
        Millisecond,
        Microsecond
    };

    SleepType sleepType;
    int interval;

    int elapsed; // result, in *MILLISECONDS*

    void run()
    {
        std::unique_lock<std::mutex> lock(mutex);
        const auto now = timeSinceEpochMSecs();
        switch (sleepType)
        {
            case Second: this->sleep(interval); break;
            case Millisecond: this->msleep(interval); break;
            case Microsecond: this->usleep(interval); break;
        }
        elapsed = timeSinceEpochMSecs() - now;
        cond.notify_one();
    }
};
class WaitingThread : public PlatformThread
{
public:
    enum
    {
        WaitTime = 800
    };
    std::mutex mutex;
    std::condition_variable cond1;
    std::condition_variable cond2;

protected:
    void run()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond1.wait(lock);
        cond2.wait_for(lock, std::chrono::milliseconds(WaitTime));
    }
};

void noop(void *)
{
}
class ThreadWrapper
{
public:
    typedef void (*FunctionPointer)(void *);

    ThreadWrapper() { }
    virtual ~ThreadWrapper() { }

    PlatformThread *platformThread() const { return mPlatformThread; }

    void start(FunctionPointer functionPointer = noop, void *data = 0)
    {
        this->mFunctionPointer = functionPointer;
        this->mData = data;
        mThread = std::thread(std::bind(&ThreadWrapper::run, this));
    }
    void startAndWait(FunctionPointer functionPointer = noop, void *data = 0)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        this->start(functionPointer, data);
        mStartCondition.wait(lock);
    }
    void join() { mThread.join(); }
    void setWaitForStop() { mWaitForStop = true; }
    void stop()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mWaitForStop = false;
        mStopCondition.notify_one();
    }

protected:
    static void run(void *that)
    {
        auto threadWrapper = reinterpret_cast<ThreadWrapper *>(that);

        // Adopt thread, create QThread object.
        threadWrapper->mPlatformThread = PlatformThread::currentThread();

        // Release main thread.
        {
            std::unique_lock<std::mutex> lock(threadWrapper->mMutex);
            threadWrapper->mStartCondition.notify_one();
        }

        // Run function.
        threadWrapper->mFunctionPointer(threadWrapper->mData);

        // Wait for stop.
        {
            std::unique_lock<std::mutex> lock(threadWrapper->mMutex);
            if (threadWrapper->mWaitForStop)
            {
                threadWrapper->mStopCondition.wait(lock);
            }
        }
    }

private:
    std::thread mThread;
    PlatformThread *mPlatformThread{nullptr};

    std::mutex mMutex;
    bool mWaitForStop{false};
    std::condition_variable mStartCondition;
    std::condition_variable mStopCondition;

    void *mData;
    FunctionPointer mFunctionPointer;
};
} // namespace

TEST(PlatformThreadTest, DefaultConstructedIsInvalid)
{
    PlatformThread thread;
    EXPECT_EQ(thread.threadId(), 0);
}

TEST(PlatformThreadTest, CurrentThreadId)
{
    for (int i = 0; i < 50; ++i)
    {
        CurrentThread thread;
        thread.start();
        thread.waitLoopWait();
        EXPECT_EQ(thread.id, thread.threadId());
        thread.stopLoopWait();
        EXPECT_TRUE(thread.wait(1000)); // 1000ms
        EXPECT_NE(thread.id, 0);
        EXPECT_NE(thread.id, PlatformThread::currentThreadId());
    }
}

TEST(PlatformThreadTest, CurrentThread)
{
    EXPECT_NE(PlatformThread::currentThread(), nullptr);

    CurrentThread thread;
    thread.start();
    thread.waitLoopWait();
    thread.stopLoopWait();
    EXPECT_TRUE(thread.wait(1000)); // 1000ms
    EXPECT_EQ(thread.thread, &thread);
}

TEST(PlatformThreadTest, IsFinished)
{
    SimpleThread thread;

    EXPECT_FALSE(thread.isFinished());

    std::unique_lock<std::mutex> lock(thread.mutex);
    thread.start();
    EXPECT_FALSE(thread.isFinished());
    thread.cond.wait(lock);
    EXPECT_TRUE(thread.wait(1000)); // 1000ms
    EXPECT_TRUE(thread.isFinished());
}

TEST(PlatformThreadTest, IsRunning)
{
    SimpleThread thread;

    EXPECT_FALSE(thread.isRunning());

    std::unique_lock<std::mutex> lock(thread.mutex);
    thread.start();
    EXPECT_TRUE(thread.isRunning());
    thread.cond.wait(lock);
    EXPECT_TRUE(thread.wait(1000)); // 1000ms
    EXPECT_FALSE(thread.isRunning());
}

TEST(PlatformThreadTest, SetPriority)
{
    SimpleThread thread;
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);

    // cannot change the priority, since the thread is not running
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kIdle).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kLowest).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kLow).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kNormal).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kHigh).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kHighest).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kTimeCritical).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kInherit).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);

    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    std::unique_lock<std::mutex> lock(thread.mutex);
    thread.start();
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kInherit).isOk()); //"Argument cannot be InheritPriority";
    // change the priority of a running thread
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kIdle).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kIdle);
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kLowest).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kLowest);
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kLow).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kLow);
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kNormal).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kNormal);
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kHigh).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kHigh);
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kHighest).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kHighest);
    EXPECT_TRUE(thread.setPriority(PlatformThread::Priority::kTimeCritical).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kTimeCritical);
    thread.cond.wait(lock);
    EXPECT_TRUE(thread.wait(1000)); // 1000ms

    // cannot change the priority, since the thread is finished
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kIdle).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kLowest).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kLow).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kNormal).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kHigh).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kHighest).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kTimeCritical).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
    EXPECT_FALSE(thread.setPriority(PlatformThread::Priority::kInherit).isOk());
    EXPECT_EQ(thread.priority(), PlatformThread::Priority::kInherit);
}

TEST(PlatformThreadTest, SetStackSize)
{
    SimpleThread thread;
    EXPECT_EQ(thread.stackSize(), 0u);
    thread.setStackSize(8192u);
    EXPECT_EQ(thread.stackSize(), 8192u);
    thread.setStackSize(0u);
    EXPECT_EQ(thread.stackSize(), 0u);
}

TEST(PlatformThreadTest, Start)
{
    PlatformThread::Priority priorities[] = {PlatformThread::Priority::kIdle,
                                             PlatformThread::Priority::kLowest,
                                             PlatformThread::Priority::kLow,
                                             PlatformThread::Priority::kNormal,
                                             PlatformThread::Priority::kHigh,
                                             PlatformThread::Priority::kHighest,
                                             PlatformThread::Priority::kTimeCritical,
                                             PlatformThread::Priority::kInherit};
    const int prio_count = sizeof(priorities) / sizeof(PlatformThread::Priority);

    for (auto i = 0; i < prio_count; ++i)
    {
        SimpleThread thread;
        EXPECT_FALSE(thread.isFinished());
        EXPECT_FALSE(thread.isRunning());
        std::unique_lock<std::mutex> lock(thread.mutex);
        thread.start(priorities[i]);
        EXPECT_TRUE(thread.isRunning());
        EXPECT_FALSE(thread.isFinished());
        thread.cond.wait(lock);
        EXPECT_TRUE(thread.wait(1000)); // 1000ms
        EXPECT_TRUE(thread.isFinished());
        EXPECT_FALSE(thread.isRunning());
    }
}

TEST(PlatformThreadTest, Terminate)
{
#if defined(CXXKIT_OS_WINRT) || defined(CXXKIT_OS_ANDROID)
    CXXKIT_WARNING("PlatformThread termination is not supported on WinRT or Android.");
#endif
    TerminateThread thread;
    {
        std::unique_lock<std::mutex> lock(thread.mutex);
        thread.start();
        EXPECT_EQ(std::cv_status::no_timeout, thread.cond.wait_for(lock, std::chrono::milliseconds(kFiveMinutes)));
        thread.terminate();
        thread.cond.notify_one();
    }
    EXPECT_TRUE(thread.wait(kFiveMinutes));
}

TEST(PlatformThreadTest, Sleep)
{
    SleepThread thread;
    thread.sleepType = SleepThread::Second;
    thread.interval = 2;
    thread.start();
    EXPECT_TRUE(thread.wait(kFiveMinutes));
    EXPECT_TRUE(thread.elapsed >= 2000) << "elapsed:" << std::to_string(thread.elapsed);
}

TEST(PlatformThreadTest, MSleep)
{
    SleepThread thread;
    thread.sleepType = SleepThread::Millisecond;
    thread.interval = 120;
    thread.start();
    EXPECT_TRUE(thread.wait(kFiveMinutes));
    EXPECT_TRUE(thread.elapsed >= 120) << "elapsed:" << std::to_string(thread.elapsed);
}

TEST(PlatformThreadTest, USleep)
{
    SleepThread thread;
    thread.sleepType = SleepThread::Microsecond;
    thread.interval = 120000;
    thread.start();
    EXPECT_TRUE(thread.wait(kFiveMinutes));
    EXPECT_TRUE(thread.elapsed >= 120) << "elapsed:" << std::to_string(thread.elapsed);
}

bool threadAdoptedOk = false;
PlatformThread *mainThread{nullptr};
void testNativeThreadAdoption(void *)
{
    threadAdoptedOk = (PlatformThread::currentThreadId() != 0 && PlatformThread::currentThread() != 0 &&
                       PlatformThread::currentThread() != mainThread);
}
TEST(PlatformThreadTest, Adoption)
{
    threadAdoptedOk = false;
    mainThread = PlatformThread::currentThread();
    ThreadWrapper threadWrapper;
    threadWrapper.setWaitForStop();
    threadWrapper.startAndWait(testNativeThreadAdoption);
    EXPECT_NE(threadWrapper.platformThread(), nullptr);

    threadWrapper.stop();
    threadWrapper.join();

    EXPECT_TRUE(threadAdoptedOk);
}

TEST(PlatformThreadTest, AdoptedThreadSetPriority)
{
    ThreadWrapper threadWrapper;
    threadWrapper.setWaitForStop();
    threadWrapper.startAndWait();

    // change the priority of a running thread
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kInherit);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kIdle);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kIdle);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kLowest);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kLowest);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kLow);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kLow);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kNormal);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kNormal);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kHigh);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kHigh);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kHighest);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kHighest);
    threadWrapper.platformThread()->setPriority(PlatformThread::Priority::kTimeCritical);
    EXPECT_EQ(threadWrapper.platformThread()->priority(), PlatformThread::Priority::kTimeCritical);

    threadWrapper.stop();
    threadWrapper.join();
}

TEST(PlatformThreadTest, AdoptedThreadExit)
{
    ThreadWrapper threadWrapper;
    threadWrapper.setWaitForStop();

    threadWrapper.startAndWait();
    EXPECT_TRUE(threadWrapper.platformThread());
    EXPECT_TRUE(threadWrapper.platformThread()->isRunning());
    EXPECT_TRUE(!threadWrapper.platformThread()->isFinished());

    threadWrapper.stop();
    threadWrapper.join();
}

TEST(PlatformThreadTest, Waiting)
{
    WaitingThread thread;
    thread.start();
    auto start = timeSinceEpochMSecs();
    EXPECT_FALSE(thread.wait(WaitingThread::WaitTime));
    auto elapsed = timeSinceEpochMSecs() - start; // On Windows, we sometimes get (WaitTime - 9).
    EXPECT_TRUE(elapsed >= WaitingThread::WaitTime - 10) << "elapsed:" << std::to_string(elapsed);

    start = timeSinceEpochMSecs();
    thread.cond1.notify_one();
    EXPECT_TRUE(thread.wait(WaitingThread::WaitTime * 1.4));
    elapsed = timeSinceEpochMSecs() - start;
    EXPECT_TRUE(elapsed - WaitingThread::WaitTime >= -1) << "elapsed:" << std::to_string(elapsed);
}

TEST(PlatformThreadTest, Create)
{
    {
        const auto &function = []() {};
        auto thread = PlatformThread::create(function);
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        thread->start();
        EXPECT_TRUE(thread->wait());
    }

    {
        // no side effects before starting
        int i = 0;
        const auto &function = [&i]() { i = 42; };
        auto thread(PlatformThread::create(function));
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        EXPECT_EQ(i, 0);
        thread->start();
        EXPECT_TRUE(thread->wait());
        EXPECT_EQ(i, 42);
    }

    {
        // control thread progress
        Semaphore semaphore1;
        Semaphore semaphore2;

        const auto &function = [&semaphore1, &semaphore2]() -> void
        {
            semaphore1.acquire();
            semaphore2.release();
        };

        auto thread(PlatformThread::create(function));
        EXPECT_TRUE(thread);
        thread->start();
        EXPECT_TRUE(thread->isRunning());
        semaphore1.release();
        semaphore2.acquire();
        EXPECT_TRUE(thread->wait());
        EXPECT_FALSE(thread->isRunning());
    }

    {
        // ignore return values
        const auto &function = []() { return 42; };
        auto thread(PlatformThread::create(function));
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        thread->start();
        EXPECT_TRUE(thread->wait());
    }

    {
        // return value of create
        PlatformThread::UniquePtr thread;
        Semaphore s;
        const auto &function = [&thread, &s]() -> void
        {
            s.acquire();
            EXPECT_EQ(thread.get(), PlatformThread::currentThread());
        };

        thread = std::move(PlatformThread::create(function));
        EXPECT_TRUE(thread);
        thread->start();
        EXPECT_TRUE(thread->isRunning());
        s.release();
        EXPECT_TRUE(thread->wait());
    }

    {
        // move-only parameters
        struct MoveOnlyValue
        {
            explicit MoveOnlyValue(int v)
                : v(v)
            {
            }
            ~MoveOnlyValue() = default;
            MoveOnlyValue(const MoveOnlyValue &) = delete;
            MoveOnlyValue(MoveOnlyValue &&) = default;
            MoveOnlyValue &operator=(const MoveOnlyValue &) = delete;
            MoveOnlyValue &operator=(MoveOnlyValue &&) = default;
            int v;
        };

        struct MoveOnlyFunctor
        {
            explicit MoveOnlyFunctor(int *i)
                : i(i)
            {
            }
            ~MoveOnlyFunctor() = default;
            MoveOnlyFunctor(const MoveOnlyFunctor &) = delete;
            MoveOnlyFunctor(MoveOnlyFunctor &&) = default;
            MoveOnlyFunctor &operator=(const MoveOnlyFunctor &) = delete;
            MoveOnlyFunctor &operator=(MoveOnlyFunctor &&) = default;
            int operator()() { return (*i = 42); }
            int *i;
        };

        {
            int i = 0;
            MoveOnlyFunctor f(&i);
            auto thread(PlatformThread::create(std::move(f)));
            EXPECT_TRUE(thread);
            EXPECT_FALSE(thread->isRunning());
            thread->start();
            EXPECT_TRUE(thread->wait());
            EXPECT_EQ(i, 42);
        }

#if CXXKIT_PLATFORM_THREAD_HAS_INIT_CAPTURES
        {
            int i = 0;
            MoveOnlyValue mo(123);
            auto moveOnlyFunction = [&i, mo = std::move(mo)]() { i = mo.v; };
            auto thread(PlatformThread::create(std::move(moveOnlyFunction)));
            EXPECT_TRUE(thread);
            EXPECT_FALSE(thread->isRunning());
            thread->start();
            EXPECT_TRUE(thread->wait());
            EXPECT_EQ(i, 123);
        }
#endif // CXXKIT_PLATFORM_THREAD_HAS_INIT_CAPTURES

#if CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE
        {
            int i = 0;
            const auto &function = [&i](MoveOnlyValue &&mo) { i = mo.v; };
            auto thread(PlatformThread::create(function, MoveOnlyValue(123)));
            EXPECT_TRUE(thread);
            EXPECT_FALSE(thread->isRunning());
            thread->start();
            EXPECT_TRUE(thread->wait());
            EXPECT_EQ(i, 123);
        }

        {
            int i = 0;
            const auto &function = [&i](MoveOnlyValue &&mo) { i = mo.v; };
            MoveOnlyValue mo(-1);
            auto thread(PlatformThread::create(function, std::move(mo)));
            EXPECT_TRUE(thread);
            EXPECT_FALSE(thread->isRunning());
            thread->start();
            EXPECT_TRUE(thread->wait());
            EXPECT_EQ(i, -1);
        }
#endif // CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE
    }

#if CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE
    {
        // simple parameter passing
        int i = 0;
        const auto &function = [&i](int j, int k) { i = j * k; };
        auto thread(PlatformThread::create(function, 3, 4));
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        EXPECT_EQ(i, 0);
        thread->start();
        EXPECT_TRUE(thread->wait());
        EXPECT_EQ(i, 12);
    }

    {
        // ignore return values (with parameters)
        const auto &function = [](double d) { return d * 2.0; };
        auto thread(PlatformThread::create(function, 3.14));
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        thread->start();
        EXPECT_TRUE(thread->wait());
    }

    {
        // handling of pointers to member functions, std::ref, etc.
        struct S
        {
            S()
                : v(0)
            {
            }
            void doSomething() { ++v; }
            int v;
        };

        S object;

        EXPECT_EQ(object.v, 0);

        PlatformThread::UniquePtr thread;
        thread = PlatformThread::create(&S::doSomething, object);
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        thread->start();
        EXPECT_TRUE(thread->wait());

        EXPECT_EQ(object.v, 0); // a copy was passed, this should still be 0

        thread = PlatformThread::create(&S::doSomething, std::ref(object));
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        thread->start();
        EXPECT_TRUE(thread->wait());

        EXPECT_EQ(object.v, 1);

        thread = PlatformThread::create(&S::doSomething, &object);
        EXPECT_TRUE(thread);
        EXPECT_FALSE(thread->isRunning());
        thread->start();
        EXPECT_TRUE(thread->wait());

        EXPECT_EQ(object.v, 2);
    }

    {
        // std::ref into ordinary reference
        int i = 42;
        const auto &function = [](int &i) { i *= 2; };
        auto thread(PlatformThread::create(function, std::ref(i)));
        EXPECT_TRUE(thread);
        thread->start();
        EXPECT_TRUE(thread->wait());
        EXPECT_EQ(i, 84);
    }

#    if CXXKIT_HAS_EXCEPTIONS
    {
        // exceptions when copying/decaying the arguments are thrown at build side and won't terminate
        class ThreadException : public std::exception
        {
        };

        struct ThrowWhenCopying
        {
            ThrowWhenCopying() = default;
            ThrowWhenCopying(const ThrowWhenCopying &) { throw ThreadException(); }
            ~ThrowWhenCopying() = default;
            ThrowWhenCopying &operator=(const ThrowWhenCopying &) = default;
        };

        const auto &function = [](const ThrowWhenCopying &) {};
        PlatformThread::UniquePtr thread;
        ThrowWhenCopying t;
        EXPECT_THROW(thread = PlatformThread::create(function, t), ThreadException);
        EXPECT_FALSE(thread);
    }
#    endif // CXXKIT_HAS_EXCEPTIONS
#endif     // CXXKIT_PLATFORM_THREAD_HAS_VARIADIC_CREATE
}

CXXKIT_END_NAMESPACE