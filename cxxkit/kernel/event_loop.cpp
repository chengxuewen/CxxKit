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

#include <cxxkit/kernel/detail/event_loop_p.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/logging.hpp>

#include <chrono>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace
{
thread_local EventLoop *t_current_loop = nullptr;
} // namespace

EventLoop *EventLoop::current()
{
    return t_current_loop;
}

EventLoopPrivate::EventLoopPrivate(EventLoop *p)
    : ObjectPrivate(p)
{
}

EventLoopPrivate::~EventLoopPrivate()
{
}

EventLoop::EventLoop(std::unique_ptr<AbstractEventDispatcher> dispatcher, Object *parent)
    : Object(parent)
{
    CXXKIT_CHECK(dispatcher != nullptr) << "EventLoop requires a dispatcher";
    // The Object(parent) ctor installed a plain ObjectPrivate in mDPtr; replace it with the
    // EventLoopPrivate this class actually uses (virtual dtor keeps the unique_ptr delete safe).
    mDPtr.reset(new EventLoopPrivate(this));
    CXXKIT_D(EventLoop);
    d->mDispatcher = std::move(dispatcher);
}

EventLoop::~EventLoop()
{
    CXXKIT_D(EventLoop);
    // 析构排空：delete_later 闭包必须执行到——丢弃 = 对象泄漏。swap-under-lock、锁外逐个执行
    // （S10/I4 排空不变量）。
    // 已知限制（L1）：排空期间闭包内再 post（如级联 delete_later）会投到将死环的新队列静默丢失；
    // （L2）排空执行时 current 已不指向自身（exec-only 语义，dtor 无清理）——闭包内 delete_later
    // 会 fatal 而非静默丢。二者均文档化限制，Qt 靠 ~QObject 清 pending DeferredDelete，本版不做。
    std::deque<std::function<void()>> tasks = d->take_post_queue();
    while (!tasks.empty())
    {
        std::function<void()> fn = tasks.front();
        tasks.pop_front();
        if (fn)
        {
            fn();
        }
    }
}

bool EventLoop::is_running() const
{
    CXXKIT_D(const EventLoop);
    return !d->mExit.load() && d->mInExec;
}

void EventLoop::post(std::function<void()> fn)
{
    CXXKIT_CHECK(fn != nullptr) << "EventLoop::post requires a callable";
    CXXKIT_D(EventLoop);
    {
        std::lock_guard<std::mutex> lock(d->mPostMutex);
        d->mPostQueue.push_back(std::move(fn));
    }
    d->mDispatcher->wake_up();
}

int EventLoop::start_timer(uint64_t interval_ms, std::function<void()> fn, bool repeat)
{
    CXXKIT_CHECK(fn != nullptr) << "EventLoop::start_timer requires a callable";
    CXXKIT_D(EventLoop);
    const int timerId = d->mNextTimerId.fetch_add(1) + 1; // ids start at 1
    if (repeat)
    {
        // Zero-period repeating timer stays on the engine: fires every round (Qt semantics).
        d->mDispatcher->start_timer(timerId, interval_ms, std::move(fn));
    }
    else if (interval_ms == 0)
    {
        // Zero-interval one-shot fast path (Qt singleShotImpl precedent): delegate to the posted queue — FIFO with post(fn), no engine registration. timerId is a ghost id;
        // stop_timer on it is a harmless no-op (documented in the header).
        this->post(std::move(fn));
    }
    else
    {
        // M3 one-shot shell wrap: the callback stops itself before running, so it fires
        // exactly once even if the driver keeps the registration. The wrapper captures this —
        // the loop must outlive a pending one-shot timer (lifecycle contract, spec appendix C).
        EventLoop *self = this;
        d->mDispatcher->start_timer(timerId,
                                    interval_ms,
                                    [self, timerId, fn]
                                    {
                                        self->stop_timer(timerId);
                                        fn();
                                    });
    }
    return timerId;
}

void EventLoop::stop_timer(int timer_id)
{
    CXXKIT_D(EventLoop);
    d->mDispatcher->stop_timer(timer_id);
}

bool EventLoop::process_events(ProcessFlags flags)
{
    CXXKIT_D(EventLoop);
    // Drain posted tasks first: swap the whole queue under the lock, run the callbacks outside
    // it — re-entrant post() from a callback enqueues instead of deadlocking (S10/I4).
    std::deque<std::function<void()>> tasks = d->take_post_queue();
    if (!tasks.empty())
    {
        while (!tasks.empty())
        {
            std::function<void()> fn = tasks.front();
            tasks.pop_front();
            if (fn)
            {
                fn();
            }
        }
        return true;
    }
    return d->mDispatcher->process_events(flags);
}

