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

#include <cxxkit/thread/mutex.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

TEST(Mutex, LockUnlock)
{
    cxxkit::Mutex mu;
    mu.lock();
    mu.unlock();
}

TEST(Mutex, MutexLockRAII)
{
    cxxkit::Mutex mu;
    {
        cxxkit::MutexLock lock(mu);
        // lock held
    }
    // lock released
}

TEST(Mutex, ConcurrentIncrement)
{
    cxxkit::Mutex mu;
    int counter = 0;
    auto worker = [&]()
    {
        for (int i = 0; i < 10000; ++i)
        {
            cxxkit::MutexLock lock(mu);
            ++counter;
        }
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back(worker);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    EXPECT_EQ(counter, 40000);
}

TEST(RWLock, MultipleReaders)
{
    cxxkit::RWLock rw;
    int value = 42;
    {
        cxxkit::ReadLock rlock(rw);
        EXPECT_EQ(value, 42);
    }
    {
        cxxkit::WriteLock wlock(rw);
        value = 100;
    }
    {
        cxxkit::ReadLock rlock(rw);
        EXPECT_EQ(value, 100);
    }
}

TEST(RWLock, ConcurrentReadWrite)
{
    cxxkit::RWLock rw;
    int shared_value = 0;
    std::atomic<int> read_sum{0};

    auto writer = [&]()
    {
        for (int i = 0; i < 100; ++i)
        {
            cxxkit::WriteLock wlock(rw);
            shared_value = i;
        }
    };

    auto reader = [&]()
    {
        for (int i = 0; i < 100; ++i)
        {
            cxxkit::ReadLock rlock(rw);
            read_sum.fetch_add(shared_value, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(writer);
    threads.emplace_back(reader);
    threads.emplace_back(reader);
    for (auto &t : threads)
    {
        t.join();
    }
    // read_sum should be > 0 (readers saw some values)
    EXPECT_GT(read_sum.load(), 0);
}

TEST(RecursiveMutex, Reentrant)
{
    cxxkit::RecursiveMutex mu;
    mu.lock();
    mu.lock(); // reentrant
    mu.unlock();
    mu.unlock();
}
