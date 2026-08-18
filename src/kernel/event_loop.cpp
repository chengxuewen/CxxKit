/***********************************************************************************************************************
**
** Library: cxxkit
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

#include "cxxkit/kernel/detail/event_loop_p.hpp"
#include "cxxkit/tools/logging.hpp"

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

EventLoopPrivate::EventLoopPrivate(EventLoop *p)
    : ObjectPrivate(p)
{
}

EventLoopPrivate::~EventLoopPrivate()
{
}

EventLoop::EventLoop(Object *parent)
    : Object(parent)
{
}

EventLoop::~EventLoop()
{
}

bool EventLoop::isRunning() const
{
    CXXKIT_D(const EventLoop);
    return !d->mExit.load();
}

void EventLoop::processEvents(ProcessFlags flags, int maximumTime)
{
}

bool EventLoop::processEvents(ProcessFlags flags)
{
    CXXKIT_D(EventLoop);
    // auto threadData = d->threadData.loadRelaxed();
    // if (!threadData->hasEventDispatcher())
    // {
    //     return false;
    // }
    // return threadData->eventDispatcher.loadRelaxed()->processEvents(flags);
    return false;
}

int EventLoop::exec(ProcessFlags flags)
{
    CXXKIT_D(EventLoop);
    if (d->mInExec)
    {
        CXXKIT_WARNING("EventLoop::exec: instance {} has already called exec()", utils::fmt::ptr(this));
        return -1;
    }

    while (!d->mExit.load())
    {
        this->processEvents(flags | ProcessFlag::kWaitForMoreEvents | ProcessFlag::kEventLoopExec);
    }

    return d->mRetCode.load();
}

void EventLoop::wakeUp()
{
}

void EventLoop::exit(int retCode)
{
}

void EventLoop::quit()
{
    this->exit(0);
}

bool EventLoop::event(Event *event)
{
    if (event->type() == Event::Type::kQuit)
    {
        this->quit();
        return true;
    }
    else
    {
        return Object::event(event);
    }
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL