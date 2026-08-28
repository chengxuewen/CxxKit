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

#include <cxxkit/thread/detail/thread_pool_p.hpp>
#include <cxxkit/tools/exception.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/tools/assert.hpp>

#include <thread>

CXXKIT_DEFINE_LOGGER_WITH_LEVEL("cxxkit::ThreadPool", CXXKIT_THREAD_POOL_LOGGER, cxxkit::LogLevel::Warning)

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
namespace tls
{
static thread_local ThreadPoolLocalData currentThreadData;
} // namespace tls
} // namespace detail

ThreadPoolLocalData::ThreadPoolLocalData()
{
    // CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "ThreadPoolLocalData::ThreadPoolLocalData");
}
ThreadPoolLocalData::~ThreadPoolLocalData()
{
    // CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "ThreadPoolLocalData::~ThreadPoolLocalData:start");
    if (thread.get())
    {
        std::lock_guard<std::mutex> lock(thread->d_func()->mMutex);
        thread->d_func()->mDoneCondition.notify_all();
        thread->d_func()->mInFinish.store(false);
        thread->d_func()->mRunning.store(false);
        thread.reset();
    }
    // CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "ThreadPoolLocalData::~ThreadPoolLocalData:stop");
}
ThreadPoolLocalData *ThreadPoolLocalData::current()
{
    auto data = &detail::tls::currentThreadData;
    if (!data->thread.get())
    {
        auto thread = new ThreadPool::Thread(true);
        thread->d_func()->mInFinish.store(false);
        thread->d_func()->mRunning.store(true);
        data->thread.reset(thread);
    }
    data->thread->d_func()->mThreadId = std::this_thread::get_id();
    return data;
}
void ThreadPoolLocalData::init(const ThreadPool::Thread::SharedPtr &thread)
{
    detail::tls::currentThreadData.thread = thread;
    thread->d_func()->mThreadId = std::this_thread::get_id();
}

void ThreadPoolTaskThread::start()
{
    CXXKIT_ASSERT_X(!this->is_running(), "ThreadPoolThread::start", "still in running");
    if (mThread.joinable())
    {
        mThread.join();
    }
    mThread = std::thread(&ThreadPoolTaskThread::run, this);
}

void ThreadPoolTaskThread::exit_wait()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} exit_wait", utils::fmt::ptr(this));
    mExit.store(true);
    if (mThread.joinable())
    {
        CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} exit_wait join", utils::fmt::ptr(this));
        mThread.join();
    }
}

void ThreadPoolTaskThread::init(const StringView name, const WeakPtr &weakThis)
{
    std::call_once(mInitFlag,
                   [this, name, weakThis]()
                   {
                       mName = name.data();
                       mWeakThis = weakThis;
                   });
}

void ThreadPoolTaskThread::wake()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} wake", utils::fmt::ptr(this));
    mTaskReadyCondition.notify_one();
}

void ThreadPoolTaskThread::wake_all()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} wake all", utils::fmt::ptr(this));
    mTaskReadyCondition.notify_all();
}

