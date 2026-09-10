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

#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/tools/checks.hpp>

#include <algorithm>

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
}

Object::~Object()
{
    CXXKIT_D(Object);
    this->destroying(); // 预销毁锚点：派生成员仍存活、children 未级联（析构期派发只到 Object 层）
    // T3：清 pending（含 DeferredDeleteEvent）——父子同投限制解除的根基。父析构触发的本调用在锁内
    // 从 Event 队列本体移除子的未派发条目，pop 到即不存在（与 pop_event_entry 锁内互斥）。
    // 无环线程 = no-op（purge 静态内部 null 容忍）；本体遍历，非快照缓存。
    EventLoop::purge_pending(this);
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
    EventLoop *loop = EventLoop::current();
    CXXKIT_CHECK(loop != nullptr) << "Object::delete_later: no running EventLoop on this thread";
    // Momus F1：直推 enqueue_event 绕过 post_event 的 kDeferredDelete 守卫——唯一合法生产者
    // 恰好被唯一通道拒绝会自相矛盾；post_event 守卫照旧（用户侧禁投 DeferredDelete）。
    // 派发路径：Event 队列 pop → send_event（filter 链生效）→ event() kDeferredDelete 分支 delete this。
    EventLoop::enqueue_event(this, new DeferredDeleteEvent);
}

void Object::post_event(Object *receiver, Event *event)
{
    CXXKIT_CHECK(receiver != nullptr && event != nullptr) << "post_event requires receiver/event";
    // DeferredDelete 拒绝先于 current 检查：owner 语义（DeleteInEventHandler）不可绕过
    CXXKIT_CHECK(event->type() != Event::Type::kDeferredDelete)
        << "post_event: use delete_later() for deferred delete (owner semantics)";
    EventLoop::enqueue_event(receiver, event); // friend 通道：null current = fatal（exec-only 契约）
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
            CXXKIT_D(Object);
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
    this->d_func()->mFilters.push_back(filter);
}

void Object::remove_event_filter(Object *filter)
{
    CXXKIT_D(Object);
    std::vector<Object *> &filters = d->mFilters;
    filters.erase(std::remove(filters.begin(), filters.end(), filter), filters.end());
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
