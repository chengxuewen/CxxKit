// cxxkit::thread TaskQueueFactory tests — coverage for task_queue_factory.cpp.
#include <cxxkit/thread/task_queue_factory.hpp>
#include <cxxkit/thread/task_queue_thread.hpp>

#include <gtest/gtest.h>

#include <thread>

#include <atomic>

using namespace cxxkit;

TEST(TaskQueueFactory, CreateDefaultReturnsFactory)
{
    auto factory = TaskQueueFactory::CreateDefault();
    EXPECT_TRUE(factory != nullptr);
}

TEST(TaskQueueFactory, CreateTaskQueueRunsPostedTask)
{
    auto factory = TaskQueueFactory::CreateDefault();
    ASSERT_TRUE(factory != nullptr);

    auto queue = factory->CreateTaskQueue("factory-test", TaskQueueFactory::Priority::kNormal);
    ASSERT_TRUE(queue != nullptr);

    std::atomic<bool> ran{false};
    queue->postTask([&ran]() { ran.store(true); });
    // Give the worker a moment to pick the task up (task queue is async).
    for (int i = 0; i < 1000 && !ran.load(); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(ran.load());
}