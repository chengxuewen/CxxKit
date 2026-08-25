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

    auto worker = [&]() {
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

    auto worker = [&]() {
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

    auto worker = [&](bool drop) {
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
