/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2019 The WebRTC Project Authors. All rights reserved.
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

#include <cxxkit/thread/task_queue.hpp>
#include <cxxkit/text/string_view.hpp>

#include <memory>

CXXKIT_BEGIN_NAMESPACE

// The implementation of this interface must be thread-safe.
class TaskQueueFactory
{
public:
    // TaskQueue priority levels. On some platforms these will map to thread
    // priorities, on others such as Mac and iOS, GCD queue priorities.
    enum class Priority
    {
        kNormal = 0,
        kHigh,
        kLow
    };

    virtual ~TaskQueueFactory() = default;
    virtual std::unique_ptr<TaskQueueBase, TaskQueueBase::Deleter> CreateTaskQueue(StringView name,
                                                                                   Priority priority) const = 0;

    static std::unique_ptr<TaskQueueFactory> CreateDefault();
};

CXXKIT_END_NAMESPACE