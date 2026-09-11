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

#include <cxxkit/kernel/object.hpp>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class EventLoop;

class ObjectPrivate
{
public:
    using Children = Object::Children;

    explicit ObjectPrivate(Object *p);
    virtual ~ObjectPrivate();

    void attach_child(Object *child); // children.push_back(child)
    void detach_child(Object *child); // children.remove(child)

    Object *mParent{nullptr};
    Children mChildren;
    std::vector<Object *> mFilters;  // head-insert order (back() = newest, filtered first)
    std::vector<Object *> mWatching; // reverse registry: filters installed ON other objects by me
    EventLoop *mThread{nullptr};     // affinity: loop owning this object (null = detached)
    std::vector<int> mActiveTimers;  // A1: live timer ids (dispatcher-side armed); same-thread only
    std::string mObjectName;         // name follows the object (not touched by move_to_thread)
    std::map<const void *, std::unique_ptr<Object::UserData>> mUserData; // owned; released by member dtor

    /** @brief T7 funnel bridge: Application::notify (a plain member of the DERIVED Application
     *  class) cannot call Object's private send_event_internal directly — ObjectPrivate is the
     *  friend. Static, stateless; forwards to Object::send_event_internal. Defined in object.cpp.
     *  Kernel-internal (detail header), not part of the public API. */
    static bool deliver_via_funnel(Object *app, Object *receiver, Event *event);

protected:
    CXXKIT_DEFINE_PPTR(Object)
    CXXKIT_DECLARE_PUBLIC(Object)
    CXXKIT_DISABLE_COPY_MOVE(ObjectPrivate)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL