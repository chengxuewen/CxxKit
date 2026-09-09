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

#include <cxxkit/kernel/object.hpp>
#include <cxxkit/tools/enum_flags.hpp>
#include <cxxkit/base/core_config.hpp>

#include <cstdint>
#include <functional>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class AbstractEventDispatcher;
class EventLoopPrivate;
class CXXKIT_KERNEL_API EventLoop : public Object
{
public:
    enum class ProcessFlag
    {
        kAllEvents = 0x00,
        kExcludeUserInputEvents = 0x01,
        kExcludeSocketNotifiers = 0x02,
        kWaitForMoreEvents = 0x04,
        kX11ExcludeTimers = 0x08,
        kEventLoopExec = 0x20,
        kDialogExec = 0x40
    };
    CXXKIT_DECLARE_ENUM_FLAGS(ProcessFlags, ProcessFlag)

    /**
     * @brief Creates an EventLoop driven by @p dispatcher.
     *
     * The dispatcher is injected once and owned exclusively by the loop. A null dispatcher is
     * a fatal error — there is no valid dispatcher-less state.
     */
    explicit EventLoop(std::unique_ptr<AbstractEventDispatcher> dispatcher, Object *parent = nullptr);
    ~EventLoop() override;

    /**
     * @brief Processes pending events without blocking; returns what the dispatcher returned.
     */
    bool process_events(ProcessFlags flags = ProcessFlag::kAllEvents);

    /**
     * @brief Processes events for at most @p maximum_ms.
     *
     * Shell synthesis: a one-shot interrupt timer is registered through the public timer path;
     * the loop drains posted tasks and delegates to the dispatcher with kWaitForMoreEvents until
     * the timeout fires or an event arrives. Returns true if at least one event was processed,
     * false on timeout.
     */
    bool process_events(ProcessFlags flags, uint64_t maximum_ms);

    /**
     * @brief Enqueues @p fn for execution on the loop thread. Thread-safe; callable from any thread.
     */
    void post(std::function<void()> fn);

    /**
     * @brief Registers a timer; returns its id (unique, non-zero).
     *
     * With repeat == false the shell wraps @p fn into "stop_timer(id) first, then fn" so the
     * callback fires exactly once (M3). The wrapper captures this — see the lifecycle contract
     * (event-loop design spec, appendix C): the loop must outlive pending one-shot timers.
     *
     * Zero-interval one-shot fast path: start_timer(0, fn, false) delegates to post(fn)
     * (Qt singleShotImpl precedent) — the callback runs FIFO with posted work on the next
     * drain, no engine timer registration. The returned id is a ghost id: unique and non-zero,
     * but no dispatcher entry exists for it, so stop_timer(ghostId) is a no-op (safe to call).
     * repeat == true with interval_ms == 0 still registers through the engine: a zero-period
     * repeating timer fires every loop round (Qt semantics).
     */
    int start_timer(uint64_t interval_ms, std::function<void()> fn, bool repeat = true);

    /**
     * @brief Cancels a timer previously registered by start_timer.
     */
    void stop_timer(int timer_id);

    /**
     * @brief Returns the EventLoop running on the calling thread, or nullptr if none.
     *
     * Set by exec() for its duration (nested exec restores the previous value on exit);
     * cleared in ~EventLoop if it still points here.
     */
    static EventLoop *current();

    int exec(ProcessFlags flags = ProcessFlag::kAllEvents);
    void wake_up();

    void exit(int retCode = 0);
    void quit();

    bool is_running() const;

    /**
     * @brief Returns the loop's injected dispatcher (phase-2 IO consumers).
     *
     * The dispatcher is owned exclusively by the loop and is never null (construction contract).
     * Phase-2 IO classes (TcpSocket/TcpServer over cxxkit::uv) reach the engine through this accessor
     * instead of each carrying their own engine reference. The dispatcher's own threading contract
     * applies — engine registrations are loop-thread only.
     */
    AbstractEventDispatcher &dispatcher();

    bool event(Event *event) override;

private:
    CXXKIT_DECLARE_PRIVATE(EventLoop)
    CXXKIT_DISABLE_COPY_MOVE(EventLoop)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
