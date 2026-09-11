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

#pragma once

// Header-only Qt host bridge (D39): QtEventDispatcher + its private implementation + the
// make_qt_dispatcher factory collapsed into one self-contained header. Including this header
// REQUIRES Qt headers on the include path and linking Qt6::Core in the consuming target —
// the kernel itself has zero Qt build-time dependency (dormant unless included).

#include <cxxkit/kernel/kernel_global.hpp>

#include <cxxkit/kernel/abstract_event_dispatcher.hpp>

#include <cxxkit/tools/checks.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <atomic>
#include <functional>
#include <limits>
#include <map>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Private implementation of @ref QtEventDispatcher (value member, D39 single-header form).
 */
class QtEventDispatcherPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(QtEventDispatcherPrivate)

public:
    explicit QtEventDispatcherPrivate(QObject *context)
        : mContext(context)
    {
    }
    /// Null → @c QCoreApplication::instance(); the CXXKIT_CHECK lives here so the public constructor
    /// stays a pure member-init list (one resolution point shared with the factory).
    static QObject *resolve_context(QObject *context)
    {
        if (context == nullptr)
        {
            context = QCoreApplication::instance();
        }
        CXXKIT_CHECK(context != nullptr) << "QtEventDispatcher: no context object and no QCoreApplication instance — "
                                            "create the application first or pass an explicit QObject context";
        return context;
    }

    inline void check_loop_thread(const char *api) const
    {
        // I1/R-B2-5: always-on, not debug-only. QPointer null (context destroyed) → skip: there is no
        // thread left to compare against and no state left to protect (I6 declaration-order contract).
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

    QPointer<QObject> mContext;      /// host context; goes null after its destruction (F8-Qt doorbell guard)
    std::map<int, QTimer *> mTimers; /// timer_id -> live QTimer (heap-allocated, deleteLater'd)
    std::atomic<bool> mBellPending{
        false}; /// doorbell coalescing: set by wake_up, cleared at process_events entry (R-C1-6)
    std::atomic<bool> mInterrupted{
        false}; /// set by interrupt(); near no-op under the pass-through shape (R-C1-8). Retained on purpose: no consumer after the honest blocking downgrade — kept for a future blocking form / diagnostics extension.
};

/**
 * @brief Event-loop driver bridging an @ref EventLoop onto a host Qt event loop (header-only host bridge,
 *        @c cxxkit/kernel/qt_dispatcher.hpp).
 *
 * Implements the five @ref AbstractEventDispatcher operations over a host-side @c QObject context (default
 * @c QCoreApplication::instance()):
 * - @c process_events non-blocking is a @c QCoreApplication::processEvents() pass-through; it also clears the
 *   pending-bell flag (return value approximation, R-C1-7). @c kWaitForMoreEvents is an honest downgrade: the
 *   pass-through still does not block (blocking waits are the host loop's job — app.exec() or a bridge QTimer
 *   cadence owns the rhythm; R-C1-8).
 * - @c wake_up posts a queued @c QMetaObject::invokeMethod doorbell (empty body — the wake-up effect is the
 *   posted control event itself). A pending-flag test-and-set coalesces doorbells (R-C1-6): a wake storm posts
 *   at most one queued event until the next process_events clears the flag.
 * - @c start_timer gives each id its own repeating @c QTimer (host is the first-class source; one-shot is a
 *   shell-level wrapper). @c stop_timer stops + deleteLater's the timer.
 * - @c interrupt sets a flag; under the pass-through shape it is a near no-op (host loop owns return timing).
 *
 * Embedded form (spec §5.2): the host runs @c app.exec(); timers fire natively through it. The shell's own post
 * queue is drained only when someone calls @c EventLoop::process_events — embed a bridge QTimer that pumps
 * @c loop.process_events(kAllEvents) on a short cadence (documented constraint of the Qt bridge, R-C1-5).
 *
 * Threading (I1): construct, @c process_events, @c start_timer, @c stop_timer and destruction are loop-thread
 * only — enforced by comparing @c QThread::currentThread() with the context's thread() (R-B2-5; violations are
 * fatal bugs, not diagnostics). @c wake_up and @c interrupt are thread-safe. After the context object is
 * destroyed the QPointer goes null; doorbells are dropped and the thread assertion is skipped (F8-Qt).
 *
 * Nesting (I5): Qt event loops nest natively — re-entering @c process_events from a callback is allowed
 * (unlike UvEventDispatcher, which is fatal there).
 *
 * Lifecycle (I6): declaration-order contract — @b the dispatcher must be destroyed @b before its context object
 * (declare the context first, or scope the loop above it). The destructor stops and deleteLater's every QTimer;
 * it must run on the context thread or after the application has quit.
 *
 * Qt compatibility: requires Qt ≥ 5.10 for the functor overload of @c QMetaObject::invokeMethod (6 verified;
 * 5.10+ is a documentation-level claim, not verified).
 */
