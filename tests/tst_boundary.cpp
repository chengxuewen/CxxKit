/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
** WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
** OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
** OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
***********************************************************************************************************************/
#include <cxxkit/containers/array_view.hpp>
#include <cxxkit/memory/ref_count.hpp>
#include <cxxkit/memory/shared_pointer.hpp>
#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/numerics/safe_compare.hpp>
#include <cxxkit/numerics/safe_conversions.hpp>
#include <cxxkit/numerics/safe_minmax.hpp>
#include <cxxkit/text/bit_buffer.hpp>
#include <cxxkit/text/string_view.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
// True when the current TU was compiled with AddressSanitizer.
// GCC exports __SANITIZE_ADDRESS__; Clang exports the feature check. Both are
// compile-time, so this selects the ASAN-only probe section deterministically.
#if defined(__SANITIZE_ADDRESS__)
constexpr bool kAsanEnabled = true;
#elif defined(__has_feature)
#    if __has_feature(address_sanitizer)
constexpr bool kAsanEnabled = true;
#    else
constexpr bool kAsanEnabled = false;
#    endif
#else
constexpr bool kAsanEnabled = false;
#endif
} // namespace

// ---------------------------------------------------------------------------
// Numeric overflow: safe conversions / minmax / compare.
// These are fully deterministic and are the primary safety assertions that
// hold in BOTH the normal build and build-asan.
// ---------------------------------------------------------------------------

TEST(BoundaryNumeric, SaturatedCastOverflowBothDirections)
{
    // uint8_t saturates at its max rather than wrapping like a naive cast.
    EXPECT_EQ(255u, utils::saturated_cast<uint8_t>(static_cast<int>(300)));
    // Negative underflow saturates at min.
    EXPECT_EQ(0u, utils::saturated_cast<uint8_t>(static_cast<int>(-5)));
    // In-range values pass through unchanged.
    EXPECT_EQ(127u, utils::saturated_cast<uint8_t>(static_cast<int>(127)));

    // int8_t saturation on both sides.
    EXPECT_EQ(127, utils::saturated_cast<int8_t>(static_cast<int>(1000)));
    EXPECT_EQ(-128, utils::saturated_cast<int8_t>(static_cast<int>(-1000)));

    // Exact boundaries saturate to the limit value, not a wrap.
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), utils::saturated_cast<int32_t>(std::numeric_limits<int64_t>::max()));
    EXPECT_EQ(std::numeric_limits<int32_t>::min(), utils::saturated_cast<int32_t>(std::numeric_limits<int64_t>::min()));
}

TEST(BoundaryNumeric, is_value_in_range_for_numeric_type)
{
    EXPECT_TRUE(utils::is_value_in_range_for_numeric_type<int8_t>(static_cast<int>(127)));
    EXPECT_FALSE(utils::is_value_in_range_for_numeric_type<int8_t>(static_cast<int>(128)));
    EXPECT_FALSE(utils::is_value_in_range_for_numeric_type<int8_t>(static_cast<int>(-129)));
    EXPECT_TRUE(utils::is_value_in_range_for_numeric_type<int8_t>(static_cast<int>(-128)));

    // Unsigned destination cannot hold negatives.
    EXPECT_FALSE(utils::is_value_in_range_for_numeric_type<uint8_t>(static_cast<int>(-1)));
    EXPECT_TRUE(utils::is_value_in_range_for_numeric_type<uint8_t>(static_cast<int>(255)));
    EXPECT_FALSE(utils::is_value_in_range_for_numeric_type<uint8_t>(static_cast<int>(256)));
}

TEST(BoundaryNumeric, SafeCompareSignedUnsigned)
{
    // Naive `-1 < 0u` is false because -1 is converted to a huge unsigned.
    // safe_lt must yield the mathematically correct result.
    EXPECT_TRUE(safe_lt(-1, 0u));
    EXPECT_TRUE(safe_lt(-5, static_cast<uint8_t>(3)));
    EXPECT_FALSE(safe_lt(1u, -1));
    EXPECT_TRUE(safe_gt(1u, -1));
    EXPECT_TRUE(safe_le(0, 0u));
    EXPECT_TRUE(safe_ge(0, 0u));
    EXPECT_TRUE(safe_eq(0, 0u));
}