void ThreadPoolTaskThread::run()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} run enter", utils::fmt::ptr(this));
    mExit.store(false);
    d_func()->mRunning.store(true);
    d_func()->mInFinish.store(false);
    ThreadPoolLocalData::init(mWeakThis.lock());
    std::unique_lock<std::mutex> lock(mManager->mMutex);
    while (!mExit.load())
    {
        auto task = std::move(mTask);
        do
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} do", utils::fmt::ptr(this));
            if (task)
            {
                lock.unlock();
                CXXKIT_TRY
                {
                    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                         "thread {} do run task:{}",
                                         utils::fmt::ptr(this),
                                         utils::fmt::ptr(task.get()));
                    mManager->mTasksDispatchedCount.fetch_add(1);
                    task->run();
                    mManager->mTasksCompletedCount.fetch_add(1);
                }
                CXXKIT_CATCH(...)
                {
                    CXXKIT_LOGGING_WARNING(CXXKIT_THREAD_POOL_LOGGER(),
                                           "\nOCTK Concurrent has caught an exception thrown from a worker thread.\n"
                                           "This is not supported, exceptions thrown in worker threads must be\n"
                                           "caught before control returns to OCTK Concurrent.");
                    this->register_thread_inactive();
                    CXXKIT_RETHROW;
                }
                lock.lock();
            }

            // if too many threads are active, exit do task loop
            if (mManager->is_too_many_threads_active())
            {
                CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                     "thread {} do is_too_many_threads_active true",
                                     utils::fmt::ptr(this));
                break;
            }
            // if task queue is empty, exit do task loop
            task = mManager->mTaskQueue.pop();
            if (!task)
            {
                CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                     "thread {} do task queue empty",
                                     utils::fmt::ptr(this));
                break;
            }
        } while (!mExit.load());

        // if too many threads are active or exit flag is set, expire this thread
        bool expired = mManager->is_too_many_threads_active() || mExit.load();
        if (!expired)
        {
            // CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread %p is_too_many_threads_active false", this);
            // start enter waiting state
            CXXKIT_ASSERT(nullptr == mTask.get());
            mManager->mWaitingThreads.push_back(this);
            this->register_thread_inactive();
            if (mExit.load())
            {
                CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                     "thread {} is exit set expired",
                                     utils::fmt::ptr(this));
                expired = true;
            }
            else
            {
                // wait for work, exiting after the expiry timeout is reached
                CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                     "thread {} TaskReadyCondition start wait, expiry timeout: {} ms, joinable:{}",
                                     utils::fmt::ptr(this),
                                     mManager->mExpiryTimeout,
                                     mThread.joinable());
                mTaskReadyCondition.wait_for(lock, std::chrono::milliseconds(mManager->mExpiryTimeout));
                CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                     "thread {} TaskReadyCondition finish wait, expiry timeout: {} ms",
                                     utils::fmt::ptr(this),
                                     mManager->mExpiryTimeout);
                // start exit waiting state
                ++mManager->mActiveThreadCount;
            }
            // erase if this thread is still in the waiting list
            {
                const auto iter = std::find(mManager->mWaitingThreads.begin(), mManager->mWaitingThreads.end(), this);
                if (mManager->mWaitingThreads.end() != iter)
                {
                    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                         "thread {} is still in the waiting list",
                                         utils::fmt::ptr(this));
                    mManager->mWaitingThreads.erase(iter);
                    expired = true;
                }
            }
            // check if this thread is no longer in the all threads list (manager maybe reset)
            {
                const auto iter = mManager->mAllThreads.find(this);
                if (mManager->mAllThreads.end() == iter)
                {
                    // can not use "expired = true;", avoid mExpiredThreads set
                    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                         "thread {} is not in the all threads list",
                                         utils::fmt::ptr(this));
                    this->register_thread_inactive();
                    break;
                }
            }
        }
        if (expired)
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} is expired", utils::fmt::ptr(this));
            mManager->mExpiredThreads.push_back(this);
            this->register_thread_inactive();
            break;
        }
    }
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} run exit", utils::fmt::ptr(this));
    d_func()->mInFinish.store(true);
    d_func()->mRunning.store(false);
}

void ThreadPoolTaskThread::register_thread_inactive()
{
    CXXKIT_ASSERT_X(mManager->mActiveThreadCount > 0,
                    "ThreadPoolThread::register_thread_inactive()",
                    "mActiveThreadCount must be greater than 0");
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} register_thread_inactive", utils::fmt::ptr(this));
    if (--mManager->mActiveThreadCount == 0)
    {
        CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                             "thread {} register_thread_inactive mNoActiveThreadsCondition",
                             utils::fmt::ptr(this));
        mManager->mNoActiveThreadsCondition.notify_all();
    }
}

ThreadPool::Thread::Thread(bool adopted)
    : mDPtr(new ThreadPrivate(this, adopted))
{
}

ThreadPool::Thread::~Thread()
{
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "ThreadPool::Thread::~Thread() {}", utils::fmt::ptr(this));
}

ThreadPool::Thread::Id ThreadPool::Thread::thread_id() const
{
    CXXKIT_D(const Thread);
    std::lock_guard<std::mutex> lock(d->mMutex);
    return d->mThreadId;
}

bool ThreadPool::Thread::is_finished() const
{
    CXXKIT_D(const Thread);
    return !d->mRunning.load() && !d->mInFinish.load();
}

bool ThreadPool::Thread::is_running() const
{
    CXXKIT_D(const Thread);
    return d->mRunning.load();
}

bool ThreadPool::Thread::is_adopted() const
{
    CXXKIT_D(const Thread);
    return d->mAdopted;
}

bool ThreadPool::Thread::wait(unsigned int msecs)
{
    CXXKIT_D(Thread);
    if (this->thread_id() == Thread::current_thread_id())
    {
        CXXKIT_LOGGING_WARNING(CXXKIT_THREAD_POOL_LOGGER(), "ThreadPool::Thread::wait: Thread tried to wait on itself");
        return false;
    }

    std::unique_lock<std::mutex> lock(d->mMutex);
    if (!d->mRunning)
    {
        return true;
    }

    while (d->mRunning)
    {
        if (kWaitForeverMSecs == msecs)
        {
            d->mDoneCondition.wait(lock);
        }
        else
        {
            if (std::cv_status::timeout == d->mDoneCondition.wait_for(lock, std::chrono::milliseconds(msecs)))
            {
                return false;
            }
        }
    }
    return true;
}

