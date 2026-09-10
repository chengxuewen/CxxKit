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

#include <cxxkit/kernel/detail/event_loop_p.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/tools/checks.hpp>

#include <algorithm>
#include <deque>
#include <mutex>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

ObjectPrivate::ObjectPrivate(Object *p)
    : mPPtr(p)
{
}

ObjectPrivate::~ObjectPrivate()
{
}

void ObjectPrivate::attach_child(Object *child)
{
    mChildren.push_back(child);
}

void ObjectPrivate::detach_child(Object *child)
{
    mChildren.remove(child);
}

Object::Object(Object *parent)
    : Object(new ObjectPrivate(this))
{
    if (parent != nullptr)
    {
        this->set_parent(parent); // 统一走 set_parent（构造期父链上有 ChildEvent 派发）
    }
}

Object::Object(ObjectPrivate *d)
    : mDPtr(d)
{
    // Creation affinity: an object belongs to the loop running exec on the constructing
    // thread (spec §4-1). Outside exec current() is null -> no affinity. Both public ctors
    // funnel through this delegation target.
    this->d_func()->mThread = EventLoop::current();
}

Object::~Object()
{
    CXXKIT_D(Object);
    this->destroying(); // 预销毁锚点：派生成员仍存活、children 未级联（析构期派发只到 Object 层）
    // T3/T2: clear pending entries (incl. DeferredDeleteEvent) — the foundation of the parent-child
    // same-queue delete_later contract. A purge triggered by a parent dtor during dispatch removes the
    // child's undelivered entries from the queue body under the lock (mutually exclusive with
    // pop_event_entry), so a popped entry can no longer exist — no dangling window.
    // T2 double purge: affinity loop + caller-thread current() residual (same-loop second purge is a harmless no-op).
    EventLoop::purge_pending(d->mThread, this);
    EventLoop::purge_pending(EventLoop::current(), this);
    EventLoop::purge_pending(d->mThread, this);
    EventLoop::purge_pending(EventLoop::current(), this);
    // Bidirectional filter self-detach (Momus F2): (a) I die as a FILTER — remove myself from
    // every watched object's chain; (b) I die as a WATCHED — detach my installed filters so a
    // surviving filter's later teardown never dereferences me (UAF without this).
    // remove_event_filter detaches both sides per call, so each loop consumes exactly one entry
    // per iteration — terminates with both registries empty.
    while (!d->mWatching.empty())
    {
        d->mWatching.back()->remove_event_filter(this);
    }
    while (!d->mFilters.empty())
    {
        this->remove_event_filter(d->mFilters.back());
    }
    // 级联析构：子 dtor 会通过 set_parent(nullptr)/detach 自摘链，所以 while(!empty) 安全。
    while (!d->mChildren.empty())
    {
        delete d->mChildren.front();
    }
    if (d->mParent != nullptr)
    {
        d->mParent->d_func()->detach_child(this);
    }
}

void Object::destroying()
{
}

void Object::delete_later()
{
    EventLoop *loop = this->d_func()->mThread; // affinity-first (D35 evolution)
    if (loop == nullptr)
    {
        loop = EventLoop::current(); // legacy fallback for affinity-less objects
    }
    CXXKIT_CHECK(loop != nullptr) << "Object::delete_later: no affinity and no running EventLoop on this thread";
    // Momus F1: push straight through enqueue_event, bypassing post_event's kDeferredDelete guard —
    // the sole legitimate producer rejected by its only channel would be self-contradictory;
    // the post_event guard stays (user-side DeferredDelete posting forbidden).
    // Dispatch path: Event queue pop → send_event (filter chain applies) → event() kDeferredDelete branch deletes this.
    EventLoop::enqueue_event(loop, this, new DeferredDeleteEvent); // R6: only Object constructs it
}

