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

// cxxkit::kernel Signal multithreading tests — MT keep-alive / recursion / connect-disconnect-emit stress
// pins for signals.hpp (sigslot port). Deterministic assertions only (count invariants, never timing).
#include <cxxkit/kernel/signals.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Test bodies live inside namespace cxxkit so the header's flat Signal/SignalUnsafe
// aliases and unqualified signals:: names resolve, mirroring library-side usage.
CXXKIT_BEGIN_NAMESPACE
using signals::Connection;
using signals::Signal;
using signals::SignalUnsafe;

namespace
{

// T5 keep-alive token: a payload whose only job is to have a lifetime. The slot
// dereferences it via a raw pointer (safe *because of* the keep-alive guarantee).
struct KeepAliveCounter
{
    std::atomic<int> hits{0};
};

// Spin until an atomic reaches a threshold, bounded so the test can never hang.
// Used only for pacing (making overlap likely), never for assertions.
void spin_until(const std::atomic<int> &value, int threshold)
{
    int spins = 0;
    while (value.load() < threshold && spins < 2000000)
    {
        std::this_thread::yield();
        ++spins;
    }
}

} // namespace

// Task 5 regression pin: tracked-slot keep-alive. A tracked slot holds its target
// alive for the duration of each invocation (slot_tracked::call_slot locks the
// weak_ptr into a local shared_ptr). Destroying the tracked object mid-emission
// must never corrupt an in-flight invocation: every invocation that started runs
// to completion (started == completed == hits), and the ASAN run is the real UAF gate.
TEST(SignalMt, TrackedSlotKeepAliveMidEmission)
{
    Signal<int> sig;
    std::atomic<int> started{0};
    std::atomic<int> completed{0};
    // The tracked payload is deliberately inert: the point is its lifetime, not its
    // content. Shape: the raw pointer is captured by value and dereferenced strictly
    // inside the slot body — the signal's keep-alive (weak_ptr::lock() inside
    // slot_tracked::call_slot) guarantees the object is alive for the duration of
    // each invocation, even after the test drops @p obj mid-emission. Post-join
    // assertions read only the test-scope mirror atomics, never @p raw (the last
    // in-flight keep-alive may have freed the object by then).
    KeepAliveCounter *raw = new KeepAliveCounter();
    std::shared_ptr<KeepAliveCounter> obj(raw);
    std::atomic<int> hits{0};
    Connection conn = sig.connect(
        [raw, &started, &completed, &hits](int)
        {
            ++started;
            // Dereferencing @p raw here is exactly what the keep-alive guarantee
            // covers: the tracked slot has promoted its weak_ptr into a shared_ptr
            // for the duration of this invocation, so the object outlives the slot
            // body even after the test drops @p obj below. ASAN is the real gate.
            raw->hits.fetch_add(1);
            hits.fetch_add(1); // test-scope mirror: survives object destruction
            ++completed;
        },
        obj);

    const int kEmissions = 1000;
    std::thread emitter(
        [&sig, kEmissions]()
        {
            for (int i = 0; i < kEmissions; ++i)
            {
                sig(i);
            }
        });

    // Drop the tracked object mid-emission (pacing spin makes overlap likely).
    // The mutex only serializes the *test's* deref-vs-reset race on its own
    // shared_ptr — the guarantee under test is that the signal's keep-alive holds
    // the object across the slot body even once all outer owners are gone.
    spin_until(started, 50);
    obj.reset(); // now the only owners are in-flight slot keep-alives

    emitter.join();

    // No torn state: every invocation that started completed both its through-the-
    // keep-alive increment and its test-scope mirror increment.
    EXPECT_EQ(started.load(), completed.load());
    EXPECT_EQ(hits.load(), completed.load());
    EXPECT_LE(completed.load(), kEmissions);

    // Post-reset emission: expired weak_ptr -> slot skipped and auto-disconnected.
    sig(0);
    EXPECT_FALSE(conn.connected());
    EXPECT_EQ(0u, sig.slot_count());
}

// Task 6.1 pin: reentrant emission is supported today — emit does not hold the
// lock while invoking slots, in both the mutex and NullMutex variants.
TEST(SignalMt, UnsafeSignalDirectRecursion)
{
    SignalUnsafe<int> sig;
    std::vector<int> order;
    sig.connect(
        [&sig, &order](int depth)
        {
            order.push_back(depth);
            if (depth > 0)
            {
                sig(depth - 1);
            }
        });
    sig(3);
    ASSERT_EQ(4u, order.size());
    EXPECT_EQ(3, order[0]);
    EXPECT_EQ(2, order[1]);
    EXPECT_EQ(1, order[2]);
    EXPECT_EQ(0, order[3]);
}

TEST(SignalMt, MutexSignalRecursionBackToBack)
{
    Signal<int> sig;
    std::vector<int> order;
    sig.connect(
        [&sig, &order](int depth)
        {
            order.push_back(depth);
            if (depth > 0)
            {
                sig(depth - 1); // nested emission while outer emission is in progress
            }
        });
    sig(3);
    sig(1);
    ASSERT_EQ(6u, order.size());
    EXPECT_EQ(3, order[0]);
    EXPECT_EQ(2, order[1]);
    EXPECT_EQ(1, order[2]);
    EXPECT_EQ(0, order[3]);
    EXPECT_EQ(1, order[4]);
    EXPECT_EQ(0, order[5]);
}

