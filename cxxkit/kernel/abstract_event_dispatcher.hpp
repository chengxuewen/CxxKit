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

#include <cxxkit/kernel/kernel_global.hpp>

#include <cxxkit/kernel/event_loop.hpp>

#include <functional>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class EventLoop;

/**
 * @brief Driver interface behind an @ref EventLoop.
 *
 * EventLoop is the user-facing handle; AbstractEventDispatcher is the driver interface; concrete drivers
 * (e.g. Uv/Qt dispatchers) implement this.
 *
 * A dispatcher is injected into its EventLoop once at construction and owns the platform event source.
 * Callbacks (timer callbacks and post tasks) always run on the loop thread. Exceptions escaping a
 * callback terminate the process (no exception crosses the event loop).
 */
class CXXKIT_KERNEL_API AbstractEventDispatcher
{
public:
    AbstractEventDispatcher() = default;
    virtual ~AbstractEventDispatcher() = default;

    /**
     * @brief Process pending events.
     *
     * @param flags Event selection flags; kWaitForMoreEvents controls blocking behaviour.
     * @return true if at least one event was processed.
     */
    virtual bool process_events(EventLoop::ProcessFlags flags) = 0;

    /**
     * @brief Kick the loop thread out of a blocking process_events. Thread-safe; may coalesce (I4).
     */
    virtual void wake_up() = 0;

    /**
     * @brief Make process_events return as soon as possible (exec teardown path).
     */
    virtual void interrupt() = 0;

    /**
     * @brief Register a timer. The callback runs on the loop thread; repeat behaviour is a shell-level
     *          wrapper (not part of this interface).
     */
    virtual void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) = 0;

    /**
     * @brief Cancel a timer registered by start_timer.
     */
    virtual void stop_timer(int timer_id) = 0;

private:
    CXXKIT_DISABLE_COPY_MOVE(AbstractEventDispatcher)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
