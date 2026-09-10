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
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/event.hpp>
#include "fake_dispatcher.hpp"
#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL
using cxxkit::FakeDispatcher;

#    include <algorithm>
#    include <atomic>
#    include <memory>
#    include <thread>
#    include <vector>

namespace
{
class RecordingObject : public cxxkit::Object
{
public:
    explicit RecordingObject(cxxkit::Object *parent = nullptr)
        : Object(parent)
    {
    }
    std::vector<cxxkit::Event::Type> events;
    std::vector<cxxkit::Object *> children_seen;
    bool is_deleted{false}; // T3：delete-later 死亡标志（dtor 置位；事后验证走静态 s_last_recording_dtor_seen）

protected:
    void child_event(cxxkit::ChildEvent *event) override
    {
        events.push_back(event->type());
        children_seen.push_back(event->child());
    }
    void custom_event(cxxkit::Event *event) override
    {
        events.push_back(event->type()); // T2：记录 kUser 等用户事件（队列异步路径）
    }

public:
    ~RecordingObject() override
    {
        is_deleted = true;
        s_last_recording_dtor_seen = true;
    }
    static bool s_last_recording_dtor_seen; // T3：死亡证明事后读取（dtor 后 this 已亡，实例标志不可再读）
};
bool RecordingObject::s_last_recording_dtor_seen = false;

// T3：只观测不拦截的 filter（记录类型序列，始终放行）
class ObserverFilter : public cxxkit::Object
{
public:
    std::vector<cxxkit::Event::Type> types;
    bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
    {
        CXXKIT_UNUSED(watched);
        types.push_back(event->type());
        return false;
    }
};
} // namespace

TEST(Object, constructor_wires_parent_and_child_lists)
{
    RecordingObject parent;
    RecordingObject *child = new RecordingObject(&parent);
    EXPECT_EQ(child->parent(), &parent);
    ASSERT_EQ(parent.children().size(), 1u);
    EXPECT_EQ(parent.children().front(), child);
}

TEST(Object, set_parent_relinks_and_dispatches_child_events)
{
    RecordingObject p1;
    RecordingObject p2;
    RecordingObject child(&p1);
    ASSERT_EQ(p1.children().size(), 1u);

    child.set_parent(&p2); // 摘旧挂新
    EXPECT_EQ(p1.children().size(), 0u);
    ASSERT_EQ(p2.children().size(), 1u);
    // 旧父收到 ChildRemoved、新父收到 ChildAdded（同步派发）
    // p1 先在构造时收到 kChildAdded，set_parent 后收到 kChildRemoved
    ASSERT_EQ(p1.events.size(), 2u);
    EXPECT_EQ(p1.events[0], cxxkit::Event::Type::kChildAdded);
    EXPECT_EQ(p1.events[1], cxxkit::Event::Type::kChildRemoved);
    ASSERT_EQ(p2.events.size(), 1u);
    EXPECT_EQ(p2.events.back(), cxxkit::Event::Type::kChildAdded);

    child.set_parent(nullptr); // 脱离
    EXPECT_EQ(p2.children().size(), 0u);
}

namespace
{
struct Counter
{
    static int alive;
    Counter() { ++alive; }
    ~Counter() { --alive; }
};
int Counter::alive = 0;
class CountedObject : public cxxkit::Object, Counter
{
public:
    explicit CountedObject(cxxkit::Object *parent = nullptr)
        : Object(parent)
    {
    }
};
} // namespace

TEST(Object, destructor_cascades_and_clears_child_parent_pointers)
{
    EXPECT_EQ(Counter::alive, 0);
    {
        CountedObject parent;
        new CountedObject(&parent);
        new CountedObject(&parent);
        EXPECT_EQ(Counter::alive, 3);
    } // 级联 delete：子 dtor 先跑（-2），父 dtor 后跑（-1）
    EXPECT_EQ(Counter::alive, 0);
}

