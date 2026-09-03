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

// cxxkit::kernel Signal/ScopedConnection tests — coverage for signals.hpp (sigslot port).
#include <cxxkit/kernel/signals.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

// Test bodies live inside namespace cxxkit so the header's flat Signal/SignalUnsafe
// aliases and unqualified signals:: names resolve, mirroring library-side usage.
CXXKIT_BEGIN_NAMESPACE
using signals::Connection;
using signals::ScopedConnection;
using signals::Signal;
using signals::SignalUnsafe;

namespace
{

struct ValueCollector
{
    void on_int(int value) { mValues.push_back(value); }
    std::vector<int> mValues;
};

struct Observer : CXXKIT_NAMESPACE::signals::observer
{
    void on_int(int value) { mValues.push_back(value); }
    using CXXKIT_NAMESPACE::signals::observer::disconnect_all;
    std::vector<int> mValues;
};

} // namespace

TEST(Signal, ConnectEmitReceivesArguments)
{
    Signal<int> sig;
    int received = 0;
    Connection conn = sig.connect([&received](int value) { received = value; });
    ASSERT_TRUE(conn.valid());
    EXPECT_TRUE(conn.connected());

    sig(42);
    EXPECT_EQ(42, received);
}

TEST(Signal, MemberFunctionSlotObserverPattern)
{
    Signal<int> sig;
    ValueCollector collector;
    Connection conn = sig.connect(&ValueCollector::on_int, &collector);
    sig(7);
    sig(9);
    ASSERT_EQ(2u, collector.mValues.size());
    EXPECT_EQ(7, collector.mValues[0]);
    EXPECT_EQ(9, collector.mValues[1]);
}

TEST(Signal, MultipleSlotsFireInOrder)
{
    Signal<int> sig;
    std::vector<int> sink;
    sig.connect([&sink](int value) { sink.push_back(value); });
    sig.connect([&sink](int value) { sink.push_back(value * 2); });

    sig(21);
    ASSERT_EQ(2u, sink.size());
    EXPECT_EQ(21, sink[0]);
    EXPECT_EQ(42, sink[1]);
}

TEST(Signal, DisconnectStopsDelivery)
{
    Signal<int> sig;
    int calls = 0;
    Connection conn = sig.connect([&calls](int) { ++calls; });
    sig(1);
    EXPECT_EQ(1, calls);
    ASSERT_TRUE(conn.disconnect());
    EXPECT_FALSE(conn.connected());
    sig(2);
    EXPECT_EQ(1, calls);
    EXPECT_FALSE(conn.disconnect());
}

TEST(Signal, DisconnectAllClearsEverySlot)
{
    Signal<int> sig;
    int calls = 0;
    Connection c1 = sig.connect([&calls](int) { ++calls; });
    Connection c2 = sig.connect([&calls](int) { ++calls; });
    EXPECT_EQ(2u, sig.slot_count());
    sig.disconnect_all();
    EXPECT_EQ(0u, sig.slot_count());
    // slot objects are destroyed with the list, so the weak state handles expire.
    EXPECT_FALSE(c1.valid());
    EXPECT_FALSE(c1.connected());
    EXPECT_FALSE(c2.valid());
    EXPECT_FALSE(c2.connected());
    sig(5);
    EXPECT_EQ(0, calls);
}

TEST(Signal, DisconnectByGroupIdRemovesOnlyThatGroup)
{
    Signal<int> sig;
    int group_a = 0;
    int group_b = 0;
    sig.connect([&group_a](int) { ++group_a; }, 1);
    sig.connect([&group_b](int) { ++group_b; }, 2);
    EXPECT_EQ(2u, sig.slot_count());
    EXPECT_EQ(1u, sig.disconnect(static_cast<signals::GroupId>(1)));
    sig(0);
    EXPECT_EQ(0, group_a);
    EXPECT_EQ(1, group_b);
}

TEST(Signal, DisconnectByPmfAndObject)
{
    Signal<int> sig;
    ValueCollector collector;
    sig.connect(&ValueCollector::on_int, &collector);
    sig.connect(&ValueCollector::on_int, &collector);
    // member-function + object overload: both pmf/object slots match and go.
    EXPECT_EQ(2u, sig.disconnect(&ValueCollector::on_int, &collector));
    sig(1);
    EXPECT_TRUE(collector.mValues.empty());
}

TEST(Signal, ScopedConnectionDisconnectsAtScopeEnd)
{
    Signal<int> sig;
    int calls = 0;
    {
        ScopedConnection scoped = sig.connect_scoped([&calls](int) { ++calls; });
        EXPECT_TRUE(scoped.connected());
        sig(1);
        EXPECT_EQ(1, calls);
    }
    sig(2);
    EXPECT_EQ(1, calls);
}

