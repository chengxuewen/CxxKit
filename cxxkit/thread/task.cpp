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

#include <cxxkit/thread/task.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
struct TaskDeleter final
{
    const bool autoDelete;
    void operator()(Task *task) const
    {
        if (autoDelete)
        {
            delete task;
        }
    }
};

class FunctionTask : public Task
{
    std::function<void()> mFunction;

public:
    FunctionTask(std::function<void()> &&function)
        : mFunction(std::move(function))
    {
    }

protected:
    void run() override { mFunction(); }
};

class UniqueFunctionTask : public Task
{
    UniqueFunction<void() &&> mFunction;

public:
    UniqueFunctionTask(UniqueFunction<void() &&> function)
        : mFunction(std::move(function))
    {
    }

protected:
    void run() override { std::move(mFunction)(); }
};
} // namespace detail

Task::SharedPtr Task::create(Func function)
{
    return SharedPtr(new detail::FunctionTask(std::move(function)), detail::TaskDeleter{true});
}

Task::SharedPtr Task::create(UniqueFunc function)
{
    return SharedPtr(new detail::UniqueFunctionTask(std::move(function)), detail::TaskDeleter{true});
}

Task::SharedPtr Task::make_shared(Task *task, bool autoDelete)
{
    return SharedPtr(task, detail::TaskDeleter{autoDelete});
}

CXXKIT_END_NAMESPACE
