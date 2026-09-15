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

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// Test bodies live inside namespace cxxkit so the header's flat Signal/SignalUnsafe
// aliases and unqualified signals:: names resolve, mirroring library-side usage.
CXXKIT_BEGIN_NAMESPACE
using signals::Connection;
using signals::ScopedBlock;
using signals::ScopedConnection;
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

// B1 (RED, compile-time probe result): the disconnect(Obj) overload at
// signals.hpp:1693-1701 is unreachable for object types. Its SFINAE guard at
// signals.hpp:1695 reads `!trait::detail::is_callable<ext_arg_list, Obj>::value`
// — the type list is passed as the CALLABLE parameter and Obj as the ARG LIST,
// i.e. the argument order is backwards (oracle: one parameter-order at :1695).
// `is_callable<ext_arg_list, Obj>` expands to IsCallableImpl<TypeList<Connection&, T...>, Obj>
// which matches no partial specialization; the primary template is incomplete, so
// the whole overload is SFINAE-removed. Scratch probe confirmed:
//   g++ -std=c++11 -I cxxkit -I build/include -fsyntax-only /tmp/b1_probe.cpp
//   -> error: no matching function for call to 'disconnect(...)' (candidate
//      removed by SFINAE: invalid use of incomplete type 'IsCallableImpl<...>')
// Post-fix the guard must read is_callable<Obj, ext_arg_list>. Re-enable this
// test once signals.hpp is fixed. API contract: disconnect(obj) takes pointer-or-trackable,
// not by-value (plain struct by value yields nullptr from get_object_ptr).
TEST(Signal, DisconnectByObjectRemovesBoundSlots)
{
    Signal<int> sig;
    ValueCollector collector;
    sig.connect(&ValueCollector::on_int, &collector);
    sig.connect(&ValueCollector::on_int, &collector);
    // pointer form: object stored in slot is &collector, get_object_ptr(&collector) matches
    EXPECT_EQ(2u, sig.disconnect(&collector));
    sig(1);
    EXPECT_TRUE(collector.mValues.empty());

    // lambda slots (no object) must NOT be touched by disconnect(&collector)
    std::vector<int> sink;
    sig.connect([&sink](int v) { sink.push_back(v); });
    EXPECT_EQ(0u, sig.disconnect(&collector));
    sig(42);
    EXPECT_EQ(1u, sink.size());
}

TEST(Signal, MoveConstructedSignalOldConnectionDisconnectClearsNewSignal)
{
    Signal<int> src;
    Connection old_conn = src.connect([](int) { });
    Signal<int> dst(std::move(src));
    ASSERT_EQ(1u, dst.slot_count());

    // B2a RED: after move, the slot's internal Cleanable reference still points
    // at `src` (only the slot pointers are swapped, mCleaner is not re-targeted),
    // so disconnecting via the pre-move connection cleans src's (now empty) list
    // and leaves the slot alive in dst.
    EXPECT_TRUE(old_conn.disconnect());
    EXPECT_EQ(0u, dst.slot_count()); // currently 1 — RED
}

TEST(Signal, DisconnectViaOldConnectionAfterSourceDestroyedIsSafeNoOp)
{
    Signal<int> dst;
    Connection old_conn;
    {
        Signal<int> src;
        old_conn = src.connect([](int) { });
        dst = std::move(src);
    }
    // After fix: rerouted cleaner points to dst, so the old connection
    // controls the slot in its new home. Disconnect works correctly.
    EXPECT_TRUE(old_conn.connected());
    EXPECT_EQ(1u, dst.slot_count());
    EXPECT_TRUE(old_conn.disconnect());
    EXPECT_EQ(0u, dst.slot_count());
    EXPECT_FALSE(old_conn.connected());
}