// F1 修订契约：destroying() 在 ~Object 顶部（Object 层）被调——析构期虚派发只到当前层，
// 派生 override 不会被调（Qt 同款规则）。可观测时序锚点：
// ① parent 派生成员析构先于 children 级联（此刻 child 存活计数 > 0，即 ~Object 期 destroying 时刻存活性）；
// ② child 派生层先于自身成员析构；③ 派生 destroying() override 不被派发（order 无该记录）。
namespace
{
struct OrderTracker
{
    enum Tag
    {
        kParentDtorBody,
        kParentMemberDtor,
        kChildDtorBody,
        kChildMemberDtor,
        kDerivedDestroying, // 预期不出现：派发只到 Object 层
    };
    static std::vector<int> order;
    static std::vector<int> alive_snapshots;
    static int child_alive;
    static void reset()
    {
        order.clear();
        alive_snapshots.clear();
        child_alive = 0;
    }
    static void record(Tag tag)
    {
        if (tag == kChildMemberDtor)
        {
            --child_alive; // child 派生成员析构 = 存活期终点的可观测下界
        }
        order.push_back(static_cast<int>(tag));
        alive_snapshots.push_back(child_alive);
    }
};
std::vector<int> OrderTracker::order;
std::vector<int> OrderTracker::alive_snapshots;
int OrderTracker::child_alive = 0;

// 非 Object 成员：dtor 在 ~TrackedObject 主体之后、~Object（destroying + 级联）之前运行
struct TrackerMember
{
    OrderTracker::Tag tag;
    explicit TrackerMember(OrderTracker::Tag t)
        : tag(t)
    {
    }
    ~TrackerMember() { OrderTracker::record(tag); }
};

class TrackedObject : public cxxkit::Object, Counter
{
public:
    explicit TrackedObject(OrderTracker::Tag body_tag, OrderTracker::Tag member_tag, cxxkit::Object *parent = nullptr)
        : Object(parent)
        , mMember(member_tag)
    {
        CXXKIT_UNUSED(body_tag);
        if (mMember.tag == OrderTracker::kChildMemberDtor)
        {
            ++OrderTracker::child_alive; // 构造即存活（child 存活计数语义）
        }
    }
    ~TrackedObject() override
    {
        // body_tag 与 member_tag 同族：kParentMemberDtor ↔ kParentDtorBody、kChildMemberDtor ↔ kChildDtorBody
        OrderTracker::record(mMember.tag == OrderTracker::kParentMemberDtor ? OrderTracker::kParentDtorBody
                                                                            : OrderTracker::kChildDtorBody);
    }

protected:
    void destroying() override
    {
        // F1 修订：析构期派发只到 Object 层——这里永不执行；记录以断言其缺席
        OrderTracker::record(OrderTracker::kDerivedDestroying);
    }

private:
    TrackerMember mMember;
};
} // namespace

TEST(Object, destroying_called_before_children_cascade)
{
    OrderTracker::reset();
    Counter::alive = 0;

    TrackedObject *parent = new TrackedObject(OrderTracker::kParentDtorBody, OrderTracker::kParentMemberDtor);
    TrackedObject *child = new TrackedObject(OrderTracker::kChildDtorBody, OrderTracker::kChildMemberDtor, parent);
    EXPECT_EQ(Counter::alive, 2);
    EXPECT_TRUE(OrderTracker::order.empty());

    delete parent; // ~TrackedObject → 成员析构 → ~Object：destroying → children 级联

    // 时序：parent body → parent 成员析构（=Object 层前哨）→ 级联 child body → child 成员析构
    ASSERT_EQ(OrderTracker::order.size(), 4u);
    EXPECT_EQ(OrderTracker::order[0], static_cast<int>(OrderTracker::kParentDtorBody));
    EXPECT_EQ(OrderTracker::order[1], static_cast<int>(OrderTracker::kParentMemberDtor));
    EXPECT_EQ(OrderTracker::order[2], static_cast<int>(OrderTracker::kChildDtorBody));
    EXPECT_EQ(OrderTracker::order[3], static_cast<int>(OrderTracker::kChildMemberDtor));
    // ~Object（destroying 时刻）child 存活：parent Object 层前哨快照 > 0；child 派生层先于自身成员析构
    EXPECT_EQ(OrderTracker::alive_snapshots[1], 1);
    EXPECT_EQ(OrderTracker::alive_snapshots[2], 1);
    // F1 契约：派生 destroying() override 不被派发
    EXPECT_TRUE(std::find(OrderTracker::order.begin(),
                          OrderTracker::order.end(),
                          static_cast<int>(OrderTracker::kDerivedDestroying)) == OrderTracker::order.end());
    EXPECT_EQ(Counter::alive, 0);
    EXPECT_EQ(OrderTracker::child_alive, 0);
}

