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

#ifndef _CXXKIT_THREAD_POOL_HPP
#define _CXXKIT_THREAD_POOL_HPP

#include <cxxkit/patterns/singleton.hpp>
#include <cxxkit/thread/task.hpp>

#include <functional>
#include <thread>

/**
 * @addtogroup core
 * @{
 * @addtogroup ThreadPool
 * @brief The ThreadPool class manages a collection of threads.
 * @{
 * @details
 * ThreadPool manages and recyles individual std::thread to help reduce thread creation costs in programs that
 * use threads. Each cxxkit application has one global ThreadPool object, which can be accessed by calling
 * ThreadPool::instance().
 *
 * To use one of the ThreadPool threads, subclass ThreadPool::Task and implement the run() virtual function.
 * Then create an object of that class and pass it to ThreadPool::start().
 *
 * ThreadPool deletes the ThreadPool::Task automatically by param autoDelete is true.
 *
 * ThreadPool supports executing the same ThreadPool::Task more than once by calling tryStartNow(this) from
 * within ThreadPool::Task::run().
 * If autoDelete is enabled the ThreadPool::Task will be deleted when the last thread exits the run function.
 * Calling start() multiple times with the same ThreadPool::Task when autoDelete is enabled creates a race
 * condition and is not recommended.
 *
 * Threads that are unused for a certain amount of time will expire.
 * The default expiry timeout is 30000 milliseconds (30 seconds).
 * This can be changed using setExpiryTimeout().
 * Setting a negative expiry timeout disables the expiry mechanism.
 *
 * Call maxThreadCount() to query the maximum number of threads to be used.
 * If needed, you can change the limit with setMaxThreadCount().
 * The default maxThreadCount() is ThreadPool::idealThreadCount().
 * The activeThreadCount() function returns the number of threads currently doing work.
 *
 * The reserveThread() function reserves a thread for external use.
 * Use releaseThread() when your are done with the thread, so that it may be reused.
 * Essentially, these functions temporarily increase or reduce the active thread count and are useful when
 * implementing time-consuming operations that are not visible to the ThreadPool.
 *
 * Note that ThreadPool is a low-level class for managing threads,
 * see the cxxkit Concurrent module for higher level alternatives.
 *
 * @sa ThreadPool::Task
 */

CXXKIT_BEGIN_NAMESPACE

class ThreadPoolLocalData;

class ThreadPoolPrivate;
class CXXKIT_CORE_API ThreadPool : public AutoSingleton<ThreadPool>
{
public:
    enum Priority : int8_t
    {
        kLowest = -128,
        kLow = -64,
        kNormal = 0,
        kHigh = +64,
        kHighest = +127
    };

    class ThreadPrivate;
    class CXXKIT_CORE_API Thread
    {
        friend class ThreadPoolLocalData;
        friend class ThreadPoolTaskThread;
        CXXKIT_DEFINE_DPTR(Thread)
        CXXKIT_DECLARE_PRIVATE(Thread)
        CXXKIT_DISABLE_COPY_MOVE(Thread)
        Thread(bool adopted);

    public:
        using Id = std::thread::id;
        using SharedPtr = std::shared_ptr<Thread>;

        virtual ~Thread();

        Id threadId() const;

        bool isFinished() const;
        bool isRunning() const;
        bool isAdopted() const;

        CXXKIT_STATIC_CONSTANT_NUMBER(kWaitForeverMSecs, std::numeric_limits<unsigned int>::max())
        bool wait(unsigned int msecs = kWaitForeverMSecs);

        static SharedPtr current() noexcept;
        static Id currentThreadId() noexcept;
    };

    ThreadPool();
    ThreadPool(ThreadPoolPrivate *d);
    virtual ~ThreadPool() override;

    static ThreadPool *defaultInstance();

    /**
     * Reserves a thread and uses it to run @a function, unless this thread will make the current thread count
     * exceed maxThreadCount().  In that case, @a function is added to a run queue instead.
     * The @a priority argument can be used to control the run queue's order of execution.
     *
     * @param function
     * @param priority
     */
    void start(std::function<void()> function, Priority priority = Priority::kNormal);
    /**
     * Attempts to reserve a thread to run @a function.
     * If no threads are available at the time of calling, then this function
     * does nothing and returns @c false.  Otherwise, @a function is run immediately using one available thread
     * and this function returns @c true.
     *
     * @param function
     * @return
     */
    bool tryStartNow(std::function<void()> function);

    /**
     * Reserves a thread and uses it to run @a task, unless this thread will make the current thread count exceed
     * maxThreadCount().  In that case, @a task is added to a run queue instead. The @a priority argument can
     * be used to control the run queue's order of execution.
     * @param task
     * @param priority
     */
    void start(const Task::SharedPtr &task, Priority priority = Priority::kNormal);

    /**
     * Attempts to reserve a thread to run @a runnable.
     *
     * If no threads are available at the time of calling, then this function does nothing and returns @c false.
     * Otherwise, @a runnable is run immediately using one available thread and this function returns @c true.
     *
     * @param task
     * @param autoDelete
     * @return
     */
    bool tryStartNow(const Task::SharedPtr &task);

