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

// cxxkit::kernel SignalR tests — S2-style return-value combiners over lazy slot iterators
// (Task 8). Deterministic assertions only. R != void is a compile-time contract
// (static_assert in SignalBaseR); SignalR<void,...> rejection is therefore not
// runtime-testable — the static_assert fires at compile time by construction.
#include <cxxkit/kernel/signals.hpp>

#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

// Test bodies live inside namespace cxxkit so the header's flat aliases resolve,
// mirroring tst_signal_mt.cpp.
CXXKIT_BEGIN_NAMESPACE
using signals::Connection;
using signals::SignalR;
using signals::SignalUnsafeR;

namespace
{

// counting invoker: returns a value AND counts how many times it ran
struct CountingSlot
{
    int value;
    int *counter;

    int operator()() const
    {
        ++*counter;
        return value;
    }
};

// short-circuit combiner: returns the first result above threshold and STOPS —
// later slots must never be invoked (asserted via the counters)
template <typename R, R kThreshold>
struct first_above_threshold
{
    using result_type = Optional<R>;

    template <typename InputIterator>
    Optional<R> operator()(InputIterator first, InputIterator last) const
    {
        while (first != last)
        {
            const R value = *first;
            if (kThreshold < value)
            {
                return value;
            }
            ++first;
        }
        return Optional<R>();
    }
};

// aggregate-all combiner: collects every result (proves full-range iteration)
template <typename R>
struct collect_all
{
    using result_type = std::vector<R>;

    template <typename InputIterator>
    std::vector<R> operator()(InputIterator first, InputIterator last) const
    {
        std::vector<R> out;
        while (first != last)
        {
            out.push_back(*first);
            ++first;
        }
        return out;
    }
};

// combiner that derefs the same iterator twice: proves deref caching
// (one slot invocation for two dereferences of the same iterator)
struct double_deref_combiner
{
    using result_type = Optional<int>;

    template <typename InputIterator>
    Optional<int> operator()(InputIterator first, InputIterator last) const
    {
        if (first == last)
        {
            return Optional<int>();
        }
        const int a = *first;
        const int b = *first; // second deref of the SAME iterator
        EXPECT_EQ(a, b);
        ++first;
        EXPECT_TRUE(first == last);
        return a;
    }
};


} // namespace

TEST(SignalR, SingleSlotReturnValue)
{
    SignalR<int> sig;
    sig.connect([]() { return 42; });

    const Optional<int> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(42, result.value());
}

TEST(SignalR, MultipleSlotsCombinerSeesAllInOrder)
{
    SignalR<int, collect_all<int>> sig;
    sig.connect([]() { return 1; });
    sig.connect([]() { return 2; });
    sig.connect([]() { return 3; });

    const std::vector<int> result = sig();
    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(1, result[0]);
    EXPECT_EQ(2, result[1]);
    EXPECT_EQ(3, result[2]);
}

TEST(SignalR, ZeroSlotsYieldsEmptyResult)
{
    SignalR<int> sig;
    const Optional<int> result = sig();
    EXPECT_FALSE(result.has_value());
}

TEST(SignalR, DefaultCombinerTakesLastValue)
{
    SignalR<std::string> sig;
    sig.connect([]() { return std::string("first"); });
    sig.connect([]() { return std::string("second"); });
    sig.connect([]() { return std::string("third"); });

    const Optional<std::string> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("third", result.value());
}

TEST(SignalR, MaximumCombinerPicksLargest)
{
    SignalR<int, signals::maximum<int>> sig;
    sig.connect([]() { return 7; });
    sig.connect([]() { return 3; });
    sig.connect([]() { return 9; });
    sig.connect([]() { return 5; });

    const Optional<int> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(9, result.value());
}