class QtEventDispatcher : public AbstractEventDispatcher
{
public:
    /**
     * @brief Creates the dispatcher bound to @p context (null → @c QCoreApplication::instance()).
     *
     * A null context with no application instance is a fatal error. The context's thread becomes the loop
     * thread. The context is held via QPointer — doorbells after its destruction are dropped (F8-Qt).
     */
    inline explicit QtEventDispatcher(QObject *context = nullptr)
        : mD(QtEventDispatcherPrivate::resolve_context(context))
    {
    }

    /** @brief Stops and deleteLater's every QTimer, then drops the QPointer (I6). Context thread only. */
    inline ~QtEventDispatcher() override
    {
        // I6: stop + deleteLater every timer, then drop the QPointer. Declaration-order contract: this
        // destructor must run before the context object's (see the header comment). The loop-thread check is
        // skipped here — after app quit the context may already be mid-destruction.
        if (!mD.mContext.isNull())
        {
            std::map<int, QTimer *> timers;
            mD.mTimers.swap(timers);
            for (std::map<int, QTimer *>::iterator it = timers.begin(); it != timers.end(); ++it)
            {
                it->second->stop();
                it->second->deleteLater();
            }
        }
        // Context already destroyed (unsupported order): Qt deleted the child timers with it — touching the
        // map entries would be UB; drop them untouched (F8-Qt safety net on top of the I6 contract).
        mD.mTimers.clear();
        mD.mContext.clear();
    }

    inline bool process_events(EventLoop::ProcessFlags flags) override
    {
        mD.check_loop_thread("process_events");

        // R-C1-6: entering a round clears the pending-bell flag — a new wake_up can arm the next doorbell.
        // R-C1-7: Qt does not report "how many events were processed"; the cleared bell flag is the closest
        // honest approximation of activity the bridge can observe (imprecise, documented).
        const bool had_bell = mD.mBellPending.exchange(false);
        CXXKIT_UNUSED(
            flags); // kWaitForMoreEvents is an honest non-blocking downgrade (R-C1-8): host exec owns blocking.
        if (!mD.mContext.isNull())
        {
            QCoreApplication::processEvents();
        }
        // Nesting (I5) is permitted: Qt event loops nest natively, no re-entrancy latch (contrast UvEventDispatcher).
        return had_bell;
    }

    inline void wake_up() override
    {
        // F8-Qt: context destroyed → QPointer null → doorbell dropped, not fatal.
        if (mD.mContext.isNull())
        {
            return;
        }
        // R-C1-6 doorbell coalescing: if a bell is already pending, skip the invokeMethod — the queued control
        // event will kick the host loop all the same. The flag clears at the next process_events entry.
        if (mD.mBellPending.exchange(true)) // atomic<bool], not atomic_flag: exchange is the C++11 coalescing form
        {
            return;
        }
        // S10 shape: empty-body functor (Qt ≥ 5.10). The doorbell's only job is the posted control event — it
        // wakes the host loop out of its wait; draining posted tasks is the EventLoop shell's job (its
        // process_events drains the post queue at its own entry).
        QPointer<QObject> guard(mD.mContext);
        QMetaObject::invokeMethod(
            mD.mContext,
            [guard]()
            {
                if (guard.isNull())
                {
                    return; // context destroyed between queueing and delivery
                }
            },
            Qt::QueuedConnection);
    }

    inline void interrupt() override
    {
        mD.mInterrupted.store(true);
        // Near no-op under the pass-through shape (R-C1-8): process_events does not block, so there is no
        // poll to kick. A doorbell keeps semantics aligned with the uv engine (wake the host if it is
        // waiting elsewhere).
        wake_up();
    }

