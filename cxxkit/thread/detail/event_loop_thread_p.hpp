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

    /** Invoked on the worker thread: loops exec() until stop() flips the flag. */
    void thread_main();

    EventLoopThread *mPPtr;
    Runner mThread; // platform thread carrier (runs thread_main)
    std::atomic<bool> mStarted{false};
    std::atomic<bool> mExitRequested{false};
    std::mutex mStartMutex; // reserved for future restart support (start is once-only today)
};

CXXKIT_END_NAMESPACE

#endif // CXXKIT_FEATURE_ENABLE_THREAD
