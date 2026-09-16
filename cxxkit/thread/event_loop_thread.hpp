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
 * The dispatcher is created by @p factory (the @c make_default_dispatcher convention — a plain
 * function pointer; host bridges like the header-only @c make_qt_dispatcher also fit) ON THE
 * WORKER THREAD, inside the runner started by start() — not in the ctor (D43.5 T1). Thread-bound
 * engines (the default libuv dispatcher captures its loop thread at construction) then match
 * the exec() thread by construction, and the loop/dispatcher are also destroyed on that thread.
 *
 * @code
 * cxxkit::EventLoopThread elt(&cxxkit::make_default_dispatcher);
 * elt.start();                    // the loop is born on the worker thread
 * elt.loop().post([] { ... });    // cross-thread post; loop() blocks until the loop is ready
 * @endcode
 *
 * Consequence of worker-first construction: the loop does not exist between the ctor and start().
 * loop() blocks until the worker finishes building it and is fatal on a never-started thread.
 * Objects get ELT affinity by being created inside the loop (zero-affinity objects bind to
 * EventLoop::current()); migrating a foreign object with move_to_loop() requires a non-running
 * target loop, which a started ELT never is.
 *
 * The loop is a plain (parentless) member — it is NOT a child object. It is written and destroyed
 * only on the worker thread (under the private handoff mutex) and is already null when the
 * destructor's member destruction runs: the destructor body just stops and joins.
 * stop() requests the loop to exit from any thread (thread-safe exit + wake contract) and joins.
 * Destroying a running thread stops it implicitly.
 *
 * @since 0.2
 */
class EventLoopThreadPrivate;
class CXXKIT_THREAD_API EventLoopThread final : public Object
{
public:
    /** @brief Creates the loop's dispatcher (the make_default_dispatcher shape; host bridges
     *  inject their own factory, e.g. the header-only make_qt_dispatcher). */
    typedef std::unique_ptr<AbstractEventDispatcher> (*DispatcherFactory)();

    /**
     * @brief Constructs the thread. The factory is stored, not invoked — it runs on the
     * worker thread after start(). A null factory is fatal (CXXKIT_CHECK).
     */
    explicit EventLoopThread(DispatcherFactory factory, Object *parent = nullptr);

    /** @brief Stops the thread if it is still running; the loop was already destroyed on
     *  the worker (worker-first teardown). */
    ~EventLoopThread() override;

    /** @brief The loop running on the dedicated thread. Blocks until the worker has built
     *  it (after start()); fatal before start() or after the worker tore it down. */
    EventLoop &loop();

    /** @brief Implicit conversion to the owned loop's address. Same contract as loop():
     *  blocks until the loop is ready, fatal outside the started window.
     *  Lifetime: the ELT must outlive objects bound to its loop. */
    operator EventLoop *() const { return &const_cast<EventLoopThread *>(this)->loop(); }

    /** @brief Starts the worker thread: it builds the loop (worker-first) and runs exec().
     *  Once-only: double start AND restart after stop are fatal (CXXKIT_CHECK). */
    void start();

    /** @brief Thread-safe: exits the loop and joins the worker. Not started = no-op
     *  (worker-first: no loop exists either). Idempotent, TERMINAL: after stop() the
     *  instance cannot be started again (the loop was destroyed on the worker). */
    void stop();

    /** @brief True while the worker thread is alive. */
    bool is_running() const;

private:
    friend class EventLoopThreadPrivate; // worker writes the mLoop member under the handoff mutex
    CXXKIT_DEFINE_DPTR(EventLoopThread)
    CXXKIT_DISABLE_COPY_MOVE(EventLoopThread)

    std::unique_ptr<EventLoop> mLoop; // PLAIN MEMBER, no parent (Momus F4): no double
                                      // ownership, no ChildEvent noise; declared after
                                      // mDPtr so it outlives the private state in ~.
                                      // Since D43.5 written/destroyed ONLY on the worker
                                      // thread (under the private handoff mutex) — null
                                      // before start() and again after worker teardown.
};

CXXKIT_END_NAMESPACE