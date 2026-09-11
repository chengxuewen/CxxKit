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
#include <cxxkit/kernel/application.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/event.hpp>
#include "fake_dispatcher.hpp"
#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL
using cxxkit::FakeDispatcher;

#    include <functional>
#    include <memory>
#    include <vector>

namespace
{
// Factory mirroring the T8 EventLoopThread shape: produces a fresh FakeDispatcher per call.
std::unique_ptr<cxxkit::AbstractEventDispatcher> fake_factory()
{
    return std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher);
}

// Recording receiver: logs event types (custom_event path).
class RecordingTarget : public cxxkit::Object
{
public:
    std::vector<cxxkit::Event::Type> events;
    int event_calls{0};

protected:
    bool event(cxxkit::Event *e) override
    {
        ++event_calls;
        if (e->type() >= cxxkit::Event::Type::kUser)
        {
            events.push_back(e->type());
        }
        return Object::event(e);
    }
};

// Plain observing filter (records watched types, never intercepts).
class RecordingFilter : public cxxkit::Object
{
public:
    std::vector<cxxkit::Event::Type> types;
    bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
    {
        CXXKIT_UNUSED(watched);
        types.push_back(event->type());
        return false;
    }
};

// Intercepting filter: swallows everything (returns true).
class InterceptingFilter : public cxxkit::Object
{
public:
    int calls{0};
    bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
    {
        CXXKIT_UNUSED(watched);
        CXXKIT_UNUSED(event);
        ++calls;
        return true; // swallow user events; other types pass through (kThreadChange etc.)
    }
};

// Notify-counting Application: counts deliveries, keeps default behavior.
class CountingApp : public cxxkit::Application
{
public:
    explicit CountingApp(DispatcherFactory factory, cxxkit::Object *parent = nullptr)
        : Application(factory, parent)
    {
    }
    int notify_calls{0};
    std::vector<cxxkit::Event::Type> notified_types;
    bool notify(cxxkit::Object *receiver, cxxkit::Event *event) override
    {
        ++notify_calls;
        notified_types.push_back(event->type());
        return Application::notify(receiver, event);
    }
};
} // namespace

// (a) no-App baseline: covered by the 82 pre-existing suites — with no Application alive,
// Object::send_event takes the instance()==null short-circuit (byte-identical legacy path).

// (b) instance lifecycle
TEST(Application, instance_lifecycle_null_to_ptr_to_null)
{
    EXPECT_EQ(cxxkit::Application::instance(), nullptr);
    {
        cxxkit::Application app(&fake_factory);
        EXPECT_EQ(cxxkit::Application::instance(), &app);
    }
    EXPECT_EQ(cxxkit::Application::instance(), nullptr);
}

// (c) double construction is fatal (qApp semantics)
TEST(ApplicationDeathTest, double_construction_is_fatal)
{
    cxxkit::Application app(&fake_factory);
    EXPECT_DEATH(cxxkit::Application second(&fake_factory), "");
}

// (d) notify override observes queued delivery
TEST(Application, notify_override_observes_posted_events)
{
    CountingApp app(&fake_factory);
    RecordingTarget target;
    cxxkit::EventLoop *loop = app.loop();
    target.move_to_thread(loop);

    loop->post(
        [&]
        {
            cxxkit::Object::post_event(&target, new cxxkit::Event(cxxkit::Event::Type::kUser));
            loop->exit(0); // PIT-43: same-round exit chaining
        });
    EXPECT_EQ(app.exec(), 0);

    EXPECT_EQ(app.notify_calls, 1); // funnel saw exactly one delivery
    ASSERT_EQ(app.notified_types.size(), 1u);
    EXPECT_EQ(app.notified_types[0], cxxkit::Event::Type::kUser);
    ASSERT_EQ(target.events.size(), 1u); // and the core path still delivered
    EXPECT_EQ(target.events[0], cxxkit::Event::Type::kUser);
}