TEST(Signal, ConnectionBlockBlocksSingleSlot)
{
    Signal<int> sig;
    int calls = 0;
    Connection conn = sig.connect([&calls](int) { ++calls; });

    conn.block();
    EXPECT_TRUE(conn.blocked());
    sig(1);
    EXPECT_EQ(0, calls);

    conn.unblock();
    EXPECT_FALSE(conn.blocked());
    sig(2);
    EXPECT_EQ(1, calls);
}

TEST(Signal, ConnectionBlockerRaII)
{
    Signal<int> sig;
    int calls = 0;
    Connection conn = sig.connect([&calls](int) { ++calls; });

    {
        signals::ConnectionBlocker blocker = conn.blocker();
        EXPECT_TRUE(conn.blocked());
        sig(1);
        EXPECT_EQ(0, calls);
    }
    EXPECT_FALSE(conn.blocked());
    sig(2);
    EXPECT_EQ(1, calls);
}

TEST(Signal, SignalLevelBlockSuppressesEmission)
{
    Signal<int> sig;
    int calls = 0;
    sig.connect([&calls](int) { ++calls; });

    sig.block();
    EXPECT_TRUE(sig.blocked());
    sig(1);
    EXPECT_EQ(0, calls);

    sig.unblock();
    EXPECT_FALSE(sig.blocked());
    sig(2);
    EXPECT_EQ(1, calls);
}

TEST(Signal, GroupOrderAscending)
{
    Signal<int> sig;
    std::vector<int> sink;
    sig.connect([&sink](int value) { sink.push_back(value + 3); }, 3);
    sig.connect([&sink](int value) { sink.push_back(value + 1); }, 1);
    sig.connect([&sink](int value) { sink.push_back(value + 2); }, 2);
    sig.connect([&sink](int value) { sink.push_back(value); }, 0);

    sig(0);
    ASSERT_EQ(4u, sink.size());
    EXPECT_EQ(0, sink[0]);
    EXPECT_EQ(1, sink[1]);
    EXPECT_EQ(2, sink[2]);
    EXPECT_EQ(3, sink[3]);
}

TEST(Signal, TrackedWeakPtrSkipsDeadObject)
{
    Signal<int> sig;
    Connection tracked;
    {
        std::shared_ptr<ValueCollector> holder = std::make_shared<ValueCollector>();
        tracked = sig.connect(&ValueCollector::on_int, holder);
        sig(1);
        EXPECT_EQ(1u, holder->mValues.size());
        EXPECT_EQ(1, holder->mValues[0]);
    }
    // slot_tracked::connected() == !expired() && state-connected: false right after holder death.
    EXPECT_FALSE(tracked.connected());
    sig(2); // weak_ptr lock fails -> slot skipped (and auto-disconnected on emission)
    EXPECT_FALSE(tracked.connected());
}

TEST(Signal, ExtendedSlotCanSelfDisconnect)
{
    Signal<int> sig;
    std::vector<int> sink;
    Connection conn = sig.connect_extended(
        [&sink](Connection &self, int value)
        {
            sink.push_back(value);
            self.disconnect(); // one-shot slot: fire once then remove itself
        });
    ASSERT_TRUE(conn.valid());

    sig(1);
    ASSERT_EQ(1u, sink.size());
    EXPECT_EQ(1, sink[0]);
    EXPECT_FALSE(conn.connected());
    EXPECT_EQ(0u, sig.slot_count());

    sig(2);
    EXPECT_EQ(1u, sink.size());
}

TEST(SignalUnsafe, SmokeEmitAndScopedDisconnect)
{
    SignalUnsafe<int> sig;
    int calls = 0;
    {
        ScopedConnection scoped = sig.connect_scoped([&calls](int) { ++calls; });
        sig(1);
        sig(2);
        EXPECT_EQ(2, calls);
    }
    sig(3);
    EXPECT_EQ(2, calls);
}

TEST(Signal, DestructorDisconnectsAllConnections)
{
    Connection conn;
    {
        Signal<int> sig;
        conn = sig.connect([](int) { });
        ASSERT_TRUE(conn.valid());
        ASSERT_TRUE(conn.connected());
    }
    EXPECT_FALSE(conn.valid());
    EXPECT_FALSE(conn.connected());
}

TEST(Signal, ObserverBaseDisconnectAllOnExplicitCall)
{
    Signal<int> sig;
    Observer obs;
    sig.connect(&Observer::on_int, &obs);
    sig(1);
    ASSERT_EQ(1u, obs.mValues.size());
    EXPECT_EQ(1, obs.mValues[0]);
    EXPECT_EQ(1u, sig.slot_count());

    obs.disconnect_all();
    EXPECT_EQ(0u, sig.slot_count());
    sig(2);
    EXPECT_EQ(1u, obs.mValues.size());
}

} // namespace cxxkit

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
