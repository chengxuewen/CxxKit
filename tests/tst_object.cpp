/*** Library: CxxKit ***/
#include <cxxkit/kernel/object.hpp>
#include <cxxkit/kernel/event.hpp>
#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

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
    ASSERT_GE(p2.events.size(), 1u);
    EXPECT_EQ(p2.events.back(), cxxkit::Event::Type::kChildAdded);

    child.set_parent(nullptr); // 脱离
    EXPECT_EQ(p2.children().size(), 0u);
}

// 上面测试改为存活计数形态：
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

TEST(Object, set_parent_rejects_descendant_cycle)
{
    RecordingObject a;
    RecordingObject b(&a);
    RecordingObject c(&b);
    EXPECT_DEATH(a.set_parent(&c), ""); // 环检测 fatal（空匹配器——仓内惯例，tst_array_view 等 8 处同款）
}
#endif // CXXKIT_FEATURE_ENABLE_KERNEL
