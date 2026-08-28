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

#include <cxxkit/thread/thread_global.hpp>

#include <cxxkit/thread/task_queue.hpp>
#include <cxxkit/tools/logging.hpp>

CXXKIT_BEGIN_NAMESPACE

class TaskQueueThreadPrivate;
class CXXKIT_THREAD_API TaskQueueThread : public TaskQueueBase
{
protected:
    struct NextTask
    {
        bool finalTask{false};
        Task::SharedPtr runTask;
        TimeDelta sleepTime{TimeDelta::PlusInfinity()};
    };

    TaskQueueThread();

public:
    static SharedPtr make_shared();
    static UniquePtr make_unique();
    static SharedPtr create() { return make_shared(); }

    ~TaskQueueThread() override;

    void destroy() override;
    bool cancel_task(const Task *task) override;

    using TaskQueueBase::post_task;
    void post_task(const Task::SharedPtr &task, const SourceLocation &location = SourceLocation::current()) override;
    using TaskQueueBase::post_delayed_task;
    void post_delayed_task(const Task::SharedPtr &task,
                         const TimeDelta &delay,
                         const SourceLocation &location = SourceLocation::current()) override;

protected:
    NextTask pop_next_task();
    void process_tasks();

    CXXKIT_DEFINE_DPTR(TaskQueueThread)
    CXXKIT_DECLARE_PRIVATE(TaskQueueThread)
    CXXKIT_DISABLE_COPY_MOVE(TaskQueueThread)
};

CXXKIT_END_NAMESPACE