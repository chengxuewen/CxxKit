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

#include "cxxkit/thread/task_queue_factory.hpp"
#include "cxxkit/thread/task_queue_thread.hpp"
#include "cxxkit/memory/memory.hpp"

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
class DefaultTaskQueueFactory : public TaskQueueFactory
{
public:
    DefaultTaskQueueFactory() = default;
    ~DefaultTaskQueueFactory() override { }
    std::unique_ptr<TaskQueueBase, TaskQueueBase::Deleter> CreateTaskQueue(StringView name,
                                                                           Priority priority) const override
    {
        auto taskQueue = TaskQueueThread::makeUnique();
        // taskQueue->setPriority(priority); // TODO
        return std::move(taskQueue);
    }
};
} // namespace detail

std::unique_ptr<TaskQueueFactory> TaskQueueFactory::CreateDefault()
{
    return utils::make_unique<detail::DefaultTaskQueueFactory>();
}

CXXKIT_END_NAMESPACE