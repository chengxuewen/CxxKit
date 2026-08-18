/***********************************************************************************************************************
**
** Library: cxxkit
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

#include <cxxkit/functional/unique_function.hpp>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_CORE_API Task
{
    CXXKIT_DISABLE_COPY_MOVE(Task)
public:
    using Id = uint64_t;
    using SharedPtr = std::shared_ptr<Task>;
    using Func = std::function<void()>;
    using UniqueFunc = UniqueFunction<void() &&>;

    Task() = default;
    virtual ~Task() = default;

    virtual void run() = 0;

    static SharedPtr create(Func function);
    static SharedPtr create(UniqueFunc function);
    static SharedPtr makeShared(Task *task, bool autoDelete = false);
    template <typename T, typename... Args>
    static SharedPtr makeShared(Args &&...args)
    {
        return makeShared(new T(std::forward<Args>(args)...), true);
    }
};

CXXKIT_END_NAMESPACE