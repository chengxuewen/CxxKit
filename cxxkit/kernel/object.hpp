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

#include <cxxkit/kernel/event.hpp>

#include <list>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class ObjectPrivate;
class Object
{
public:
    using Children = std::list<Object *>;

    explicit Object(Object *parent = nullptr);
    Object(ObjectPrivate *d);
    virtual ~Object();

    Object *parent() const;
    void set_parent(Object *parent);

    const Children &children() const;

    /** @brief 同步投递：filter 链前置（后装先过滤），未拦截则 receiver->event()。返回 is_accepted()；被 filter 拦截返回 false。 */
    static bool send_event(Object *receiver, Event *event);

    /**
     * @brief 异步投递：将 @p event 入队到当前线程 EventLoop，下一次排空时派发给 @p receiver。
     *
     * 所有权转移：队列持有 event，派发（send_event 内部逻辑 → filter 链生效）后 delete；
     * 未派发条目由 ~Object purge（remove_pending_events）或 ~EventLoop 排空 delete。
     * fatal：receiver/event 为空；kDeferredDelete（owner 语义仅 delete_later 可投）；当前线程无运行中环。
     * @since 0.2
     */
    static void post_event(Object *receiver, Event *event);

    /** @brief 头插 filter（后装先过滤，Qt 同款）。契约：filter 须比 watched 长寿或自行 remove。 */
    void install_event_filter(Object *filter);

    /** @brief 线性查找移除；无此 filter 为 no-op。 */
    void remove_event_filter(Object *filter);

    virtual bool event(Event *event);
    virtual bool event_filter(Object *watched, Event *event);

protected:
    /**
     * @brief 预销毁回调：~Object 顶部调用（派生成员仍存活、children 未级联）。
     *
     * 注意：析构期虚派发只到当前动态类型层（Object）——派生类 override 不会被调用（Qt 同款规则）。
     * 钩子价值 = Object 层内部清理时序锚点 + 外部观察者可见的“children 级联前”时刻；
     * signals 断连不依赖它（cxxkit signals sender 析构自断连）。
     * @since 0.2
     */
    virtual void destroying();

public:
    /**
     * @brief 请求在当前线程 EventLoop 下一次排空时删除 this（Qt deleteLater 语义）。
     *
     * 内部通道：Event 队列投递 DeferredDeleteEvent（Momus F1 直推 enqueue_event 绕过
     * post_event 的 kDeferredDelete 守卫）；派发 = send_event（filter 链生效）→ event()
     * kDeferredDelete 分支 delete this。
     *
     * 仅在当前线程存在**运行中**（exec 内）的 EventLoop 时合法；否则（含环已构造但未 exec）= fatal。
     * T3 限制①解除：父与子可同时 delete_later——父先派发时其 ~Object purge 在锁内从队列
     * 本体移除子的未派发条目，级联 delete 子后子条目已不存在，无二次 delete。
     * 已知限制：②~EventLoop 析构排空期间闭包内再 post（如级联 delete_later）投到将死环新队列静默丢失，
     * 且排空时 current 已不指向自身——闭包内 delete_later 会 fatal。文档化限制。
     * @since 0.2
     */
    void delete_later();

protected:
    virtual void timer_event(TimerEvent *event);
    virtual void child_event(ChildEvent *event);
    virtual void custom_event(Event *event);

protected:
    CXXKIT_DEFINE_DPTR(Object)
    CXXKIT_DECLARE_PRIVATE(Object)
    CXXKIT_DISABLE_COPY_MOVE(Object)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL