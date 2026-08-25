#include <cxxkit/thread/pending_task_safety_flag.hpp>

#include <gtest/gtest.h>

#include <functional>
#include <thread>

TEST(PendingTaskSafetyFlag, AliveAfterCreate)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::Create();
    EXPECT_TRUE(flag->alive());
}

TEST(PendingTaskSafetyFlag, NotAliveAfterSetNotAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::Create();
    EXPECT_TRUE(flag->alive());
    flag->SetNotAlive();
    EXPECT_FALSE(flag->alive());
}

TEST(PendingTaskSafetyFlag, SafeTaskExecutesWhenAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::Create();
    int value = 0;
    auto task = cxxkit::SafeTask(flag, [&]() { value = 42; });
    task();
    EXPECT_EQ(value, 42);
}

TEST(PendingTaskSafetyFlag, SafeTaskSkipsWhenNotAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::Create();
    int value = 0;
    auto task = cxxkit::SafeTask(flag, [&]() { value = 42; });
    flag->SetNotAlive();
    task();
    EXPECT_EQ(value, 0);
}

TEST(PendingTaskSafetyFlag, SafeTaskSkipsWhenFlagDestroyed)
{
    int value = 0;
    std::function<void()> task;
    {
        auto flag = cxxkit::PendingTaskSafetyFlag::Create();
        task = cxxkit::SafeTask(flag, [&]() { value = 42; });
        flag->SetNotAlive();
    }
    // flag shared_ptr destroyed, but task holds a copy
    task();
    EXPECT_EQ(value, 0);
}

TEST(PendingTaskSafetyFlag, AsyncScenario)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::Create();
    int value = 0;
    auto task = cxxkit::SafeTask(flag, [&]() { value = 42; });

    std::thread t(task);
    t.join();
    EXPECT_EQ(value, 42);
}

TEST(PendingTaskSafetyFlag, AsyncSkipsAfterSetNotAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::Create();
    std::atomic<int> value{0};
    flag->SetNotAlive();
    auto task = cxxkit::SafeTask(flag, [&]() { value.store(42); });

    std::thread t(task);
    t.join();
    EXPECT_EQ(value.load(), 0);
}