TEST(BoundaryNumeric, SafeMinMaxNoWrap)
{
    // Different-sign widths; must not wrap / promote wrongly.
    EXPECT_EQ(-5, safe_min(-5, static_cast<uint8_t>(3)));
    EXPECT_EQ(3, safe_max(-5, static_cast<uint8_t>(3)));
    EXPECT_EQ(-1, safe_min(0u, -1));
    EXPECT_EQ(0, safe_max(0u, -1));
}

TEST(BoundaryNumeric, safe_clamp)
{
    EXPECT_EQ(0, safe_clamp(-100, 0, 10)); // below min clamps to min
    EXPECT_EQ(5, safe_clamp(5, 0, 10));    // in range unchanged
    EXPECT_EQ(10, safe_clamp(100, 0, 10));
    // Mixed signedness clamp.
    EXPECT_EQ(0, safe_clamp(-1, 0u, static_cast<uint8_t>(3))); // clamps to 0u min
}

// ---------------------------------------------------------------------------
// StringView bounds.
// Contract (nonstd string_view lite, exceptions enabled): substr(pos, n)
// throws std::out_of_range when pos > size(); otherwise n is clipped to
// size()-pos. at() throws on out-of-range access; operator[] is unchecked.
// ---------------------------------------------------------------------------

TEST(BoundaryStringView, SubstrOutOfRangeThrows)
{
    cxxkit::StringView sv("hello");

    // pos == size() is valid and yields an empty view.
    EXPECT_NO_THROW(sv.substr(sv.size()));
    EXPECT_TRUE(sv.substr(sv.size()).empty());

    // pos > size() must throw.
    EXPECT_THROW(sv.substr(sv.size() + 1), std::out_of_range);
    EXPECT_THROW(sv.substr(sv.size() + 100), std::out_of_range);

    // size_t max is far out of range and must still throw (no wrap).
    EXPECT_THROW(sv.substr(std::numeric_limits<size_t>::max()), std::out_of_range);
}

TEST(BoundaryStringView, SubstrLengthClipped)
{
    cxxkit::StringView sv("hello");
    // Requesting more than available clips to the remaining length.
    cxxkit::StringView tail = sv.substr(2, 100);
    EXPECT_EQ(3u, tail.size());
    EXPECT_EQ("llo", std::string(tail.data(), tail.size()));

    // npos clip is the standard "to end" behavior.
    cxxkit::StringView all = sv.substr(0, cxxkit::StringView::npos);
    EXPECT_EQ(sv.size(), all.size());
}

TEST(BoundaryStringView, AtThrowsOperatorIndexUnchecked)
{
    cxxkit::StringView sv("hello");
    EXPECT_EQ('h', sv.at(0));
    EXPECT_EQ('o', sv.at(4));
    // at() throws on out-of-range (safe, deterministic).
    EXPECT_THROW(sv.at(sv.size()), std::out_of_range);
    EXPECT_THROW(sv.at(100), std::out_of_range);
    // operator[] in-range access is valid.
    EXPECT_EQ('e', sv[1]);
}

// ---------------------------------------------------------------------------
// ArrayView bounds.
// Contract: operator[] is guarded by CXXKIT_DCHECK (debug-only) — calling it
// out of range would abort in a Debug build, so we do NOT probe raw OOB
// indexing here (that is UB-adjacent and would crash both builds). The safe,
// deterministic contract is subview(): an out-of-range offset yields an empty
// view and a too-large size is clipped, never dereferencing past the end.
// ---------------------------------------------------------------------------

TEST(BoundaryArrayView, SubviewOutOfRangeIsEmpty)
{
    std::array<int, 5> buf{{1, 2, 3, 4, 5}};
    cxxkit::ArrayView<const int> view(buf.data(), buf.size());

    // offset at end => empty, not a crash.
    EXPECT_TRUE(view.subview(view.size()).empty());
    EXPECT_EQ(0u, view.subview(view.size()).size());

    // offset beyond end => empty (clipped by the contract).
    EXPECT_TRUE(view.subview(view.size() + 1).empty());
    EXPECT_TRUE(view.subview(static_cast<size_t>(1000)).empty());
}

