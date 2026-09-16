/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
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

#ifndef CXXKIT_THREAD_EVENT_LOOP_THREAD_P_H
#define CXXKIT_THREAD_EVENT_LOOP_THREAD_P_H

#include <cxxkit/thread/event_loop_thread.hpp>
#include <cxxkit/thread/platform_thread.hpp>
#include <cxxkit/kernel/event_loop.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Private implementation of @ref EventLoopThread (thread module pimpl discipline).
 *
 * Owns the worker thread wrapper and the start/stop synchronization state; the public
 * object keeps only the d-pointer. The loop itself is a plain member of the public class
 * (plan Momus F4) so it outlives the private state during destruction.
 */
class EventLoopThreadPrivate
{
public:
    explicit EventLoopThreadPrivate(EventLoopThread *p);

    /** Worker subclass: runs the loop's exec() exactly once. */
    class Runner : public PlatformThread
    {
    public:
        explicit Runner(EventLoopThreadPrivate *d)
            : mD(d)
        {
        }
        ~Runner() override = default;

    protected:
        void run() override;

    private:
        EventLoopThreadPrivate *mD;
    };

    /** Invoked on the worker thread: builds the dispatcher + loop (worker-first, D43.5 T1),
     *  loops exec() until exit fires (stop() just joins), then destroys the loop on the worker too
     *  (uv handles are loop-thread bound — teardown must run where exec ran). */
    void thread_main();

    EventLoopThread *mPPtr;
    Runner mThread;                                       // platform thread carrier (runs thread_main)
    EventLoopThread::DispatcherFactory mFactory{nullptr}; // invoked ON the worker (loop thread)
    std::atomic<bool> mStarted{false};
    std::mutex mStartMutex; // reserved for future restart support (start is once-only today)

    /** Blocks until the worker-built loop is consumable (D43.5 T1 worker-first contract).
     *  Returns nullptr when the loop can never appear (never started) or is already gone
     *  (worker teardown after exec returned) — the public accessor turns that into a fatal
     *  with the contract message; the started-but-building case waits on the cv. */
    EventLoop *wait_for_loop();

    // Loop handoff state (guarded by mLoopMutex): the loop object itself lives in the
    // public class member mLoop, written ONLY on the worker thread under this mutex.
    std::mutex mLoopMutex;
    std::condition_variable mLoopCv;
    bool mLoopReady{false}; // worker finished EventLoop construction — loop() may return it
    bool mLoopGone{false};  // worker destroyed the loop after exec() — terminal for this ELT
};

CXXKIT_END_NAMESPACE

#endif // CXXKIT_FEATURE_ENABLE_THREAD
