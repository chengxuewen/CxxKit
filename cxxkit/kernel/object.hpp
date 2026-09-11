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
#include <map>
#include <memory>
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class ObjectPrivate;
class EventLoop;
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

    /** @brief Base for type-keyed attached data (Chromium SupportsUserData shape).
     *  Derive and attach via set_user_data(); Object owns and destroys instances.
     *  Not thread-safe (Object is single-threaded by design, D33). @since 0.2 */
    class UserData
    {
    public:
        virtual ~UserData();
    };

    /** @brief Attaches @p data under @p key, taking ownership. A null key is fatal
     *  (CXXKIT_CHECK). Attaching under an existing key destroys the old instance;
     *  a null @p data removes (and destroys) any entry under @p key — no-op if absent.
     *  Not thread-safe (Object is single-threaded by design). @since 0.2 */
    void set_user_data(const void *key, std::unique_ptr<UserData> data);

    /** @brief Returns the data attached under @p key, or nullptr when absent. A null
     *  key is fatal (CXXKIT_CHECK). Not thread-safe (Object is single-threaded by
     *  design). @since 0.2 */
    UserData *user_data(const void *key) const;

    /** @brief Returns the object name (empty by default). @since 0.2 */
    const std::string &object_name() const;

    /** @brief Sets the object name. The name follows the object — move_to_thread does not
     *  affect it. @since 0.2 */
    void set_object_name(std::string name);

    /** @brief Renders this object and its subtree as a deterministic multi-line string:
     *  each line is indent*2 spaces + "{name} {typeid(*this).name()}" + newline; children
     *  recurse with indent+1. @since 0.2 */
    std::string to_tree_string(int indent = 0) const;

    /** @brief Debug helper: prints to_tree_string(indent) to stderr (fprintf; no logging
     *  dependency). Qt dumpObjectTree analog. @since 0.2 */
    void dump_object_tree(int indent = 0) const;

    /** @brief Returns the loop this object is affined to (null = no affinity).
     *  Lifetime contract: the affinity loop must outlive objects bound to it — purge /
     *  delete_later / post_event dereference it (dangling loop = UAF). @since 0.2 */
    EventLoop *thread() const;

    /**
     * @brief Static migration: moves this object and its whole subtree to @p target.
     *
     * Guards: same-loop is a no-op; source loop running or target loop running is fatal
     * (CXXKIT_CHECK). Null target detaches affinity (pending events are dropped and
     * deleted). Pending queued events migrate per object from its OWN old loop (I3:
     * mixed-affinity subtrees — a child may sit on a different loop than the root);
     * timers do NOT migrate.
     * @note Caller contract (Momus F5): no concurrent post_event to subtree objects
     *       during the call — affinity metadata is unsynchronized by design (static
     *       migration is a setup-phase operation).
     * @note Lifetime contract: the affinity loop must outlive objects bound to it
     *       (dangling-loop dereference in purge/delete_later/post_event otherwise).
     * @since 0.2
     */
    void move_to_thread(EventLoop *target);

    /** @brief 同步投递：filter 链前置（后装先过滤），未拦截则 receiver->event()。返回 is_accepted()；被 filter 拦截返回 false。 */
    static bool send_event(Object *receiver, Event *event);

    /**
     * @brief Asynchronous delivery: routes @p event to the receiver's AFFINITY loop
     *        (receiver->thread()); dispatched on that loop's next drain.
     *
     * Ownership transfers to the queue: the event is deleted after dispatch (send_event
     * internals — the filter chain applies); undelivered entries are deleted by the
     * ~Object purge (remove_pending_events) or the ~EventLoop drain. Cross-thread posting
     * is legal: the enqueue wakes the target loop (thread-safe dispatcher contract).
     * Fatal (CXXKIT_CHECK): receiver/event null; kDeferredDelete (owner semantics — only
     * delete_later may enqueue it); receiver has no thread affinity (construct it inside
     * a loop's exec or move_to_thread it first).
     * @since 0.2
     */
    static void post_event(Object *receiver, Event *event);

    /** @brief Front-inserts @p filter (newest filters first, Qt-style); reinstalling an
     *  installed filter moves it to the front. Bidirectional registry auto-cleanup (T3):
     *  destruction of EITHER the filter or the watched object detaches the pair — no
     *  lifetime-ordering contract. */
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
     * @brief Requests deletion of this on an EventLoop's next drain (Qt deleteLater).
     *
     * Loop selection: affinity-first (thread()) — CROSS-THREAD LEGAL: the affinity loop
     * may be exec'ing on another thread; the enqueue wakes it so a blocked drain
     * re-checks its queues. Without affinity, falls back to EventLoop::current() on the
     * calling thread; both absent is fatal (CXXKIT_CHECK).
     * Channel: DeferredDeleteEvent pushed via enqueue_event (bypassing post_event's
     * kDeferredDelete guard — owner semantics); dispatch = send_event (filter chain
     * applies) -> event() kDeferredDelete branch deletes this.
     * Parent and child may delete_later together: the parent's ~Object purge removes the
     * child's undelivered entries from the queue body under the lock before the cascade —
     * no double delete (T3).
     * Known limitation: re-posting into a draining ~EventLoop (cascade delete_later) is
     * silently lost / fatal (documented, D33/D34).
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