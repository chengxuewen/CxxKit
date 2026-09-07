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

#include <cxxkit/qt/qt_global.hpp>

#include <cxxkit/kernel/abstract_event_dispatcher.hpp>

#include <functional>

#if CXXKIT_FEATURE_ENABLE_KERNEL

// Qt's QObject lives in the global namespace — forward-declared there so the public header stays Qt-include-free
// (D2: consumers do not need Qt headers; the pimpl carries all Qt state).
class QObject;

CXXKIT_BEGIN_NAMESPACE

class QtEventDispatcherPrivate;

/**
 * @brief Event-loop driver bridging an @ref EventLoop onto a host Qt event loop (opt-in @c cxxkit::qt sublibrary).
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
class CXXKIT_QT_API QtEventDispatcher : public AbstractEventDispatcher
{
public:
    /**
     * @brief Creates the dispatcher bound to @p context (null → @c QCoreApplication::instance()).
     *
     * A null context with no application instance is a fatal error. The context's thread becomes the loop
     * thread. The context is held via QPointer — doorbells after its destruction are dropped (F8-Qt).
     */
    explicit QtEventDispatcher(QObject *context = nullptr);

    /** @brief Stops and deleteLater's every QTimer, then drops the QPointer (I6). Context thread only. */
    ~QtEventDispatcher() override;

    bool process_events(EventLoop::ProcessFlags flags) override;
    void wake_up() override;
    void interrupt() override;
    void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) override;
    void stop_timer(int timer_id) override;

private:
    CXXKIT_DECLARE_PRIVATE(QtEventDispatcher)
    CXXKIT_DEFINE_DPTR(QtEventDispatcher)
    CXXKIT_DISABLE_COPY_MOVE(QtEventDispatcher)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