void Object::post_event(Object *receiver, Event *event)
{
    CXXKIT_CHECK(receiver != nullptr && event != nullptr) << "post_event requires receiver/event";
    // DeferredDelete rejection precedes the affinity check: owner semantics (DeleteInEventHandler)
    // must not be bypassable. Unreachable from user code since R6 (private ctor); kept as
    // internal-misuse guard.
    CXXKIT_CHECK(event->type() != Event::Type::kDeferredDelete)
        << "post_event: use delete_later() for deferred delete (owner semantics)";
    EventLoop *loop = receiver->d_func()->mThread; // target-affinity routing (T2/D35): the receiver's loop
    CXXKIT_CHECK(loop != nullptr)
        << "post_event: receiver has no thread affinity — create it inside a loop's exec or move_to_thread";
    EventLoop::enqueue_event(loop, receiver, event);
    loop->wake_up(); // cross-thread wake (thread-safe dispatcher contract)
}

Object *Object::parent() const
{
    CXXKIT_D(const Object);
    return d->mParent;
}

void Object::set_parent(Object *parent)
{
    CXXKIT_D(Object);
    if (d->mParent == parent)
    {
        return;
    }
    if (parent != nullptr)
    {
        // 环检测：new parent 不能是自己的后代（沿 parent 链上行不能遇到 this）
        for (Object *it = parent; it != nullptr; it = it->parent())
        {
            CXXKIT_CHECK(it != this) << "Object::set_parent: cycle detected";
        }
    }
    if (d->mParent != nullptr)
    {
        d->mParent->d_func()->detach_child(this);
        ChildEvent removed(Event::Type::kChildRemoved, this);
        d->mParent->event(&removed);
        d->mParent = nullptr;
    }
    if (parent != nullptr)
    {
        d->mParent = parent;
        parent->d_func()->attach_child(this);
        ChildEvent added(Event::Type::kChildAdded, this);
        parent->event(&added);
    }
}
EventLoop *Object::thread() const
{
    CXXKIT_D(const Object);
    return d->mThread;
}

void Object::move_to_thread(EventLoop *target)
{
    CXXKIT_D(Object);
    EventLoop *source = d->mThread;
    if (source == target)
    {
        return; // same-loop no-op (Qt-tolerant shape)
    }
    CXXKIT_CHECK((source == nullptr) || (!source->is_running())) << "move_to_thread: source loop is running";
    CXXKIT_CHECK((target == nullptr) || (!target->is_running())) << "move_to_thread: target loop is running";

    // Subtree pre-order walk: collect all migrated objects (self first), then relink
    // affinity + notify each.
    std::vector<Object *> moved;
    moved.push_back(this);
    std::deque<Object *> pending;
    pending.push_back(this);
    while (!pending.empty())
    {
        Object *current = pending.front();
        pending.pop_front();
        const Children &children = current->children();
        for (Children::const_iterator it = children.begin(); it != children.end(); ++it)
        {
            moved.push_back(*it);
            pending.push_back(*it);
        }
    }
    // Per-object migrate step (member scope so protected d_func() is accessible):
    // relink affinity, then notify via a plain kThreadChange event. Direct event() call —
    // the filter chain is NOT applied (Qt-faithful: QEvent::ThreadChange goes straight to
    // the handler, it is not a filterable event).
    struct Migrator
    {
        static void migrate(Object *obj, EventLoop *targetLoop)
        {
            obj->d_func()->mThread = targetLoop;
            Event change(Event::Type::kThreadChange);
            obj->event(&change);
        }
    };
    for (size_t i = 0; i < moved.size(); ++i)
    {
        Migrator::migrate(moved[i], target);
    }

    // Migrate pending queue entries per receiver: drain the source queue first, then push
    // into the target — the two locks are never held simultaneously (no lock-order need).
    for (size_t i = 0; i < moved.size(); ++i)
    {
        std::deque<EventEntry> entries;
        if (source != nullptr)
        {
            entries = source->d_func()->take_events_for(moved[i]);
        }
        if (target != nullptr)
        {
            EventLoopPrivate *targetPriv = target->d_func();
            std::lock_guard<std::mutex> lock(targetPriv->mEventMutex);
            while (!entries.empty())
            {
                targetPriv->mEventQueue.push_back(entries.front());
                entries.pop_front();
            }
        }
        else
        {
            // Detached affinity: delivering on no loop is impossible — delete to avoid a leak
            // (matches the ~Object purge spirit).
            while (!entries.empty())
            {
                delete entries.front().mEvent;
                entries.pop_front();
            }
        }
    }
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
            this->timer_event(dynamic_cast<TimerEvent *>(event));
            break;
        }

        case Event::Type::kChildAdded:
        case Event::Type::kChildPolished:
        case Event::Type::kChildRemoved:
        {
            this->child_event(dynamic_cast<ChildEvent *>(event));
            break;
        }
        case Event::Type::kDeferredDelete:
        {
            // DeleteInEventHandler 正名化（Qt 同款）；仅 Event 队列派发路径会到达（delete_later
            // 迁移到 Event 队列），用户侧直发被 post_event 守卫拒绝。T3：delete this 后立即返回 true——
            // this 已死，禁止触碰成员/base 子对象；process_events 尾部 delete entry.mEvent 合法
            //（事件对象独立于 receiver）。
            delete this;
            return true;
        }
        case Event::Type::kThreadChange:
        {
            // Migration notification: affinity already updated by move_to_thread.
            // Deliberate no-op hook — derived classes may override event() to react.
            break;
        }
        default:
            if (event->type() >= Event::Type::kUser)
            {
                this->custom_event(event);
                break;
            }
            return false;
    }
    return true;
}