bool EventLoop::process_events(ProcessFlags flags, uint64_t maximum_ms)
{
    // D10 shell synthesis: an immediate drain round first (posted work or a non-blocking
    // dispatcher poll); if nothing was processed, repeated kWaitForMoreEvents rounds bounded
    // by a monotonic deadline, with a shell-owned one-shot interrupt timer so a real blocking
    // driver returns on timeout. maximum_ms == 0 never enters the timed loop.
    if (this->process_events(flags))
    {
        return true;
    }
    if (maximum_ms == 0)
    {
        return false;
    }

    CXXKIT_D(EventLoop);
    // Exit-state capture (B2 acceptance finding): a fresh loop is born with mExit == true
    // (exec owns flipping it), so gating the timed loop on !mExit made process_events(flags, ms)
    // return immediately when called outside exec — the deadline was silently lost. The timed
    // path is bounded by the deadline and the interrupt timer; exit() short-circuits via the
    // dispatcher's interrupt round only if it fires during the wait, which is the documented
    // "return as soon as possible" semantic either way.
    const bool exitRequested = d->mExit.load() && d->mHasExitCode.load();
    // C14 discipline (flaky hang root-caused 2026-09-08): the interrupt timer MUST repeat. A one-shot
    // timer fires exactly once — if its async doorbell gets consumed by the very next UV_RUN_ONCE round
    // (pending cleared by the empty wake callback, mHadEvents still false), the following round enters
    // uv_run with no timers and an already-drained async: backend_timeout == -1 blocks in epoll forever
    // and the shell never re-checks the deadline (it only checks between rounds). A repeating timer keeps
    // a finite poll timeout armed every round until the shell stops it at the deadline.
    EventLoop *self = this;
    const int timeoutId = this->start_timer(maximum_ms, [self] { self->d_func()->mDispatcher->interrupt(); }, true);
    bool processed = false;
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
                                                           std::chrono::milliseconds(maximum_ms);
    while (!exitRequested && std::chrono::steady_clock::now() < deadline)
    {
        if (this->process_events(flags | ProcessFlag::kWaitForMoreEvents))
        {
            processed = true;
            break;
        }
        if (d->mExit.load() && d->mHasExitCode.load())
        {
            break; // exit() was called while we waited — honor it without consuming the deadline
        }
    }
    this->stop_timer(timeoutId);
    return processed;
}

int EventLoop::exec(ProcessFlags flags)
{
    CXXKIT_D(EventLoop);
    if (d->mInExec)
    {
        CXXKIT_WARNING("EventLoop::exec: instance {} has already called exec()", utils::fmt::ptr(this));
        return -1;
    }

    // exit() before exec(): the preset code is returned without running a round (shell
    // contract). mHasExitCode distinguishes a real preset exit from the never-exited fresh
    // state — mRetCode alone cannot (exit(-1) is a legal preset).
    if (d->mExit.load() && d->mHasExitCode.load())
    {
        return d->mRetCode.load();
    }

    // thread_local 当前环维护：save 在两个 early-return 之后，restore 在最终 return 前（裁定 4）
    EventLoop *previous = t_current_loop;
    t_current_loop = this;
    d->mInExec = true;
    d->mExit.store(false); // a fresh loop is born exited; exec owns the running state now
    while (!d->mExit.load())
    {
        if (this->process_events(flags | ProcessFlag::kWaitForMoreEvents | ProcessFlag::kEventLoopExec))
        {
            continue; // work was processed; loop condition re-checks mExit before the next round
        }
        // D7 checkpoint: re-check after process_events returned, before the next (blocking) round —
        // an exit() racing with wake_up must not leave the loop blocked or spinning.
        if (d->mExit.load())
        {
            break;
        }
    }
    d->mInExec = false;
    t_current_loop = previous;
    return d->mRetCode.load();
}

void EventLoop::wake_up()
{
    CXXKIT_D(EventLoop);
    d->mDispatcher->wake_up();
}

void EventLoop::exit(int retCode)
{
    CXXKIT_D(EventLoop);
    d->mRetCode.store(retCode);
    d->mHasExitCode.store(true);
    d->mExit.store(true);
    // M4: unconditionally ring the bell — a loop blocked in process_events never wakes otherwise.
    d->mDispatcher->wake_up();
}

void EventLoop::quit()
{
    this->exit(0);
}

bool EventLoop::event(Event *event)
{
    if (event->type() == Event::Type::kQuit)
    {
        this->quit();
        return true;
    }
    else
    {
        return Object::event(event);
    }
}

AbstractEventDispatcher &EventLoop::dispatcher()
{
    CXXKIT_D(EventLoop);
    return *d->mDispatcher;
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
