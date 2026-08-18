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

#include <cxxkit/thread/task_queue_thread.hpp>
#include <cxxkit/thread/repeating_task.hpp>
#include <cxxkit/time/elapsed_timer.hpp>
#include <cxxkit/thread/semaphore.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/memory/memory.hpp>
#include <cxxkit/tools/utility.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

CXXKIT_BEGIN_NAMESPACE

namespace
{
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::MockFunction;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::WithArg;

class MockClosure
{
public:
    MOCK_METHOD(TimeDelta, Call, ());
    MOCK_METHOD(void, Delete, ());
};

// NOTE: Since this utility class holds a raw pointer to a variable that likely
// lives on the stack, it's important that any repeating tasks that use this
// class be explicitly stopped when the test criteria have been met. If the
// task is not stopped, an instance of this class can be deleted when the
// pointed-to MockClosure has been deleted and we end up trying to call a
// virtual method on a deleted object in the dtor.
class MoveOnlyClosure
{
public:
    explicit MoveOnlyClosure(MockClosure *mock)
        : mMock(mock)
    {
    }
    MoveOnlyClosure(const MoveOnlyClosure &) = delete;
    MoveOnlyClosure(MoveOnlyClosure &&other)
        : mMock(other.mMock)
    {
        other.mMock = nullptr;
    }
    ~MoveOnlyClosure()
    {
        if (mMock)
        {
            mMock->Delete();
        }
    }
    TimeDelta operator()() { return mMock->Call(); }

private:
    MockClosure *mMock;
};

CXXKIT_CXX14_CONSTEXPR TimeDelta kTimeout = TimeDelta::Millis(1000);
} // namespace

TEST(TaskQueueThreadTest, PostDelayedTask)
{
    std::mutex mutex;
    ElapsedTimer timer;
    std::condition_variable condition;
    auto taskQueueThread = TaskQueueThread::makeShared();
    timer.start();
    taskQueueThread->postDelayedTask(
        [taskQueueThread, &condition]()
        {
            EXPECT_TRUE(taskQueueThread->isCurrent());
            condition.notify_one();
        },
        TimeDelta::Millis(3));
    std::unique_lock<std::mutex> lock(mutex);
    EXPECT_EQ(std::cv_status::no_timeout, condition.wait_for(lock, std::chrono::seconds(1)));
    const auto elapsed = timer.elapsed();
    CXXKIT_DEBUG("TaskQueueThreadTest::PostDelayedTask: elapsed %dms", elapsed);
    EXPECT_GE(elapsed, 3);
}

TEST(RepeatingTaskTest, CancelDelayedTaskBeforeItRuns)
{
    Semaphore done;
    MockClosure mock;
    EXPECT_CALL(mock, Call).Times(0);
    EXPECT_CALL(mock, Delete).WillOnce(Invoke([&done] { done.release(); }));
    auto taskQueueThread = TaskQueueThread::makeShared();
    auto handle = RepeatingTaskHandle::delayedStart(taskQueueThread.get(),
                                                    TimeDelta::Millis(100),
                                                    MoveOnlyClosure(&mock));
    {
        auto handleMove = utils::makeMoveWrapper(std::move(handle));
        taskQueueThread->postTask([handleMove]() mutable { handleMove.move().stop(); });
    }
    EXPECT_TRUE(done.tryAcquireFor(1, std::chrono::microseconds(kTimeout.us())));
}

TEST(RepeatingTaskTest, CancelTaskAfterItRuns)
{
    Semaphore done;
    MockClosure mock;
    EXPECT_CALL(mock, Call).WillOnce(Return(TimeDelta::Millis(100)));
    EXPECT_CALL(mock, Delete).WillOnce(Invoke([&done] { done.release(); }));
    auto taskQueueThread = TaskQueueThread::makeShared();
    auto handle = RepeatingTaskHandle::start(taskQueueThread.get(), MoveOnlyClosure(&mock));
    {
        auto handleMove = utils::makeMoveWrapper(std::move(handle));
        taskQueueThread->postTask([handleMove]() mutable { handleMove.move().stop(); });
    }
    EXPECT_TRUE(done.tryAcquireFor(1, std::chrono::microseconds(kTimeout.us())));
}

TEST(RepeatingTaskTest, ZeroReturnValueRepostsTheTask)
{
    NiceMock<MockClosure> closure;
    Semaphore done;
    ElapsedTimer timer;
    EXPECT_CALL(closure, Call())
        .WillOnce(Return(TimeDelta::Zero()))
        .WillOnce(Invoke(
            [&]
            {
                done.release();
                return TimeDelta::PlusInfinity();
            }));
    auto taskQueueThread = TaskQueueThread::makeShared();
    timer.start();
    RepeatingTaskHandle::start(taskQueueThread.get(), MoveOnlyClosure(&closure));
    EXPECT_TRUE(done.tryAcquireFor(1, std::chrono::microseconds(kTimeout.us()))) << "elapsed:" << timer.elapsed();
}

