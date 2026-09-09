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
#include <cxxkit/kernel/object.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/kernel/event.hpp>
#include "fake_dispatcher.hpp"
#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL
using cxxkit::FakeDispatcher;

#    include <algorithm>
#    include <memory>
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

protected:
    void child_event(cxxkit::ChildEvent *event) override
    {
        events.push_back(event->type());
        children_seen.push_back(event->child());
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
    victim->delete_later();
    loop.post([&loop]() { loop.exit(0); }); // FIFO：delete_later 闭包先执行、后 exit——确定性
    EXPECT_EQ(loop.exec(), 0);
    EXPECT_EQ(Counter::alive, 0); // delete_later 在 exec 期间执行了 delete
}

TEST(Object, delete_later_without_current_loop_fails)
{
    CountedObject victim; // 栈对象：当前环为空的线程上下文直接 fatal
    EXPECT_DEATH(victim.delete_later(), "");
}

#endif // CXXKIT_FEATURE_ENABLE_KERNEL
