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

#include <cxxkit/thread/barrier.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

TEST(Barrier, BasicSync)
{
    const int n = 4;
    cxxkit::Barrier barrier(n);
    std::atomic<int> counter{0};

    auto worker = [&]()
    {
        counter.fetch_add(1, std::memory_order_relaxed);
        barrier.arrive_and_wait();
        // All threads have arrived before any proceeds.
        EXPECT_EQ(counter.load(), n);
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < n; ++i)
    {
        threads.emplace_back(worker);
    }
    for (auto &t : threads)
    {
        t.join();
    }
}

TEST(Barrier, MultiplePhases)
{
    const int n = 3;
    cxxkit::Barrier barrier(n);
    std::atomic<int> phase_count{0};

    auto worker = [&]()
    {
        for (int p = 0; p < 3; ++p)
        {
            barrier.arrive_and_wait();
            phase_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < n; ++i)
    {
        threads.emplace_back(worker);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    // Each phase: n threads pass, 3 phases = 3*n increments total.
    EXPECT_EQ(phase_count.load(), n * 3);
}

TEST(Barrier, ArriveAndDrop)
{
    const int n = 3;
    cxxkit::Barrier barrier(n);
    std::atomic<int> counter{0};

    auto worker = [&](bool drop)
    {
        counter.fetch_add(1, std::memory_order_relaxed);
        if (drop)
        {
            barrier.arrive_and_drop();
        }
        else
        {
            barrier.arrive_and_wait();
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(worker, true);  // thread 1: drop
    threads.emplace_back(worker, false); // thread 2: wait
    threads.emplace_back(worker, false); // thread 3: wait
    for (auto &t : threads)
    {
        t.join();
    }
    EXPECT_EQ(counter.load(), 3);
}