TEST(SignalR, ShortCircuitCombinerSkipsLaterSlots)
{
    SignalR<int, first_above_threshold<int, 4>> sig;

    int invocations = 0;
    sig.connect(CountingSlot{1, &invocations});
    sig.connect(CountingSlot{5, &invocations});
    sig.connect(CountingSlot{9, &invocations});
    sig.connect(CountingSlot{100, &invocations});

    // threshold 4: slot 1 (value 1) is below -> advance; slot 2 (value 5) is
    // above -> return 5 and STOP. Slots 3/4 must never run.
    const Optional<int> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(5, result.value());
    EXPECT_EQ(2, invocations);
}

TEST(SignalR, SlotArgumentsAreForwarded)
{
    SignalR<int, collect_all<int>, int, int> sig;
    sig.connect([](int a, int b) { return a + b; });
    sig.connect([](int a, int b) { return a * b; });

    const std::vector<int> result = sig(3, 4);
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ(7, result[0]);
    EXPECT_EQ(12, result[1]);
}

TEST(SignalR, GroupOrderingIsAscending)
{
    SignalR<int, collect_all<int>> sig;
    sig.connect([]() { return 1; }, 10);
    sig.connect([]() { return 2; }, -5);
    sig.connect([]() { return 3; }, 0);

    const std::vector<int> result = sig();
    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(2, result[0]); // gid -5 first
    EXPECT_EQ(3, result[1]); // gid 0
    EXPECT_EQ(1, result[2]); // gid 10
}

TEST(SignalR, DisconnectedSlotIsNotInvoked)
{
    SignalR<int, collect_all<int>> sig;
    Connection c1 = sig.connect([]() { return 1; });
    sig.connect([]() { return 2; });

    ASSERT_TRUE(c1.disconnect());

    const std::vector<int> result = sig();
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(2, result[0]);
}

TEST(SignalR, BlockedSlotIsSkippedByCombiner)
{
    SignalR<int, collect_all<int>> sig;
    Connection c1 = sig.connect([]() { return 1; });
    sig.connect([]() { return 2; });

    c1.block();

    const std::vector<int> result = sig();
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(2, result[0]);
}

TEST(SignalR, BlockedSignalRunsCombinerOverEmptyRange)
{
    SignalR<int> sig;
    sig.connect([]() { return 1; });

    sig.block();

    // S2 semantics: blocked signal still returns the combiner's empty-range
    // result (empty Optional), NOT a discarded value.
    const Optional<int> result = sig();
    EXPECT_FALSE(result.has_value());
}

TEST(SignalR, RepeatedDereferenceDoesNotReinvokeSlot)
{
    // the combiner derefs the same iterator twice: exactly one invocation
    SignalR<int, double_deref_combiner> sig;
    int invocations = 0;
    sig.connect(CountingSlot{7, &invocations});

    const Optional<int> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(7, result.value());
    EXPECT_EQ(1, invocations);
}

TEST(SignalR, ConnectExtendedReceivesConnection)
{
    SignalR<int> sig;
    sig.connect_extended([](Connection &self) { return self.connected() ? 1 : 0; });

    const Optional<int> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
}

TEST(SignalR, ConnectOnceStyleSelfDisconnectViaExtended)
{
    SignalR<int, collect_all<int>> sig;
    int invocations = 0;
    sig.connect_extended(
        [&invocations](Connection &self)
        {
            ++invocations;
            self.disconnect();
            return invocations;
        });
    sig.connect([]() { return 99; });

    const std::vector<int> first = sig();
    ASSERT_EQ(2u, first.size());
    EXPECT_EQ(1, first[0]);

    const std::vector<int> second = sig();
    ASSERT_EQ(1u, second.size());
    EXPECT_EQ(99, second[0]);
}

TEST(SignalR, SignalUnsafeRSmoke)
{
    SignalUnsafeR<int> sig;
    sig.connect([]() { return 11; });
    sig.connect([]() { return 22; });

    const Optional<int> result = sig();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(22, result.value());
}

TEST(SignalR, EmptySignalWithCustomCombiner)
{
    SignalR<int, collect_all<int>> sig;
    const std::vector<int> result = sig();
    EXPECT_TRUE(result.empty());
}

CXXKIT_END_NAMESPACE
