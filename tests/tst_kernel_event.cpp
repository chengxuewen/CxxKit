/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
***********************************************************************************************************************/
#include <cxxkit/kernel/event.hpp>

#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

#    include <set>
#    include <mutex>
#    include <thread>
#    include <vector>

TEST(KernelEventTest, TypeAndAcceptFlags)
{
    cxxkit::Event event(cxxkit::Event::Type::kQuit);
    EXPECT_EQ(cxxkit::Event::Type::kQuit, event.type());
    EXPECT_FALSE(event.is_accepted());
    event.accept();
    EXPECT_TRUE(event.is_accepted());
    event.ignore();
    EXPECT_FALSE(event.is_accepted());
    event.set_accepted(true);
    EXPECT_TRUE(event.is_accepted());
}

TEST(KernelEventTest, CopySemantics)
{
    cxxkit::Event original(cxxkit::Event::Type::kTimer);
    original.accept();
    cxxkit::Event copied(original);
    EXPECT_EQ(cxxkit::Event::Type::kTimer, copied.type());
    EXPECT_TRUE(copied.is_accepted());
    cxxkit::Event assigned(cxxkit::Event::Type::kQuit);
    assigned = original;
    EXPECT_EQ(cxxkit::Event::Type::kTimer, assigned.type());
    EXPECT_TRUE(assigned.is_accepted());
}

TEST(KernelEventTest, TimerEventCarriesId)
{
    cxxkit::TimerEvent timer_event(77);
    EXPECT_EQ(cxxkit::Event::Type::kTimer, timer_event.type());
    EXPECT_EQ(77, timer_event.timer_id());
}

TEST(KernelEventTest, ChildEventKindPredicates)
{
    cxxkit::ChildEvent added(cxxkit::Event::Type::kChildAdded, nullptr);
    cxxkit::ChildEvent removed(cxxkit::Event::Type::kChildRemoved, nullptr);
    cxxkit::ChildEvent polished(cxxkit::Event::Type::kChildPolished, nullptr);
    EXPECT_TRUE(added.added());
    EXPECT_TRUE(removed.removed());
    EXPECT_TRUE(polished.polished());
    EXPECT_EQ(nullptr, added.child());
}

TEST(KernelEventTest, RegisterEventTypeHintInRange)
{
    // Pick an unclaimed high hint so this test is order-independent.
    const int hint = 65000;
    EXPECT_EQ(hint, cxxkit::Event::register_event_type(hint));
    // Same hint claimed again must be rejected.
    EXPECT_EQ(-1, cxxkit::Event::register_event_type(hint));
}

TEST(KernelEventTest, RegisterEventTypeAutoIncrements)
{
    const int first = cxxkit::Event::register_event_type();
    const int second = cxxkit::Event::register_event_type();
    const int third = cxxkit::Event::register_event_type();
    EXPECT_GE(first, static_cast<int>(cxxkit::Event::Type::kUser));
    EXPECT_GT(second, first);
    EXPECT_GT(third, second);
}

TEST(KernelEventTest, RegisterEventTypeHintOutOfRange)
{
    EXPECT_EQ(-1, cxxkit::Event::register_event_type(0));
    EXPECT_EQ(-1, cxxkit::Event::register_event_type(999));
    EXPECT_EQ(-1, cxxkit::Event::register_event_type(65536));
    EXPECT_EQ(-1, cxxkit::Event::register_event_type(-2));
}

TEST(KernelEventTest, RegisterEventTypeConcurrentUnique)
{
    std::mutex mutex;
    std::vector<int> ids;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t)
    {
        threads.push_back(std::thread(
            [t, &mutex, &ids]()
            {
                for (int i = 0; i < 250; ++i)
                {
                    // Mixed hint/auto: every 10th registration tries a hint (collisions return
                    // -1 and fall back to auto).
                    int id = -1;
                    if (i % 10 == 0)
                    {
                        id = cxxkit::Event::register_event_type(1000 + ((t * 250 + i) % 500));
                    }
                    if (id < 0)
                    {
                        id = cxxkit::Event::register_event_type();
                    }
                    std::lock_guard<std::mutex> lock(mutex);
                    ids.push_back(id);
                }
            }));
    }
    for (std::size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    ASSERT_EQ(1000u, ids.size());
    std::set<int> unique(ids.begin(), ids.end());
    EXPECT_EQ(ids.size(), unique.size());
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        EXPECT_GE(ids[i], static_cast<int>(cxxkit::Event::Type::kUser));
    }
}

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