TEST(Object, delete_later_runs_on_loop_exec)
{
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    Counter::alive = 0;
    CountedObject *victim = new CountedObject;
    // exec-only：闭包在 exec 期执行 → current 已置 → delete_later 合法；
    // 同闭包尾部再投 exit——FIFO 保证下一轮排空 delete 先于 exit 执行
    loop.post(
        [victim, &loop]()
        {
            victim->delete_later();
            loop.post([&loop]() { loop.exit(0); });
        });
    EXPECT_EQ(loop.exec(), 0);
    EXPECT_EQ(Counter::alive, 0); // delete_later 在 exec 期间执行了 delete
}

TEST(Object, delete_later_without_current_loop_fails)
{
    CountedObject victim; // 栈对象：当前环为空的线程上下文直接 fatal
    EXPECT_DEATH(victim.delete_later(), "");
}

TEST(Object, delete_later_before_exec_fails)
{
    CountedObject victim;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    // exec-only 语义钉子：环已构造但未 exec —— current 未置，delete_later fatal
    EXPECT_DEATH(victim.delete_later(), "");
    EXPECT_EQ(Counter::alive, 1); // fatal 未实际删除 victim
}

// T3 迁移观测：delete_later 走 Event 队列（DeferredDeleteEvent），filter 链看到 kDeferredDelete——
// 老形态（post 闭包内 delete this）filter 永不见此事件类型。送达即死亡：event() 分支 delete this
TEST(Object, delete_later_uses_deferred_delete_event)
{
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    RecordingObject::s_last_recording_dtor_seen = false;
    RecordingObject *victim = new RecordingObject;
    ObserverFilter filter;
    victim->install_event_filter(&filter);
    loop.post(
        [&loop, &victim, &filter]
        {
            victim->delete_later();
            EXPECT_TRUE(filter.types.empty()); // 入队后未派发：filter 未观测
            EXPECT_FALSE(victim->is_deleted);  // 仅入队，未同步删——实际删除走 event() 分支
            loop.exit(0);
        });
    EXPECT_EQ(loop.exec(), 0);
    ASSERT_EQ(filter.types.size(), 1u); // Event 队列派发路径经过 filter 链
    EXPECT_EQ(filter.types[0], cxxkit::Event::Type::kDeferredDelete);
    EXPECT_TRUE(RecordingObject::s_last_recording_dtor_seen); // 死亡证明：event() kDeferredDelete 分支已删 receiver
    victim = nullptr;                                         // 已亡：防测试尾部二次析构（所有权已被 delete this 收走）
}

// T3 限制①解除正名：父与子同时 delete_later——exec 一轮双亡无二删（老形态 = 二次 delete UAF）
// RED 证据：无 ~Object purge 时此用例 UAF 崩（T2 遗留窗口闭合证明）
TEST(Object, delete_later_parent_and_child_no_double_delete)
{
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    Counter::alive = 0;
    CountedObject *parent = new CountedObject;
    CountedObject *child = new CountedObject(parent);
    loop.post(
        [parent, child, &loop]
        {
            parent->delete_later(); // 父子同投——D33 时代 = double delete
            child->delete_later();
            loop.post([&loop]() { loop.exit(0); });
        });
    EXPECT_EQ(loop.exec(), 0);
    EXPECT_EQ(Counter::alive, 0); // 双亡无二删——ASAN 树兜底
}

TEST(Object, set_parent_rejects_descendant_cycle)
{
    RecordingObject a;
    RecordingObject b(&a);
    RecordingObject c(&b);
    EXPECT_DEATH(a.set_parent(&c), ""); // 空匹配器仓内惯例
}

// ---- send_event / filter 链（同步路径） ----

TEST(Object, send_event_delivers_and_returns_accepted)
{
    RecordingObject obj;
    cxxkit::Event event(cxxkit::Event::Type::kUser); // kUser 走 custom_event，RecordingObject 未覆写
    event.accept();
    EXPECT_TRUE(cxxkit::Object::send_event(&obj, &event));
}

TEST(Object, send_event_returns_false_when_ignored)
{
    RecordingObject obj;
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    event.ignore();
    EXPECT_FALSE(cxxkit::Object::send_event(&obj, &event));
}

TEST(Object, event_filter_can_intercept)
{
    class Interceptor : public cxxkit::Object
    {
    public:
        bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
        {
            CXXKIT_UNUSED(watched);
            return event->type() == cxxkit::Event::Type::kUser; // 拦 kUser
        }
    };
    Interceptor filter;
    RecordingObject watched;
    watched.install_event_filter(&filter);

    cxxkit::Event user(cxxkit::Event::Type::kUser);
    EXPECT_FALSE(cxxkit::Object::send_event(&watched, &user)); // 被拦截 = false
    EXPECT_TRUE(watched.events.empty());                       // 未送达
}

namespace
{
class Interceptor : public cxxkit::Object
{
public:
    explicit Interceptor(char tag)
        : mTag(tag)
    {
    }
    bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
    {
        CXXKIT_UNUSED(watched);
        order.push_back(mTag);
        ++count;
        return false; // 全放行只记录顺序
    }
    char mTag;
    static std::vector<char> order;
    int count{0};
};
std::vector<char> Interceptor::order;
} // namespace

TEST(Object, filter_chain_is_lifo)
{
    Interceptor::order.clear();
    Interceptor a('a');
    Interceptor b('b');
    RecordingObject watched;
    watched.install_event_filter(&a); // 头插
    watched.install_event_filter(&b); // 头插 → 后装先过滤
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &event);
    ASSERT_EQ(Interceptor::order.size(), 2u);
    EXPECT_EQ(Interceptor::order[0], 'b'); // 后装先过滤
    EXPECT_EQ(Interceptor::order[1], 'a');
}


// ---- post_event / Event 队列（异步路径，T2） ----

namespace
{
// 计数型 Event 子类（dtor 计数）：验证队列释放所有权——派发后必删 / ~EventLoop 排空必删。
// 用静态计数而非实例成员：事件派发后即被队列 delete，实例成员从此不可访问。
class DeletedEvent : public cxxkit::Event
{
public:
    explicit DeletedEvent(cxxkit::Event::Type type)
        : Event(type)
    {
        ++alive;
    }
    ~DeletedEvent() override { --alive; }
    static int alive;
};
int DeletedEvent::alive = 0;
} // namespace

TEST(Object, remove_event_filter_stops_interception)
{
    Interceptor::order.clear();
    Interceptor a('a');
    Interceptor b('b');
    RecordingObject watched;
    watched.install_event_filter(&a);
    watched.install_event_filter(&b);

    // 初始态：两 filter 都活跃（LIFO 顺序 b→a）
    DeletedEvent e1(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &e1);
    ASSERT_EQ(Interceptor::order.size(), 2u);
    EXPECT_EQ(Interceptor::order[0], 'b');
    EXPECT_EQ(Interceptor::order[1], 'a');

    // remove a → 仅 b 拦截
    watched.remove_event_filter(&a);
    a.count = 0;
    b.count = 0;
    DeletedEvent e2(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &e2);
    EXPECT_EQ(a.count, 0); // a 已移除，不被调用
    EXPECT_EQ(b.count, 1); // b 仍活跃

    // 重复 remove（no-op 契约）→ 仍仅 b
    watched.remove_event_filter(&a);
    b.count = 0;
    DeletedEvent e3(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &e3);
    EXPECT_EQ(a.count, 0);
    EXPECT_EQ(b.count, 1);
}

TEST(Object, destroyed_filter_is_auto_removed_from_watched)
{
    Interceptor::order.clear(); // isolate from prior filter tests' residue
    RecordingObject watched;
    {
        Interceptor filter('x'); // records into Interceptor::order
        watched.install_event_filter(&filter);
    } // filter dies here — reverse registry auto-detaches it (D34 weak contract lifted)
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    event.accept();
    EXPECT_TRUE(cxxkit::Object::send_event(&watched, &event)); // no dangling call, delivered
    EXPECT_EQ(Interceptor::order.size(), 0u);                  // dead filter never invoked
}

TEST(Object, reinstalled_filter_moves_to_front)
{
    Interceptor a('a');
    Interceptor b('b');
    RecordingObject watched;
    watched.install_event_filter(&a);
    watched.install_event_filter(&b); // front: b
    watched.install_event_filter(&a); // reinstall a — moves to front (R5 dedup)
    Interceptor::order.clear();
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &event);
    ASSERT_EQ(Interceptor::order.size(), 2u);
    EXPECT_EQ(Interceptor::order[0], 'a'); // reinstalled a fires FIRST now
    EXPECT_EQ(Interceptor::order[1], 'b');
}