// Task 6.2: 4-thread stress — 2 emitters + 1 connector + 1 disconnector racing on
// one mutex-variant signal. Invariant: no crash, counters never negative, and once
// every pooled connection has been disconnected, no slots remain.
TEST(SignalMt, ConcurrentConnectDisconnectEmitStress)
{
    Signal<int> sig;
    std::atomic<int> fired{0};
    std::mutex pool_mutex;
    std::vector<Connection> pool;
    const int kRounds = 2000;

    std::thread emitter_a(
        [&sig, kRounds]()
        {
            for (int i = 0; i < kRounds; ++i)
            {
                sig(i);
            }
        });
    std::thread emitter_b(
        [&sig, kRounds]()
        {
            for (int i = 0; i < kRounds; ++i)
            {
                sig(i);
            }
        });
    std::thread connector(
        [&sig, &pool, &pool_mutex, &fired, kRounds]()
        {
            for (int i = 0; i < kRounds; ++i)
            {
                Connection conn = sig.connect([&fired](int) { fired.fetch_add(1); });
                std::lock_guard<std::mutex> lock(pool_mutex);
                pool.push_back(conn);
            }
        });
    std::thread disconnector(
        [&pool, &pool_mutex]()
        {
            for (;;)
            {
                Connection conn;
                {
                    std::lock_guard<std::mutex> lock(pool_mutex);
                    if (pool.empty())
                    {
                        return;
                    }
                    conn = pool.back();
                    pool.pop_back();
                }
                conn.disconnect();
            }
        });

    emitter_a.join();
    emitter_b.join();
    connector.join();
    disconnector.join();

    // Drain whatever raced in after the disconnector saw an empty pool:
    // disconnect is idempotent, so leftover pool entries (or already-dead
    // connections) all land at zero.
    for (Connection &conn : pool)
    {
        conn.disconnect();
    }
    pool.clear();

    EXPECT_EQ(0u, sig.slot_count());
}

// Task 6.3a: concurrent emit vs connect/disconnect (cow contract MT pin).
TEST(SignalMt, EmitWhileAnotherThreadConnectsAndDisconnects)
{
    Signal<int> sig;
    std::atomic<int> fired{0};
    const int kEmissions = 2000;
    const int kMutations = 2000;

    std::thread emitter(
        [&sig, &fired, kEmissions]()
        {
            for (int i = 0; i < kEmissions; ++i)
            {
                sig(i);
            }
        });
    std::thread mutator(
        [&sig, &fired, kMutations]()
        {
            for (int i = 0; i < kMutations; ++i)
            {
                Connection conn = sig.connect([&fired](int) { fired.fetch_add(1); });
                if ((i % 2) == 0)
                {
                    conn.disconnect();
                }
                // odd rounds leave the connection: signal dtor cleans up.
            }
        });

    emitter.join();
    mutator.join();
    sig.disconnect_all();
    EXPECT_EQ(0u, sig.slot_count());
}

// Task 6.3b: Connection held by a non-emitting thread disconnects mid-emission-loop.
TEST(SignalMt, CrossThreadDisconnectDuringEmissionLoop)
{
    Signal<int> sig;
    std::atomic<int> fired{0};
    Connection conn = sig.connect([&fired](int) { fired.fetch_add(1); });
    const int kEmissions = 2000;

    std::thread emitter(
        [&sig, kEmissions]()
        {
            for (int i = 0; i < kEmissions; ++i)
            {
                sig(i);
            }
        });
    std::thread disconnector([&conn]() { conn.disconnect(); });

    emitter.join();
    disconnector.join();

    EXPECT_FALSE(conn.connected());
    const int after = fired.load();
    sig(0); // post-disconnect emission must not fire the slot
    EXPECT_EQ(after, fired.load());
    EXPECT_EQ(0u, sig.slot_count());
}

// Task 6.3c: move-constructed signal keeps old connections routable — the old
// Connection must disconnect through the moved-into signal, even from another
// thread while the new signal is being emitted (B2 reroute MT regression pin).
TEST(SignalMt, MovedSignalOldConnectionCrossThreadDisconnect)
{
    Signal<int> src;
    std::atomic<int> fired{0};
    Connection conn = src.connect([&fired](int) { fired.fetch_add(1); });

    Signal<int> dst(std::move(src));
    EXPECT_EQ(1u, dst.slot_count());
    EXPECT_TRUE(conn.connected());

    const int kEmissions = 2000;
    std::thread emitter(
        [&dst, kEmissions]()
        {
            for (int i = 0; i < kEmissions; ++i)
            {
                dst(i);
            }
        });
    std::thread disconnector([&conn]() { conn.disconnect(); }); // old connection, new signal

    emitter.join();
    disconnector.join();

    EXPECT_FALSE(conn.connected());
    EXPECT_EQ(0u, dst.slot_count());
    const int after = fired.load();
    dst(0);
    EXPECT_EQ(after, fired.load());
}

} // namespace cxxkit

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
