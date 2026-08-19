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

#include <cxxkit/kernel/event.hpp>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

Event::Event(Type type)
    : mType(static_cast<ushort>(type))
    , mPosted(false)
    , mAccept(false)
{
}

Event::Event(const Event &other)
    : mType(static_cast<ushort>(other.mType))
    , mPosted(other.mPosted)
    , mAccept(other.mAccept)
{
}

Event::~Event()
{
}

Event &Event::operator=(const Event &other)
{
    if (this != &other)
    {
        mType = static_cast<ushort>(other.mType);
        mPosted = other.mPosted;
        mAccept = other.mAccept;
    }
    return *this;
}

TimerEvent::TimerEvent(int timerId)
    : Event(Type::kTimer)
    , mTimerId(timerId)
{
}

TimerEvent::~TimerEvent()
{
}

ChildEvent::ChildEvent(Type type, Object *child)
    : Event(type)
    , mChild(child)
{
}

ChildEvent::~ChildEvent()
{
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL