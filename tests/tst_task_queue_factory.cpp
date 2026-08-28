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

// cxxkit::thread TaskQueueFactory tests — coverage for task_queue_factory.cpp.
#include <cxxkit/thread/task_queue_factory.hpp>
#include <cxxkit/thread/task_queue_thread.hpp>

#include <gtest/gtest.h>

#include <thread>

#include <atomic>

using namespace cxxkit;

TEST(TaskQueueFactory, CreateDefaultReturnsFactory)
{
    auto factory = TaskQueueFactory::create_default();
    EXPECT_TRUE(factory != nullptr);
}

TEST(TaskQueueFactory, CreateTaskQueueRunsPostedTask)
{
    auto factory = TaskQueueFactory::create_default();
    ASSERT_TRUE(factory != nullptr);

    auto queue = factory->create_task_queue("factory-test", TaskQueueFactory::Priority::kNormal);
    ASSERT_TRUE(queue != nullptr);

    std::atomic<bool> ran{false};
    queue->post_task([&ran]() { ran.store(true); });
    // Give the worker a moment to pick the task up (task queue is async).
    for (int i = 0; i < 1000 && !ran.load(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(ran.load());
}