ThreadPool::Thread::SharedPtr ThreadPool::Thread::current() noexcept
{
    return ThreadPoolLocalData::current()->thread;
}

ThreadPool::Thread::Id ThreadPool::Thread::current_thread_id() noexcept
{
    return std::this_thread::get_id();
}

ThreadPoolPrivate::ThreadPoolPrivate(ThreadPool *p)
    : mPPtr(p)
{
}

ThreadPoolPrivate::~ThreadPoolPrivate()
{
}

ThreadPoolTaskThread::SharedPtr ThreadPoolPrivate::find_thread(ThreadPoolTaskThread *thread)
{
    const auto iter = mAllThreads.find(thread);
    return mAllThreads.end() != iter ? iter->second : nullptr;
}

void ThreadPoolPrivate::enqueue_task(const Task::SharedPtr &task, Priority priority)
{
    CXXKIT_ASSERT(nullptr != task);
    mTaskQueue.push(task, priority);
}

void ThreadPoolPrivate::start_thread(const Task::SharedPtr &task)
{
    CXXKIT_ASSERT(nullptr != task.get());
    ThreadPoolTaskThread::SharedPtr thread(new ThreadPoolTaskThread(this));
    // if this assert hits, we have an ABA problem (deleted threads don't get removed here)
    CXXKIT_ASSERT(mAllThreads.find(thread.get()) == mAllThreads.end());
    mAllThreads.insert(std::make_pair(thread.get(), thread));
    thread->init(("Thread (pooled)"), thread);
    ++mActiveThreadCount;
    thread->set_task(task);
    thread->start();
}

bool ThreadPoolPrivate::try_start(const Task::SharedPtr &task)
{
    CXXKIT_ASSERT(task != nullptr);

    if (mAllThreads.empty())
    {
        // always create at least one thread
        this->start_thread(task);
        return true;
    }

    // can't do anything if we're over the limit
    if (this->active_thread_count() >= mMaxThreadCount)
    {
        return false;
    }

    if (mWaitingThreads.size() > 0)
    {
        // recycle an available thread
        this->enqueue_task(task, Priority::kHighest);
        auto thread = mWaitingThreads.front();
        CXXKIT_ASSERT(!thread->task().get());
        mWaitingThreads.pop_front();
        thread->wake();
        return true;
    }

    if (!mExpiredThreads.empty())
    {
        // restart an expired thread
        auto thread = mExpiredThreads.front();
        CXXKIT_ASSERT(!thread->task().get());
        mExpiredThreads.pop_front();
        ++mActiveThreadCount;
        thread->set_task(task);
        thread->start();
        return true;
    }

    // start a new thread
    this->start_thread(task);
    return true;
}

void ThreadPoolPrivate::try_to_start_more_threads()
{
    // try to push tasks on the queue to any available threads
    while (!mTaskQueue.empty())
    {
        auto task = mTaskQueue.first();
        if (task.get())
        {
            if (!this->try_start(task))
            {
                break;
            }
            mTaskQueue.pop();
        }
    }
}

bool ThreadPoolPrivate::is_too_many_threads_active() const
{
    const int active_thread_count = this->active_thread_count();
    return active_thread_count > mMaxThreadCount && (active_thread_count - mReservedThreadCount) > 1;
}

int ThreadPoolPrivate::active_thread_count() const
{
    return mAllThreads.size() - mExpiredThreads.size() - mWaitingThreads.size() + mReservedThreadCount;
}

bool ThreadPoolPrivate::is_done() const
{
    return mTaskQueue.empty() && 0 == mActiveThreadCount;
}

void ThreadPoolPrivate::reset()
{
    const auto allThreads = std::move(mAllThreads);
    mExpiredThreads.clear();
    mWaitingThreads.clear();
    mMutex.unlock();
    for (auto &item : allThreads)
    {
        auto thread = item.second;
        if (!thread->is_finished())
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(),
                                 "thread {} is not finished, wake and exit_wait",
                                 utils::fmt::ptr(thread.get()));
            thread->wake_all();
            thread->exit_wait();
            CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "thread {} exit_wait done", utils::fmt::ptr(thread.get()));
        }
    }
    mMutex.lock();
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "reset done");
}

ThreadPool::ThreadPool()
    : ThreadPool(new ThreadPoolPrivate(this))
{
}

ThreadPool::ThreadPool(ThreadPoolPrivate *d)
    : mDPtr(d)
{
}

ThreadPool::~ThreadPool()
{
    this->wait_for_done();
}

ThreadPool *ThreadPool::default_instance()
{
    static std::once_flag once;
    static ThreadPool *instance;
    std::call_once(once, [=]() { instance = new ThreadPool; });
    return instance;
}

void ThreadPool::start(std::function<void()> function, Priority priority)
{
    if (function)
    {
        this->start(Task::create(std::move(function)), priority);
    }
}

