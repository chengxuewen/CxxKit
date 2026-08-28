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

// cxxkit::tools OnceFlag tests — coverage for once_flag.cpp (thread_local accessor + states).
#include <cxxkit/tools/once_flag.hpp>

#include <gtest/gtest.h>

#include <thread>

using namespace cxxkit;

TEST(OnceFlag, LocalAccessorSameThreadStable)
{
    // thread_local singleton: same pointer within a thread.
    EXPECT_EQ(OnceFlag::local_once_flag(), OnceFlag::local_once_flag());
}

TEST(OnceFlag, LocalAccessorPerThread)
{
    void *mainPtr = OnceFlag::local_once_flag();
    void *otherPtr = nullptr;
    std::thread t([&otherPtr]() { otherPtr = OnceFlag::local_once_flag(); });
    t.join();
    EXPECT_NE(mainPtr, otherPtr); // different thread => different thread_local instance
}

TEST(OnceFlag, StateTransitions)
{
    OnceFlag flag;
    EXPECT_TRUE(flag.is_never_called());
    EXPECT_FALSE(flag.is_done());
    EXPECT_FALSE(flag.is_in_process());

    EXPECT_TRUE(flag.enter());
    EXPECT_TRUE(flag.is_in_process());
    EXPECT_FALSE(flag.is_done());

    flag.leave();
    EXPECT_TRUE(flag.is_done());
    EXPECT_FALSE(flag.enter()); // cannot re-enter after done
}

TEST(OnceFlag, CallOnce)
{
    OnceFlag flag;
    int calls = 0;
    auto fn = [&calls]() { ++calls; };
    EXPECT_TRUE(flag.call(fn));
    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(flag.call(fn)); // second call skipped
    EXPECT_EQ(calls, 1);
}