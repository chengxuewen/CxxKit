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

#include <cxxkit/time/elapsed_timer.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/tools/logging.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

CXXKIT_BEGIN_NAMESPACE

namespace
{
static Semaphore *semaphore = nullptr;

class Thread
{
public:
    Thread() { }
    virtual ~Thread()
    {
        // CXXKIT_DEBUG("Thread %p ~Thread", this);
        this->waitQuit();
    }

    void start()
    {
        // CXXKIT_DEBUG("Thread %p start", this);
        CXXKIT_ASSERT(mFinished.exchange(false));
        mThread = std::thread(&Thread::threadRun, this);
    }

    void waitQuit()
    {
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    bool is_finished() const { return mFinished.load(); }

    CXXKIT_STATIC_CONSTANT_NUMBER(kWaitForeverMSecs, std::numeric_limits<unsigned long>::max())
    bool wait(unsigned long msecs = kWaitForeverMSecs)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (std::this_thread::get_id() == mThreadId)
        {
            CXXKIT_WARNING("PlatformThread::wait: Thread tried to wait on itself");
            return false;
        }
        if (mFinished)
        {
            // CXXKIT_DEBUG("Thread %p wait finished", this);
            return true;
        }

        while (!mFinished)
        {
            if (kWaitForeverMSecs == msecs)
            {
                mDoneCondition.wait(lock);
            }
            else
            {
                if (std::cv_status::timeout == mDoneCondition.wait_for(lock, std::chrono::milliseconds(msecs)))
                {
                    // CXXKIT_DEBUG("Thread %p wait timeout", this);
                    return false;
                }
            }
        }
        // CXXKIT_DEBUG("Thread %p wait no_timeout", this);
        return true;
    }

protected:
    virtual void run() = 0;

private:
    void threadRun()
    {
        // CXXKIT_DEBUG("Thread %p threadRun start", this);
        std::unique_lock<std::mutex> lock(mMutex);
        mThreadId = std::this_thread::get_id();
        lock.unlock();
        this->run();
        lock.lock();
        // CXXKIT_DEBUG("Thread %p threadRun mFinished", this);
        mFinished.store(true);
        mDoneCondition.notify_all();
        // CXXKIT_DEBUG("Thread %p threadRun finish", this);
    }

    std::thread mThread;
    std::thread::id mThreadId;
    std::atomic<bool> mFinished{true};
    mutable std::mutex mMutex;
    mutable std::condition_variable mDoneCondition;
};

class ThreadOne : public Thread
{
public:
    ThreadOne() { }

protected:
    void run() override
    {
        int i = 0;
        while (i < 100)
        {
            semaphore->acquire();
            i++;
            semaphore->release();
        }
    }
};


class ThreadN : public Thread
{
    int N;

public:
    ThreadN(int n)
        : N(n)
    {
    }

protected:
    void run() override
    {
        int i = 0;
        while (i < 100)
        {
            semaphore->acquire(N);
            i++;
            semaphore->release(N);
        }
    }
};
} // namespace

TEST(PerformanceTest, Acquire)
{
    {
        // old incrementOne() test
        ASSERT_TRUE(!semaphore);
        semaphore = new Semaphore;
        // make some "thing" available
        semaphore->release();

        ThreadOne t1;
        ThreadOne t2;

        t1.start();
        t2.start();

        ASSERT_TRUE(t1.wait(4000));
        ASSERT_TRUE(t2.wait(4000));

        delete semaphore;
        semaphore = 0;
    }

    // old incrementN() test
    {
        ASSERT_TRUE(!semaphore);
        semaphore = new Semaphore;
        // make 4 "things" available
        semaphore->release(4);

        ThreadN t1(2);
        ThreadN t2(3);

        t1.start();
        t2.start();

        ASSERT_TRUE(t1.wait(4000));
        ASSERT_TRUE(t2.wait(4000));

        delete semaphore;
        semaphore = 0;
    }

    Semaphore semaphore;

    ASSERT_EQ(semaphore.available(), 0);
    semaphore.release();
    ASSERT_EQ(semaphore.available(), 1);
    semaphore.release();
    ASSERT_EQ(semaphore.available(), 2);
    semaphore.release(10);
    ASSERT_EQ(semaphore.available(), 12);
    semaphore.release(10);
    ASSERT_EQ(semaphore.available(), 22);

    semaphore.acquire();
    ASSERT_EQ(semaphore.available(), 21);
    semaphore.acquire();
    ASSERT_EQ(semaphore.available(), 20);
    semaphore.acquire(10);
    ASSERT_EQ(semaphore.available(), 10);
    semaphore.acquire(10);
    ASSERT_EQ(semaphore.available(), 0);
}