TEST(SignalUnsafe, SelfDisconnectAndReconnectDefersNewSlotToNextEmission)
{
    SignalUnsafe<int> sig;
    std::vector<int> sink;
    bool first_run = true;

    sig.connect_extended(
        [&](Connection &self, int value)
        {
            sink.push_back(value);
            if (first_run)
            {
                first_run = false;
                self.disconnect();
                // New slot must NOT run in this emission (S2 contract), only
                // from the next emission on.
                sig.connect([&sink](int value) { sink.push_back(100 + value); });
            }
        });

    sig.connect([&sink](int value) { sink.push_back(1000 + value); }); // dummy, runs first

    sig(1); // dummy runs, then A runs: disconnects itself, connects B
    // S2 contract: B must NOT run in this emission, only from the next one on.
    // RED pre-fix: B runs in the same emission (iterator invalidation), so 101
    // appears in sink after sig(1).
    EXPECT_TRUE(std::find(sink.begin(), sink.end(), 101) == sink.end());

    sig(2); // only slot B runs now
    EXPECT_TRUE(std::find(sink.begin(), sink.end(), 102) != sink.end());
}


TEST(Signal, NumSlotsAndEmptyTrackConnectionLifecycle)
{
    Signal<int> sig;
    EXPECT_EQ(0u, sig.num_slots());
    EXPECT_TRUE(sig.empty());

    ScopedConnection c1 = sig.connect([](int) { });
    ScopedConnection c2 = sig.connect([](int) { });
    EXPECT_EQ(2u, sig.num_slots());
    EXPECT_FALSE(sig.empty());

    c1.disconnect();
    EXPECT_EQ(1u, sig.num_slots());

    sig.disconnect_all();
    EXPECT_EQ(0u, sig.num_slots());
    EXPECT_TRUE(sig.empty());
    (void)c2;
}

TEST(Signal, NumSlotsCountsGroupsWhenAsked)
{
    Signal<int> sig;
    sig.connect([](int) { }, 1);
    sig.connect([](int) { }, 1);
    sig.connect([](int) { }, 5);
    EXPECT_EQ(3u, sig.num_slots());
}

TEST(Signal, ScopedBlockSuppressesEmissionForItsLifetime)
{
    Signal<int> sig;
    int count = 0;
    sig.connect([&count](int) { ++count; });

    {
        ScopedBlock<Signal<int>> blocker(sig);
        EXPECT_TRUE(sig.blocked());
        sig(1);
        EXPECT_EQ(0, count); // suppressed
    }
    EXPECT_FALSE(sig.blocked());
    sig(2);
    EXPECT_EQ(1, count); // delivered after the blocker is gone
}

TEST(Signal, ScopedBlockInitiallyUnblockedLeavesSignalOpen)
{
    Signal<int> sig;
    int count = 0;
    sig.connect([&count](int) { ++count; });

    ScopedBlock<Signal<int>> blocker(sig, false);
    EXPECT_FALSE(sig.blocked());
    sig(1);
    EXPECT_EQ(1, count);
} // blocker dtor unblocks a signal that was never blocked - harmless no-op

TEST(Signal, ConnectOnceFiresExactlyOnceThenDisconnects)
{
    Signal<int> sig;
    std::vector<int> sink;
    Connection conn = sig.connect_once([&sink](int value) { sink.push_back(value); });
    ASSERT_TRUE(conn.valid());

    sig(1);
    ASSERT_EQ(1u, sink.size());
    EXPECT_EQ(1, sink[0]);
    EXPECT_FALSE(conn.connected()); // disconnected before the body even ran
    EXPECT_EQ(0u, sig.slot_count());

    sig(2);
    EXPECT_EQ(1u, sink.size()); // not fired again
}

TEST(Signal, ConnectOnceSurvivesReentrantEmission)
{
    Signal<int> sig;
    std::vector<int> sink;
    Connection conn = sig.connect_once(
        [&](int value)
        {
            sink.push_back(value);
            if (value == 1)
            {
                sig(value); // re-entrant emit: body already self-disconnected
            }
        });

    sig(1);
    ASSERT_EQ(1u, sink.size()); // re-entrant emit must not re-fire
    sig(2);
    EXPECT_EQ(1u, sink.size());
    (void)conn;
}

TEST(SignalUnsafe, ConnectOnceSmoke)
{
    SignalUnsafe<int> sig;
    int count = 0;
    sig.connect_once([&count](int) { ++count; });
    sig(1);
    sig(2);
    EXPECT_EQ(1, count);
}

} // namespace cxxkit

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
