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

#include <cxxkit/thread/pending_task_safety_flag.hpp>

#include <gtest/gtest.h>

#include <functional>
#include <thread>

TEST(PendingTaskSafetyFlag, AliveAfterCreate)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::create();
    EXPECT_TRUE(flag->alive());
}

TEST(PendingTaskSafetyFlag, NotAliveAfterSetNotAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::create();
    EXPECT_TRUE(flag->alive());
    flag->set_not_alive();
    EXPECT_FALSE(flag->alive());
}

TEST(PendingTaskSafetyFlag, SafeTaskExecutesWhenAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::create();
    int value = 0;
    auto task = cxxkit::safe_task(flag, [&]() { value = 42; });
    task();
    EXPECT_EQ(value, 42);
}

TEST(PendingTaskSafetyFlag, SafeTaskSkipsWhenNotAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::create();
    int value = 0;
    auto task = cxxkit::safe_task(flag, [&]() { value = 42; });
    flag->set_not_alive();
    task();
    EXPECT_EQ(value, 0);
}

TEST(PendingTaskSafetyFlag, SafeTaskSkipsWhenFlagDestroyed)
{
    int value = 0;
    std::function<void()> task;
    {
        auto flag = cxxkit::PendingTaskSafetyFlag::create();
        task = cxxkit::safe_task(flag, [&]() { value = 42; });
        flag->set_not_alive();
    }
    // flag shared_ptr destroyed, but task holds a copy
    task();
    EXPECT_EQ(value, 0);
}

TEST(PendingTaskSafetyFlag, AsyncScenario)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::create();
    int value = 0;
    auto task = cxxkit::safe_task(flag, [&]() { value = 42; });

    std::thread t(task);
    t.join();
    EXPECT_EQ(value, 42);
}

TEST(PendingTaskSafetyFlag, AsyncSkipsAfterSetNotAlive)
{
    auto flag = cxxkit::PendingTaskSafetyFlag::create();
    std::atomic<int> value{0};
    flag->set_not_alive();
    auto task = cxxkit::safe_task(flag, [&]() { value.store(42); });

    std::thread t(task);
    t.join();
    EXPECT_EQ(value.load(), 0);
}