TEST(RepeatingTaskTest, StartPeriodicTask)
{
    MockFunction<TimeDelta()> closure;
    Semaphore done;
    EXPECT_CALL(closure, Call())
        .WillOnce(Return(TimeDelta::Millis(20)))
        .WillOnce(Return(TimeDelta::Millis(20)))
        .WillOnce(Invoke(
            [&]
            {
                done.release();
                return TimeDelta::PlusInfinity();
            }));
    auto taskQueueThread = TaskQueueThread::makeShared();
    RepeatingTaskHandle::start(taskQueueThread.get(), closure.AsStdFunction());
    EXPECT_TRUE(done.tryAcquireFor(1, std::chrono::microseconds(kTimeout.us())));
}

TEST(RepeatingTaskTest, Example)
{
    class ObjectOnTaskQueue
    {
    public:
        void DoPeriodicTask() { }
        TimeDelta TimeUntilNextRun() { return TimeDelta::Millis(100); }
        void StartPeriodicTask(RepeatingTaskHandle *handle, TaskQueueBase *taskQueueThread)
        {
            *handle = RepeatingTaskHandle::start(taskQueueThread,
                                                 [this]
                                                 {
                                                     DoPeriodicTask();
                                                     return TimeUntilNextRun();
                                                 });
        }
    };
    auto taskQueueThread = TaskQueueThread::makeShared();
    auto object = utils::make_unique<ObjectOnTaskQueue>();
    // Create and start the periodic task.
    RepeatingTaskHandle handle;
    object->StartPeriodicTask(&handle, taskQueueThread.get());
    // Restart the task
    {
        auto handleMove = utils::makeMoveWrapper(std::move(handle));
        taskQueueThread->postTask([handleMove]() mutable { handleMove.move().stop(); });
    }
    object->StartPeriodicTask(&handle, taskQueueThread.get());
    {
        auto handleMove = utils::makeMoveWrapper(std::move(handle));
        taskQueueThread->postTask([handleMove]() mutable { handleMove.move().stop(); });
    }
    struct Destructor
    {
        void operator()() { object.reset(); }
        std::unique_ptr<ObjectOnTaskQueue> object;
    };
    taskQueueThread->postTask(Destructor{std::move(object)});
    // Do not wait for the destructor closure in order to create a race between
    // task queue destruction and running the desctructor closure.
}


TEST(SafetyFlagTest, Basic)
{
    TaskQueueBase::SafetyFlag::SharedPtr safetyFlag;
    {
        // Scope for the `owner` instance.
        class Owner
        {
        public:
            Owner() = default;
            ~Owner() { mFlag->setNotAlive(); }

            TaskQueueBase::SafetyFlag::SharedPtr mFlag = TaskQueueBase::SafetyFlag::create();
        } owner;
        EXPECT_TRUE(owner.mFlag->isAlive());
        safetyFlag = owner.mFlag;
        EXPECT_TRUE(safetyFlag->isAlive());
    }
    // `owner` now out of scope.
    EXPECT_FALSE(safetyFlag->isAlive());
}

TEST(SafetyFlagTest, BasicScoped)
{
    TaskQueueBase::SafetyFlag::SharedPtr safetyFlag;
    {
        struct Owner
        {
            TaskQueueBase::SafetyFlag::Scoped safety;
        } owner;
        safetyFlag = owner.safety.flag();
        EXPECT_TRUE(safetyFlag->isAlive());
    }
    // `owner` now out of scope.
    EXPECT_FALSE(safetyFlag->isAlive());
}

TEST(SafetyFlagTest, PendingTaskSuccess)
{
    auto tq1 = TaskQueueThread::makeShared();
    auto tq2 = TaskQueueThread::makeShared();

    class Owner
    {
    public:
        Owner()
            : mTaskQueue(TaskQueueBase::current())
        {
            CXXKIT_DCHECK(mTaskQueue);
        }
        ~Owner()
        {
            CXXKIT_DCHECK(mTaskQueue->isCurrent());
            mFlag->setNotAlive();
        }

        void DoStuff()
        {
            CXXKIT_DCHECK(!mTaskQueue->isCurrent());
            TaskQueueBase::SafetyFlag::SharedPtr safe = mFlag;
            mTaskQueue->postTask(
                [safe, this]()
                {
                    if (!safe->isAlive())
                    {
                        return;
                    }
                    mStuffDone = true;
                });
        }

        bool stuff_done() const { return mStuffDone; }

    private:
        TaskQueueBase *const mTaskQueue;
        bool mStuffDone = false;
        TaskQueueBase::SafetyFlag::SharedPtr mFlag = TaskQueueBase::SafetyFlag::create();
    };

    Semaphore blocker;
    std::unique_ptr<Owner> owner;
    tq1->postTask(
        [&owner, &blocker]()
        {
            owner = std::make_unique<Owner>();
            EXPECT_FALSE(owner->stuff_done());
            blocker.release();
        });
    blocker.acquire();
    ASSERT_TRUE(owner);
    ASSERT_EQ(blocker.available(), 0);
    tq2->postTask(
        [&owner, &blocker]()
        {
            owner->DoStuff();
            blocker.release();
        });
    blocker.acquire(); // wait owner->DoStuff();
    tq1->postTask(
        [&owner, &blocker]()
        {
            EXPECT_TRUE(owner->stuff_done());
            owner.reset();
            blocker.release(2);
        });
    blocker.acquire(2);
    ASSERT_FALSE(owner);
}

TEST(SafetyFlagTest, PendingTaskDropped)
{
    auto tq1 = TaskQueueThread::makeShared();
    auto tq2 = TaskQueueThread::makeShared();

    class Owner
    {
    public:
        explicit Owner(bool *stuff_done)
            : mTaskQueue(TaskQueueBase::current())
            , mStuffDone(stuff_done)
        {
            CXXKIT_DCHECK(mTaskQueue);
            *mStuffDone = false;
        }
        ~Owner() { CXXKIT_DCHECK(mTaskQueue->isCurrent()); }

        void DoStuff()
        {
            CXXKIT_DCHECK(!mTaskQueue->isCurrent());
            mTaskQueue->postTask(TaskQueueThread::createSafeTask(mSafety.flag(), [this]() { *mStuffDone = true; }));
        }

    private:
        TaskQueueBase *const mTaskQueue;
        bool *const mStuffDone;
        TaskQueueBase::SafetyFlag::Scoped mSafety;
    };

    std::unique_ptr<Owner> owner;
    bool stuff_done = false;
    Semaphore blocker;
    tq1->postTask(
        [&owner, &stuff_done, &blocker]()
        {
            owner = std::make_unique<Owner>(&stuff_done);
            blocker.release();
        });
    blocker.acquire();
    ASSERT_TRUE(owner);
    ASSERT_EQ(blocker.available(), 0);

    // Queue up a task on tq1 that will execute before the 'DoStuff' task
    // can, and delete the `owner` before the 'stuff' task can execute.
    tq1->postTask(
        [&blocker, &owner]()
        {
            blocker.acquire(); // wait owner->DoStuff();
            owner.reset();
            blocker.release(2);
        });

    // Queue up a DoStuff...
    tq2->postTask(
        [&owner, &blocker]()
        {
            owner->DoStuff();
            blocker.release(); // notify owner.reset();
        });

    ASSERT_TRUE(owner);

    // Run an empty task on tq1 to flush all the queued tasks.
    blocker.acquire(2); // wait owner.reset();
    ASSERT_FALSE(owner);
    EXPECT_FALSE(stuff_done);
}

TEST(SafetyFlagTest, PendingTaskNotAliveInitialized)
{
    auto tq = TaskQueueThread::makeShared();

    // Create a new flag that initially not `alive`.
    auto flag = TaskQueueThread::SafetyFlag::createDetachedInactive();
    tq->postTask([flag]() { EXPECT_FALSE(flag->isAlive()); });

    bool task_1_ran = false;
    bool task_2_ran = false;
    Semaphore blocker;
    tq->postTask(TaskQueueThread::createSafeTask(flag, [&task_1_ran]() { task_1_ran = true; }));
    tq->postTask(
        [&flag, &blocker]()
        {
            flag->setAlive();
            blocker.release(); // notify post task_2_ran = true; task
        });
    blocker.acquire(); // wait flag->setAlive();
    tq->postTask(TaskQueueThread::createSafeTask(flag,
                                                 [&task_2_ran, &blocker]()
                                                 {
                                                     task_2_ran = true;
                                                     blocker.release(); // notify EXPECT_TRUE(task_2_ran);
                                                 }));
    blocker.acquire(); // wait task_2_ran = true; task finish
    EXPECT_FALSE(task_1_ran);
    EXPECT_TRUE(task_2_ran);
}

TEST(SafetyFlagTest, PendingTaskInitializedForTaskQueue)
{
    auto tq = TaskQueueThread::makeShared();

    // Create a new flag that initially `alive`, attached to a specific TQ.
    auto flag = TaskQueueThread::SafetyFlag::createAttachedToTaskQueue(true, tq.get());
    tq->postTask([flag]() { EXPECT_TRUE(flag->isAlive()); });
    // Repeat the same steps but initialize as inactive.
    flag = TaskQueueThread::SafetyFlag::createAttachedToTaskQueue(false, tq.get());
    tq->postTask([flag]() { EXPECT_FALSE(flag->isAlive()); });
}

TEST(SafetyFlagTest, SafeTask)
{
    TaskQueueBase::SafetyFlag::SharedPtr flag = TaskQueueBase::SafetyFlag::create();

    int count = 0;
    // Create two identical tasks that increment the `count`.
    auto task1 = TaskQueueBase::createSafeTask(flag, [&count] { ++count; });
    auto task2 = TaskQueueBase::createSafeTask(flag, [&count] { ++count; });

    EXPECT_EQ(count, 0);
    task1->run();
    EXPECT_EQ(count, 1);
    flag->setNotAlive();
    // Now task2 should actually not run.
    task2->run();
    EXPECT_EQ(count, 1);
}

CXXKIT_END_NAMESPACE