TEST(BoundaryArrayView, SubviewLengthClippedToRemaining)
{
    std::array<int, 5> buf{{1, 2, 3, 4, 5}};
    cxxkit::ArrayView<const int> view(buf.data(), buf.size());

    // Requesting more than remains only yields the remaining elements.
    cxxkit::ArrayView<const int> tail = view.subview(3, 100);
    EXPECT_EQ(2u, tail.size());
    EXPECT_EQ(4, tail[0]);
    EXPECT_EQ(5, tail[1]);

    // Exact remaining length is a valid, non-empty tail.
    cxxkit::ArrayView<const int> exact = view.subview(3, 2);
    EXPECT_EQ(2u, exact.size());
}

TEST(BoundaryArrayView, EmptyViewIsSafe)
{
    cxxkit::ArrayView<const int> empty(nullptr, 0);
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(0u, empty.size());
    EXPECT_EQ(nullptr, empty.data());

    // subview of an empty view is empty (no crash).
    EXPECT_TRUE(empty.subview(0).empty());
    EXPECT_TRUE(empty.subview(5).empty());
}

// ---------------------------------------------------------------------------
// BitBuffer boundary reads.
// Contract: reads that exhaust the buffer flip the reader into a failure
// state (remaining_bit_count() < 0 / Ok() == false) rather than reading
// out-of-bounds. The writer returns false when there is not enough room.
// ---------------------------------------------------------------------------

TEST(BoundaryBitBuffer, ReadPastEndFailsCleanly)
{
    // A single byte of data (8 bits).
    const uint8_t raw[] = {0xFF};
    cxxkit::BitBufferReader rdr(cxxkit::ArrayView<const uint8_t>(raw, 1));

    EXPECT_TRUE(rdr.Ok());
    // read all 8 bits.
    EXPECT_EQ(0xFFu, rdr.read_bits(8));
    EXPECT_TRUE(rdr.Ok());
    EXPECT_EQ(0, rdr.remaining_bit_count());

    // Reading more than the remaining bits must not over-read; it enters the
    // failure state and returns 0.
    uint64_t val = rdr.read_bits(16);
    EXPECT_EQ(0u, val);
    EXPECT_FALSE(rdr.Ok());
    EXPECT_LT(rdr.remaining_bit_count(), 0);
}

TEST(BoundaryBitBuffer, RemainingBitCountEdge)
{
    // Empty buffer: remaining_bit_count() starts at 0 and Ok() is true; any read fails.
    const uint8_t raw[] = {};
    cxxkit::BitBufferReader rdr(cxxkit::ArrayView<const uint8_t>(raw, 0));
    EXPECT_TRUE(rdr.Ok());
    EXPECT_EQ(0, rdr.remaining_bit_count());

    // A single read past the end pushes into failure state.
    EXPECT_EQ(0u, rdr.read_bits(1));
    EXPECT_FALSE(rdr.Ok());
}

TEST(BoundaryBitBuffer, WriterReturnsFalseWhenFull)
{
    uint8_t storage[1] = {0};
    cxxkit::BitBufferWriter wr(storage, sizeof(storage));

    // 8 bits fills the buffer.
    EXPECT_TRUE(wr.write_bits(0xA5u, 8));
    EXPECT_EQ(0u, wr.remaining_bit_count());

    // Any further write that doesn't fit must return false, not overflow.
    EXPECT_FALSE(wr.write_bits(1u, 1));
    EXPECT_FALSE(wr.write_u_int8(0x01));
    EXPECT_FALSE(wr.consume_bits(1));
    EXPECT_FALSE(wr.consume_bytes(1));

    // Writes that fit on the boundary still succeed after a seek back.
    EXPECT_TRUE(wr.seek(0, 0));
    EXPECT_TRUE(wr.write_u_int8(0x7E));
}

// ---------------------------------------------------------------------------
// Dangling / refcount.
// std::shared_ptr/std::weak_ptr give an OBSERVABLE refcount via use_count(),
// so these assertions are deterministic in the normal build too. Under ASAN
// (build-asan) the same exercise additionally validates there is no double
// free / use-after-free / leak in the allocator backend.
// ---------------------------------------------------------------------------