TEST(PerformanceTest, MultiRelease)
{
    class MultiReleaseThread : public Thread
    {
    public:
        Semaphore &sem;
        Semaphore startup;
        MultiReleaseThread(Semaphore &sem)
            : sem(sem)
        {
        }

        void run() override
        {
            startup.release(); // signal: this thread has reached acquire()
            sem.acquire();
        }
    };

    Semaphore sem;
    std::vector<MultiReleaseThread *> threads;
    threads.resize(4);

    for (MultiReleaseThread *&t : threads)
    {
        t = new MultiReleaseThread(sem);
    }
    for (MultiReleaseThread *&t : threads)
    {
        t->start();
    }

    // wait for all threads to reach the sem.acquire() (startup signals), then release them all
    for (MultiReleaseThread *&t : threads)
    {
        t->startup.acquire();
    }
    sem.release(threads.size());

    for (MultiReleaseThread *&t : threads)
    {
        t->wait();
        delete t;
    }
}

TEST(PerformanceTest, MultiAcquireRelease)
{
    class MultiAcquireReleaseThread : public Thread
    {
    public:
        Semaphore &sem;
        Semaphore startup;
        MultiAcquireReleaseThread(Semaphore &sem)
            : sem(sem)
        {
        }

        void run() override
        {
            startup.release(); // signal: this thread has reached acquire()
            sem.acquire();
            sem.release();
        }
    };

    Semaphore sem;
    std::vector<MultiAcquireReleaseThread *> threads;
    threads.resize(4);

    for (MultiAcquireReleaseThread *&t : threads)
    {
        t = new MultiAcquireReleaseThread(sem);
    }
    for (MultiAcquireReleaseThread *&t : threads)
    {
        t->start();
    }

    // wait for all threads to reach the sem.acquire() (startup signals), then release
    for (MultiAcquireReleaseThread *&t : threads)
    {
        t->startup.acquire();
    }
    sem.release();

    for (MultiAcquireReleaseThread *&t : threads)
    {
        t->wait();
        delete t;
    }
}

TEST(PerformanceTest, TryAcquire)
{
    Semaphore semaphore;

    ASSERT_EQ(semaphore.available(), 0);

    semaphore.release();
    ASSERT_EQ(semaphore.available(), 1);
    ASSERT_TRUE(!semaphore.try_acquire(2));
    ASSERT_TRUE(!semaphore.try_acquire(2, 0));
    ASSERT_EQ(semaphore.available(), 1);

    semaphore.release();
    ASSERT_EQ(semaphore.available(), 2);
    ASSERT_TRUE(!semaphore.try_acquire(3));
    ASSERT_TRUE(!semaphore.try_acquire(3, 0));
    ASSERT_EQ(semaphore.available(), 2);

    semaphore.release(10);
    ASSERT_EQ(semaphore.available(), 12);
    ASSERT_TRUE(!semaphore.try_acquire(100));
    ASSERT_TRUE(!semaphore.try_acquire(100, 0));
    ASSERT_EQ(semaphore.available(), 12);

    semaphore.release(10);
    ASSERT_EQ(semaphore.available(), 22);
    ASSERT_TRUE(!semaphore.try_acquire(100));
    ASSERT_TRUE(!semaphore.try_acquire(100, 0));
    ASSERT_EQ(semaphore.available(), 22);

    ASSERT_TRUE(semaphore.try_acquire());
    ASSERT_EQ(semaphore.available(), 21);

    ASSERT_TRUE(semaphore.try_acquire());
    ASSERT_EQ(semaphore.available(), 20);

    semaphore.release(2);
    ASSERT_TRUE(semaphore.try_acquire(1, 0));
    ASSERT_EQ(semaphore.available(), 21);

    ASSERT_TRUE(semaphore.try_acquire(1, 0));
    ASSERT_EQ(semaphore.available(), 20);

    ASSERT_TRUE(semaphore.try_acquire(10));
    ASSERT_EQ(semaphore.available(), 10);

    semaphore.release(10);
    ASSERT_TRUE(semaphore.try_acquire(10, 0));
    ASSERT_EQ(semaphore.available(), 10);

    ASSERT_TRUE(semaphore.try_acquire(10));
    ASSERT_EQ(semaphore.available(), 0);

    // should not be able to acquire more
    ASSERT_TRUE(!semaphore.try_acquire());
    ASSERT_TRUE(!semaphore.try_acquire(1, 0));
    ASSERT_EQ(semaphore.available(), 0);

    ASSERT_TRUE(!semaphore.try_acquire());
    ASSERT_TRUE(!semaphore.try_acquire(1, 0));
    ASSERT_EQ(semaphore.available(), 0);

    ASSERT_TRUE(!semaphore.try_acquire(10));
    ASSERT_TRUE(!semaphore.try_acquire(10, 0));
    ASSERT_EQ(semaphore.available(), 0);

    ASSERT_TRUE(!semaphore.try_acquire(10));
    ASSERT_TRUE(!semaphore.try_acquire(10, 0));
    ASSERT_EQ(semaphore.available(), 0);
}

