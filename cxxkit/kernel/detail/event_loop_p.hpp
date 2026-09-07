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

#pragma once

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/abstract_event_dispatcher.hpp>
#include <cxxkit/kernel/detail/object_p.hpp>
#include <cxxkit/thread/reference_counter.hpp>

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class EventLoopPrivate : public ObjectPrivate
{
    CXXKIT_DECLARE_PUBLIC(EventLoop)
    CXXKIT_DISABLE_COPY_MOVE(EventLoopPrivate)
public:
    explicit EventLoopPrivate(EventLoop *p);
    ~EventLoopPrivate() override;

    void ref() { mRefCounter.ref(); }

    void deref()
    {
        if (!mRefCounter.deref() && mInExec)
        {
            // qApp->postEvent(mPPtr, new Event(Event::Type::kQuit));
        }
    }

    /**
     * @brief Moves out and returns all pending posted tasks (lock held only for the swap).
     *
     * Swap-under-lock keeps callbacks outside the mutex: re-entrant post() from a callback
     * cannot deadlock (S10/I4 drain invariant).
     */
    std::deque<std::function<void()>> take_post_queue()
    {
        std::lock_guard<std::mutex> lock(mPostMutex);
        std::deque<std::function<void()>> tasks;
        tasks.swap(mPostQueue);
        return tasks;
    }

    std::unique_ptr<AbstractEventDispatcher> mDispatcher;
    std::mutex mPostMutex;
    std::deque<std::function<void()>> mPostQueue;
    std::atomic<int> mNextTimerId{0};
    bool mInExec{false};
    std::atomic<bool> mExit{true};
    std::atomic<int> mRetCode{-1};
    std::atomic<bool> mHasExitCode{false}; // exit() ran — distinguishes preset exit from fresh state
    ReferenceCounter mRefCounter;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
