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

#include <cxxkit/thread/thread_global.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/object.hpp>

#include <functional>
#include <memory>

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Runs an owned @ref EventLoop on a dedicated @ref PlatformThread (composition, Qt
 * QEventLoopThread analog).
 *
 * The dispatcher is created by @p factory (the @c make_uv_dispatcher / @c make_qt_dispatcher
 * convention — a plain function pointer) at construction time, so the loop exists and can
 * accept affinity binding BEFORE start() is called:
 *
 * @code
 * cxxkit::EventLoopThread elt(&cxxkit::make_uv_dispatcher);
 * worker.move_to_thread(&elt.loop()); // static migration: the loop is not running yet
 * elt.start();
 * @endcode
 *
 * The loop is a plain (parentless) member — it is NOT a child object, so destruction order
 * is deterministic: the derived destructor joins the thread first, then the loop is destroyed,
 * then the Object base. stop() requests the loop to exit from any thread (thread-safe exit +
 * wake contract) and joins. Destroying a running thread stops it implicitly.
 *
 * @since 0.2
 */
class EventLoopThreadPrivate;
class CXXKIT_THREAD_API EventLoopThread final : public Object
{
public:
    /** @brief Creates the loop's dispatcher (the make_uv_dispatcher / make_qt_dispatcher shape). */
    typedef std::unique_ptr<AbstractEventDispatcher> (*DispatcherFactory)();

    /**
     * @brief Constructs the thread and its loop.
     *
     * The factory is invoked exactly once; a null product is fatal (CXXKIT_CHECK).
     */
    explicit EventLoopThread(DispatcherFactory factory, Object *parent = nullptr);

    /** @brief Stops the thread if it is still running, then destroys the loop. */
    ~EventLoopThread() override;

    /** @brief The loop running on the dedicated thread (constructed in the ctor; never null).
     *  Valid for affinity binding immediately — before start(). */
    EventLoop &loop();

    /** @brief Starts the worker thread (it runs loop().exec()). Double start is fatal
     *  (CXXKIT_CHECK). */
    void start();

    /** @brief Thread-safe: exits the loop and joins the worker. Not started = no-op.
     *  Idempotent. */
    void stop();

    /** @brief True while the worker thread is alive. */
    bool is_running() const;

private:
    CXXKIT_DEFINE_DPTR(EventLoopThread)
    CXXKIT_DISABLE_COPY_MOVE(EventLoopThread)

    std::unique_ptr<EventLoop> mLoop; // PLAIN MEMBER, no parent (Momus F4): no double
                                      // ownership, no ChildEvent noise; declared after
                                      // mDPtr so it outlives the private state in ~
};

CXXKIT_END_NAMESPACE