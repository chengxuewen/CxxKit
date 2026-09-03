/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
**
** License: MIT License
**
** This file contains code ported from the WebRTC project (https://webrtc.org), originally governed
** by a BSD-style license (WebRTC source tree LICENSE file). Modified for CxxKit.
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

#include <cxxkit/numerics/sequence_number_unwrapper.hpp>
#include <cxxkit/numerics/sequence_number_util.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

// Tests for the wraparound sequence-number utilities (sequence_number_util.hpp) and the
// unwrapper (sequence_number_unwrapper.hpp), ported from WebRTC.
namespace
{

using cxxkit::forward_diff;
using cxxkit::reverse_diff;
using cxxkit::min_diff;
using cxxkit::ahead_of;
using cxxkit::ahead_or_at;
using cxxkit::AscendingSeqNumComp;
using cxxkit::DescendingSeqNumComp;
using cxxkit::SeqNumUnwrapper;
using cxxkit::rtp_timestamp_unwrapper;
using cxxkit::rtp_sequence_number_unwrapper;

constexpr uint8_t kMod8 = 20;  // small odd modulus for full enumeration
constexpr uint8_t kMod10 = 10; // small even modulus for max-distance tie tests

// All ordered pairs (a, b) in [0, M): forward_diff(a, b) + forward_diff(b, a) == M.
TEST(ForwardDiffTest, SmallModulusFullEnumeration)
{
    for (uint8_t a = 0; a < kMod8; ++a)
    {
        for (uint8_t b = 0; b < kMod8; ++b)
        {
            if (a == b)
            {
                continue; // degenerate pair: both diffs are 0
            }
            const uint8_t diff_sum = static_cast<uint8_t>(forward_diff<uint8_t, kMod8>(a, b) +
                                                          forward_diff<uint8_t, kMod8>(b, a));
            EXPECT_EQ(kMod8, diff_sum);
        }
    }
}

// All ordered pairs (a, b) in [0, M): reverse_diff(a, b) + reverse_diff(b, a) == M.
TEST(ReverseDiffTest, SmallModulusFullEnumeration)
{
    for (uint8_t a = 0; a < kMod8; ++a)
    {
        for (uint8_t b = 0; b < kMod8; ++b)
        {
            if (a == b)
            {
                continue; // degenerate pair: both diffs are 0
            }
            const uint8_t diff_sum = static_cast<uint8_t>(reverse_diff<uint8_t, kMod8>(a, b) +
                                                          reverse_diff<uint8_t, kMod8>(b, a));
            EXPECT_EQ(kMod8, diff_sum);
        }
    }
}

// Full-range (M == 0) forward diff crosses the uint8 wraparound.
TEST(ForwardDiffTest, WrapsAcrossZeroUint8)
{
    uint8_t x = 253;
    uint8_t y = 2;
    EXPECT_EQ(5, forward_diff(x, y));
    EXPECT_EQ(251, forward_diff(y, x));
    EXPECT_EQ(0, forward_diff(x, x));
    EXPECT_EQ(253, (forward_diff<uint8_t, 0>(0, 253))); // pure subtraction, no M bound
}

// reverse_diff is forward_diff with swapped direction.
TEST(ReverseDiffTest, WrapsAcrossZeroUint8)
{
    uint8_t x = 253;
    uint8_t y = 2;
    EXPECT_EQ(5, reverse_diff(y, x));
    EXPECT_EQ(251, reverse_diff(x, y));
    EXPECT_EQ(0, reverse_diff(x, x));
}

// min_diff picks the shorter arc; at M == 0 the shorter arc is min(fwd, rev).
TEST(MinDiffTest, PicksShorterArc)
{
    uint8_t x = 253;
    uint8_t y = 2;
    EXPECT_EQ(5, min_diff(x, y));
    EXPECT_EQ(5, min_diff(y, x));
    EXPECT_EQ(0, min_diff(x, x));
    EXPECT_EQ(1, (min_diff<uint8_t, kMod8>(0, 1)));
    EXPECT_EQ(1, (min_diff<uint8_t, kMod8>(1, 0)));
}

// ahead_of: walk 254 -> 255 -> 0 -> 1 forward is always "ahead"; same value is not.
TEST(AheadOfTest, WraparoundBoundaryUint8)
{
    EXPECT_TRUE(ahead_of<uint8_t>(255, 254)); // 255 ahead of 254
    EXPECT_TRUE(ahead_of<uint8_t>(0, 255));   // wraps past 255
    EXPECT_TRUE(ahead_of<uint8_t>(1, 0));     // wraps past 255 again
    EXPECT_FALSE(ahead_of<uint8_t>(254, 1));  // backwards across the wrap
    EXPECT_FALSE(ahead_of<uint8_t>(1, 1));    // equal is not ahead
    EXPECT_FALSE(ahead_of<uint8_t>(255, 1));  // 255 is behind 1 across the wrap
}

// ahead_or_at equals ahead_of, or true for equal values.
TEST(AheadOrAtTest, MatchesAheadOfForDistinctValues)
{
    EXPECT_TRUE(ahead_or_at<uint8_t>(1, 254)); // wraps past 255
    EXPECT_TRUE(ahead_or_at<uint8_t>(0, 255)); // wraps past 255
    EXPECT_TRUE(ahead_or_at<uint8_t>(1, 1));   // the "or at" part
    EXPECT_FALSE(ahead_or_at<uint8_t>(255, 1));
    EXPECT_TRUE((ahead_or_at<uint8_t, kMod8>(8, 0)));
    EXPECT_FALSE((ahead_or_at<uint8_t, kMod8>(0, 8)));
}

// Even-modulus max-distance tie: the higher value is considered ahead (254->0 and 0->254
// both sit at max distance 10 of modulus 20; 0 > 254 in value terms is false, 0 ahead wins
// only on the forward arc of the higher value... keep it concrete below).
TEST(AheadOrAtTest, EvenModulusMaxDistanceTie)
{
    // M = 10, max distance = 5. Pairs at distance 5: (0,5) (1,6) (2,7) (3,8) (4,9).
    // For each pair the higher value must be "ahead or at".
    for (uint8_t low = 0; low < 5; ++low)
    {
        const uint8_t high = low + 5;
        EXPECT_TRUE((ahead_or_at<uint8_t, kMod10>(high, low)));
        EXPECT_FALSE((ahead_or_at<uint8_t, kMod10>(low, high)));
    }
}

// Comparators order a short window of wrapping values in a continuous fashion.
TEST(SeqNumComparatorTest, AscendingAndDescending)
{
    typedef AscendingSeqNumComp<uint8_t> Ascending;
    typedef DescendingSeqNumComp<uint8_t> Descending;
    // comp(a, b) == ahead_of(a, b); "ahead" is only true within the half-range window.
    EXPECT_TRUE(Ascending()(0, 254)); // 0 is ahead of 254: 254 -> 255 -> 0 is 2 steps
    EXPECT_FALSE(Ascending()(254, 0));
    EXPECT_TRUE(Descending()(254, 0)); // descending = "b ahead of a": 254 before 0
    EXPECT_FALSE(Descending()(0, 254));

    // Sorting the window {1, 0, 254, 255} into continuous order (oldest first):
    // a before b iff b is ahead of a — that is exactly the descending comparator.
    uint8_t values[] = {1, 0, 254, 255};
    const std::size_t count = sizeof(values) / sizeof(values[0]);
    std::sort(values, values + count, Descending());
    const uint8_t expected[] = {254, 255, 0, 1};
    EXPECT_TRUE(std::equal(values, values + count, expected));
}

// Continuous unwrap across wraparound stays monotonically increasing.
TEST(SeqNumUnwrapperTest, ContinuousUnwrapMonotonicAcrossWraparound)
{
    SeqNumUnwrapper<uint8_t> unwrapper;
    EXPECT_EQ(254, unwrapper.unwrap(254));
    EXPECT_EQ(255, unwrapper.unwrap(255));
    EXPECT_EQ(256, unwrapper.unwrap(0)); // wraparound grows past int64 anchor
    EXPECT_EQ(257, unwrapper.unwrap(1));
    EXPECT_EQ(258, unwrapper.unwrap(2));
    EXPECT_EQ(262, unwrapper.unwrap(6)); // 6 is ahead of 2 in wrap order: delta +4
}

// First unwrap returns the value itself; forward steps accumulate.
TEST(SeqNumUnwrapperTest, FirstUnwrapAnchorsAtValue)
{
    SeqNumUnwrapper<uint8_t> unwrapper;
    EXPECT_EQ(100, unwrapper.unwrap(100));
    EXPECT_EQ(101, unwrapper.unwrap(101));
    EXPECT_EQ(130, unwrapper.unwrap(130));
    EXPECT_EQ(100, unwrapper.unwrap(100)); // backwards jump: signed delta -30
}

// peek_unwrap reports the would-be result without changing internal state.
TEST(SeqNumUnwrapperTest, PeekDoesNotChangeState)
{
    SeqNumUnwrapper<uint8_t> unwrapper;
    EXPECT_EQ(7, unwrapper.peek_unwrap(7)); // no state yet: identity
    EXPECT_EQ(7, unwrapper.unwrap(7));
    EXPECT_EQ(8, unwrapper.peek_unwrap(8));
    EXPECT_EQ(8, unwrapper.peek_unwrap(8)); // repeated peek is stable
    EXPECT_EQ(6, unwrapper.peek_unwrap(6)); // 6 is "behind" 7: peek still counts +1
    // State unchanged by any peek: unwrap still moves from the 7 anchor.
    EXPECT_EQ(8, unwrapper.unwrap(8));
    EXPECT_EQ(6, unwrapper.unwrap(6)); // then backwards: signed delta -2
}

// reset() re-anchors: the next unwrap returns its argument again.
TEST(SeqNumUnwrapperTest, ResetReanchors)
{
    SeqNumUnwrapper<uint16_t> unwrapper;
    EXPECT_EQ(60000, unwrapper.unwrap(60000));
    EXPECT_EQ(65536, unwrapper.unwrap(0)); // wrapped into the next cycle
    unwrapper.reset();
    EXPECT_EQ(0, unwrapper.peek_unwrap(0)); // fresh state: identity again
    EXPECT_EQ(123, unwrapper.unwrap(123));  // fresh anchor after reset
    EXPECT_EQ(124, unwrapper.unwrap(124));
}

// Modulus-bounded unwrapper (uint8, M = 20) tracks values inside the modulus.
TEST(SeqNumUnwrapperTest, ModulusBoundedUnwrap)
{
    SeqNumUnwrapper<uint8_t, kMod8> unwrapper;
    EXPECT_EQ(7, unwrapper.unwrap(7));
    EXPECT_EQ(9, unwrapper.unwrap(9));
    EXPECT_EQ(0, unwrapper.unwrap(0)); // 0 is behind 9 in mod-20 order: delta -9
    EXPECT_EQ(1, unwrapper.unwrap(1)); // 1 is ahead of 0: delta +1
}

// uint16 RTP sequence-number unwrapper over the classic 65536 wraparound.
TEST(SeqNumUnwrapperTest, RtpSequenceNumberUnwrapper)
{
    rtp_sequence_number_unwrapper unwrapper;
    EXPECT_EQ(65533, unwrapper.unwrap(65533));
    EXPECT_EQ(65534, unwrapper.unwrap(65534));
    EXPECT_EQ(65535, unwrapper.unwrap(65535));
    EXPECT_EQ(65536, unwrapper.unwrap(0)); // classic sequence-number wraparound
    EXPECT_EQ(65537, unwrapper.unwrap(1));
}

// uint32 RTP timestamp unwrapper over the 2^32 wraparound.
TEST(SeqNumUnwrapperTest, RtpTimestampUnwrapper)
{
    rtp_timestamp_unwrapper unwrapper;
    const uint32_t near_max = std::numeric_limits<uint32_t>::max() - 1;
    EXPECT_EQ(4294967294LL, unwrapper.unwrap(near_max));
    EXPECT_EQ(4294967295LL, unwrapper.unwrap(near_max + 1));
    EXPECT_EQ(4294967296LL, unwrapper.unwrap(0)); // 2^32 wraparound
    EXPECT_EQ(4294967297LL, unwrapper.unwrap(1));
}

} // namespace