TEST(PerformanceTest, TryAcquireWithTimeout)
{
    std::vector<int> timeouts = {200, 2000};
    for (auto timeout : timeouts)
    {
        // timers are not guaranteed to be accurate down to the last millisecond,
        // so we permit the elapsed times to be up to this far from the expected value.
        int fuzz = 50 + (timeout / 20);

        Semaphore semaphore;
        ElapsedTimer time;

#define FUZZYCOMPARE(a, e)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        int a1 = a;                                                                                                    \
        int e1 = e;                                                                                                    \
        ASSERT_TRUE(std::abs(a1 - e1) < fuzz)                                                                          \
            << "(" << #a << "=" << std::to_string(a1) << ") is more than " << std::to_string(fuzz)                     \
            << " milliseconds different from (" << #e << "=" << std::to_string(e1) << ")";                             \
    } while (0)

        ASSERT_EQ(semaphore.available(), 0);

        semaphore.release();
        ASSERT_EQ(semaphore.available(), 1);
        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(2, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 1);

        semaphore.release();
        ASSERT_EQ(semaphore.available(), 2);
        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(3, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 2);

        semaphore.release(10);
        ASSERT_EQ(semaphore.available(), 12);
        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(100, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 12);

        semaphore.release(10);
        ASSERT_EQ(semaphore.available(), 22);
        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(100, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 22);

        time.start();
        ASSERT_TRUE(semaphore.try_acquire(1, timeout));
        FUZZYCOMPARE(time.elapsed(), 0);
        ASSERT_EQ(semaphore.available(), 21);

        time.start();
        ASSERT_TRUE(semaphore.try_acquire(1, timeout));
        FUZZYCOMPARE(time.elapsed(), 0);
        ASSERT_EQ(semaphore.available(), 20);

        time.start();
        ASSERT_TRUE(semaphore.try_acquire(10, timeout));
        FUZZYCOMPARE(time.elapsed(), 0);
        ASSERT_EQ(semaphore.available(), 10);

        time.start();
        ASSERT_TRUE(semaphore.try_acquire(10, timeout));
        FUZZYCOMPARE(time.elapsed(), 0);
        ASSERT_EQ(semaphore.available(), 0);

        // should not be able to acquire more
        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(1, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 0);

        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(1, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 0);

        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(10, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 0);

        time.start();
        ASSERT_TRUE(!semaphore.try_acquire(10, timeout));
        FUZZYCOMPARE(time.elapsed(), timeout);
        ASSERT_EQ(semaphore.available(), 0);

#undef FUZZYCOMPARE
    }
}

TEST(PerformanceTest, TryAcquireWithTimeoutStarvation)
{
    class TryAcquireWithTimeoutStarvationThread : public Thread
    {
    public:
        Semaphore startup;
        Semaphore *semaphore;
        int amountToConsume, timeout;

        void run()
        {
            startup.release();
            while (true)
            {
                if (!semaphore->try_acquire(amountToConsume, timeout))
                {
                    break;
                }
                semaphore->release(amountToConsume);
            }
        }
    };

    Semaphore semaphore;
    semaphore.release(1);

    TryAcquireWithTimeoutStarvationThread consumer;
    consumer.semaphore = &semaphore;
    consumer.amountToConsume = 1;
    consumer.timeout = 1000;

    // start the thread and wait for it to start consuming
    consumer.start();
    consumer.startup.acquire();

    // try to consume more than the thread we started is, and provide a longer
    // timeout... we should timeout, not wait indefinitely
    ASSERT_TRUE(!semaphore.try_acquire(consumer.amountToConsume * 2, consumer.timeout * 2));

    // the consumer should still be running
    ASSERT_TRUE(!consumer.is_finished());

    // acquire, and wait for smallConsumer to timeout
    semaphore.acquire();
    ASSERT_TRUE(consumer.wait());
}

