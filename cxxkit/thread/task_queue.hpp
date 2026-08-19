/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#include <cxxkit/tools/source_location.hpp>
#include <cxxkit/memory/nullability.hpp>
#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/thread/task.hpp>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_CORE_API TaskQueueBase
{
public:
    struct Deleter final
    {
        void operator()(TaskQueueBase *taskQueue) const { taskQueue->destroy(); }
    };
    using SharedPtr = std::shared_ptr<TaskQueueBase>;
    using UniquePtr = std::unique_ptr<TaskQueueBase, Deleter>;

    class CXXKIT_CORE_API CurrentSetter final
    {
        TaskQueueBase *const mPrevious;
        CXXKIT_DISABLE_COPY_MOVE(CurrentSetter)
    public:
        explicit CurrentSetter(TaskQueueBase *taskQueue);
        ~CurrentSetter();
    };

    /**
     * The PendingTaskSafetyFlag and the ScopedTaskSafety are designed to address the issue where you have a task
     * to be executed later that has references, but cannot guarantee that the referenced object is alive when the
     * task is executed.
     *
     * This mechanism can be used with tasks that are created and destroyed on a single thread / task queue, and
     * with tasks posted to the same thread/task queue, but tasks can be posted from any thread/TQ.
     *
     * Typical usage:
     *    When posting a task, post a copy (capture by-value in a lambda) of the flag reference and before performing
     *    the work, check the `alive()` state. Abort if alive() returns `false`:
     *
     *    class ExampleClass
     *    {
     *    ....
     *        SharedRefPtr<PendingTaskSafetyFlag> flag = safety_flag;
     *        task_queue->postTask(
     *        [flag = std::move(flag), this] {
     *          // Now running on the main thread.
     *          if (!flag->isAlive())
     *          {
     *              return;
     *          }
     *          MyMethod();
     *        });
     *    ....
     *      ~ExampleClass() { safety_flag->setNotAlive(); }
     *      SafetyFlag::SharedPtr safety_flag = SafetyFlag::create();
     *    }
     *
     * SafeTask makes this check automatic:
     *    task_queue->postTask(SafeTask(safety_flag, [this] { MyMethod(); }));
     */
    class SafetyFlagPrivate;
    class CXXKIT_CORE_API SafetyFlag final
    {
        CXXKIT_DEFINE_DPTR(SafetyFlag)
        CXXKIT_DECLARE_PRIVATE(SafetyFlag)
        CXXKIT_DISABLE_COPY_MOVE(SafetyFlag)
    protected:
        explicit SafetyFlag(bool alive);
        SafetyFlag(bool alive, Nonnull<TaskQueueBase *> attachedQueue);

    public:
        using SharedPtr = std::shared_ptr<SafetyFlag>;

        /**
         * The ScopedTaskSafety makes using PendingTaskSafetyFlag very simple. It does automatic PTSF creation and
         * signalling of destruction when the ScopedTaskSafety instance goes out of scope.
         * Example usage:
         *  my_task_queue->postTask(SafeTask(scoped_task_safety.flag(),
         *     my_task_queue->postTask(SafeTask(scoped_task_safety.flag(),
         *        [this] {
         *             // task goes here
         *        }
         *
         * This should be used by the class that wants tasks dropped after destruction.
         * The requirement is that the instance has to be constructed and destructed on
         */
        class Scoped final
        {
        public:
            Scoped() = default;
            explicit Scoped(const SharedPtr &flag)
                : mFlag(flag)
            {
            }
            ~Scoped() { mFlag->setNotAlive(); }

            // Returns a new reference to the safety flag.
            SharedPtr flag() const { return mFlag; }

            // Marks the current flag as not-alive and attaches to a new one.
            void reset(const SharedPtr &newFlag = SafetyFlag::create())
            {
                mFlag->setNotAlive();
                mFlag = newFlag;
            }

        private:
            SharedPtr mFlag = SafetyFlag::create();
        };

        /**
         * Like ScopedTaskSafety, but allows construction on a different thread than where the flag will be used.
         */
        class ScopedDetached final
        {
        public:
            ScopedDetached() = default;
            ~ScopedDetached() { mFlag->setNotAlive(); }

            // Returns a new reference to the safety flag.
            SharedPtr flag() const { return mFlag; }

        private:
            SharedPtr mFlag = SafetyFlag::createDetached();
        };

        /**
         * Creates a flag, but with its SequenceChecker initially detached.
         * Hence, it may be created on a different thread than the flag will be used on.
         */
        static SharedPtr create();
        /**
         * Creates a flag, but with its SequenceChecker explicitly initialized for a given task queue and
         * the `alive()` flag specified.
         */
        static SharedPtr createDetached();
        /**
         * Same as `CreateDetached()` except the initial state of the returned flag will be `!alive()`.
         * @return
         */
        static SharedPtr createDetachedInactive();
        /**
         * Same as `CreateDetached()` except the initial state of the returned flag will be `!alive()`.
         * @param attachedQueue
         * @return
         */
        static SharedPtr createAttachedToTaskQueue(bool alive, Nonnull<TaskQueueBase *> attachedQueue);
        ~SafetyFlag();

        bool isAlive() const;
        void setNotAlive();
        /**
         * The SetAlive method is intended to support Start/Stop/Restart usecases.
         * When a class has called SetNotAlive on a flag used for posted tasks, and decides it wants to post new
         * tasks and have them run, there are two reasonable ways to do that:
         *
         * (i) Use the below SetAlive method. One subtlety is that any task posted prior to SetNotAlive, and still
         *    in the queue, is resurrected and will run.
         *
         * (ii) Create a fresh flag, and just drop the reference to the old one. This avoids the above problem, and
         *    ensures that tasks poster prior to SetNotAlive stay cancelled. Instead, there's a potential data race
         *    on the flag pointer itself. Some synchronization is required between the thread overwriting the flag
         *    pointer, and the threads that want to post tasks and therefore read that same pointer.
         */
        void setAlive();
    };
    static Task::SharedPtr createSafeTask(const SafetyFlag::SharedPtr &flag, Task *task, bool autoDelete);
    static Task::SharedPtr createSafeTask(const SafetyFlag::SharedPtr &flag, UniqueFunction<void() &&> function);

    virtual ~TaskQueueBase() = default;

    virtual void destroy() = 0;
    virtual bool cancelTask(const Task *task) = 0;

    void sendTask(Task *task, bool autoDelete, const SourceLocation &location = SourceLocation::current())
    {
        this->sendTask(Task::makeShared(task, autoDelete), location);
    }
    void sendTask(UniqueFunction<void() &&> function, const SourceLocation &location = SourceLocation::current())
    {
        this->sendTask(Task::create(std::move(function)), location);
    }
    void sendTask(const Task::SharedPtr &task, const SourceLocation &location = SourceLocation::current());


    template <typename Functor,
              typename ReturnT = traits::invoke_result_t<Functor>,
              typename = typename std::enable_if_t<!traits::is_void_v<ReturnT>>>
    ReturnT sendTask(Functor &&functor, const SourceLocation &location = SourceLocation::current())
    {
        ReturnT result;
        this->sendTask([&] { result = std::forward<Functor>(functor)(); }, location);
        return result;
    }

    void postTask(Task *task, bool autoDelete, const SourceLocation &location = SourceLocation::current())
    {
        this->postTask(Task::makeShared(task, autoDelete), location);
    }
    void postTask(UniqueFunction<void() &&> function, const SourceLocation &location = SourceLocation::current())
    {
        this->postTask(Task::create(std::move(function)), location);
    }
    virtual void postTask(const Task::SharedPtr &task, const SourceLocation &location = SourceLocation::current()) = 0;


    void postDelayedTask(Task *task,
                         bool autoDelete,
                         const TimeDelta &delay,
                         const SourceLocation &location = SourceLocation::current())
    {
        this->postDelayedTask(Task::makeShared(task, autoDelete), delay, location);
    }
    void postDelayedTask(UniqueFunction<void() &&> function,
                         const TimeDelta &delay,
                         const SourceLocation &location = SourceLocation::current())
    {
        this->postDelayedTask(Task::create(std::move(function)), delay, location);
    }
    virtual void postDelayedTask(const Task::SharedPtr &task,
                                 const TimeDelta &delay,
                                 const SourceLocation &location = SourceLocation::current()) = 0;


    bool isCurrent() const { return this->current() == this; }

    static TaskQueueBase *current();
};

CXXKIT_END_NAMESPACE

CXXKIT_DECLARE_LOGGER(CXXKIT_CORE_API, CXXKIT_TASK_QUEUE_LOGGER)