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

#include <cxxkit/uv/detail/uv_event_dispatcher_p.hpp>
#include <cxxkit/uv/uv_event_dispatcher.hpp>

#include <cxxkit/tools/checks.hpp>

#include <thread>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace
{

/** @brief Per-timer callback storage: heap std::function carried in handle->data (Node HandleWrap pattern). */
struct TimerCallback
{
    std::function<void()> mFn;
    int mTimerId;
};

} // namespace

UvEventDispatcherPrivate::UvEventDispatcherPrivate(UvEventDispatcher *p)
    : mLoopThreadId(std::this_thread::get_id())
{
    CXXKIT_UNUSED(p); // no p-pointer member; the public class reaches this pimpl via CXXKIT_DEFINE_DPTR only
    const int rc = uv_loop_init(&mLoop);
    CXXKIT_CHECK(rc == 0) << "UvEventDispatcher: uv_loop_init failed (" << rc << ")";

    mWakeAsync = new uv_async_t;
    mWakeAsync->data = nullptr;
    // R-B2-2: the doorbell callback is an empty body. uv_async_send's only job is to break a blocking uv_run
    // out of its poll; draining posted tasks is the EventLoop shell's job (its process_events drains the
    // post queue at its own entry, before delegating here). An empty callback keeps the two layers single-queued.
    const int async_rc = uv_async_init(&mLoop, mWakeAsync, &UvEventDispatcherPrivate::on_wake_async);
    CXXKIT_CHECK(async_rc == 0) << "UvEventDispatcher: uv_async_init failed (" << async_rc << ")";
}

UvEventDispatcherPrivate::~UvEventDispatcherPrivate()
{
    // I6 teardown discipline. Order matters:
    //   1. mClosed: reject wake_up()/interrupt() arriving after this point (in-flight send protection).
    //   2. uv_close everything: async + every live timer (close callbacks free the TimerCallback).
    //   3. one final uv_run(NOWAIT): lets the close callbacks execute so all heap state is freed.
    //   4. uv_loop_close: fails with UV_EBUSY if any handle survived — that is a teardown bug, fatal.
    mClosed.store(true);

    if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(mWakeAsync)))
    {
        uv_close(reinterpret_cast<uv_handle_t *>(mWakeAsync), &UvEventDispatcherPrivate::on_handle_closed);
    }

    std::map<int, uv_timer_t *> timers;
    mTimers.swap(timers);
    for (std::map<int, uv_timer_t *>::iterator it = timers.begin(); it != timers.end(); ++it)
    {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(it->second)))
        {
            uv_close(reinterpret_cast<uv_handle_t *>(it->second), &UvEventDispatcherPrivate::on_handle_closed);
        }
    }

    uv_run(&mLoop, UV_RUN_NOWAIT); // drain close callbacks (delete fn + delete handle)

    const int rc = uv_loop_close(&mLoop);
    CXXKIT_CHECK(rc == 0) << "UvEventDispatcher: uv_loop_close failed (" << rc << ") — a handle was not closed";

    // mWakeAsync was deleted by on_handle_closed during the drain round above (uv__loop_alive is true
    // while closing_handles is non-empty, so one NOWAIT round is guaranteed to run the close callbacks).
    mWakeAsync = nullptr;
}

void UvEventDispatcherPrivate::on_wake_async(uv_async_t *handle)
{
    // R-B2-2: intentionally empty. See the constructor comment — the doorbell only needs to exist as a wake
    // source; waking the loop out of its poll is uv_async_send's side effect on the loop itself.
    CXXKIT_UNUSED(handle);
}

void UvEventDispatcherPrivate::on_timer_expired(uv_timer_t *handle)
{
    TimerCallback *cb = static_cast<TimerCallback *>(handle->data);
    if (cb != nullptr && cb->mFn)
    {
        UvEventDispatcherPrivate *d = static_cast<UvEventDispatcherPrivate *>(handle->loop->data);
        if (d != nullptr)
        {
            d->mHadEvents = true; // R-B2-3 activity tracking (only loop-thread state, we are on it)
        }
        cb->mFn();
    }
}

void UvEventDispatcherPrivate::on_handle_closed(uv_handle_t *handle)
{
    if (handle->data != nullptr)
    {
        delete static_cast<TimerCallback *>(handle->data); // frees the std::function
        handle->data = nullptr;
    }
    delete handle; // timers and the wake async are heap cells; freeing here is uniform
}

void UvEventDispatcherPrivate::check_loop_thread(const char *api) const
{
    // R-B2-5: always-on, not debug-only. Calling these APIs off the loop thread is a fatal contract bug
    // (I1), and the runtime cost is one thread-id compare.
    if (std::this_thread::get_id() != mLoopThreadId)
    {
        CXXKIT_FATAL() << "UvEventDispatcher::" << api << " called from a non-loop thread (loop thread "
                       << std::hash<std::thread::id>()(mLoopThreadId) << ", caller thread "
                       << std::hash<std::thread::id>()(std::this_thread::get_id())
                       << "). Use EventLoop::post() from other threads instead.";
    }
}

