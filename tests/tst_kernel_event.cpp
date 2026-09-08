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