TEST(Object, post_event_delivers_on_process_events)
{
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    RecordingObject obj;
    obj.move_to_thread(&loop); // F1: affinity routing requires a target loop (static phase — loop idle)
    DeletedEvent::alive = 0;
    bool delivered = false;

    // Arrange: counting Event subclass, kUser event; enqueue + drain inside one exec closure
    // (PIT-43 same-round exit chaining guards against cross-round chain breaks).
    loop.post(
        [&]
        {
            DeletedEvent *event = new DeletedEvent(cxxkit::Event::Type::kUser);
            event->accept();
            EXPECT_TRUE(obj.events.empty());         // 入队后未排空：不可见（AAA Arrange）
            cxxkit::Object::post_event(&obj, event); // 所有权转移给队列
            EXPECT_EQ(DeletedEvent::alive, 1);       // 队列持有所有权

            // Act：排空——post 队列空，Event 队列派发 → filter 链 → custom_event 记录 → delete event
            delivered = loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents);
            loop.exit(0);
        });
    EXPECT_EQ(loop.exec(), 0);

    // Assert：kUser 送达（RecordingObject::custom_event 记录）且事件已被队列 delete（所有权释放）
    EXPECT_TRUE(delivered);
    ASSERT_EQ(obj.events.size(), 1u);
    EXPECT_EQ(obj.events[0], cxxkit::Event::Type::kUser);
    EXPECT_EQ(DeletedEvent::alive, 0);
}

TEST(Object, post_event_fifo_order_within_one_drain)
{
    // Same-round FIFO + same-round dispatch contract: entries queued in the post segment
    // are dispatched in the **same round** Event segment (spec §4.1: both queues per round,
    // post first then event — re-entrant enqueue visible same-round, PIT-43 anti-chain-break).
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    RecordingObject obj;
    obj.move_to_thread(&loop); // F1: affinity routing requires a target loop (static phase — both idle)
    DeletedEvent::alive = 0;

    loop.post(
        [&]
        {
            cxxkit::Object::post_event(&obj, new DeletedEvent(cxxkit::Event::Type::kUser));
            cxxkit::Object::post_event(
                &obj,
                new DeletedEvent(static_cast<cxxkit::Event::Type>(static_cast<int>(cxxkit::Event::Type::kUser) + 1)));
            loop.exit(0); // PIT-43: same-round exit chaining
        });
    EXPECT_EQ(loop.exec(), 0);

    // Both entries dispatched (kUser, kUser+1 in order) and deleted by the queue (ownership released)
    ASSERT_EQ(obj.events.size(), 2u);
    EXPECT_EQ(obj.events[0], cxxkit::Event::Type::kUser);
    EXPECT_EQ(obj.events[1], static_cast<cxxkit::Event::Type>(static_cast<int>(cxxkit::Event::Type::kUser) + 1));
    EXPECT_EQ(DeletedEvent::alive, 0);
}

TEST(Object, post_event_respects_filter_chain)
{
    // 队列派发路径走 send_event 内部逻辑：filter 拦截后不送 receiver->event()，但事件仍被 delete
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(new FakeDispatcher));
    class Interceptor : public cxxkit::Object
    {
    public:
        bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
        {
            CXXKIT_UNUSED(watched);
            return event->type() == cxxkit::Event::Type::kUser; // 拦 kUser
        }
    };
    Interceptor filter;
    RecordingObject watched;
    watched.install_event_filter(&filter);
    watched.move_to_thread(&loop); // F1: affinity routing requires a target loop (static phase — loop idle)

    loop.post(
        [&]
        {
            cxxkit::Object::post_event(&watched, new cxxkit::Event(cxxkit::Event::Type::kUser));
            EXPECT_TRUE(loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // 有事件处理 = true
            loop.exit(0);
        });
    EXPECT_EQ(loop.exec(), 0);
    EXPECT_TRUE(watched.events.empty()); // 被拦截未送达
}

TEST(Object, post_event_without_affinity_fails)
{
    RecordingObject obj; // created outside exec -> no affinity
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    EXPECT_DEATH(cxxkit::Object::post_event(&obj, event), "");
    delete event; // ownership not transferred on the fatal path
}

TEST(Object, delete_later_fails_without_affinity_and_current)
{
    // Owner semantics: only delete_later may enqueue a DeferredDeleteEvent (R6 — its ctor is
    // private, friend Object), so post_event's type guard is unreachable from user code. What
    // this test actually pins is delete_later's dual-absence fatal: no affinity and no running
    // EventLoop on the calling thread. (Death-test child exits; no leak accounting needed.)
    CountedObject target; // no affinity, no current — the only reachable fatal on this path
    EXPECT_DEATH(target.delete_later(), "");
}

