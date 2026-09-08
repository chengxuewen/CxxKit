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
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#include <cxxkit/qt/detail/qt_event_dispatcher_p.hpp>
#include <cxxkit/qt/qt_event_dispatcher.hpp>

#include <cxxkit/tools/checks.hpp>

#include <limits>


#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

QtEventDispatcherPrivate::QtEventDispatcherPrivate(QtEventDispatcher *p, QObject *context)
    : mContext(context)
{
    CXXKIT_UNUSED(p); // no p-pointer member; the public class reaches this pimpl via CXXKIT_DEFINE_DPTR only
}

void QtEventDispatcherPrivate::check_loop_thread(const char *api) const
{
    // I1/R-B2-5: always-on, not debug-only. QPointer null (context destroyed) → skip: there is no thread left
    // to compare against and no state left to protect (I6 declaration-order contract).
    if (mContext.isNull())
    {
        return;
    }
    if (QThread::currentThread() != mContext->thread())
    {
        CXXKIT_FATAL() << "QtEventDispatcher::" << api
                       << " called from a non-context thread (context lives in another QThread). Use "
                          "EventLoop::post() from other threads instead.";
    }
}

QtEventDispatcher::QtEventDispatcher(QObject *context)
    : AbstractEventDispatcher()
{
    if (context == nullptr)
    {
        context = QCoreApplication::instance();
    }
    CXXKIT_CHECK(context != nullptr) << "QtEventDispatcher: no context object and no QCoreApplication instance — "
                                        "create the application first or pass an explicit QObject context";
    mDPtr.reset(new QtEventDispatcherPrivate(this, context));
}

QtEventDispatcher::~QtEventDispatcher()
{
    // I6: stop + deleteLater every timer, then drop the QPointer. Declaration-order contract: this destructor
    // must run before the context object's (see the header comment). The loop-thread check is skipped here —
    // after app quit the context may already be mid-destruction.
    CXXKIT_D(QtEventDispatcher);
    if (!d->mContext.isNull())
    {
        std::map<int, QTimer *> timers;
        d->mTimers.swap(timers);
        for (std::map<int, QTimer *>::iterator it = timers.begin(); it != timers.end(); ++it)
        {
            it->second->stop();
            it->second->deleteLater();
        }
    }
    // Context already destroyed (unsupported order): Qt deleted the child timers with it — touching the
    // map entries would be UB; drop them untouched (F8-Qt safety net on top of the I6 contract).
    d->mTimers.clear();
    d->mContext.clear();
}

bool QtEventDispatcher::process_events(EventLoop::ProcessFlags flags)
{
    CXXKIT_D(QtEventDispatcher);
    d->check_loop_thread("process_events");

    // R-C1-6: entering a round clears the pending-bell flag — a new wake_up can arm the next doorbell.
    // R-C1-7: Qt does not report "how many events were processed"; the cleared bell flag is the closest
    // honest approximation of activity the bridge can observe (imprecise, documented).
    const bool had_bell = d->mBellPending.exchange(false);
    CXXKIT_UNUSED(flags); // kWaitForMoreEvents is an honest non-blocking downgrade (R-C1-8): host exec owns blocking.
    if (!d->mContext.isNull())
    {
        QCoreApplication::processEvents();
    }
    // Nesting (I5) is permitted: Qt event loops nest natively, no re-entrancy latch (contrast UvEventDispatcher).
    return had_bell;
}

void QtEventDispatcher::wake_up()
{
    CXXKIT_D(QtEventDispatcher);
    // F8-Qt: context destroyed → QPointer null → doorbell dropped, not fatal.
    if (d->mContext.isNull())
    {
        return;
    }
    // R-C1-6 doorbell coalescing: if a bell is already pending, skip the invokeMethod — the queued control
    // event will kick the host loop all the same. The flag clears at the next process_events entry.
    if (d->mBellPending.exchange(true)) // atomic<bool], not atomic_flag: exchange is the C++11 coalescing form
    {
        return;
    }
    // S10 shape: empty-body functor (Qt ≥ 5.10). The doorbell's only job is the posted control event — it
    // wakes the host loop out of its wait; draining posted tasks is the EventLoop shell's job (its
    // process_events drains the post queue at its own entry).
    QPointer<QObject> guard(d->mContext);
    QMetaObject::invokeMethod(
        d->mContext,
        [guard]()
        {
            if (guard.isNull())
            {
                return; // context destroyed between queueing and delivery
            }
        },
        Qt::QueuedConnection);
}

void QtEventDispatcher::interrupt()
{
    CXXKIT_D(QtEventDispatcher);
    d->mInterrupted.store(true);
    // Near no-op under the pass-through shape (R-C1-8): process_events does not block, so there is no poll to
    // kick. A doorbell keeps semantics aligned with the uv engine (wake the host if it is waiting elsewhere).
    wake_up();
}

void QtEventDispatcher::start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn)
{
    CXXKIT_D(QtEventDispatcher);
    d->check_loop_thread("start_timer");
    CXXKIT_CHECK(fn != nullptr) << "QtEventDispatcher::start_timer requires a callable";
    CXXKIT_CHECK(!d->mContext.isNull()) << "QtEventDispatcher::start_timer: context object already destroyed";

    // Duplicate id: replace the old registration (shell ids are unique, defensive only).
    if (d->mTimers.find(timer_id) != d->mTimers.end())
    {
        stop_timer(timer_id);
    }

    QTimer *timer = new QTimer(d->mContext); // parented to the context: host reaps it even if we never do
    // R-B2-4 shape: repeat is always interval_ms. The shell passes repeating timers through unchanged
    // (periodicity lives here) and shell-wraps one-shot timers as "stop_timer(id); fn();" — stop_timer erases
    // the map entry and deleteLater's, so a second fire is impossible. Uniform engine, shell semantics.
    timer->setSingleShot(false);
    CXXKIT_CHECK(interval_ms <= static_cast<uint64_t>(std::numeric_limits<int>::max()))
        << "QtEventDispatcher: interval too large (ms > INT_MAX)";
    timer->setInterval(static_cast<int>(interval_ms));
    QObject::connect(timer,
                     &QTimer::timeout,
                     [fn]()
                     {
                         fn(); // no stop here: one-shot wrappers arrive via the shell's stop-first path
                     });
    timer->start();
    d->mTimers[timer_id] = timer;
}

void QtEventDispatcher::stop_timer(int timer_id)
{
    CXXKIT_D(QtEventDispatcher);
    d->check_loop_thread("stop_timer");
    // I6 defense line: after the context died (unsupported order) the child QTimers died with it —
    // refuse instead of touching dangling pointers (mirrors start_timer's guard).
    CXXKIT_CHECK(!d->mContext.isNull()) << "QtEventDispatcher::stop_timer: context object already destroyed";
    std::map<int, QTimer *>::iterator it = d->mTimers.find(timer_id);
    if (it == d->mTimers.end())
    {
        return; // unknown id is a no-op (P2-3)
    }
    QTimer *timer = it->second;
    d->mTimers.erase(it);
    timer->stop();
    // deleteLater is asynchronous: safe when called from inside the timer's own timeout callback (I3) —
    // the current callback finishes first.
    timer->deleteLater();
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