TEST(PerformanceTest, TryAcquireWithTimeoutForever)
{
    std::vector<int> timeouts = {-1, std::numeric_limits<int>::max()};
    enum
    {
        WaitTime = 1000
    };
    struct TryAcquireWithTimeoutForeverThread : public Thread
    {
        Semaphore sem;

        void run() override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(WaitTime));
            sem.release(2);
        }
    };
    for (auto timeout : timeouts)
    {
        TryAcquireWithTimeoutForeverThread t;

        // sanity check it works if we can immediately acquire
        t.sem.release(11);
        ASSERT_TRUE(t.sem.try_acquire(1, timeout));
        ASSERT_TRUE(t.sem.try_acquire(10, timeout));

        // verify that we do wait for at least WaitTime if we can't acquire immediately
        ElapsedTimer timer;
        timer.start();
        t.start();
        ASSERT_TRUE(t.sem.try_acquire(1, timeout));
        ASSERT_TRUE(timer.elapsed() >= WaitTime);

        ASSERT_TRUE(t.wait());

        ASSERT_EQ(t.sem.available(), 1);
    }
}

namespace
{

const char alphabet[] = "ACGTH";
const int AlphabetSize = sizeof(alphabet) - 1;

const int BufferSize = 4096; // GCD of BufferSize and alphabet size must be 1
char buffer[BufferSize];

const int ProducerChunkSize = 3;
const int ConsumerChunkSize = 7;
const int Multiplier = 10;

// note: the code depends on the fact that DataSize is a multiple of
// ProducerChunkSize, ConsumerChunkSize, and BufferSize
const int DataSize = ProducerChunkSize * ConsumerChunkSize * BufferSize * Multiplier;

Semaphore freeSpace(BufferSize);
Semaphore usedSpace;

class Producer : public Thread
{
public:
    void run();
};

static const int Timeout = 60 * 1000; // 1min

void Producer::run()
{
    for (int i = 0; i < DataSize; ++i)
    {
        ASSERT_TRUE(freeSpace.try_acquire(1, Timeout));
        buffer[i % BufferSize] = alphabet[i % AlphabetSize];
        usedSpace.release();
    }
    for (int i = 0; i < DataSize; ++i)
    {
        if ((i % ProducerChunkSize) == 0)
        {
            ASSERT_TRUE(freeSpace.try_acquire(ProducerChunkSize, Timeout));
        }
        buffer[i % BufferSize] = alphabet[i % AlphabetSize];
        if ((i % ProducerChunkSize) == (ProducerChunkSize - 1))
        {
            usedSpace.release(ProducerChunkSize);
        }
    }
}

class Consumer : public Thread
{
public:
    void run();
};

void Consumer::run()
{
    for (int i = 0; i < DataSize; ++i)
    {
        usedSpace.acquire();
        ASSERT_EQ(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
        freeSpace.release();
    }
    for (int i = 0; i < DataSize; ++i)
    {
        if ((i % ConsumerChunkSize) == 0)
        {
            usedSpace.acquire(ConsumerChunkSize);
        }
        ASSERT_EQ(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
        if ((i % ConsumerChunkSize) == (ConsumerChunkSize - 1))
        {
            freeSpace.release(ConsumerChunkSize);
        }
    }
}
} // namespace
TEST(PerformanceTest, ProducerConsumer)
{
    Producer producer;
    Consumer consumer;
    producer.start();
    consumer.start();
    producer.wait();
    consumer.wait();
}

TEST(PerformanceTest, Raii)
{
    Semaphore sem;

    ASSERT_EQ(sem.available(), 0);

    // basic operation:
    {
        Semaphore::Releaser r0;
        const Semaphore::Releaser r1(sem);
        const Semaphore::Releaser r2(sem, 2);

        ASSERT_EQ(r0.semaphore(), nullptr);
        ASSERT_EQ(r1.semaphore(), &sem);
        ASSERT_EQ(r2.semaphore(), &sem);
    }

    ASSERT_EQ(sem.available(), 3);

    // cancel:
    {
        const Semaphore::Releaser r1(sem);
        Semaphore::Releaser r2(sem, 2);

        ASSERT_EQ(r2.cancel(), &sem);
        ASSERT_EQ(r2.semaphore(), nullptr);
    }

    ASSERT_EQ(sem.available(), 4);

    // move-assignment:
    {
        const Semaphore::Releaser r1(sem);
        Semaphore::Releaser r2(sem, 2);

        ASSERT_EQ(sem.available(), 4);

        r2 = Semaphore::Releaser();

        ASSERT_EQ(sem.available(), 6);

        r2 = Semaphore::Releaser(sem, 42);

        ASSERT_EQ(sem.available(), 6);
    }

    ASSERT_EQ(sem.available(), 49);
}

CXXKIT_END_NAMESPACE