// ---- thread affinity (Phase 3, T1) ----

TEST(Object, thread_binds_to_creation_loop)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher)));
    RecordingObject outside; // constructed outside exec -> current() null -> thread() == nullptr
    EXPECT_EQ(outside.thread(), nullptr);
    loop.post(
        [&loop]
        {
            RecordingObject inside; // constructed inside exec -> current() == &loop
            EXPECT_EQ(inside.thread(), &loop);
            loop.exit(0);
        });
    EXPECT_EQ(loop.exec(), 0);
}

TEST(Object, move_to_thread_relinks_subtree)
{
    FakeDispatcher *d1 = new FakeDispatcher;
    FakeDispatcher *d2 = new FakeDispatcher;
    cxxkit::EventLoop loop1((std::unique_ptr<cxxkit::AbstractEventDispatcher>(d1)));
    cxxkit::EventLoop loop2((std::unique_ptr<cxxkit::AbstractEventDispatcher>(d2)));
    // Two loops, neither running. Objects created outside exec (thread() == null).
    RecordingObject parent;
    RecordingObject *child = new RecordingObject(&parent);
    EXPECT_EQ(parent.thread(), nullptr);

    // Migrate to loop2 (source affinity is null — allowed).
    parent.move_to_thread(&loop2);
    EXPECT_EQ(parent.thread(), &loop2);
    EXPECT_EQ(child->thread(), &loop2); // subtree migrated

    // Same-loop migration is a no-op.
    parent.move_to_thread(&loop2);
    EXPECT_EQ(parent.thread(), &loop2);

    // Detach (null target) is allowed.
    parent.move_to_thread(nullptr);
    EXPECT_EQ(parent.thread(), nullptr);
    EXPECT_EQ(child->thread(), nullptr);


    // Pending-event migration covered by move_to_thread_migrates_pending_events_cross_loop (T2).
}

// Fatal path: the source loop runs exec while its subtree object migrates away.
// EXPECT_DEATH (empty matcher, repo convention) observes the logging-owned fatal abort.
TEST(ObjectDeathTest, move_to_thread_fails_when_source_loop_running)
{
    // NOTE: loop2 must be declared BEFORE loop1. The death statement (loop1.exec()) runs
    // only in the death-test child; the parent's loop1 still holds the posted closure and
    // drains it in ~EventLoop. With loop2 declared first, loop1 is destroyed first, so the
    // parent's drained closure sees a still-alive loop2 (reverse declaration order would
    // dangle loop2 — is_running() on a destroyed loop SEGVs at d_func()==null).
    FakeDispatcher *d2 = new FakeDispatcher;
    cxxkit::EventLoop loop2((std::unique_ptr<cxxkit::AbstractEventDispatcher>(d2)));
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop1((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher)));
    RecordingObject obj;
    obj.move_to_thread(&loop1);
    loop1.post(
        [&loop1, &loop2]
        {
            RecordingObject victim;        // created inside exec -> affinity loop1
            victim.move_to_thread(&loop2); // source (loop1) IS running -> fatal
            loop1.exit(0);                 // never reached
        });
    EXPECT_DEATH(loop1.exec(), "");
}

// Double-queue order hitchhiker: the post segment fully drains (1 then 2) before the
// Event segment dispatches — the event enqueued inside the first closure lands after both.
TEST(EventLoop, post_functions_run_before_posted_events)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher)));
    RecordingObject obj;
    std::vector<int> order;
    obj.move_to_thread(&loop); // T2: affinity routing targets the object's own loop
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    event->accept();
    loop.post(
        [&order, &obj, event]
        {
            cxxkit::Object::post_event(&obj, event); // event queued AFTER this closure runs
            order.push_back(1);                      // closure body first
        });
    loop.post([&order]() { order.push_back(2); }); // second closure
    loop.post([&loop]() { loop.exit(0); });
    EXPECT_EQ(loop.exec(), 0);
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    ASSERT_EQ(obj.events.size(), 1u); // event delivered after both closures
    EXPECT_EQ(obj.events[0], cxxkit::Event::Type::kUser);
}