bool ThreadPool::try_start_now(std::function<void()> function)
{
    if (!function)
    {
        return false;
    }

    CXXKIT_D(ThreadPool);
    std::unique_lock<std::mutex> lock(d->mMutex);
    if (!d->mAllThreads.empty() && d->active_thread_count() >= d->mMaxThreadCount)
    {
        return false;
    }

    auto task = Task::create(std::move(function));
    if (!d->try_start(task))
    {
        return false;
    }

    return true;
}

void ThreadPool::start(const Task::SharedPtr &task, Priority priority)
{
    if (task)
    {
        CXXKIT_D(ThreadPool);
        std::unique_lock<std::mutex> lock(d->mMutex);
        if (!d->try_start(task))
        {
            if (!d->mWaitingThreads.empty())
            {
                auto thread = d->mWaitingThreads.front();
                CXXKIT_ASSERT(!thread->task().get());
                d->mWaitingThreads.pop_front();
                thread->set_task(task);
                thread->wake();
            }
            else
            {
                d->enqueue_task(task, priority);
            }
        }
    }
}

bool ThreadPool::try_start_now(const Task::SharedPtr &task)
{
    if (!task)
    {
        return false;
    }

    CXXKIT_D(ThreadPool);
    std::unique_lock<std::mutex> lock(d->mMutex);
    if (!d->mAllThreads.empty() && d->active_thread_count() >= d->mMaxThreadCount)
    {
        return false;
    }

    if (!d->try_start(task))
    {
        return false;
    }

    return true;
}

int ThreadPool::max_thread_count() const
{
    CXXKIT_D(const ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    return d->mMaxThreadCount;
}

void ThreadPool::set_max_thread_count(int count)
{
    CXXKIT_D(ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    if (count != d->mMaxThreadCount)
    {
        d->mMaxThreadCount = count;
        d->try_to_start_more_threads();
    }
}

int ThreadPool::expiry_timeout() const
{
    CXXKIT_D(const ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    return d->mExpiryTimeout;
}

void ThreadPool::set_expiry_timeout(int msecs)
{
    CXXKIT_D(ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    if (msecs != d->mExpiryTimeout)
    {
        d->mExpiryTimeout = msecs;
    }
}

bool ThreadPool::wait_for_done(unsigned int msecs)
{
    CXXKIT_D(ThreadPool);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(msecs);
    std::unique_lock<std::mutex> lock(d->mMutex);
    do
    {
        CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "wait_for_done() do");
        if (kWaitForeverMSecs == msecs)
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "wait_for_done() do wait forever");
            d->mNoActiveThreadsCondition.wait(lock, [d]() { return d->is_done(); });
        }
        else
        {
            CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "wait_for_done() do wait {} ms", msecs);
            d->mNoActiveThreadsCondition.wait_until(lock, deadline);
            if (!d->is_done())
            {
                CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "wait_for_done() do !is_done return false");
                return false;
            }
        }
        d->reset();
        // More threads can be started during reset(), in that case continue waiting if we still have time left.
    } while (!d->is_done() && std::chrono::steady_clock::now() < deadline);
    CXXKIT_LOGGING_TRACE(CXXKIT_THREAD_POOL_LOGGER(), "wait_for_done() do finish:{}", d->is_done());
    return d->is_done();
}

bool ThreadPool::cancel(Task *task)
{
    CXXKIT_D(ThreadPool);
    if (nullptr == task)
    {
        return false;
    }
    std::unique_lock<std::mutex> lock(d->mMutex);
    return d->mTaskQueue.cancel(task);
}

void ThreadPool::clear()
{
    CXXKIT_D(ThreadPool);
    std::unique_lock<std::mutex> lock(d->mMutex);
    d->mTaskQueue.clear();
}

void ThreadPool::reserve_thread()
{
    CXXKIT_D(ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    ++d->mReservedThreadCount;
}

void ThreadPool::release_thread()
{
    CXXKIT_D(ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    --d->mReservedThreadCount;
    d->try_to_start_more_threads();
}

int ThreadPool::active_thread_count() const
{
    CXXKIT_D(const ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    return d->active_thread_count();
}

uint64_t ThreadPool::task_count() const
{
    CXXKIT_D(const ThreadPool);
    std::lock_guard<std::mutex> lock(d->mMutex);
    return d->mTaskQueue.size();
}
uint64_t ThreadPool::tasks_completed_count() const
{
    CXXKIT_D(const ThreadPool);
    return d->mTasksCompletedCount.load();
}

uint64_t ThreadPool::tasks_dispatched_count() const
{
    CXXKIT_D(const ThreadPool);
    return d->mTasksDispatchedCount.load();
}

int ThreadPool::ideal_thread_count()
{
    return std::thread::hardware_concurrency();
}

CXXKIT_END_NAMESPACE