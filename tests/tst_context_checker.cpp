/***********************************************************************************************************************
**
** Library: cxxkit
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

#include "cxxkit/thread/task_queue_thread.hpp"
#include "cxxkit/thread/context_checker.hpp"
#include "cxxkit/thread/platform_thread.hpp"
#include "cxxkit/functional/function_view.hpp"
#include "cxxkit/time/date_time.hpp"
#include "cxxkit/thread/semaphore.hpp"
#include "cxxkit/tools/checks.hpp"
#include "cxxkit/memory/memory.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <memory>

CXXKIT_BEGIN_NAMESPACE

using testing::HasSubstr;

namespace
{

// This class is dead code, but its purpose is to make sure that
// ContextChecker is compatible with the CXXKIT_ATTRIBUTE_GUARDED_BY and CXXKIT_RUN_ON
// attributes that are checked at compile-time.
class CompileTimeTestForGuardedBy
{
public:
    int CalledOnSequence() CXXKIT_RUN_ON(mContextChecker) { return guarded_; }

    void CallMeFromSequence()
    {
        CXXKIT_DCHECK_RUN_ON(&mContextChecker);
        guarded_ = 41;
    }

private:
    int guarded_ CXXKIT_ATTRIBUTE_GUARDED_BY(mContextChecker);
    ContextChecker mContextChecker;
};

void RunOnDifferentThread(FunctionView<void()> run)
{
    Semaphore thread_has_run;
    auto thread = std::thread(
        [&]
        {
            run();
            thread_has_run.release();
        });

    EXPECT_TRUE(thread_has_run.tryAcquire(1, 1000));
    thread.join();
}
} // namespace

TEST(ContextCheckerTest, CallsAllowedOnSameThread)
{
    ContextChecker contextChecker;
    EXPECT_TRUE(contextChecker.isCurrent());
}

TEST(ContextCheckerTest, DestructorAllowedOnDifferentThread)
{
    auto contextChecker = utils::make_unique<ContextChecker>();
    RunOnDifferentThread(
        [&]
        {
            // Verify that the destructor doesn't assert when called on a different thread.
            contextChecker.reset();
        });
}

TEST(ContextCheckerTest, Detach)
{
    ContextChecker contextChecker;
    contextChecker.detach();
    RunOnDifferentThread([&] { EXPECT_TRUE(contextChecker.isCurrent()); });
}

TEST(ContextCheckerTest, DetachFromThreadAndUseOnTaskQueue)
{
    ContextChecker contextChecker;
    contextChecker.detach();
    auto queue = TaskQueueThread::makeShared();
    queue->postTask([&] { EXPECT_TRUE(contextChecker.isCurrent()); });
}

TEST(ContextCheckerTest, InitializeForDifferentTaskQueue)
{
    auto queue = TaskQueueThread::makeShared();
    ContextChecker contextChecker(queue.get());
    EXPECT_EQ(contextChecker.isCurrent(), !CXXKIT_DCHECK_IS_ON);
    queue->postTask([&] { EXPECT_TRUE(contextChecker.isCurrent()); });
}

TEST(ContextCheckerTest, DetachFromTaskQueueAndUseOnThread)
{
    auto queue = TaskQueueThread::makeShared();
    queue->postTask(
        []
        {
            ContextChecker contextChecker;
            contextChecker.detach();
            RunOnDifferentThread([&] { EXPECT_TRUE(contextChecker.isCurrent()); });
        });
}

TEST(ContextCheckerTest, MethodNotAllowedOnDifferentThreadInDebug)
{
    ContextChecker contextChecker;
    RunOnDifferentThread([&] { EXPECT_EQ(contextChecker.isCurrent(), !CXXKIT_DCHECK_IS_ON); });
}

#if CXXKIT_DCHECK_IS_ON
TEST(ContextCheckerTest, OnlyCurrentOnOneThread)
{
    ContextChecker contextChecker(ContextChecker::InitialState::kDetached);
    RunOnDifferentThread(
        [&]
        {
            EXPECT_TRUE(contextChecker.isCurrent());
            // Spawn a new thread from within the first one to guarantee that we have
            // two concurrently active threads (and that there's no chance of the
            // thread ref being reused).
            RunOnDifferentThread([&] { EXPECT_FALSE(contextChecker.isCurrent()); });
        });
}
#endif

TEST(ContextCheckerTest, MethodNotAllowedOnDifferentTaskQueueInDebug)
{
    ContextChecker contextChecker;
    auto queue = TaskQueueThread::makeShared();
    queue->postTask([&] { EXPECT_EQ(contextChecker.isCurrent(), !CXXKIT_DCHECK_IS_ON); });
}

TEST(ContextCheckerTest, DetachFromTaskQueueInDebug)
{
    ContextChecker contextChecker;
    contextChecker.detach();

    auto queue1 = TaskQueueThread::makeShared();
    queue1->postTask([&] { EXPECT_TRUE(contextChecker.isCurrent()); });

    // isCurrent should return false in debug builds after moving to another task queue.
    auto queue2 = TaskQueueThread::makeShared();
    queue2->postTask([&] { EXPECT_EQ(contextChecker.isCurrent(), !CXXKIT_DCHECK_IS_ON); });
}

TEST(ContextCheckerTest, ExpectationToString)
{
    auto queue1 = TaskQueueThread::makeShared();
    ContextChecker contextChecker(ContextChecker::InitialState::kDetached);

    Semaphore blocker;
    queue1->postTask(
        [&blocker, &contextChecker]()
        {
            (void)contextChecker.isCurrent();
            blocker.release();
        });
    blocker.acquire();

#if CXXKIT_DCHECK_IS_ON
    EXPECT_THAT(ContextChecker::expectationToString(&contextChecker), HasSubstr("# Expected: TaskQueue:"));
#else
    GTEST_ASSERT_EQ(ContextChecker::expectationToString(&contextChecker), "");
#endif
}

TEST(ContextCheckerTest, Initiallydetached)
{
    auto queue1 = TaskQueueThread::makeShared();
    ContextChecker contextChecker(ContextChecker::InitialState::kDetached);

    Semaphore blocker;
    queue1->postTask(
        [&blocker, &contextChecker]()
        {
            EXPECT_TRUE(contextChecker.isCurrent());
            blocker.release();
        });
    blocker.acquire();

#if CXXKIT_DCHECK_IS_ON
    EXPECT_FALSE(contextChecker.isCurrent());
#endif
}

class TestAnnotations
{
public:
    TestAnnotations()
        : mTestVar(false)
    {
    }

    void ModifyTestVar()
    {
        CXXKIT_DCHECK_RUN_ON(&mChecker);
        mTestVar = true;
    }

private:
    bool mTestVar CXXKIT_ATTRIBUTE_GUARDED_BY(&mChecker);
    ContextChecker mChecker;
};

TEST(ContextCheckerTest, TestAnnotations)
{
    TestAnnotations annotations;
    annotations.ModifyTestVar();
}

#if GTEST_HAS_DEATH_TEST && !defined(CXXKIT_OS_ANDROID)
void TestAnnotationsOnWrongQueue()
{
    Semaphore blocker;
    TestAnnotations annotations;
    auto queue = TaskQueueThread::makeShared();
    queue->postTask(
        [&]
        {
            annotations.ModifyTestVar();
            blocker.release();
        });
    blocker.acquire();
}
#    if CXXKIT_DCHECK_IS_ON
/**
 * Note: Ending the test suite name with 'DeathTest' is important as it causes gtest to order this test before any
 * other non-death-tests, to avoid potential global process state pollution such as shared worker threads being started
 * (e.g. a side effect of calling InitCocoaMultiThreading() on Mac causes one or two additional threads to be created).
 */
TEST(ContextCheckerDeathTest, TestAnnotationsOnWrongQueueDebug)
{
    ASSERT_DEATH({ TestAnnotationsOnWrongQueue(); }, "");
}
#    else
TEST(ContextCheckerTest, TestAnnotationsOnWrongQueueRelease)
{
    TestAnnotationsOnWrongQueue();
}
#    endif
#endif // GTEST_HAS_DEATH_TEST

CXXKIT_END_NAMESPACE