// Momus F4 relocated from T1: enqueueing a pending event before move_to_thread relies on T2's
// affinity routing (receiver->thread() targets the enqueue), so the whole scenario lives here.
TEST(Object, move_to_thread_migrates_pending_events_cross_loop)
{
    FakeDispatcher *d1 = new FakeDispatcher;
    FakeDispatcher *d2 = new FakeDispatcher;
    cxxkit::EventLoop loop1((std::unique_ptr<cxxkit::AbstractEventDispatcher>(d1)));
    cxxkit::EventLoop loop2((std::unique_ptr<cxxkit::AbstractEventDispatcher>(d2)));
    RecordingObject parent;
    parent.move_to_thread(&loop1); // affinity loop1
    cxxkit::Event *pending = new cxxkit::Event(cxxkit::Event::Type::kUser);
    pending->accept();
    cxxkit::Object::post_event(&parent, pending); // routed to loop1 (affinity, T2 semantics)
    EXPECT_FALSE(loop2.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // nothing on loop2
    parent.move_to_thread(&loop2);                                                  // migrate WITH the pending entry
    EXPECT_TRUE(parent.events.empty());
    EXPECT_TRUE(loop2.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // delivered on loop2
    ASSERT_EQ(parent.events.size(), 1u);
    EXPECT_EQ(parent.events[0], cxxkit::Event::Type::kUser);
}

// Cross-thread routing: posting from another thread lands on the receiver's affinity loop
// and wakes it; the drain happens on the test thread via process_events (loop not exec'ing).
TEST(Object, post_event_routes_to_receiver_affinity_across_threads)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher)));
    RecordingObject *obj = nullptr;
    // Create the object inside exec so it binds affinity to `loop`.
    loop.post(
        [&loop, &obj]()
        {
            obj = new RecordingObject();
            EXPECT_EQ(obj->thread(), &loop);
            loop.exit(0);
        });
    EXPECT_EQ(loop.exec(), 0);
    ASSERT_NE(obj, nullptr);

    // From ANOTHER thread, post to obj — must route to `loop` and wake it.
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    event->accept();
    std::thread poster(
        [obj, event]()
        {
            cxxkit::Object::post_event(obj, event); // cross-thread: routes to loop, wakes it
        });
    poster.join();
    // Drain on this thread (loop not exec'ing — process_events pulls the entry).
    EXPECT_TRUE(loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents));
    ASSERT_EQ(obj->events.size(), 1u);
    delete obj;
}

TEST(Object, delete_later_uses_affinity_loop_when_present)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher)));
    Counter::alive = 0;
    CountedObject *victim = new CountedObject(); // OUTSIDE exec — Phase 2 would fatal here
    victim->move_to_thread(&loop);               // bind affinity (Momus F4: no in-closure enqueue —
                                                 //  a delete_later inside the closure would dispatch
    // The loop must actually run: an exit closure enqueued BEFORE exec gives one empty post-segment
    // round — no delete_later inside the closure, so no dispatch-time delete (F4d UAF avoided).
    loop.post([&loop]() { loop.exit(0); });
    EXPECT_EQ(loop.exec(), 0); // loop ran empty — victim untouched
    // Loop NOT exec'ing now: Phase 2 would fatal (no current); T2 routes to affinity.
    victim->delete_later();
    EXPECT_TRUE(loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // drains the DeferredDelete
    EXPECT_EQ(Counter::alive, 0);
    // cleanup: victim deleted via the queue
}

// ---- final-review fixes (I1 / I3 / M4) ----

namespace
{
// I1 probe: destructor sets an atomic flag — the delete dispatches on the exec thread
// while the posting thread polls it (a plain bool would be a data race).
class CrossThreadVictim : public cxxkit::Object
{
public:
    ~CrossThreadVictim() override { deleted.store(true); }
    static std::atomic<bool> deleted;
};
std::atomic<bool> CrossThreadVictim::deleted{false};
} // namespace

