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
     * 无当前环 = fatal。已知限制：父与子不可同时 delete_later——队列 [delete 父, delete 子]
     * 时父级联已直接 delete 子，残留闭包再 delete = 二次 delete（Qt 靠 ~QObject 清 pending
     * DeferredDelete，本版不做，文档化限制）。
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