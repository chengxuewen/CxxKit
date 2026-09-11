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

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
     *  Destruction order: in ~Object the child cascade runs BEFORE user data is
     *  released — a UserData destructor must not access the owning object or any
     *  of its children.
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

    /** @brief Pre-order DFS for the first child of type @p T. A child matches when
     *  dynamic_cast<T*> succeeds and (name is empty or equals object_name()). With
     *  recursive=false only direct children are visited; returns nullptr when nothing
     *  matches. Header-only (template); dynamic_cast requires a polymorphic T (Object
     *  has a vtable). The object itself is never a candidate (children only). Not
     *  thread-safe (Object is single-threaded by design). @since 0.2 */
    template <typename T>
    T *find_child(const std::string &name = std::string(), bool recursive = true) const
    {
        const Children &kids = this->children();
        for (Children::const_iterator it = kids.begin(); it != kids.end(); ++it)
        {
            Object *child = *it;
            T *typed = dynamic_cast<T *>(child);
            if (typed != nullptr && (name.empty() || child->object_name() == name))
            {
                return typed;
            }
            if (recursive)
            {
                T *found = child->find_child<T>(name, recursive);
                if (found != nullptr)
                {
                    return found;
                }
            }
        }
        return nullptr;
    }

    /** @brief Pre-order DFS collecting every child of type @p T (same match rule as
     *  find_child); results are in visit order. With recursive=false only direct
     *  children are visited. Header-only (template). Not thread-safe (Object is
     *  single-threaded by design). @since 0.2 */
    template <typename T>
    std::vector<T *> find_children(const std::string &name = std::string(), bool recursive = true) const
    {
        std::vector<T *> result;
        this->find_children_internal<T>(name, recursive, result);
        return result;
    }

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

    /** @brief Starts a timer on this object's affinity loop; every tick synchronously
     *  delivers a TimerEvent carrying the returned id to this object (the filter chain
     *  applies).
     *
     *  Dispatch semantics (intentional Qt difference): the event is dispatched
     *  SYNCHRONOUSLY from the loop's timer callback via send_event — it is NOT queued,
     *  so event priority and DeferredDelete compression do not apply, and the tick runs
     *  re-entrantly inside the dispatcher callback.
     *
     *  Thread constraints: requires affinity (thread() != null, fatal otherwise) and must
     *  be called ON the affinity thread (EventLoop::current() == thread(), fatal
     *  otherwise) — dispatcher timer registration is loop-thread-only (uv constraint).
     *
     *  Ids are unique and non-zero (EventLoop::start_timer contract); repeat=false fires
     *  exactly once. A zero-interval one-shot takes the posted fast path (ghost id —
     *  kill_timer on it stays a safe no-op).
     *
     *  Lifetime contract (A1): active timers are stopped by ~Object when destruction runs
     *  on the affinity thread. Destroying the object OFF its affinity thread — or before
     *  killing its timers — leaves the dispatcher-side timer armed so it fires into a dead
     *  object (UAF): the caller must destroy on the affinity thread or kill_timer() every
     *  id first. Timers are NOT migrated by move_to_thread (an id stays registered on the
     *  loop it was started on).
     *  @return the timer id (never 0).
     *  @since 0.2 */
    int start_timer(uint64_t interval_ms, bool repeat = true);

    /** @brief Cancels a timer started by start_timer(). Same thread constraints as
     *  start_timer (affinity + affinity-thread, both fatal when violated). Unknown or
     *  already-stopped ids are a no-op (delegates to EventLoop::stop_timer, documented
     *  no-op for ghost/unknown ids). The id must not be used afterwards.
     *  @since 0.2 */
    void kill_timer(int timer_id);

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

    /** @brief Removes and deletes every queued (not yet dispatched) event for @p receiver:
     *  queued events are dropped without dispatch; the receiver owns nothing afterwards.
     *
     *  Sweeps the receiver's affinity loop queue (post_event routes there) and, mirroring the
     *  ~Object purge, also the calling thread's current() loop (covers a delete_later
     *  current()-fallback residual; a same-loop second sweep is a harmless no-op). A receiver
     *  with no affinity and no current loop cannot have queued entries — no-op.
     *  Call only when no event can be concurrently posted (typically before deletion or from
     *  the receiver's own thread); the receiver pointer must stay valid for the duration of
     *  the call. The scan+erase runs under the target loop's event mutex — atomic with
     *  respect to enqueue and dispatch.
     *  @since 0.2 */
    static void remove_pending_events(Object *receiver);

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

private:
    /** @brief Pre-order recursion shared by find_children; appends matching children
     *  to @p result. Header-only (template). */
    template <typename T>
    void find_children_internal(const std::string &name, bool recursive, std::vector<T *> &result) const
    {
        const Children &kids = this->children();
        for (Children::const_iterator it = kids.begin(); it != kids.end(); ++it)
        {
            Object *child = *it;
            T *typed = dynamic_cast<T *>(child);
            if (typed != nullptr && (name.empty() || child->object_name() == name))
            {
                result.push_back(typed);
            }
            if (recursive)
            {
                child->find_children_internal<T>(name, recursive, result);
            }
        }
    }

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
     *     no double delete (T3).
     *     Compression (Momus F1): repeated delete_later() calls collapse to the FIRST queued
     *     DeferredDeleteEvent — duplicates are detected and deleted under the queue lock at
     *     enqueue time and never dispatched, so the object is deleted exactly once.
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