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
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/kernel/abstract_event_dispatcher.hpp>

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Scriptable driver double for EventLoop shell tests (header-only, tests tree only).
 *
 * No real thread and no real event source: the test drives the loop manually via
 * EventLoop::process_events / exec, and FakeDispatcher records what it was asked to do.
 *
 * Scripting points:
 * - @ref mOnProcessEvents — override the default "drain posted tasks" behaviour.
 * - @ref drain() — manual helper replicating the default drain, for tests that override
 *   mOnProcessEvents but still want to run posted tasks.
 *
 * Anti-busy-spin recipe (P3): before exec(), post a task that calls exit(); the default
 * process_events drains it and returns true; exec's next round sees mExit at the top of
 * the loop and stops.
 */
class FakeDispatcher : public AbstractEventDispatcher
{
public:
    FakeDispatcher() = default;
    ~FakeDispatcher() override = default;

    bool process_events(EventLoop::ProcessFlags flags) override
    {
        ++mProcessCount;
        if (mOnProcessEvents)
        {
            return mOnProcessEvents(flags);
        }
        return this->drain();
    }

    void wake_up() override { ++mWakeUpCount; }

    void interrupt() override { ++mInterruptCount; }

    void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) override
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        if (!fn)
        {
            return;
        }
        mTimerIds.push_back(timer_id);
        mTimers[timer_id] = std::make_pair(interval_ms, std::move(fn));
    }

    void stop_timer(int timer_id) override
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        mTimers.erase(timer_id);
    }

    /** Runs all posted tasks FIFO; returns true if at least one ran (dispatcher contract). */
    bool drain()
    {
        mPostedMutex.lock();
        std::deque<std::function<void()>> tasks;
        tasks.swap(mPosted);
        mPostedMutex.unlock();
        bool processed = false;
        while (!tasks.empty())
        {
            std::function<void()> fn = tasks.front();
            tasks.pop_front();
            if (fn)
            {
                fn();
            }
            processed = true;
        }
        return processed;
    }

    /** Test-side helper: simulates a producer thread enqueueing work the dispatcher would have received. */
    void enqueue_posted(std::function<void()> fn)
    {
        std::lock_guard<std::mutex> lock(mPostedMutex);
        mPosted.push_back(std::move(fn));
    }

    /** Test-side helper: snapshot the callback EventLoop registered for @p timer_id (null if absent). */
    std::function<void()> take_timer_fn(int timer_id)
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        auto it = mTimers.find(timer_id);
        if (it == mTimers.end())
        {
            return nullptr;
        }
        return it->second.second;
    }

    bool has_timer(int timer_id)
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        return mTimers.find(timer_id) != mTimers.end();
    }

    /** Total registrations (not live count); used to attribute which id a registration carried. */
    int timer_registration_count()
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        return static_cast<int>(mTimerIds.size());
    }

    int timer_id_at(int index)
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        return mTimerIds.at(static_cast<size_t>(index));
    }

    std::atomic<int> mProcessCount{0};
    std::atomic<int> mWakeUpCount{0};
    std::atomic<int> mInterruptCount{0};
    std::function<bool(EventLoop::ProcessFlags)> mOnProcessEvents;

    std::mutex mTimerMutex;
    std::map<int, std::pair<uint64_t, std::function<void()>>> mTimers;
    std::vector<int> mTimerIds;

    std::mutex mPostedMutex;
    std::deque<std::function<void()>> mPosted;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
