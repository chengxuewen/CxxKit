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

#include <cxxkit/kernel/detail/object_p.hpp>
#include <cxxkit/kernel/application.hpp>

#include <cxxkit/kernel/abstract_event_dispatcher.hpp>
#include <cxxkit/tools/checks.hpp>

#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace
{
Application *s_instance = nullptr; // set/cleared by ctor/dtor only; construction is explicit
} // namespace

Application::Application(DispatcherFactory factory, Object *parent)
    : Object(parent)
{
    CXXKIT_CHECK(factory != nullptr) << "Application requires a dispatcher factory";
    // Singleton gate BEFORE anything else: a second live instance is fatal (qApp semantics).
    CXXKIT_CHECK(s_instance == nullptr) << "Application: an instance already exists (one per process)";
    s_instance = this; // published BEFORE the loop exists: the notify funnel must be live
                       // during loop construction (send_event may fire inside the factory)
    std::unique_ptr<AbstractEventDispatcher> dispatcher = factory();
    CXXKIT_CHECK(dispatcher != nullptr) << "Application: factory returned a null dispatcher";
    // Plain member, parent = nullptr (Momus F4 pattern, EventLoopThread same): no double
    // ownership, no ChildEvent noise. Declared last in the header — outlives the Object base.
    mLoop.reset(new EventLoop(std::move(dispatcher)));
}

Application::~Application()
{
    s_instance = nullptr; // cleared FIRST: everything after sees the no-App path
}

Application *Application::instance()
{
    return s_instance;
}

EventLoop *Application::loop() const
{
    return mLoop.get();
}

int Application::exec()
{
    return mLoop->exec();
}

void Application::quit()
{
    mLoop->exit(0); // thread-safe (exit + wake, D35 I1)
}

bool Application::notify(Object *receiver, Event *event)
{
    // Default funnel: the Application object's OWN filter chain = global filters (Qt's
    // qApp->installEventFilter mechanism), then the un-funneled core delivery. The call
    // routes through ObjectPrivate::deliver_via_funnel — a friend-member of Object (the
    // CXXKIT_DECLARE_PRIVATE friend grant) — so the private core stays private.
    return ObjectPrivate::deliver_via_funnel(this, receiver, event);
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