    /**
     * Reserves a thread and uses it to run @a runnable, unless this thread will make the current thread count
     * exceed maxThreadCount().  In that case, @a runnable is added to a run queue instead.
     * The @a priority argument can be used to control the run queue's order of execution.
     *
     * Note that on success the thread pool takes ownership of the @a runnable if @a autoDelete is true,
     * and the @a runnable will be deleted automatically by the thread pool after the task run.
     * If @a autoDelete is false, ownership of @a runnable remains with the caller.
     * @param task
     * @param autoDelete
     * @param priority
     */
    void start(Task *task, bool autoDelete = true, Priority priority = Priority::kNormal)
    {
        return this->start(Task::makeShared(task, autoDelete), priority);
    }
    /**
     * Attempts to reserve a thread to run @a runnable.
     *
     * If no threads are available at the time of calling, then this function does nothing and returns @c false.
     * Otherwise, @a runnable is run immediately using one available thread and this function returns @c true.
     *
     * Note that on success the thread pool takes ownership of the @a runnable if @a autoDelete is true,
     * and the @a runnable will be deleted automatically by the thread pool after the task run.
     * If @a autoDelete is false, ownership of @a runnable remains with the caller.
     *
     * @param task
     * @param autoDelete
     * @return
     */
    bool tryStartNow(Task *task, bool autoDelete = true)
    {
        return this->tryStartNow(Task::makeShared(task, autoDelete));
    }

    /**
     * This property represents the maximum number of threads used by the thread pool.
     *
     * @note The thread pool will always use at least 1 thread, even if @a maxThreadCount limit is zero or negative.
     * The default @a maxThreadCount is QThread::idealThreadCount().
     * @return
     */
    int maxThreadCount() const;
    void setMaxThreadCount(int count);

    /**
     * Threads that are unused for @a expiryTimeout milliseconds are considered to have expired and will exit.
     * Such threads will be restarted as needed.
     * The default @a expiryTimeout is 30000 milliseconds (30 seconds).
     * If @a expiryTimeout is negative, newly created threads will not expire, e.g.,
     * they will not exit until the thread pool is destroyed.
     *
     * Note that setting @a expiryTimeout has no effect on already running threads.
     * Only newly created threads will use the new @a expiryTimeout.
     * We recommend setting the @a expiryTimeout immediately after creating the thread pool, but before calling start().
     * @return
     */
    int expiryTimeout() const;
    void setExpiryTimeout(int msecs);

    CXXKIT_STATIC_CONSTANT_NUMBER(kWaitForeverMSecs, std::numeric_limits<unsigned int>::max())
    /**
     * Waits up to @a msecs milliseconds for all threads to exit and removes all threads from the thread pool.
     * Returns @c true if all threads were removed;
     * otherwise it returns @c false. If @a msecs is -1 (the default),
     * the timeout is ignored (waits for the last thread to exit).
     * @param msecs
     * @return
     */
    bool waitForDone(unsigned int msecs = kWaitForeverMSecs);
    /**
     * Removes the specified @a runnable from the queue if it is not yet started.
     * The tasks for which autoDelete set true are deleted.
     * @param task 
     * @return 
     */
    bool cancel(Task *task);
    /**
     * Removes the tasks that are not yet started from the queue.
     * @sa start()
     */
    void clear();

    /**
     * Reserves one thread, disregarding activeThreadCount() and maxThreadCount().
     * Once you are done with the thread, call releaseThread() to allow it to be reused.
     *
     * @note This function will always increase the number of active threads.
     * This means that by using this function, it is possible for activeThreadCount() to return a value
     * greater than maxThreadCount() .
     *
     * @sa releaseThread()
     */
    void reserveThread();
    /**
     * Releases a thread previously reserved by a call to reserveThread().
     * @note Calling this function without previously reserving a thread temporarily increases maxThreadCount().
     * This is useful when athread goes to sleep waiting for more work, allowing other threads to continue.
     * Be sure to call reserveThread() when done waiting, so that the thread pool can correctly maintain the
     * activeThreadCount().
     *
     * @sa reserveThread()
     */
    void releaseThread();

    /**
     * This property represents the number of active threads in the thread pool.
     * @note It is possible for this function to return a value that is greater than maxThreadCount().
     * See reserveThread() for more details.
     * @sa reserveThread(), releaseThread()
     * @return
     */
    int activeThreadCount() const;

    uint64_t taskCount() const;
    /**
     * This property represents the number of tasks that have been completed by the thread pool.
     * @return
     */
    uint64_t tasksCompletedCount() const;
    /**
     * This property represents the number of tasks that have been dispatched by the thread pool.
     * @return
     */
    uint64_t tasksDispatchedCount() const;


    static int idealThreadCount();

protected:
    CXXKIT_DEFINE_DPTR(ThreadPool)
    CXXKIT_DECLARE_PRIVATE(ThreadPool)
    CXXKIT_DISABLE_COPY_MOVE(ThreadPool)
};

CXXKIT_END_NAMESPACE

/**
 * @}
 * @}
 */

#endif // _CXXKIT_THREAD_POOL_HPP
