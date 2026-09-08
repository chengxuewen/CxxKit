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

#include <cxxkit/uv/uv_event_dispatcher.hpp>

#include <cxxkit/base/macros.hpp>

#include <cxxkit/3rdparty/libuv/uv.h>

#include <atomic>
#include <functional>
#include <map>
#include <thread>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/** @brief Private implementation of @ref UvEventDispatcher (CXXKIT_DEFINE_DPTR pimpl partner; flat namespace
 *  per project convention — same layout as kernel's detail/event_loop_p.hpp).
 */
class UvEventDispatcherPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(UvEventDispatcherPrivate)
public:
    explicit UvEventDispatcherPrivate(UvEventDispatcher *p);
    ~UvEventDispatcherPrivate();

    /** @brief uv C callbacks (static trampolines) — handle->data routes back to C++ state. */
    static void on_wake_async(uv_async_t *handle);
    static void on_timer_expired(uv_timer_t *handle);
    static void on_poll_ready(uv_poll_t *handle, int status, int events);
    static void on_handle_closed(uv_handle_t *handle);
    static void on_poll_closed(uv_handle_t *handle);

    void check_loop_thread(const char *api) const;

    uv_loop_t mLoop;                       /// owned by value; uv_loop_init/uv_loop_close manage it
    uv_async_t *mWakeAsync{nullptr};       /// doorbell handle (I4); empty-body callback (R-B2-2)
    std::map<int, uv_timer_t *> mTimers;   /// timer_id -> live uv_timer_t (heap-allocated)
    std::atomic<bool> mInterrupted{false}; /// set by interrupt(), consumed at process_events entry
    std::atomic<bool> mClosed{false};      /// destructor guard: reject in-flight uv_async_send (I6)
    bool mHadEvents{false};                /// set by timer/async callbacks; approximates uv_run activity (R-B2-3)
    bool mInProcessEvents{false};          /// re-entrancy latch for the nesting fatal (I5)
    std::thread::id mLoopThreadId;         /// the constructing thread; loop-thread-only APIs compare against it (I1)

    /** @brief Per-fd poll state (phase-2): handle + callback + current interests, keyed by fd in mPolls. */
    struct PollEntry
    {
        uv_poll_t *mHandle{nullptr};                                       /// heap cell; freed in on_poll_closed
        std::function<void(AbstractEventDispatcher::SocketEventMask)> mFn; /// F8-①: copied before invocation
        AbstractEventDispatcher::SocketEventMask mMask{AbstractEventDispatcher::SocketEventMask::kRead}; /// interests
    };

    std::map<int, PollEntry> mPolls; /// fd -> live poll registration (phase-2 socket notifier)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