UvEventDispatcher::UvEventDispatcher()
{
    mDPtr.reset(new UvEventDispatcherPrivate(this));
    mDPtr->mLoop.data = mDPtr.get(); // callbacks recover the pimpl from the loop handle (Node HandleWrap pattern)
}

UvEventDispatcher::~UvEventDispatcher() = default;

bool UvEventDispatcher::process_events(EventLoop::ProcessFlags flags)
{
    CXXKIT_D(UvEventDispatcher);
    d->check_loop_thread("process_events");

    if (d->mInProcessEvents)
    {
        // I5/S6: libuv has no nested iteration; this fires when a callback re-enters the dispatcher.
        CXXKIT_FATAL()
            << "UvEventDispatcher::process_events() re-entered from a callback — libuv has no nested iteration. "
               "Return from the callback and schedule follow-up with post(). (QtEventDispatcher permits nesting.)";
    }
    d->mInProcessEvents = true;

    d->mInterrupted.exchange(false); // last round's interrupt does not poison this round (R-B2 ruling)
    d->mHadEvents = false;

    const bool wait = flags.test_flag(EventLoop::ProcessFlag::kWaitForMoreEvents);
    uv_run(&d->mLoop, wait ? UV_RUN_ONCE : UV_RUN_NOWAIT);

    d->mInProcessEvents = false;
    // R-B2-3 refinement: a round ended by interrupt() is a kick, not a processed event — the classic
    // producer is the shell's maximumTime interrupt timer, whose whole job is ending the wait. Reporting
    // it as "events processed" would make process_events(flags, ms) return true on pure timeout.
    return d->mHadEvents && !d->mInterrupted.load();
}

void UvEventDispatcher::wake_up()
{
    CXXKIT_D(UvEventDispatcher);
    if (d->mClosed.load())
    {
        return; // I6: in-flight send during teardown is dropped, not fatal
    }
    // uv_async_send coalesces (libuv guarantee, I4) and is thread-safe — the only thread-safe uv call here.
    uv_async_send(d->mWakeAsync);
}

void UvEventDispatcher::interrupt()
{
    CXXKIT_D(UvEventDispatcher);
    d->mInterrupted.store(true);
    if (!d->mClosed.load())
    {
        uv_async_send(d->mWakeAsync); // kick the loop out of a blocking UV_RUN_ONCE poll
    }
}

void UvEventDispatcher::start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn)
{
    CXXKIT_D(UvEventDispatcher);
    d->check_loop_thread("start_timer");
    CXXKIT_CHECK(fn != nullptr) << "UvEventDispatcher::start_timer requires a callable";
    if (d->mClosed.load())
    {
        return; // tearing down: no new handles
    }

    // Duplicate id: replace the old registration (shell ids are unique, defensive only).
    if (d->mTimers.find(timer_id) != d->mTimers.end())
    {
        stop_timer(timer_id);
    }

    uv_timer_t *handle = new uv_timer_t;
    TimerCallback *cb = new TimerCallback();
    cb->mFn = std::move(fn);
    cb->mTimerId = timer_id;
    handle->data = cb;

    const int init_rc = uv_timer_init(&d->mLoop, handle);
    CXXKIT_CHECK(init_rc == 0) << "UvEventDispatcher::start_timer: uv_timer_init failed (" << init_rc << ")";

    // R-B2-4: repeat is always interval_ms. The shell passes repeating timers through unchanged (periodicity
    // lives here), and shell-wraps one-shot timers as "stop_timer(id); fn();" — the timer is stopped before
    // the callback runs, so a repeating re-arm can never fire a second time. Uniform engine, shell semantics.
    const int start_rc = uv_timer_start(handle, &UvEventDispatcherPrivate::on_timer_expired, interval_ms, interval_ms);
    CXXKIT_CHECK(start_rc == 0) << "UvEventDispatcher::start_timer: uv_timer_start failed (" << start_rc << ")";

    d->mTimers[timer_id] = handle;
}

void UvEventDispatcher::stop_timer(int timer_id)
{
    CXXKIT_D(UvEventDispatcher);
    d->check_loop_thread("stop_timer");

    std::map<int, uv_timer_t *>::iterator it = d->mTimers.find(timer_id);
    if (it == d->mTimers.end())
    {
        return; // unknown id is a no-op (P2-3)
    }
    uv_timer_t *handle = it->second;
    d->mTimers.erase(it);
    uv_timer_stop(handle);
    // uv_close is asynchronous: the close callback (delete fn + delete handle) runs on a later loop round.
    // Calling stop from inside a timer callback is legal (I3) — the current callback finishes first.
    if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(handle)))
    {
        uv_close(reinterpret_cast<uv_handle_t *>(handle), &UvEventDispatcherPrivate::on_handle_closed);
    }
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