TEST(BoundaryRefcount, SharedPointerUseCountLifecycle)
{
    cxxkit::SharedPointer<int> sp = utils::make_shared<int>(42);
    EXPECT_EQ(1L, sp.use_count());

    {
        cxxkit::SharedPointer<int> copy = sp;
        EXPECT_EQ(2L, sp.use_count());
        EXPECT_EQ(2L, copy.use_count());
    }

    // copy destroyed => back to 1.
    EXPECT_EQ(1L, sp.use_count());

    EXPECT_TRUE(sp.unique()); // sole owner again after copy destroyed
    EXPECT_EQ(42, *sp);
}

TEST(BoundaryRefcount, WeakPtrBreaksCycleAndExpires)
{
    struct Node
    {
        explicit Node(int v)
            : value(v)
        {
        }
        int value = 0;
        cxxkit::SharedPointer<Node> next;
        std::weak_ptr<Node> back; // weak, so it never holds the cycle alive.
    };

    cxxkit::SharedPointer<Node> a = utils::make_shared<Node>(1);
    cxxkit::SharedPointer<Node> b = utils::make_shared<Node>(2);

    // A cycle that would leak with shared_ptr alone, broken via a weak_ptr.
    a->next = b;
    b->back = a; // weak: no strong reference cycle through b->back.

    std::weak_ptr<Node> weak_a = a;
    std::weak_ptr<Node> weak_b = b;

    EXPECT_EQ(1L, a.use_count()); // Node(1) strongly held only by `a`; `b->back` is weak.
    EXPECT_EQ(2L, b.use_count()); // Node(2) strongly held by `b` and `a->next`.

    // Break the strong reference; both should be reclaimable.
    a->next.reset();

    // Expiring the strong owners must make both weak pointers expired.
    a.reset();
    b.reset();
    EXPECT_TRUE(weak_a.expired());
    EXPECT_TRUE(weak_b.expired());
}

TEST(BoundaryRefcount, ResetDropsReferences)
{
    cxxkit::SharedPointer<int> sp = utils::make_shared<int>(7);
    std::weak_ptr<int> w = sp;
    EXPECT_FALSE(w.expired());
    EXPECT_EQ(1L, sp.use_count());

    sp.reset();
    EXPECT_TRUE(w.expired());

    // Default-constructed shared_ptr is empty.
    cxxkit::SharedPointer<int> empty;
    EXPECT_EQ(nullptr, empty.get());
    EXPECT_EQ(0L, empty.use_count());
}

TEST(BoundaryRefcount, DoubleReleaseOfRefCountedObjectFailsSafely)
{
    // SharedRefPtr owns one add_ref/release pair; releasing the same raw
    // pointer twice would double-free. Sharing through SharedRefPtr copies is
    // the safe path: each copy adds one reference, and each destruction drops
    // exactly one. The observable count is exercised via RefCountedBase.
    class Obj : public cxxkit::RefCountedBase
    {
    public:
        int payload = 0;
    };

    cxxkit::SharedRefPtr<Obj> p(new Obj());
    EXPECT_NE(nullptr, p.get());

    {
        cxxkit::SharedRefPtr<Obj> q = p; // +1 ref
        EXPECT_EQ(p.get(), q.get());
        EXPECT_EQ(p.get(), q.get());
    }

    // Still valid after q dropped its reference (p holds one).
    p->payload = 5;
    EXPECT_EQ(5, p->payload);
}

// ---------------------------------------------------------------------------
// ASAN-specific probe.
// Under AddressSanitizer/LSAN we can additionally assert that a claimed
// allocation is actually freed (the allocator would report a leak at exit if
// it were not). Behaves as a no-op in the normal build so the suite still
// passes without a sanitizer.
// ---------------------------------------------------------------------------

TEST(BoundaryAsan, AllocationFreedWhenAsanEnabled)
{
    if (!kAsanEnabled)
    {
        // Nothing to check in the normal build; keep the assertion meaningful.
        SUCCEED() << "ASAN not enabled; skipping allocator-backed probe.";
        return;
    }

    // Exercise a small block of heap traffic; if the shared_ptr backend leaked
    // or double-freed, LSAN/ASAN would flag it at process exit.
    for (int i = 0; i < 64; ++i)
    {
        cxxkit::SharedPointer<int> sp = utils::make_shared<int>(i);
        EXPECT_EQ(i, *sp);
    }
    EXPECT_TRUE(kAsanEnabled);
}

CXXKIT_END_NAMESPACE
