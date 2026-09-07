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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/uv/uv_global.hpp>

#include <cxxkit/kernel/abstract_event_dispatcher.hpp>

#include <functional>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class UvEventDispatcherPrivate;

/**
 * @brief Event-loop driver on vendored libuv (opt-in @c cxxkit::uv sublibrary).
 *
 * Implements the five @ref AbstractEventDispatcher operations over a private @c uv_loop_t:
 * - @c process_events runs one @c uv_run round (blocking @c UV_RUN_ONCE when @c kWaitForMoreEvents is set,
 *   non-blocking @c UV_RUN_NOWAIT otherwise) and reports whether a timer or doorbell event fired.
 * - @c wake_up rings a @c uv_async_t doorbell (uv_async_send coalesces — I4). The async callback is an empty
 *   body by design (R-B2-2): waking the loop is the doorbell's only job; draining posted tasks is the
 *   EventLoop shell's job (EventLoop::process_events drains its own queue before delegating here).
 * - @c start_timer maps one @c uv_timer_t per id with @c uv_timer_start(cb, ms, ms) — repeat is always @p ms
 *   (R-B2-4): the shell passes repeating callbacks through unchanged, and one-shot callbacks are shell-wrapped
 *   to stop the timer before running, so the repeating re-arm never fires a second time.
 * - @c interrupt sets a flag and rings the doorbell; the next @c process_events returns promptly.
 *
 * Threading (I1): construct, @c process_events, @c start_timer, @c stop_timer and destruction are loop-thread
 * only — enforced with always-on thread-id checks (R-B2-5; violations are fatal bugs, not diagnostics).
 * @c wake_up and @c interrupt are thread-safe.
 *
 * Nesting (I5): libuv has no nested iteration — re-entering @c process_events from a callback is fatal. Return
 * from the callback and schedule follow-up work with @c EventLoop::post instead. (QtEventDispatcher permits
 * nesting.)
 *
 * Lifecycle (I6): destruction must happen on the loop thread (or after the loop has stopped). The destructor
 * closes every handle, runs the loop to drain close callbacks, and closes the loop — no uv handle or callback
 * data survives it.
 */
class CXXKIT_UV_API UvEventDispatcher : public AbstractEventDispatcher
{
public:
    /**
     * @brief Creates the dispatcher; the creating thread becomes the loop thread.
     *
     * The loop thread is fixed at construction. No libuv iteration runs yet — the first
     * @c process_events / exec call on the loop thread drives it.
     */
    UvEventDispatcher();

    /** @brief Closes all handles, drains close callbacks, then closes the loop (I6). Loop thread only. */
    ~UvEventDispatcher() override;

    bool process_events(EventLoop::ProcessFlags flags) override;
    void wake_up() override;
    void interrupt() override;
    void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) override;
    void stop_timer(int timer_id) override;

private:
    CXXKIT_DECLARE_PRIVATE(UvEventDispatcher)
    CXXKIT_DEFINE_DPTR(UvEventDispatcher)
    CXXKIT_DISABLE_COPY_MOVE(UvEventDispatcher)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