    inline void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) override
    {
        mD.check_loop_thread("start_timer");
        CXXKIT_CHECK(fn != nullptr) << "QtEventDispatcher::start_timer requires a callable";
        CXXKIT_CHECK(!mD.mContext.isNull()) << "QtEventDispatcher::start_timer: context object already destroyed";

        // Duplicate id: replace the old registration (shell ids are unique, defensive only).
        if (mD.mTimers.find(timer_id) != mD.mTimers.end())
        {
            stop_timer(timer_id);
        }

        QTimer *timer = new QTimer(mD.mContext); // parented to the context: host reaps it even if we never do
        // R-B2-4 shape: repeat is always interval_ms. The shell passes repeating timers through unchanged
        // (periodicity lives here) and shell-wraps one-shot timers as "stop_timer(id); fn();" — stop_timer
        // erases the map entry and deleteLater's, so a second fire is impossible. Uniform engine, shell semantics.
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
        mD.mTimers[timer_id] = timer;
    }

    inline void stop_timer(int timer_id) override
    {
        mD.check_loop_thread("stop_timer");
        // I6 defense line: after the context died (unsupported order) the child QTimers died with it —
        // refuse instead of touching dangling pointers (mirrors start_timer's guard).
        CXXKIT_CHECK(!mD.mContext.isNull()) << "QtEventDispatcher::stop_timer: context object already destroyed";
        std::map<int, QTimer *>::iterator it = mD.mTimers.find(timer_id);
        if (it == mD.mTimers.end())
        {
            return; // unknown id is a no-op (P2-3)
        }
        QTimer *timer = it->second;
        mD.mTimers.erase(it);
        timer->stop();
        // deleteLater is asynchronous: safe when called from inside the timer's own timeout callback (I3) —
        // the current callback finishes first.
        timer->deleteLater();
    }

    /**
     * @brief Phase-2 socket notifier is NOT implemented on the Qt bridge. @since 0.2
     *
     * Explicit override of the base fatal default (E1) so the refusal is visible in this class, not
     * inherited: the Qt bridge has no QSocketNotifier integration yet — for readiness IO use
     * UvEventDispatcher; for file IO use cxxkit thread_pool instead.
     */
    inline void register_socket_notifier(int fd, SocketEventMask mask, std::function<void(SocketEventMask)> fn) override
    {
        CXXKIT_UNUSED(fd);
        CXXKIT_UNUSED(mask);
        CXXKIT_UNUSED(fn);
        // E1 (explicit override of the base fatal default): the Qt bridge has no QSocketNotifier integration
        // yet. Fail-loud instead of a silent no-op — an fd that never becomes ready is much harder to debug.
        CXXKIT_CHECK(false) << "QtEventDispatcher::register_socket_notifier: not supported by this dispatcher "
                               "(phase-2 feature; UvEventDispatcher implements it). For file IO use cxxkit thread_pool "
                               "instead.";
    }

    /** @brief No-op mirror of the base default: unregistering an fd the bridge never registered is not a bug. */
    inline void unregister_socket_notifier(int fd) override
    {
        CXXKIT_UNUSED(fd); // silent no-op mirror of the base default (P2-3: unknown fd is not a bug)
    }

private:
    QtEventDispatcherPrivate mD;
    CXXKIT_DISABLE_COPY_MOVE(QtEventDispatcher)
};

/**
 * @brief Creates a @ref QtEventDispatcher as its @ref AbstractEventDispatcher interface (kernel @c
 *        make_default_dispatcher mirror).
 *
 * Flat cxxkit factory name (D12): @c make_qt_dispatcher, not @c make_default — kernel depends on no Qt
 * build-time wiring; consumers opt in by including this header, linking Qt6::Core in their target, and
 * injecting the result into @c EventLoop(std::unique_ptr<AbstractEventDispatcher>).
 *
 * @param context Host @c QObject context (null → @c QCoreApplication::instance()).
 * @since 0.2
 */
inline std::unique_ptr<AbstractEventDispatcher> make_qt_dispatcher(QObject *context = nullptr)
{
    return std::unique_ptr<AbstractEventDispatcher>(new QtEventDispatcher(context));
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
