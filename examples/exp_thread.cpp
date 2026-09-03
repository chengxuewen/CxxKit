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

// exp_thread: concurrency walkthrough — ThreadPool / TaskQueue / Barrier.

#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#include <cxxkit/thread/barrier.hpp>
#include <cxxkit/thread/pending_task_safety_flag.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/thread/task_queue_thread.hpp>
#include <cxxkit/thread/thread_pool.hpp>

using namespace cxxkit;

namespace
{
std::string to_signed_string(int value)
{
    return std::to_string(value);
}
} // namespace

int main()
{
    // --- 1. ThreadPool: submit N work items, wait_for_done, report after join. ---
    // Workers only touch shared containers; all printing happens after wait_for_done,
    // so the main thread is the sole stdout writer (deterministic output).
    {
        std::cout << "\n--- 1. ThreadPool: 6 work items on default_instance ---" << std::endl;
        ThreadPool *pool = ThreadPool::default_instance();
        std::vector<std::string> results(6);
        std::atomic<int> sum(0);
        for (int i = 0; i < 6; ++i)
        {
            pool->start(
                [i, &results, &sum]()
                {
                    sum.fetch_add(i * i);
                    results[i] = "work " + to_signed_string(i) + " -> " + to_signed_string(i * i);
                },
                ThreadPool::Priority::kNormal);
        }
        pool->wait_for_done();
        std::cout << "  active_thread_count=" << pool->active_thread_count()
                  << " tasks_completed_count=" << pool->tasks_completed_count() << std::endl;
        for (size_t i = 0; i < results.size(); ++i)
        {
            std::cout << "  " << results[i] << std::endl;
        }
        std::cout << "  sum of squares=" << sum.load() << std::endl;
    }

    // --- 2. TaskQueueThread: FIFO serialization on one worker thread. ---
    {
        std::cout << "\n--- 2. TaskQueueThread: FIFO serialization ---" << std::endl;
        TaskQueueThread::SharedPtr queue = TaskQueueThread::make_shared();
        std::vector<std::string> order(4);
        for (int i = 0; i < 4; ++i)
        {
            queue->post_task([&order, i]() { order[i] = "task " + to_signed_string(i) + " ran on the queue thread"; });
        }
        Semaphore done(0);
        queue->post_task([&done]() { done.release(); }); // sentinel: runs after the 4 tasks
        done.acquire();                                  // FIFO => tasks above have run before we read
        queue.reset();                                   // join worker thread
        std::cout << "  order confirmed on queue thread:" << std::endl;
        for (size_t i = 0; i < order.size(); ++i)
        {
            std::cout << "    " << order[i] << std::endl;
        }
    }

    // --- 3. safe_task: callback dropped once the safety flag is dead. ---
    {
        std::cout << "\n--- 3. safe_task: PendingTaskSafetyFlag guards a stale callback ---" << std::endl;
        std::shared_ptr<PendingTaskSafetyFlag> flag = PendingTaskSafetyFlag::create();
        bool ran = false;
        std::function<void()> guarded = safe_task(flag, [&ran]() { ran = true; });
        flag->set_not_alive(); // owner "destroyed" before the callback is dispatched
        guarded();
        std::cout << "  callback ran after set_not_alive? " << (ran ? "yes" : "no (guarded by safety flag)")
                  << std::endl;
    }

    // --- 4. Barrier: 3 threads rendezvous, then collect after join. ---
    {
        std::cout << "\n--- 4. Barrier: 3 threads rendezvous ---" << std::endl;
        const size_t kThreadCount = 3;
        Barrier barrier(kThreadCount);
        std::vector<std::string> arrivals(kThreadCount);
        std::vector<std::thread> threads;
        std::atomic<int> arrived(0);
        for (size_t i = 0; i < kThreadCount; ++i)
        {
            threads.push_back(std::thread(
                [i, &barrier, &arrivals, &arrived]()
                {
                    arrived.fetch_add(1);
                    barrier.arrive_and_wait(); // blocks until all 3 arrived
                    arrivals[i] = "thread " + to_signed_string(static_cast<int>(i)) + " passed the barrier";
                }));
        }
        for (size_t i = 0; i < kThreadCount; ++i)
        {
            threads[i].join();
        }
        std::cout << "  arrived=" << arrived.load() << "/" << kThreadCount << ", all joined:" << std::endl;
        for (size_t i = 0; i < arrivals.size(); ++i)
        {
            std::cout << "    " << arrivals[i] << std::endl;
        }
    }

    std::cout << "\n--- done ---" << std::endl;
    return 0;
}
