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

/**
 * @brief Queued event entry: receiver + owned event (post_event path).
 *
 * Ownership contract: the queue owns @c mEvent — dispatched entries are deleted after
 * send_event, undelivered entries are deleted by ~Object purge (remove_pending_events)
 * or the ~EventLoop drain (take_event_queue).
 */
struct EventEntry
{
    Object *mReceiver{nullptr};
    Event *mEvent{nullptr};
};

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

    /**
     * @brief Moves out and returns all pending events (lock held only for the swap).
     *
     * Consumed only by ~EventLoop drain: snapshot delete of the events never touches
     * receivers, so the cascade-free shape stays safe.
     */
    std::deque<EventEntry> take_event_queue()
    {
        std::lock_guard<std::mutex> lock(mEventMutex);
        std::deque<EventEntry> entries;
        entries.swap(mEventQueue);
        return entries;
    }

    /** @brief Locks the queue and extracts all entries whose receiver == @p receiver.
     *
     *  Migration primitive: taken entries move with the migrated object; the remaining
     *  queue order is preserved (swap-back under the same lock).
     */
    std::deque<EventEntry> take_events_for(Object *receiver)
    {
        std::lock_guard<std::mutex> lock(mEventMutex);
        std::deque<EventEntry> taken;
        std::deque<EventEntry> remaining;
        while (!mEventQueue.empty())
        {
            EventEntry entry = mEventQueue.front();
            mEventQueue.pop_front();
            if (entry.mReceiver == receiver)
            {
                taken.push_back(entry);
            }
            else
            {
                remaining.push_back(entry);
            }
        }
        mEventQueue.swap(remaining);
        return taken;
    }

    /**
     * @brief ~Object 清理入口：锁内 remove+delete 匹配 receiver 的条目（含 DeferredDeleteEvent）。
     *
     * 与 pop_event_entry 锁内互斥——派发期级联析构触发的 purge 从队列本体移除未派发条目，
     * pop 到即不存在，无悬垂窗口（Qt 忠实形态，Momus F2）。
     */
    void remove_pending_events(Object *receiver)
    {
        std::lock_guard<std::mutex> lock(mEventMutex);
        for (std::deque<EventEntry>::iterator it = mEventQueue.begin(); it != mEventQueue.end();)
        {
            if (it->mReceiver == receiver)
            {
                delete it->mEvent;
                it = mEventQueue.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    /**
     * @brief Takes the head entry (lock held); empty queue returns the {nullptr, nullptr} sentinel.
     *
     * Lock-pop + dispatch-outside-the-lock (Qt-faithful): re-entrant purge during dispatch
     * mutates the queue body under the mutex, never a local snapshot.
     */
    EventEntry pop_event_entry()
    {
        std::lock_guard<std::mutex> lock(mEventMutex);
        if (mEventQueue.empty())
        {
            EventEntry sentinel;
            sentinel.mReceiver = nullptr;
            sentinel.mEvent = nullptr;
            return sentinel;
        }
        EventEntry entry = mEventQueue.front();
        mEventQueue.pop_front();
        return entry;
    }

    std::unique_ptr<AbstractEventDispatcher> mDispatcher;
    std::mutex mPostMutex;
    std::deque<std::function<void()>> mPostQueue;
    std::mutex mEventMutex;
    std::deque<EventEntry> mEventQueue;
    std::atomic<int> mNextTimerId{0};
    bool mInExec{false};
    std::atomic<bool> mExit{true};
    std::atomic<int> mRetCode{-1};
    std::atomic<bool> mHasExitCode{false}; // exit() ran — distinguishes preset exit from fresh state
    ReferenceCounter mRefCounter;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
