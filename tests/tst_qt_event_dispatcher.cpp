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

#include <cxxkit/qt/qt_event_dispatcher.hpp>

#include <cxxkit/kernel/event_loop.hpp>

#include <gtest/gtest.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include <atomic>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{

using cxxkit::EventLoop;
using cxxkit::QtEventDispatcher;

// Qt-bridge tests (C1). QCoreApplication on the stack (argc/argv locals); the embedded form's documented
// constraint (R-C1-5) is exercised literally: a bridge QTimer pumps loop.process_events(kAllEvents) on a short
// cadence while the "host loop" (here QCoreApplication::processEvents via the same pump) turns. Scenario
// numbering aligns with tst_uv_event_dispatcher where the engines are comparable.

// 1. Constructor: default context = QCoreApplication::instance(); dispatcher is usable (thread check passes
//    on the main thread), and process_events returns false when no bell was rung (R-C1-7 approximation).
TEST(QtEventDispatcherTest, DefaultContextIsAppInstance)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    EventLoop loop(std::unique_ptr<QtEventDispatcher>(new QtEventDispatcher()));
    EXPECT_FALSE(loop.process_events(EventLoop::ProcessFlag::kAllEvents));
}

// 2. post() + bridge QTimer pumping process_events: the shell queue drains and the task runs on the
//    context thread (embedded-form contract, R-C1-5 — the bridge timer is the drain authority).
TEST(QtEventDispatcherTest, PostDrainsViaBridgeTimer)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    EventLoop loop(std::unique_ptr<QtEventDispatcher>(new QtEventDispatcher()));
    std::atomic<int> ran(0);

    loop.post(
        [&]()
        {
            ran.fetch_add(1);
            loop.exit(0); // ends the pump below (process_events round observes exit)
        });

    QTimer bridge;
    bridge.setSingleShot(false);
    bridge.setInterval(5);
    QTimer::connect(&bridge, &QTimer::timeout, [&]() { loop.process_events(EventLoop::ProcessFlag::kAllEvents); });
    bridge.start();

    // Deterministic pump: spin Qt rounds until the posted task ran (event-driven, no sleeps).
    while (ran.load() == 0)
    {
        QCoreApplication::processEvents();
    }
    bridge.stop();
    EXPECT_EQ(1, ran.load());
}

// 3. Timer: 3 ticks of a repeating 5ms QTimer-driven timer (I2), then stopped — no fourth tick.
TEST(QtEventDispatcherTest, TimerThreeTicksThenStop)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    EventLoop loop(std::unique_ptr<QtEventDispatcher>(new QtEventDispatcher()));
    std::atomic<int> ticks(0);

    const int id = loop.start_timer(5, [&]() { ticks.fetch_add(1); }, true);

    while (ticks.load() < 3)
    {
        QCoreApplication::processEvents(); // drives the underlying QTimer natively (embedded form)
    }
    loop.stop_timer(id);
    const int after_stop = ticks.load();

    // Drain a few rounds: no fourth tick may arrive (generous round count, not sleeps).
    for (int i = 0; i < 50; ++i)
    {
        QCoreApplication::processEvents();
    }
    EXPECT_EQ(3, ticks.load());
    EXPECT_EQ(after_stop, 3);
}

// 4. wake_up storm ×100 self-coalesces (R-C1-6), and a post storm drains in one round: 100 posts ring at
//    most one doorbell; the shell's process_events entry swaps the whole queue — one bridge round runs all
//    100 tasks. Behavior-level assertion (mBellPending is pimpl-private): single-round drain + bell-once.
TEST(QtEventDispatcherTest, WakeAndPostStormCoalesceAndDrain)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    EventLoop loop(std::unique_ptr<QtEventDispatcher>(new QtEventDispatcher()));
    std::atomic<int> executed(0);

    for (int i = 0; i < 100; ++i)
    {
        loop.post([&executed] { executed.fetch_add(1); }); // each post rings the doorbell (thread-safe)
    }
    // Round 1: shell swaps+runs the whole 100-task queue at ITS entry and returns true without even
    // entering the dispatcher — the bell stays armed (shell drain short-circuits the dispatcher round).
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents));
    EXPECT_EQ(100, executed.load());
    // Round 2: no new posts; the dispatcher now consumes the residual armed bell + delivers the single
    // coalesced queued control event (real Qt activity) → true. Round 3: bell cleared, queue empty → false.
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents));
    EXPECT_FALSE(loop.process_events(EventLoop::ProcessFlag::kAllEvents));
}

// 5. Nesting is permitted (I5 Qt side, contrast uv's fatal): process_events from inside a timer callback
//    runs nested and returns without fatal.
TEST(QtEventDispatcherTest, NestedProcessEventsPermitted)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    EventLoop loop(std::unique_ptr<QtEventDispatcher>(new QtEventDispatcher()));
    std::atomic<int> nested(0);

    loop.start_timer(
        5,
        [&]()
        {
            // Nested round is permitted (contrast uv's fatal) and must simply return.
            loop.process_events(EventLoop::ProcessFlag::kAllEvents);
            nested.fetch_add(1);
            loop.exit(0);
        },
        true);

    while (nested.load() == 0)
    {
        QCoreApplication::processEvents();
    }
    SUCCEED();
}

// 6. Context destruction order (I6): dispatcher destroyed before context is the supported order. QPointer
//    nulls after context death; a late wake_up is dropped, not a crash (F8-Qt).
TEST(QtEventDispatcherTest, DispatcherBeforeContextAndLateWakeDropped)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    QObject *context = new QObject(); // heap so destruction order is explicit (I6)
    QtEventDispatcher *dispatcher = new QtEventDispatcher(context);
    QPointer<QObject> context_guard(context);

    {
        // Scope the loop so the dispatcher dies first (declaration-order contract, I6).
        EventLoop loop{std::unique_ptr<QtEventDispatcher>(dispatcher)}; // braces: (dispatcher) alone vexing-parses
        EXPECT_FALSE(context_guard.isNull());
        dispatcher->wake_up(); // arm a bell while everything is alive
        EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents));
    }
    // dispatcher died with the loop scope; context dies now — supported order.

    // F8-Qt late-doorbell probe: a dispatcher whose context is gone drops wake_up instead of
    // crashing. Rebuild one against a doomed context, kill the context, ring the bell.
    {
        QObject *doomed = new QObject();
        QtEventDispatcher *late = new QtEventDispatcher(doomed);
        delete doomed;   // context dies first here — the unsupported order, but QPointer is the safety net
        late->wake_up(); // must be a silent drop, not a crash
        late->interrupt();
        delete late;
    }
    EXPECT_FALSE(context_guard.isNull());
    delete context;
    EXPECT_TRUE(context_guard.isNull());
    SUCCEED();
}

// 7. interrupt() is callable and non-fatal under the pass-through shape (R-C1-8 honest downgrade).
TEST(QtEventDispatcherTest, InterruptIsNonFatalNoOp)
{
    int argc = 1;
    char arg0[] = "tst_qt_event_dispatcher";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    QtEventDispatcher *dispatcher = new QtEventDispatcher();
    EventLoop loop{std::unique_ptr<QtEventDispatcher>(dispatcher)};
    dispatcher->interrupt(); // dispatcher-level op (the shell exposes wake_up, not interrupt)
    loop.wake_up();
    EXPECT_TRUE(loop.process_events(EventLoop::ProcessFlag::kAllEvents)); // the wake armed a bell
    SUCCEED();
}

} // namespace

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
