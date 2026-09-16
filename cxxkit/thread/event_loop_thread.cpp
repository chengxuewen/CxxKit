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

#include <cxxkit/thread/event_loop_thread.hpp>
#include <cxxkit/thread/detail/event_loop_thread_p.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/abstract_event_dispatcher.hpp>
#include <cxxkit/tools/checks.hpp>

#include <utility>

CXXKIT_BEGIN_NAMESPACE

EventLoopThreadPrivate::EventLoopThreadPrivate(EventLoopThread *p)
    : mPPtr(p)
    , mThread(this)
{
}

void EventLoopThreadPrivate::Runner::run()
{
    mD->thread_main();
}

void EventLoopThreadPrivate::thread_main()
{
    EventLoopThread *p = mPPtr;
    // Worker-first construction (D43.5 T1): the dispatcher MUST be born on the thread that
    // will exec() it — uv captures the constructing thread id in its ctor and fatals when
    // exec runs elsewhere. A null product is fatal here (was fatal in the ctor before).
    std::unique_ptr<AbstractEventDispatcher> dispatcher = mFactory();
    CXXKIT_CHECK(dispatcher != nullptr) << "EventLoopThread: factory returned a null dispatcher";
    {
        std::lock_guard<std::mutex> lock(mLoopMutex);
        p->mLoop.reset(new EventLoop(std::move(dispatcher)));
        mLoopReady = true;
    }
    mLoopCv.notify_all(); // loop() waiters (and any pre-exec accessor) may proceed

    // The worker is the only exec() caller and start() joined no thread yet: the exit
    // code write races nothing. exec() returns when stop() (or a user exit) fires.
    p->loop().exec();
    mExitRequested.store(true); // exec() has returned: the thread is done

    // Teardown ON the worker thread (D43.5 T1): uv handles are loop-thread bound, so the
    // dispatcher destructor must run where exec() ran. After this, loop() waiters get a
    // fatal instead of a dangling member — the terminal state for this ELT instance.
    {
        std::lock_guard<std::mutex> lock(mLoopMutex);
        p->mLoop.reset();
        mLoopReady = false;
        mLoopGone = true;
    }
    mLoopCv.notify_all();
}

EventLoop *EventLoopThreadPrivate::wait_for_loop()
{
    // Never started: the worker does not exist, so no notify will ever come — waiting on
    // the cv here would deadlock (D43.5 T1 controller ruling). Return immediately; the
    // public accessor turns null into a fatal with the start() hint.
    if (!mStarted.load())
    {
        return nullptr;
    }
    std::unique_lock<std::mutex> lock(mLoopMutex);
    // Started-but-building: wait for the worker handoff. Terminal states pass through and
    // let the caller decide (never started / already torn down -> null).
    mLoopCv.wait(lock, [this]() { return mLoopReady || mLoopGone; });
    return mLoopReady ? mPPtr->mLoop.get() : nullptr;
}

EventLoopThread::EventLoopThread(DispatcherFactory factory, Object *parent)
    : Object(parent)
{
    CXXKIT_CHECK(factory != nullptr) << "EventLoopThread requires a dispatcher factory";
    mDPtr.reset(new EventLoopThreadPrivate(this));
    // D43.5 T1: the factory is STORED, not called. The dispatcher (and the loop wrapping it)
    // is constructed on the worker thread inside thread_main(), after start() spawns it —
    // that is the whole fix: uv's loop-thread capture then matches the exec() thread by
    // construction. The loop does not exist between ctor and start().
    mDPtr->mFactory = factory;
    // Momus F4 (plan): the loop is a PLAIN MEMBER with parent = nullptr — making it a child
    // would double-own it (member + parent cascade) and dispatch a ChildEvent during
    // construction. Declared after mDPtr in the header on purpose: it outlives the private
    // state, so the destructor body (stop) can still reach loop().exit(). Since D43.5 the
    // member is written/destroyed on the worker thread (under mLoopMutex) and is already
    // null by the time member destruction runs — the dtor body only joins.
}


EventLoopThread::~EventLoopThread()
{
    this->stop(); // implicit stop: join before the loop member is destroyed
}

EventLoop &EventLoopThread::loop()
{
    CXXKIT_CHECK(mDPtr != nullptr) << "EventLoopThread::loop: no private state";
    EventLoop *loop = mDPtr->wait_for_loop();
    CXXKIT_CHECK(loop != nullptr) << "EventLoopThread::loop: no loop — call start() first (worker-first construction: "
                                     "the loop is born on the worker thread)";
    return *loop;
}

void EventLoopThread::start()
{
    CXXKIT_CHECK(mDPtr != nullptr) << "EventLoopThread::start: no private state";
    CXXKIT_CHECK(!mDPtr->mStarted.load()) << "EventLoopThread::start: already started";
    CXXKIT_CHECK(!mDPtr->mLoopGone)
        << "EventLoopThread::start: restart after stop is unsupported (the loop was torn down on the worker thread)";
    mDPtr->mExitRequested.store(false);
    mDPtr->mStarted.store(true); // published before the thread exists — no race with thread_main
    const Status status = mDPtr->mThread.start();
    if (!status)
    {
        mDPtr->mStarted.store(false); // roll back: start failed, leave the object reusable
        CXXKIT_CHECK(false) << "EventLoopThread::start: thread creation failed";
    }
}

void EventLoopThread::stop()
{
    if (!mDPtr)
    {
        return; // defensive: nothing to stop
    }
    if (!mDPtr->mStarted.load())
    {
        return; // never started: no worker exists, and with worker-first construction no loop either
    }
    // Thread-safe exit + wake (D35 I1 contract): legal from any thread. wait_for_loop blocks
    // only while the worker is still building the loop (it always finishes building — exec
    // returns only via the exit below) and returns null if the worker already tore the loop
    // down (cannot happen before our exit: only exit() ends exec).
    if (EventLoop *loop = mDPtr->wait_for_loop())
    {
        loop->exit(0);
    }
    (void)mDPtr->mThread.wait(); // join: thread_main always returns after exit(); loop + dispatcher die on the worker
    mDPtr->mStarted.store(false);
}

bool EventLoopThread::is_running() const
{
    return mDPtr->mThread.is_running();
}

CXXKIT_END_NAMESPACE
