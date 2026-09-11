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

#pragma once

#include <cxxkit/kernel/kernel_global.hpp>

#include <cxxkit/base/global.hpp>
#include <cxxkit/base/core_config.hpp>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class Object;

class CXXKIT_KERNEL_API Event
{
public:
    enum class Type
    {
        kNone = 0,  // invalid even
        kQuit = 1,  // quit event
        kTimer = 2, // timer event

        kThreadChange = 22,   // object has changed threads
        kDeferredDelete = 52, // deferred delete event

        kParentChange = 21,         // widget has been reparented
        kParentAboutToChange = 131, // sent just before the parent change is done

        kChildAdded = 68,    // new child widget
        kChildRemoved = 71,  // deleted child widget
        kChildPolished = 69, // polished child widget

        kUser = 1000, // first user event id
        kMax = 65535  // last user event id
    };

    explicit Event(Type type);
    Event(const Event &other);
    virtual ~Event();

    Event &operator=(const Event &other);

    inline Type type() const { return static_cast<Type>(mType); }

    inline bool is_accepted() const { return mAccept; }
    inline void set_accepted(bool accepted) { mAccept = accepted; }

    inline void accept() { mAccept = true; }
    inline void ignore() { mAccept = false; }

    /** @brief Registers a custom event type id for application-defined events.
     **
     ** Returns a new unique event type id in the [kUser, kMax] range. When @p hint lies inside
     ** [kUser, kMax] and has not been claimed yet, the hint itself is returned; a claimed or
     ** out-of-range hint yields -1; @p hint = -1 (the default) requests automatic allocation.
     ** Returns -1 once the id space is exhausted. Thread-safe;
     ** intended to be called at initialization time, not on hot paths.
     ** @since 0.2
     */
    static int register_event_type(int hint = -1);


private:
    ushort mType{0};
    ushort mPosted:1;
    ushort mAccept:1;
    ushort mReserved:14;
};


class CXXKIT_KERNEL_API TimerEvent : public Event
{
public:
    explicit TimerEvent(int timer_id);
    ~TimerEvent() override;

    int timer_id() const { return mTimerId; }

protected:
    int mTimerId{0};
};


class CXXKIT_KERNEL_API ChildEvent : public Event
{
public:
    ChildEvent(Type type, Object *child);
    ~ChildEvent() override;

    Object *child() const { return mChild; }
    bool added() const { return this->type() == Type::kChildAdded; }
    bool removed() const { return this->type() == Type::kChildRemoved; }
    bool polished() const { return this->type() == Type::kChildPolished; }

protected:
    Object *mChild{nullptr};
};


/** @brief 延迟删除事件：仅 Event 队列派发路径投递，receiver->event() 内 delete this。 */
class CXXKIT_KERNEL_API DeferredDeleteEvent : public Event
{
public:
    // No public constructor — only Object::delete_later() creates this event (R6: language-level lockdown).
private:
    friend class Object;
    DeferredDeleteEvent();
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL