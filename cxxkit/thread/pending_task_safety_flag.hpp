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

#include <cxxkit/base/global.hpp>

#include <atomic>
#include <functional>
#include <memory>

CXXKIT_BEGIN_NAMESPACE

/**
 * @class PendingTaskSafetyFlag
 * @brief Ref-counted alive flag to prevent use-after-free in async callbacks.
 *
 * Port from libwebrtc api/task_queue/pending_task_safety_flag.h.
 * Use safe_task() to wrap a callable that checks flag->alive() before execution.
 *
 * Usage:
 *   auto flag = PendingTaskSafetyFlag::create();
 *   task_queue->post(safe_task(flag, [this]() { DoWork(); }));
 */
class PendingTaskSafetyFlag : public std::enable_shared_from_this<PendingTaskSafetyFlag>
{
public:
    /**
     * @brief Creates a new PendingTaskSafetyFlag in the alive state.
     * @return shared_ptr to the flag.
     */
    static std::shared_ptr<PendingTaskSafetyFlag> create()
    {
        return std::shared_ptr<PendingTaskSafetyFlag>(new PendingTaskSafetyFlag());
    }

    /**
     * @brief Returns true if the flag is still alive.
     */
    bool alive() const { return mAlive.load(std::memory_order_acquire); }

    /**
     * @brief Marks the flag as not alive. Subsequent alive() calls return false.
     */
    void set_not_alive() { mAlive.store(false, std::memory_order_release); }

private:
    PendingTaskSafetyFlag()
        : mAlive(true)
    {
    }
    PendingTaskSafetyFlag(const PendingTaskSafetyFlag &) = delete;
    PendingTaskSafetyFlag &operator=(const PendingTaskSafetyFlag &) = delete;

    std::atomic<bool> mAlive;
};

/**
 * @brief Wraps a callable so it only executes if the flag is alive.
 * @param flag The safety flag to check.
 * @param callable The callable to wrap.
 * @return A std::function<void()> that checks alive() before invoking callable.
 */
template <class Callable>
std::function<void()> safe_task(std::shared_ptr<PendingTaskSafetyFlag> flag, Callable &&callable)
{
    return [flag, callable]()
    {
        if (flag->alive())
        {
            callable();
        }
    };
}

CXXKIT_END_NAMESPACE
