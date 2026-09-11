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
    // The worker is the only exec() caller and start() joined no thread yet: the exit
    // code write races nothing. exec() returns when stop() (or a user exit) fires.
    p->loop().exec();
    mExitRequested.store(true); // exec() has returned: the thread is done
}

EventLoopThread::EventLoopThread(DispatcherFactory factory, Object *parent)
    : Object(parent)
{
    CXXKIT_CHECK(factory != nullptr) << "EventLoopThread requires a dispatcher factory";
    std::unique_ptr<AbstractEventDispatcher> dispatcher = factory();
    CXXKIT_CHECK(dispatcher != nullptr) << "EventLoopThread: factory returned a null dispatcher";
    mDPtr.reset(new EventLoopThreadPrivate(this));
    // Momus F4 (plan): the loop is a PLAIN MEMBER with parent = nullptr — making it a child
    // would double-own it (member + parent cascade) and dispatch a ChildEvent during
    // construction. Declared after mDPtr in the header on purpose: it outlives the private
    // state, so the destructor body (stop) can still reach loop().exit().
    mLoop.reset(new EventLoop(std::move(dispatcher)));
}

EventLoopThread::~EventLoopThread()
{
    this->stop(); // implicit stop: join before the loop member is destroyed
}

EventLoop &EventLoopThread::loop()
{
    return *mLoop;
}

void EventLoopThread::start()
{
    CXXKIT_CHECK(mDPtr != nullptr) << "EventLoopThread::start: no private state";
    CXXKIT_CHECK(!mDPtr->mStarted.load()) << "EventLoopThread::start: already started";
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
    // Thread-safe exit + wake (D35 I1 contract): legal from any thread, with or without a
    // running exec(); on a never-started loop it just presets the exit code (harmless).
    mLoop->exit(0);
    if (mDPtr->mStarted.load())
    {
        (void)mDPtr->mThread.wait(); // join: thread_main always returns after exit()
        mDPtr->mStarted.store(false);
    }
}

bool EventLoopThread::is_running() const
{
    return mDPtr->mThread.is_running();
}

CXXKIT_END_NAMESPACE
