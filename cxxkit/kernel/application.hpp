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

#pragma once

#include <cxxkit/kernel/kernel_global.hpp>

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/object.hpp>

#include <functional>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class ApplicationPrivate;

/**
 * @brief Process-wide application singleton owning the main event loop (Qt QApplication / qApp analog).
 *
 * Construction is explicit and unique: a second live instance is fatal (CXXKIT_CHECK) — the
 * qApp semantics. The singleton pointer is published BEFORE the main loop is created so the
 * notify funnel is already active during loop construction (send_event may fire from inside
 * the dispatcher factory or loop wiring).
 *
 * Event delivery funnel: every Object::send_event (including queue dispatch inside
 * EventLoop::process_events and synchronous timer ticks) routes through the live instance's
 * notify() when an Application exists, and falls back to the un-funneled core path when none
 * does — no-App behavior is byte-identical to pre-funnel code. The default notify() runs the
 * application object's OWN filter chain (objects installed via install_event_filter on the
 * Application act as GLOBAL filters, Qt's same mechanism) and then delegates to the core
 * delivery. Override notify() to observe or intercept every event delivery.
 *
 * @note The Application must outlive every kernel object that delivers events while it is
 *       alive (Qt qApp has the same constraint): after ~Application clears the singleton,
 *       send_event falls back to the core path, but a dead Application still registered as
 *       a filter on a surviving object would dangle — teardown order matters.
 * @since 0.2
 */
class CXXKIT_KERNEL_API Application : public Object
{
public:
    /** @brief Creates the main loop's dispatcher (the make_uv_dispatcher / make_qt_dispatcher shape,
     *  mirrored from EventLoopThread). */
    typedef std::function<std::unique_ptr<AbstractEventDispatcher>()> DispatcherFactory;

    /**
     * @brief Constructs the singleton and its main loop.
     *
     * A null factory or a null dispatcher product is fatal (CXXKIT_CHECK). A second live
     * Application is fatal — construct exactly one per process. The loop is created inside
     * the constructor with @p factory; it is a plain (parentless) member: no double ownership,
     * no ChildEvent noise (EventLoopThread pattern).
     */
    explicit Application(DispatcherFactory factory, Object *parent = nullptr);

    /** @brief Clears the singleton first, then destroys the main loop. Must be the LAST kernel
     *  object destroyed (Qt qApp same constraint) — see the class note. */
    ~Application() override;

    /** @brief Returns the live Application, or nullptr when none exists. Never constructs one. */
    static Application *instance();

    /** @brief The main loop (constructed in the ctor; never null). Plain member — its lifetime
     *  is the Application's own. */
    EventLoop *loop() const;

    /** @brief Runs the main loop's exec() and returns its exit code. */
    int exec();

    /** @brief Thread-safe exit request (exit(0) + wake, D35 I1 contract) forwarded to the main loop. */
    void quit();

    /**
     * @brief Delivery funnel: every send_event for a live Application passes through here.
     *
     * The default implementation runs THIS application object's own filter chain (global
     * filters, newest first) and then delegates to the un-funneled core delivery. Override
     * to observe/intercept every event delivery. Calling send_event for the same receiver
     * from inside notify() recurses — user responsibility (Qt same).
     * @since 0.2
     */
    virtual bool notify(Object *receiver, Event *event);

private:
    CXXKIT_DISABLE_COPY_MOVE(Application)

    // Kernel-internal cooperation (Momus F3 funnel): Object::send_event / notify call the
    // private send_event_internal core on Application's behalf. Sole friend.
    friend class Object;

    std::unique_ptr<EventLoop> mLoop; // PLAIN MEMBER, parent = nullptr (Momus F4 pattern):
                                      // declared last so it outlives the Object base in ~
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