bool Object::send_event(Object *receiver, Event *event)
{
    CXXKIT_CHECK(receiver != nullptr && event != nullptr) << "send_event requires receiver/event";
    // kDeferredDelete 不拒：队列派发复用 send_event 内部逻辑（filter 链生效，T3），用户侧直发
    // 该类型由 post_event 守卫拒绝（唯一合法生产者是 delete_later 走的 enqueue_event 通道）。
    ObjectPrivate *priv = receiver->d_func();
    // 头插序遍历：mFilters.back() 最先（后装先过滤）
    std::vector<Object *> &filters = priv->mFilters;
    for (size_t i = filters.size(); i > 0; --i)
    {
        if (filters[i - 1]->event_filter(receiver, event))
        {
            return false; // 拦截
        }
    }
    receiver->event(event);
    return event->is_accepted();
}

void Object::install_event_filter(Object *filter)
{
    CXXKIT_CHECK(filter != nullptr) << "install_event_filter requires a filter";
    CXXKIT_CHECK(filter != this) << "install_event_filter: self-filtering is not allowed";
    this->remove_event_filter(filter); // R5 dedup: reinstall moves the filter to the front (newest)
    this->d_func()->mFilters.push_back(filter);
    filter->d_func()->mWatching.push_back(this); // reverse registry
}

void Object::remove_event_filter(Object *filter)
{
    CXXKIT_D(Object);
    std::vector<Object *> &filters = d->mFilters;
    filters.erase(std::remove(filters.begin(), filters.end(), filter), filters.end());
    if (filter != nullptr)
    {
        std::vector<Object *> &watching = filter->d_func()->mWatching;
        watching.erase(std::remove(watching.begin(), watching.end(), this), watching.end());
    }
}

bool Object::event_filter(Object *watched, Event *event)
{
    CXXKIT_UNUSED(watched);
    CXXKIT_UNUSED(event);
    return false;
}

void Object::timer_event(TimerEvent *event)
{
    CXXKIT_UNUSED(event);
}

void Object::child_event(ChildEvent *event)
{
    CXXKIT_UNUSED(event);
}

void Object::custom_event(Event *event)
{
    CXXKIT_UNUSED(event);
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
