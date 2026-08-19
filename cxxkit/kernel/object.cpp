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

#include <cxxkit/kernel/detail/object_p.hpp>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

ObjectPrivate::ObjectPrivate(Object *p)
    : mPPtr(p)
{
}

ObjectPrivate::~ObjectPrivate()
{
}

Object::Object(Object *parent)
    : Object(new ObjectPrivate(this))
{
}

Object::Object(ObjectPrivate *d)
    : mDPtr(d)
{
}

Object::~Object()
{
}

Object *Object::parent() const
{
    CXXKIT_D(const Object);
    return d->mParent;
}

void Object::setParent(Object *parent)
{
    CXXKIT_D(Object);
    d->mParent = parent;
}

const Object::Children &Object::children() const
{
    CXXKIT_D(const Object);
    return d->mChildren;
}

bool Object::event(Event *event)
{
    switch (event->type())
    {
        case Event::Type::kTimer:
        {
            this->timerEvent(dynamic_cast<TimerEvent *>(event));
            break;
        }

        case Event::Type::kChildAdded:
        case Event::Type::kChildPolished:
        case Event::Type::kChildRemoved:
        {
            this->childEvent(dynamic_cast<ChildEvent *>(event));
            break;
        }
        case Event::Type::kDeferredDelete:
        {
            // DeleteInEventHandler(this);
            break;
        }
        case Event::Type::kThreadChange:
        {
            CXXKIT_D(Object);
            break;
        }
        default:
            if (event->type() >= Event::Type::kUser)
            {
                this->customEvent(event);
                break;
            }
            return false;
    }
    return true;
}

bool Object::eventFilter(Object *watched, Event *event)
{
    CXXKIT_UNUSED(watched);
    CXXKIT_UNUSED(event);
    return false;
}

void Object::timerEvent(TimerEvent *event)
{
    CXXKIT_UNUSED(event);
}

void Object::childEvent(ChildEvent *event)
{
    CXXKIT_UNUSED(event);
}

void Object::customEvent(Event *event)
{
    CXXKIT_UNUSED(event);
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
