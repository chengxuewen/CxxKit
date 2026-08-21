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

#pragma once

#include <cxxkit/thread/thread_global.hpp>

#include <cxxkit/thread/thread_pool.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/tools/assert.hpp>

#include <set>
#include <map>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

CXXKIT_DECLARE_LOGGER(CXXKIT_THREAD_API, CXXKIT_THREAD_POOL_LOGGER)

CXXKIT_BEGIN_NAMESPACE

struct ThreadPoolLocalData final
{
    static ThreadPoolLocalData *current();
    static void init(const ThreadPool::Thread::SharedPtr &thread);

    ThreadPoolLocalData();
    ~ThreadPoolLocalData();

    ThreadPool::Thread::SharedPtr thread{nullptr};
};

class ThreadPoolTaskQueue
{
public:
    using Id = Task::Id;
    using Priority = ThreadPool::Priority;

    struct Item
    {
        Task::SharedPtr task;
        uint64_t sequenceId;
        Priority priority;

        struct Compare
        {
            bool operator()(const Item &lhs, const Item &rhs) const
            {
                if (lhs.priority != rhs.priority)
                {
                    // Priority given to values with higher values
                    return static_cast<int>(lhs.priority) > static_cast<int>(rhs.priority);
                }
                // When of the same priority, the one with a lower sequence number
                return lhs.sequenceId < rhs.sequenceId;
            }
        };
    };

    ThreadPoolTaskQueue() = default;
    ~ThreadPoolTaskQueue() = default;

    Id push(const Task::SharedPtr &task, Priority priority)
    {
        // push item with sequenceId priority
        const auto id = mIdCounter++;
        mTasks.insert({task, id, priority});
#if 0
        CXXKIT_DEBUG("push %p, id:%llu, priority:%d", task.get(), id, static_cast<int>(priority));
        for (auto iter = mTasks.begin(); iter != mTasks.end(); ++iter)
        {
            CXXKIT_DEBUG("item %p, id:%llu, priority:%d",
                       iter->task.get(),
                       iter->sequenceId,
                       static_cast<int>(iter->priority));
        }
#endif
        return id;
    }

    Task::SharedPtr first()
    {
        // get first item in queue
        return mTasks.empty() ? nullptr : mTasks.begin()->task;
    }
    Task::SharedPtr pop()
    {
        auto task = this->first();
        if (task.get())
        {
            mTasks.erase(mTasks.begin());
        }
        return task;
    }

    void clear() { mTasks.clear(); }
    bool cancel(Task *task)
    {
        bool canceled = false;
        for (auto iter = mTasks.begin(); iter != mTasks.end();)
        {
            if (iter->task.get() == task)
            {
                iter = mTasks.erase(iter);
                canceled = true;
            }
            else
            {
                ++iter;
            }
        }
        return canceled;
    }
    bool cancel(Id id)
    {
        bool canceled = false;
        for (auto iter = mTasks.begin(); iter != mTasks.end(); /*++iter*/)
        {
            if (iter->sequenceId == id)
            {
                iter = mTasks.erase(iter);
                canceled = true;
            }
            else
            {
                ++iter;
            }
        }
        return canceled;
    }

    size_t size() const { return mTasks.size(); }
    bool empty() const { return mTasks.empty(); }

private:
    std::atomic<Id> mIdCounter{0};
    std::set<Item, Item::Compare> mTasks;
};

class ThreadPoolTaskThread : public ThreadPool::Thread
{
public:
    using SharedPtr = std::shared_ptr<ThreadPoolTaskThread>;
    using WeakPtr = std::weak_ptr<ThreadPoolTaskThread>;

    ThreadPoolTaskThread(ThreadPoolPrivate *manager)
        : ThreadPool::Thread(false)
        , mManager(manager)
    {
    }
    ~ThreadPoolTaskThread() override { this->exitWait(); }

    void init(const StringView name, const WeakPtr &weakThis);

    void wake();
    void wakeAll();

    void start();
    void exitWait();

    Task::SharedPtr task() { return mTask; }
    void setTask(const Task::SharedPtr &task)
    {
        CXXKIT_ASSERT(nullptr == mTask.get());
        mTask = task;
    }

protected:
    void run();
    void registerThreadInactive();

private:
    std::string mName;
    WeakPtr mWeakThis;
    std::thread mThread;
    Task::SharedPtr mTask;
    std::once_flag mInitFlag;
    std::atomic<bool> mExit{true};
    ThreadPoolPrivate *const mManager;
    std::condition_variable mTaskReadyCondition;
};

class ThreadPool::ThreadPrivate
{
    CXXKIT_DEFINE_PPTR(Thread)
    CXXKIT_DECLARE_PUBLIC(Thread)
    CXXKIT_DISABLE_COPY_MOVE(ThreadPrivate)
public:
    ThreadPrivate(Thread *p, bool adopted)
        : mPPtr(p)
        , mAdopted(adopted)
    {
    }
    virtual ~ThreadPrivate() { }

    mutable std::mutex mMutex;
    std::thread::id mThreadId;
    const bool mAdopted{false};
    std::atomic<bool> mRunning{false};
    std::atomic<bool> mInFinish{false};
    mutable std::condition_variable mDoneCondition;
};

class CXXKIT_THREAD_API ThreadPoolPrivate
{
    friend class ThreadPoolTaskThread;
    CXXKIT_DEFINE_PPTR(ThreadPool)
    CXXKIT_DECLARE_PUBLIC(ThreadPool)
    CXXKIT_DISABLE_COPY_MOVE(ThreadPoolPrivate)
public:
    using Priority = ThreadPool::Priority;

    explicit ThreadPoolPrivate(ThreadPool *p);
    virtual ~ThreadPoolPrivate();

    ThreadPoolTaskThread::SharedPtr findThread(ThreadPoolTaskThread *thread);
    void enqueueTask(const Task::SharedPtr &task, Priority priority);
    void startThread(const Task::SharedPtr &task);
    bool tryStart(const Task::SharedPtr &task);
    void tryToStartMoreThreads();

    bool isTooManyThreadsActive() const;
    int activeThreadCount() const;

    bool isDone() const;
    void reset();

    mutable std::mutex mMutex;
    std::condition_variable mCondition;

    ThreadPoolTaskQueue mTaskQueue;
    std::condition_variable mNoActiveThreadsCondition;
    std::list<ThreadPoolTaskThread *> mWaitingThreads;
    std::list<ThreadPoolTaskThread *> mExpiredThreads;
    std::map<ThreadPoolTaskThread *, ThreadPoolTaskThread::SharedPtr> mAllThreads;

    std::atomic<uint64_t> mTasksCompletedCount{0};
    std::atomic<uint64_t> mTasksDispatchedCount{0};

    int mExpiryTimeout = 30000;
    int mActiveThreadCount = 0;
    int mReservedThreadCount = 0;
    int mMaxThreadCount = ThreadPool::idealThreadCount();
};

CXXKIT_END_NAMESPACE