// I1: cross-thread delete_later liveness. Thread A runs exec(); this thread posts the
// deferred delete for an object affined to A's loop. The discriminative nail is the wake
// counter: between the baseline capture and exit(), ONLY delete_later's wake_up can move
// it (without I1 the counter stays at the baseline -> RED). The bounded yield-wait proves
// thread A actually dispatched the DeferredDeleteEvent.
TEST(Object, delete_later_wakes_affinity_loop_across_threads)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher)));
    CrossThreadVictim::deleted.store(false);
    CrossThreadVictim *victim = new CrossThreadVictim;
    victim->move_to_thread(&loop); // bind affinity while the loop is idle (static phase)

    const int wake_baseline = dispatcher->mWakeUpCount.load();
    std::thread runner(
        [&loop]()
        {
            EXPECT_EQ(loop.exec(), 0); // A: FakeDispatcher never blocks — exec spins rounds
        });

    victim->delete_later(); // B (this thread): enqueue on the affinity loop + wake (I1)
    // Discriminative nail — must be read before exit(): exit() wakes too.
    EXPECT_GT(dispatcher->mWakeUpCount.load(), wake_baseline);

    // Wait (bounded) for A to dispatch the delete, then stop the loop.
    for (int i = 0; i < 2000000 && !CrossThreadVictim::deleted.load(); ++i)
    {
        std::this_thread::yield();
    }
    loop.exit(0);
    runner.join();
    EXPECT_TRUE(CrossThreadVictim::deleted.load()); // deletion completed on thread A
}

// I3 RED->GREEN: mixed-affinity subtree — the child's pending entry lives on the CHILD's
// own old loop (Z), not the root's source loop. Pre-fix, migration drained only the root's
// source, stranding the entry on Z: Z dispatches after the migration (wrong-loop delivery)
// and W stays empty -> the first EXPECT_FALSE/EXPECT_TRUE pair below fails (RED).
TEST(Object, move_to_thread_migrates_from_per_object_old_loops)
{
    FakeDispatcher *dz = new FakeDispatcher;
    FakeDispatcher *dw = new FakeDispatcher;
    cxxkit::EventLoop loopZ((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dz)));
    cxxkit::EventLoop loopW((std::unique_ptr<cxxkit::AbstractEventDispatcher>(dw)));
    RecordingObject parent; // null affinity (created outside exec)
    RecordingObject *child = new RecordingObject(&parent);
    child->move_to_thread(&loopZ); // child affinity Z — mixed subtree

    cxxkit::Event *pending = new cxxkit::Event(cxxkit::Event::Type::kUser);
    pending->accept();
    cxxkit::Object::post_event(child, pending); // routed to the child's affinity loop (Z)

    parent.move_to_thread(&loopW); // subtree -> W; the child's entry must follow Z -> W

    EXPECT_EQ(child->thread(), &loopW);
    EXPECT_FALSE(loopZ.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // Z drained dry
    EXPECT_TRUE(loopW.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents));  // delivered on W
    ASSERT_EQ(child->events.size(), 1u);
    EXPECT_EQ(child->events[0], cxxkit::Event::Type::kUser);
}

// M4: migration notification hook — each migrated object receives exactly one kThreadChange
// through its own event() (direct call, not filterable). N objects -> N notifications.
TEST(Object, move_to_thread_notifies_each_migrated_object)
{
    class ThreadChangeRecorder : public cxxkit::Object
    {
    public:
        explicit ThreadChangeRecorder(cxxkit::Object *parent = nullptr)
            : Object(parent)
        {
        }
        int thread_change_count{0};
        bool event(cxxkit::Event *e) override
        {
            if (e->type() == cxxkit::Event::Type::kThreadChange)
            {
                ++thread_change_count;
            }
            return Object::event(e);
        }
    };
    FakeDispatcher *d = new FakeDispatcher;
    cxxkit::EventLoop loop((std::unique_ptr<cxxkit::AbstractEventDispatcher>(d)));
    ThreadChangeRecorder root;
    ThreadChangeRecorder *child1 = new ThreadChangeRecorder(&root);
    ThreadChangeRecorder *child2 = new ThreadChangeRecorder(child1); // 3-object chain

    root.move_to_thread(&loop); // one notification per migrated object
    EXPECT_EQ(root.thread_change_count, 1);
    EXPECT_EQ(child1->thread_change_count, 1);
    EXPECT_EQ(child2->thread_change_count, 1);

    root.move_to_thread(&loop); // same-loop no-op fires nothing further
    EXPECT_EQ(root.thread_change_count, 1);

    root.move_to_thread(nullptr); // detach is a real migration — notifies again
    EXPECT_EQ(root.thread_change_count, 2);
    EXPECT_EQ(child1->thread_change_count, 2);
    EXPECT_EQ(child2->thread_change_count, 2);
}

#endif // CXXKIT_FEATURE_ENABLE_KERNEL
