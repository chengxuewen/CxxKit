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

#include <cxxkit/thread/task_queue.hpp>
#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/tools/clock.hpp>

#include <memory>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_THREAD_API RepeatingTaskHandle final
{
public:
    RepeatingTaskHandle() = default;
    RepeatingTaskHandle(RepeatingTaskHandle &&other) = default;
    RepeatingTaskHandle &operator=(RepeatingTaskHandle &&other) = default;
    ~RepeatingTaskHandle();

    static RepeatingTaskHandle start(TaskQueueBase *taskQueue,
                                     UniqueFunction<TimeDelta()> closure,
                                     Clock *clock = Clock::get_real_time_clock(),
                                     const SourceLocation &location = SourceLocation::current());

    static RepeatingTaskHandle delayed_start(TaskQueueBase *taskQueue,
                                            TimeDelta firstDelay,
                                            UniqueFunction<TimeDelta()> closure,
                                            Clock *clock = Clock::get_real_time_clock(),
                                            const SourceLocation &location = SourceLocation::current());

    /**
     * Stops future invocations of the repeating task closure.
     * Can only be called from the TaskQueueBase where the task is running.
     * The closure is guaranteed to not be running after stop() returns unless stop() is called from the closure itself.
     */
    void stop();

    /**
     * Returns true until stop() was called.
     * Can only be called from the TaskQueueBase where the task is running.
     * @return True if the task is running.
     */
    bool is_running() const;

protected:
    CXXKIT_DECLARE_DISABLE_COPY(RepeatingTaskHandle)
    explicit RepeatingTaskHandle(const TaskQueueBase::SafetyFlag::SharedPtr &aliveFlag)
        : mAliveFlag(aliveFlag)
    {
    }
    TaskQueueBase::SafetyFlag::SharedPtr mAliveFlag;
};

CXXKIT_END_NAMESPACE