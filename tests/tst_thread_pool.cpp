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

#include <cxxkit/time/elapsed_timer.hpp>
#include <cxxkit/thread/thread_pool.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/tools/logging.hpp>

#include <list>
#include <atomic>
#include <thread>
#include <memory>
#include <random>
#include <utility>

#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
static std::atomic<int> testFunctionCount;
static std::mutex *functionTestMutex{nullptr};

void emptyFunct()
{
}

void noSleepTestFunction()
{
    ++testFunctionCount;
}

void noSleepTestFunctionMutex()
{
    assert(functionTestMutex);
    functionTestMutex->lock();
    ++testFunctionCount;
    functionTestMutex->unlock();
}

void sleepTestFunctionMutex()
{
    assert(functionTestMutex);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    functionTestMutex->lock();
    ++testFunctionCount;
    functionTestMutex->unlock();
}

std::atomic<bool> ran{false}; // bool
class TestTask : public Task
{
public:
    void run() override { ran.store(true); }
};

std::atomic<int> *value = nullptr;
class IntAccessor : public Task
{
public:
    void run() override
    {
        for (int i = 0; i < 100; ++i)
        {
            ++(*value);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

Semaphore threadRecyclingSemaphore;
std::thread::id recycledThreadId;
class ThreadRecorderTask : public Task
{
public:
    void run() override
    {
        recycledThreadId = std::this_thread::get_id();
        threadRecyclingSemaphore.release();
    }
};


class ExpiryTimeoutTask : public Task
{
public:
    ThreadPool::Thread::SharedPtr thread;
    std::atomic<int> runCount{0};
    Semaphore semaphore;

    ExpiryTimeoutTask() { }

    void run() override
    {
        thread = ThreadPool::Thread::current();
        ++runCount;
        semaphore.release();
    }
};


#if CXXKIT_HAS_EXCEPTIONS
class ExceptionTask : public Task
{
public:
    std::string exceptionWhat;
    void run() override { throw new int; }
};
#endif // CXXKIT_HAS_EXCEPTIONS

std::atomic<int> count;
class CountingTask : public Task
{
public:
    void run() { ++count; }
};
} // namespace

TEST(ThreadPoolTest, RunFunction)
{
    {
        ThreadPool manager;
        testFunctionCount = 0;
        manager.start(noSleepTestFunction);
    }
    EXPECT_EQ(testFunctionCount, 1);
}

TEST(ThreadPoolTest, RunFunctionLambda)
{
    int localCount = 0;
    {
        ThreadPool manager;
        manager.start([&]() { ++localCount; });
    }
    EXPECT_EQ(localCount, 1);
}

TEST(ThreadPoolTest, CreateThreadRunFunction)
{
    {
        ThreadPool manager;
        testFunctionCount = 0;
        manager.start(noSleepTestFunction);
    }
    EXPECT_EQ(testFunctionCount, 1);
}

TEST(ThreadPoolTest, RunMultiple)
{
    const int runs = 10;
    std::mutex mutex;
    functionTestMutex = &mutex;

    {
        ThreadPool manager;
        testFunctionCount = 0;
        for (int i = 0; i < runs; ++i)
        {
            manager.start(sleepTestFunctionMutex);
        }
    }
    EXPECT_EQ(testFunctionCount, runs);

    for (int j = 0; j < 100; ++j)
    {
        {
            ThreadPool manager;
            testFunctionCount = 0;
            for (int i = 0; i < runs; ++i)
            {
                manager.start(noSleepTestFunctionMutex);
            }
        }
        EXPECT_EQ(testFunctionCount, runs);
    }

    {
        ThreadPool manager;
        for (int i = 0; i < 500; ++i)
        {
            manager.start(emptyFunct);
        }
    }
}

TEST(ThreadPoolTest, WaitComplete)
{
    testFunctionCount = 0;
    const int runs = 500;
    for (int i = 0; i < 500; ++i)
    {
        ThreadPool pool;
        pool.start(noSleepTestFunction);
    }
    EXPECT_EQ(testFunctionCount, runs);
}

TEST(ThreadPoolTest, RunTask)
{
    ThreadPool manager;
    ran.store(false);
    manager.start(Task::make_shared<TestTask>());
    manager.wait_for_done();
    EXPECT_TRUE(ran.load());
}

TEST(ThreadPoolTest, Singleton)
{
    ran.store(false);
    ThreadPool::instance().start(TestTask::make_shared<TestTask>());
    ThreadPool::instance().wait_for_done();
    EXPECT_TRUE(ran.load());
}

TEST(ThreadPoolTest, Destruction)
{
    value = new std::atomic<int>();
    auto threadPool = new ThreadPool();
    threadPool->start(IntAccessor::make_shared<IntAccessor>());
    threadPool->start(IntAccessor::make_shared<IntAccessor>());
    delete threadPool;
    delete value;
    value = nullptr;
}

TEST(ThreadPoolTest, ThreadRecycling)
{
    ThreadPool threadPool;

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    auto thread1 = recycledThreadId;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    auto thread2 = recycledThreadId;
    EXPECT_EQ(thread1, thread2);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    auto thread3 = recycledThreadId;
    EXPECT_EQ(thread2, thread3);
}

TEST(ThreadPoolTest, ExpiryTimeout)
{
    ExpiryTimeoutTask task;

    ThreadPool threadPool;
    threadPool.set_max_thread_count(1);

    int expiry_timeout = threadPool.expiry_timeout();
    threadPool.set_expiry_timeout(1000);
    EXPECT_EQ(threadPool.expiry_timeout(), 1000);

    // run the task
    threadPool.start(&task, false);
    EXPECT_TRUE(task.semaphore.try_acquire(1, 10000));
    EXPECT_EQ(task.runCount.load(), 1);
    EXPECT_TRUE(!task.thread->wait(100));
    // thread should expire
    auto firstThread = task.thread;
    EXPECT_TRUE(task.thread->wait(10000));

    // run task again, thread should be restarted
    threadPool.start(&task, false);
    EXPECT_TRUE(task.semaphore.try_acquire(1, 10000));
    EXPECT_EQ(task.runCount.load(), 2);
    EXPECT_TRUE(!task.thread->wait(100));
    // thread should expire again
    EXPECT_TRUE(task.thread->wait(10000));

    // thread pool should have reused the expired thread (instead of starting a new one)
    EXPECT_EQ(firstThread, task.thread);

    threadPool.set_expiry_timeout(expiry_timeout);
    EXPECT_EQ(threadPool.expiry_timeout(), expiry_timeout);
}

TEST(ThreadPoolTest, ExpiryTimeoutRace)
{
#ifdef CXXKIT_OS_WIN
    CXXKIT_WARNING("This test is unstable on Windows.");
    return;
#endif
    ExpiryTimeoutTask task;

    ThreadPool threadPool;
    threadPool.set_max_thread_count(1);
    threadPool.set_expiry_timeout(50);
    const int numTasks = 20;
    for (int i = 0; i < numTasks; ++i)
    {
        threadPool.start(&task, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // exactly the same as the expiry timeout
    }
    EXPECT_TRUE(task.semaphore.try_acquire(numTasks, 10000));
    EXPECT_EQ(task.runCount.load(), numTasks);
    EXPECT_TRUE(threadPool.wait_for_done(2000));
}

#if CXXKIT_HAS_EXCEPTIONS
TEST(ThreadPoolTest, Exceptions)
{
    ExceptionTask task;
    {
        ThreadPool threadPool;
        //  Uncomment this for a nice crash.
        // threadPool.start(&task, false);
    }
}
#endif // CXXKIT_HAS_EXCEPTIONS

TEST(ThreadPoolTest, SetMaxThreadCount)
{
    std::vector<int> counts = {1, -1, 2, -2, 4, -4, 0, 12345, -6789, 42, -666};
    for (auto limit : counts)
    {
        ThreadPool &threadPool = ThreadPool::instance();
        int savedLimit = threadPool.max_thread_count();

        // max_thread_count() should always return the previous argument to set_max_thread_count(), regardless of input
        threadPool.set_max_thread_count(limit);
        EXPECT_EQ(threadPool.max_thread_count(), limit);

        // the value returned from max_thread_count() should always be valid input for set_max_thread_count()
        threadPool.set_max_thread_count(savedLimit);
        EXPECT_EQ(threadPool.max_thread_count(), savedLimit);

        // setting the limit on children should have no effect on the parent
        {
            ThreadPool threadPool2;
            savedLimit = threadPool2.max_thread_count();

            // max_thread_count() should always return the previous argument to set_max_thread_count(), regardless of input
            threadPool2.set_max_thread_count(limit);
            EXPECT_EQ(threadPool2.max_thread_count(), limit);

            // the value returned from max_thread_count() should always be valid input for set_max_thread_count()
            threadPool2.set_max_thread_count(savedLimit);
            EXPECT_EQ(threadPool2.max_thread_count(), savedLimit);
        }
    }
}

TEST(ThreadPoolTest, SetMaxThreadCountStartsAndStopsThreads)
{
    class WaitingTask : public Task
    {
    public:
        Semaphore waitForStarted, waitToFinish;

        WaitingTask() { }

        void run()
        {
            waitForStarted.release();
            waitToFinish.acquire();
        }
    };

    ThreadPool threadPool;
    threadPool.set_max_thread_count(1);

    WaitingTask *task = new WaitingTask;
    threadPool.start(task, false);
    EXPECT_TRUE(task->waitForStarted.try_acquire(1, 1000));

    // thread limit is 1, cannot start more tasks
    threadPool.start(task, false);
    EXPECT_TRUE(!task->waitForStarted.try_acquire(1, 1000));

    // increasing the limit by 1 should start the task immediately
    threadPool.set_max_thread_count(2);
    EXPECT_TRUE(task->waitForStarted.try_acquire(1, 1000));

    // ... but we still cannot start more tasks
    threadPool.start(task, false);
    EXPECT_TRUE(!task->waitForStarted.try_acquire(1, 1000));

    // increasing the limit should be able to start more than one at a time
    threadPool.start(task, false);
    threadPool.set_max_thread_count(4);
    EXPECT_TRUE(task->waitForStarted.try_acquire(2, 1000));

    // ... but we still cannot start more tasks
    threadPool.start(task, false);
    threadPool.start(task, false);
    EXPECT_TRUE(!task->waitForStarted.try_acquire(2, 1000));

    // decreasing the thread limit should cause the active thread count to go down
    threadPool.set_max_thread_count(2);
    EXPECT_EQ(threadPool.active_thread_count(), 4);
    task->waitToFinish.release(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(threadPool.active_thread_count(), 2);

    // ... and we still cannot start more tasks
    threadPool.start(task, false);
    threadPool.start(task, false);
    EXPECT_TRUE(!task->waitForStarted.try_acquire(2, 1000));

    // start all remaining tasks
    threadPool.start(task, false);
    threadPool.start(task, false);
    threadPool.start(task, false);
    threadPool.start(task, false);
    threadPool.set_max_thread_count(8);
    EXPECT_TRUE(task->waitForStarted.try_acquire(6, 1000));

    task->waitToFinish.release(10);
    threadPool.wait_for_done();
    delete task;
}

TEST(ThreadPoolTest, ReserveThread)
{
    std::vector<int> counts = {1, -1, 2, -2, 4, -4, 0, 12345, -6789, 42, -666};
    for (auto limit : counts)
    {
        ThreadPool &threadpool = ThreadPool::instance();
        int savedLimit = threadpool.max_thread_count();
        threadpool.set_max_thread_count(limit);

        // reserve up to the limit
        for (int i = 0; i < limit; ++i)
        {
            threadpool.reserve_thread();
        }

        // reserve_thread() should always reserve a thread, regardless of
        // how many have been previously reserved
        threadpool.reserve_thread();
        EXPECT_EQ(threadpool.active_thread_count(), (limit > 0 ? limit : 0) + 1);
        threadpool.reserve_thread();
        EXPECT_EQ(threadpool.active_thread_count(), (limit > 0 ? limit : 0) + 2);

        // cleanup
        threadpool.release_thread();
        threadpool.release_thread();
        for (int i = 0; i < limit; ++i)
        {
            threadpool.release_thread();
        }

        // reserving threads in children should not effect the parent
        {
            ThreadPool threadpool2;
            threadpool2.set_max_thread_count(limit);

            // reserve up to the limit
            for (int i = 0; i < limit; ++i)
            {
                threadpool2.reserve_thread();
            }

            // reserve_thread() should always reserve a thread, regardless
            // of how many have been previously reserved
            threadpool2.reserve_thread();
            EXPECT_EQ(threadpool2.active_thread_count(), (limit > 0 ? limit : 0) + 1);
            threadpool2.reserve_thread();
            EXPECT_EQ(threadpool2.active_thread_count(), (limit > 0 ? limit : 0) + 2);

            threadpool.reserve_thread();
            EXPECT_EQ(threadpool.active_thread_count(), 1);
            threadpool.reserve_thread();
            EXPECT_EQ(threadpool.active_thread_count(), 2);

            // cleanup
            threadpool2.release_thread();
            threadpool2.release_thread();
            threadpool.release_thread();
            threadpool.release_thread();
            while (threadpool2.active_thread_count() > 0)
            {
                threadpool2.release_thread();
            }
        }

        // reset limit on global ThreadPool
        threadpool.set_max_thread_count(savedLimit);
    }
}

TEST(ThreadPoolTest, ReleaseThread)
{
    std::vector<int> counts = {1, -1, 2, -2, 4, -4, 0, 12345, -6789, 42, -666};
    for (auto limit : counts)
    {
        ThreadPool &threadpool = ThreadPool::instance();
        int savedLimit = threadpool.max_thread_count();
        threadpool.set_max_thread_count(limit);

        // reserve up to the limit
        for (int i = 0; i < limit; ++i)
        {
            threadpool.reserve_thread();
        }

        // release should decrease the number of reserved threads
        int reserved = threadpool.active_thread_count();
        while (reserved-- > 0)
        {
            threadpool.release_thread();
            EXPECT_EQ(threadpool.active_thread_count(), reserved);
        }
        EXPECT_EQ(threadpool.active_thread_count(), 0);

        // release_thread() can release more than have been reserved
        threadpool.release_thread();
        EXPECT_EQ(threadpool.active_thread_count(), -1);
        threadpool.reserve_thread();
        EXPECT_EQ(threadpool.active_thread_count(), 0);

        // releasing threads in children should not effect the parent
        {
            ThreadPool threadpool2;
            threadpool2.set_max_thread_count(limit);

            // reserve up to the limit
            for (int i = 0; i < limit; ++i)
            {
                threadpool2.reserve_thread();
            }

            // release should decrease the number of reserved threads
            int reserved = threadpool2.active_thread_count();
            while (reserved-- > 0)
            {
                threadpool2.release_thread();
                EXPECT_EQ(threadpool2.active_thread_count(), reserved);
                EXPECT_EQ(threadpool.active_thread_count(), 0);
            }
            EXPECT_EQ(threadpool2.active_thread_count(), 0);
            EXPECT_EQ(threadpool.active_thread_count(), 0);

            // release_thread() can release more than have been reserved
            threadpool2.release_thread();
            EXPECT_EQ(threadpool2.active_thread_count(), -1);
            EXPECT_EQ(threadpool.active_thread_count(), 0);
            threadpool2.reserve_thread();
            EXPECT_EQ(threadpool2.active_thread_count(), 0);
            EXPECT_EQ(threadpool.active_thread_count(), 0);
        }

        // reset limit on global ThreadPool
        threadpool.set_max_thread_count(savedLimit);
    }
}

TEST(ThreadPoolTest, ReserveAndStart)
{
    class WaitingTask : public Task
    {
    public:
        std::atomic<int> count{0};
        Semaphore waitForStarted;
        Semaphore waitBeforeDone;

        WaitingTask() { }

        void run()
        {
            ++count;
            waitForStarted.release();
            waitBeforeDone.acquire();
        }
    };

    // set up
    ThreadPool &threadpool = ThreadPool::instance();
    int savedLimit = threadpool.max_thread_count();
    threadpool.set_max_thread_count(1);
    EXPECT_EQ(threadpool.active_thread_count(), 0);

    // reserve
    threadpool.reserve_thread();
    EXPECT_EQ(threadpool.active_thread_count(), 1);

    // start a task, to get a running thread
    WaitingTask *task = new WaitingTask;
    threadpool.start(task, false);
    EXPECT_EQ(threadpool.active_thread_count(), 2);
    task->waitForStarted.acquire();
    task->waitBeforeDone.release();
    EXPECT_EQ(task->count.load(), 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(threadpool.active_thread_count(), 1);

    // now the thread is waiting, but try_start() will fail since active_thread_count() >= max_thread_count()
    EXPECT_TRUE(!threadpool.try_start_now(task, false));
    EXPECT_EQ(threadpool.active_thread_count(), 1);

    // start() will therefore do a failing try_start(), followed by enqueue_task()
    // which will actually wake up the waiting thread.
    threadpool.start(task, false);
    EXPECT_EQ(threadpool.active_thread_count(), 2);
    task->waitForStarted.acquire();
    task->waitBeforeDone.release();
    EXPECT_EQ(task->count.load(), 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(threadpool.active_thread_count(), 1);

    threadpool.release_thread();
    EXPECT_EQ(threadpool.active_thread_count(), 0);

    delete task;

    threadpool.set_max_thread_count(savedLimit);
}

TEST(ThreadPoolTest, Start)
{
    const int runs = 1000;
    count.store(0);
    {
        ThreadPool threadPool;
        for (int i = 0; i < runs; ++i)
        {
            threadPool.start(new CountingTask());
        }
    }
    EXPECT_EQ(count.load(), runs);
}

TEST(ThreadPoolTest, TryStart)
{
    class WaitingTask : public Task
    {
    public:
        Semaphore semaphore;

        WaitingTask() { }

        void run()
        {
            semaphore.acquire();
            ++count;
        }
    };

    count.store(0);

    WaitingTask task;
    ThreadPool threadPool;
    for (int i = 0; i < threadPool.max_thread_count(); ++i)
    {
        threadPool.start(&task, false);
    }
    EXPECT_TRUE(!threadPool.try_start_now(&task, false));
    task.semaphore.release(threadPool.max_thread_count());
    threadPool.wait_for_done();
    EXPECT_EQ(count.load(), threadPool.max_thread_count());
}

namespace
{
std::mutex mutex;
std::atomic<int> activeThreads;
std::atomic<int> peakActiveThreads;
} // namespace
TEST(ThreadPoolTest, TryStartPeakThreadCount)
{
    class CounterTask : public Task
    {
    public:
        CounterTask() { }

        void run()
        {
            {
                std::unique_lock<std::mutex> lock(mutex);
                ++activeThreads;
                peakActiveThreads.store(std::max(peakActiveThreads.load(), activeThreads.load()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            {
                std::unique_lock<std::mutex> lock(mutex);
                --activeThreads;
            }
        }
    };

    CounterTask task;
    ThreadPool threadPool;

    for (int i = 0; i < 20; ++i)
    {
        if (threadPool.try_start_now(&task, false) == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    EXPECT_EQ(peakActiveThreads.load(), ThreadPool::ideal_thread_count());

    for (int i = 0; i < 20; ++i)
    {
        if (threadPool.try_start_now(&task, false) == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    EXPECT_EQ(peakActiveThreads.load(), ThreadPool::ideal_thread_count());
}

TEST(ThreadPoolTest, TryStartCount)
{
    class SleeperTask : public Task
    {
    public:
        std::atomic<int> doneCount{0};
        SleeperTask() { }

        void run()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++doneCount;
        }
    };

    SleeperTask task;
    const int runs = 1;
    ThreadPool threadPool;

    for (int i = 0; i < runs; ++i)
    {
        int count = 0;
        task.doneCount = 0;
        while (threadPool.try_start_now(&task, false))
        {
            ++count;
        }
        EXPECT_EQ(count, ThreadPool::ideal_thread_count());
        while (task.doneCount.load() != count)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        EXPECT_EQ(threadPool.active_thread_count(), 0);
    }
}

TEST(ThreadPoolTest, PriorityStart)
{
    std::vector<int> priorities = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    //    std::vector<int> priorities = {2};
    for (auto otherCount : priorities)
    {
        class Holder : public Task
        {
        public:
            Semaphore &sem;
            Holder(Semaphore &sem)
                : sem(sem)
            {
            }
            void run() { sem.acquire(); }
        };
        class Runner : public Task
        {
        public:
            std::atomic<Task *> &ptr;
            Runner(std::atomic<Task *> &p)
                : ptr(p)
            {
            }
            void run()
            {
                //                 CXXKIT_DEBUG("run %p", this);
                Task *expected = nullptr;
                ptr.compare_exchange_strong(expected, this);
            }
        };

        Semaphore sem;
        std::atomic<Task *> firstStarted{nullptr};
        Task *expected{nullptr};
        ThreadPool threadPool;
        threadPool.set_max_thread_count(1); // start only one thread at a time

        // queue the holder first
        // We need to be sure that all threads are active when we queue the two Runners
        threadPool.start(new Holder(sem));
        while (otherCount--)
        {
            auto task = new Runner(firstStarted);
            //             CXXKIT_DEBUG("Runner %p", task);
            threadPool.start(task, true, ThreadPool::Priority::kNormal); // priority kNormal
        }
        threadPool.start(expected = new Runner(firstStarted),
                         true,
                         ThreadPool::Priority::kHighest); // priority kHighest, expected
        threadPool.start(new Runner(firstStarted),
                         true,
                         ThreadPool::Priority::kHighest); // priority kHighest
                                                          //         CXXKIT_DEBUG("expected %p", expected);

        sem.release();
        EXPECT_TRUE(threadPool.wait_for_done());
        EXPECT_EQ(firstStarted.load(), expected);
    }
}

TEST(ThreadPoolTest, WaitForDone)
{
    ElapsedTimer total, pass;
    total.start();

    ThreadPool threadPool;
    while (total.elapsed() < 10000)
    {
        int runs;
        count.store(runs = 0);
        pass.restart();
        while (pass.elapsed() < 100)
        {
            threadPool.start(new CountingTask());
            ++runs;
        }
        threadPool.wait_for_done();
        EXPECT_EQ(count.load(), runs);

        count.store(runs = 0);
        pass.restart();
        while (pass.elapsed() < 100)
        {
            threadPool.start(new CountingTask());
            threadPool.start(new CountingTask());
            runs += 2;
        }
        threadPool.wait_for_done();
        EXPECT_EQ(count.load(), runs);
    }
}

TEST(ThreadPoolTest, WaitForDoneTimeout)
{
    std::mutex mutex;
    class BlockedTask : public Task
    {
    public:
        std::mutex &mutex;
        explicit BlockedTask(std::mutex &m)
            : mutex(m)
        {
        }

        void run()
        {
            mutex.lock();
            mutex.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    ThreadPool threadPool;

    mutex.lock();
    threadPool.start(new BlockedTask(mutex));
    EXPECT_TRUE(!threadPool.wait_for_done(100));
    mutex.unlock();
    EXPECT_TRUE(threadPool.wait_for_done(400));
}

TEST(ThreadPoolTest, clear)
{
    Semaphore sem(0);
    class BlockingTask : public Task
    {
    public:
        Semaphore &sem;
        BlockingTask(Semaphore &sem)
            : sem(sem)
        {
        }
        void run()
        {
            sem.acquire();
            ++count;
        }
    };

    ThreadPool threadPool;
    threadPool.set_max_thread_count(10);
    int runs = 2 * threadPool.max_thread_count();
    count.store(0);
    for (int i = 0; i <= runs; i++)
    {
        threadPool.start(new BlockingTask(sem));
    }
    threadPool.clear();
    sem.release(threadPool.max_thread_count());
    threadPool.wait_for_done();
    EXPECT_EQ(count.load(), threadPool.max_thread_count());
}

TEST(ThreadPoolTest, Cancel)
{
    Semaphore sem(0);
    Semaphore startedThreads(0);

    class BlockingTask : public Task
    {
    public:
        Semaphore &sem;
        Semaphore &startedThreads;
        std::atomic<int> &dtorCounter;
        std::atomic<int> &runCounter;
        int dummy;

        explicit BlockingTask(Semaphore &s, Semaphore &started, std::atomic<int> &c, std::atomic<int> &r)
            : sem(s)
            , startedThreads(started)
            , dtorCounter(c)
            , runCounter(r)
        {
        }

        ~BlockingTask() { dtorCounter.fetch_add(1); }

        void run()
        {
            startedThreads.release();
            runCounter.fetch_add(1);
            sem.acquire();
            ++count;
        }
    };

    enum
    {
        MaxThreadCount = 3,
        OverProvisioning = 2,
        runs = MaxThreadCount * OverProvisioning
    };

    ThreadPool threadPool;
    threadPool.set_max_thread_count(MaxThreadCount);
    BlockingTask *tasks[runs];

    count.store(0);
    std::atomic<int> dtorCounter{0};
    std::atomic<int> runCounter{0};
    for (int i = 0; i < runs; i++)
    {
        tasks[i] = new BlockingTask(sem, startedThreads, dtorCounter, runCounter);
        threadPool.cancel(tasks[i]);                       //verify NOOP for jobs not in the queue
        const bool autoDelete = i != 0 && i != (runs - 1); // one which will run and one which will not
        threadPool.start(tasks[i], autoDelete);
    }
    // wait for all worker threads to have started up:
    EXPECT_TRUE(startedThreads.try_acquire(MaxThreadCount, 60 * 1000 /* 1min */));

    for (int i = 0; i < runs; i++)
    {
        threadPool.cancel(tasks[i]);
    }
    tasks[0]->dummy = 0; //valgrind will catch this if cancel() is crazy enough to delete currently running jobs
    tasks[runs - 1]->dummy = 0;
    EXPECT_EQ(dtorCounter.load(), runs - threadPool.max_thread_count() - 1);
    sem.release(threadPool.max_thread_count());
    threadPool.wait_for_done();
    EXPECT_EQ(runCounter.load(), threadPool.max_thread_count());
    EXPECT_EQ(count.load(), threadPool.max_thread_count());
    EXPECT_EQ(dtorCounter.load(), runs - 2);
    delete tasks[0]; //if the pool deletes them then we'll get double-free crash
    delete tasks[runs - 1];
    sem.release(runs);
}

TEST(ThreadPoolTest, DestroyingWaitsForTasksToFinish)
{
    ElapsedTimer total, pass;
    total.start();

    while (total.elapsed() < 10000)
    {
        int runs;
        count.store(runs = 0);
        {
            ThreadPool threadPool;
            pass.restart();
            while (pass.elapsed() < 100)
            {
                threadPool.start(new CountingTask());
                ++runs;
            }
        }
        EXPECT_EQ(count.load(), runs);

        count.store(runs = 0);
        {
            ThreadPool threadPool;
            pass.restart();
            while (pass.elapsed() < 100)
            {
                threadPool.start(new CountingTask());
                threadPool.start(new CountingTask());
                runs += 2;
            }
        }
        EXPECT_EQ(count.load(), runs);
    }
}

TEST(ThreadPoolTest, StressTest)
{
    std::atomic<int> debugFlag{0};
    static std::atomic<int> ctorCount{0};
    static std::atomic<int> dtorCount{0};
    static std::atomic<int> waitCount{0};
    static std::atomic<int> runCount{0};
    class StressTestTask : public Task
    {
        Semaphore semaphore;

    public:
        StressTestTask() { ctorCount.fetch_add(1); }
        ~StressTestTask() override { dtorCount.fetch_add(1); }

        void start() { ThreadPool::instance().start(this, false); }

        void wait()
        {
            semaphore.acquire();
            waitCount.fetch_add(1);
        }

        void run() override
        {
            semaphore.release();
            runCount.fetch_add(1);
        }
    };

    ElapsedTimer total;
    total.start();
    int runs = 0;
    ThreadPool &threadPool = ThreadPool::instance();
    std::atomic<bool> statThreadRunning(true);
    auto statThread = std::thread(
        [&statThreadRunning, &threadPool, &runs, &total, &debugFlag]()
        {
            while (statThreadRunning.load())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#if 0
                CXXKIT_DEBUG("\n\truns:%d, elapsed:%d, debugFlag:%d, "
                           "task_count:%d, tasks_completed_count:%d, tasks_dispatched_count:%d, "
                           "ctorCount:%d, dtorCount:%d, waitCount:%d, runCount:%d",
                           runs,
                           total.elapsed(),
                           debugFlag.load(),
                           threadPool.task_count(),
                           threadPool.tasks_completed_count(),
                           threadPool.tasks_dispatched_count(),
                           ctorCount.load(),
                           dtorCount.load(),
                           waitCount.load(),
                           runCount.load());
#endif
            }
        });
    while (total.elapsed() < 30000)
    {
        debugFlag.store(0);
        StressTestTask t;
        debugFlag.store(1);
        t.start();
        debugFlag.store(2);
        t.wait();
        debugFlag.store(3);
        runs++;
    }
    CXXKIT_DEBUG("elapsed:%d, runs:%d", total.elapsed(), runs);
    statThreadRunning.store(false);
    statThread.join();
}

TEST(ThreadPoolTest, CancelAllAndIncreaseMaxThreadCount)
{
    class CancelTask : public Task
    {
    public:
        CancelTask(Semaphore *mainBarrier, Semaphore *threadBarrier)
            : m_mainBarrier(mainBarrier)
            , m_threadBarrier(threadBarrier)
        {
        }

        void run() override
        {
            m_mainBarrier->release();
            m_threadBarrier->acquire();
        }

    private:
        Semaphore *m_mainBarrier;
        Semaphore *m_threadBarrier;
    };

    Semaphore mainBarrier;
    Semaphore taskBarrier;

    ThreadPool threadPool;
    threadPool.set_max_thread_count(1);

    CancelTask *task1 = new CancelTask(&mainBarrier, &taskBarrier);
    CancelTask *task2 = new CancelTask(&mainBarrier, &taskBarrier);
    CancelTask *task3 = new CancelTask(&mainBarrier, &taskBarrier);

    threadPool.start(task1, false);
    threadPool.start(task2, false);
    threadPool.start(task3, false);

    mainBarrier.acquire();

    EXPECT_EQ(threadPool.active_thread_count(), 1);

    EXPECT_TRUE(!threadPool.cancel(task1));
    EXPECT_TRUE(threadPool.cancel(task2));
    EXPECT_TRUE(threadPool.cancel(task3));

    // A bad queue implementation can segfault here because two consecutive items in the queue have been taken
    threadPool.set_max_thread_count(4);

    // Even though we increase the max thread count, there should only be one job to run
    EXPECT_EQ(threadPool.active_thread_count(), 1);

    // Make sure jobs 2 and 3 never started
    EXPECT_EQ(mainBarrier.available(), 0);

    taskBarrier.release(1);

    threadPool.wait_for_done();

    EXPECT_EQ(threadPool.active_thread_count(), 0);

    delete task1;
    delete task2;
    delete task3;
}

namespace
{
typedef void (*FunctionPointer)();
class FunctionPointerTask : public Task
{
public:
    FunctionPointerTask(FunctionPointer function)
        : function(function)
    {
    }
    void run() { function(); }

private:
    FunctionPointer function;
};
Task *createTask(FunctionPointer pointer)
{
    return new FunctionPointerTask(pointer);
}
} // namespace
TEST(ThreadPoolTest, WaitForDoneAfterCancel)
{
    class CancelTask : public Task
    {
    public:
        CancelTask(Semaphore *mainBarrier, Semaphore *threadBarrier)
            : m_mainBarrier(mainBarrier)
            , m_threadBarrier(threadBarrier)
        {
        }

        void run()
        {
            m_mainBarrier->release();
            m_threadBarrier->acquire();
        }

    private:
        Semaphore *m_mainBarrier = nullptr;
        Semaphore *m_threadBarrier = nullptr;
    };

    int threadCount = 4;

    // Blocks the main thread from releasing the threadBarrier before all run() functions have started
    Semaphore mainBarrier;
    // Blocks the tasks from completing their run function
    Semaphore threadBarrier;

    ThreadPool manager;
    manager.set_max_thread_count(threadCount);

    // Fill all the threads with tasks that wait for the threadBarrier
    for (int i = 0; i < threadCount; i++)
    {
        auto *task = new CancelTask(&mainBarrier, &threadBarrier);
        manager.start(task);
    }

    EXPECT_TRUE(manager.active_thread_count() == manager.max_thread_count());

    // add tasks that are immediately removed from the pool queue.
    // This sets the queue elements to nullptr in ThreadPool and we want to test that
    // the threads keep going through the queue after encountering a nullptr.
    for (int i = 0; i < threadCount; i++)
    {
        auto *runnable = createTask(emptyFunct);
        manager.start(runnable, false);
        EXPECT_TRUE(manager.cancel(runnable));
        delete runnable;
    }

    // add another runnable that will not be removed
    manager.start(createTask(emptyFunct));

    // Wait for the first tasks to start
    mainBarrier.acquire(threadCount);

    EXPECT_TRUE(mainBarrier.available() == 0);
    EXPECT_TRUE(threadBarrier.available() == 0);

    // release tasks that are waiting and expect all tasks to complete
    threadBarrier.release(threadCount);

    if (!manager.wait_for_done(5 * 60 * 1000))
    {
        EXPECT_TRUE(false) << "wait_for_done returned false. Aborting to stop background threads.";
    }
}

CXXKIT_END_NAMESPACE