// (e) a filter installed ON the app is a GLOBAL filter: intercepts ANY receiver
TEST(Application, global_filter_on_app_intercepts_any_receiver)
{
    cxxkit::Application app(&fake_factory);
    InterceptingFilter global_filter;
    app.install_event_filter(&global_filter);
    RecordingTarget a;
    RecordingTarget b;
    cxxkit::EventLoop *loop = app.loop();
    a.move_to_thread(loop);
    b.move_to_thread(loop);

    loop->post(
        [&]
        {
            cxxkit::Object::post_event(&a, new cxxkit::Event(cxxkit::Event::Type::kUser));
            cxxkit::Object::post_event(&b, new cxxkit::Event(cxxkit::Event::Type::kUser));
            loop->exit(0);
        });
    EXPECT_EQ(app.exec(), 0);

    // Both deliveries intercepted before reaching any receiver's event()
    EXPECT_EQ(global_filter.calls, 2); // one per queued kUser event
    EXPECT_TRUE(a.events.empty());
    EXPECT_TRUE(b.events.empty());
    EXPECT_EQ(a.event_calls, 1); // only kThreadChange (direct, pre-filter-install)
    EXPECT_EQ(b.event_calls, 1);
}

// (f) order: app (global) filters run BEFORE receiver filters
TEST(Application, app_filter_runs_before_receiver_filter)
{
    cxxkit::Application app(&fake_factory);
    RecordingFilter app_filter;
    RecordingFilter receiver_filter;
    app.install_event_filter(&app_filter);
    RecordingTarget target;
    target.install_event_filter(&receiver_filter);
    cxxkit::EventLoop *loop = app.loop();
    target.move_to_thread(loop);

    loop->post(
        [&]
        {
            cxxkit::Object::post_event(&target, new cxxkit::Event(cxxkit::Event::Type::kUser));
            loop->exit(0);
        });
    EXPECT_EQ(app.exec(), 0);

    ASSERT_EQ(app_filter.types.size(), 1u);      // global chain saw it FIRST
    ASSERT_EQ(receiver_filter.types.size(), 1u); // then the receiver chain
    EXPECT_EQ(app_filter.types[0], cxxkit::Event::Type::kUser);
    EXPECT_EQ(receiver_filter.types[0], cxxkit::Event::Type::kUser);
    ASSERT_EQ(target.events.size(), 1u); // neither intercepted — delivered
}

// (g) exec/quit main loop: quit() from a posted task stops exec() with code 0
TEST(Application, exec_returns_after_quit_from_posted_task)
{
    cxxkit::Application app(&fake_factory);
    cxxkit::EventLoop *loop = app.loop();
    loop->post([&]() { app.quit(); }); // thread-safe exit (D35 I1) — same thread here
    EXPECT_EQ(app.exec(), 0);
}

// (h) funneled queue dispatch: notify called exactly once per event, event() once
TEST(Application, queue_dispatch_funneled_exactly_once_per_event)
{
    CountingApp app(&fake_factory);
    RecordingTarget target;
    cxxkit::EventLoop *loop = app.loop();
    target.move_to_thread(loop);

    loop->post(
        [&]
        {
            cxxkit::Object::post_event(&target, new cxxkit::Event(cxxkit::Event::Type::kUser));
            cxxkit::Object::post_event(&target, new cxxkit::Event(cxxkit::Event::Type::kUser));
            loop->exit(0);
        });
    EXPECT_EQ(app.exec(), 0);

    EXPECT_EQ(app.notify_calls, 2);      // one funnel pass per queued event
    ASSERT_EQ(target.event_calls, 3);    // 2 queued kUser + 1 kThreadChange (direct event()
    EXPECT_EQ(target.events.size(), 2u); // during move_to_thread — predates the funnel)
    EXPECT_EQ(target.events[0], cxxkit::Event::Type::kUser);
    EXPECT_EQ(target.events[1], cxxkit::Event::Type::kUser);
}

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
