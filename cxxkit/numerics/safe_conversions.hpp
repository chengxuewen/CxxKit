/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright 2016 The WebRTC Project Authors.
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

#pragma once

#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/limits.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
namespace detail
{

enum DstSign
{
    DST_UNSIGNED,
    DST_SIGNED
};

enum SrcSign
{
    SRC_UNSIGNED,
    SRC_SIGNED
};

enum DstRange
{
    OVERLAPS_RANGE,
    CONTAINS_RANGE
};

// Helper templates to statically determine if our destination type can contain
// all values represented by the source type.

template <typename Dst,
          typename Src,
          DstSign IsDstSigned = std::numeric_limits<Dst>::is_signed ? DST_SIGNED : DST_UNSIGNED,
          SrcSign IsSrcSigned = std::numeric_limits<Src>::is_signed ? SRC_SIGNED : SRC_UNSIGNED>
struct StaticRangeCheck
{
};

template <typename Dst, typename Src>
struct StaticRangeCheck<Dst, Src, DST_SIGNED, SRC_SIGNED>
{
    typedef std::numeric_limits<Dst> DstLimits;
    typedef std::numeric_limits<Src> SrcLimits;
    // Compare based on max_exponent, which we must compute for integrals.
    static const size_t kDstMaxExponent = DstLimits::is_iec559 ? DstLimits::max_exponent : (sizeof(Dst) * 8 - 1);
    static const size_t kSrcMaxExponent = SrcLimits::is_iec559 ? SrcLimits::max_exponent : (sizeof(Src) * 8 - 1);
    static const DstRange value = kDstMaxExponent >= kSrcMaxExponent ? CONTAINS_RANGE : OVERLAPS_RANGE;
};

template <typename Dst, typename Src>
struct StaticRangeCheck<Dst, Src, DST_UNSIGNED, SRC_UNSIGNED>
{
    static const DstRange value = sizeof(Dst) >= sizeof(Src) ? CONTAINS_RANGE : OVERLAPS_RANGE;
};

template <typename Dst, typename Src>
struct StaticRangeCheck<Dst, Src, DST_SIGNED, SRC_UNSIGNED>
{
    typedef std::numeric_limits<Dst> DstLimits;
    typedef std::numeric_limits<Src> SrcLimits;
    // Compare based on max_exponent, which we must compute for integrals.
    static const size_t kDstMaxExponent = DstLimits::is_iec559 ? DstLimits::max_exponent : (sizeof(Dst) * 8 - 1);
    static const size_t kSrcMaxExponent = sizeof(Src) * 8;
    static const DstRange value = kDstMaxExponent >= kSrcMaxExponent ? CONTAINS_RANGE : OVERLAPS_RANGE;
};

template <typename Dst, typename Src>
struct StaticRangeCheck<Dst, Src, DST_UNSIGNED, SRC_SIGNED>
{
    static const DstRange value = OVERLAPS_RANGE;
};

enum RangeCheckResult
{
    TYPE_VALID = 0,     // Value can be represented by the destination type.
    TYPE_UNDERFLOW = 1, // Value would overflow.
    TYPE_OVERFLOW = 2,  // Value would underflow.
    TYPE_INVALID = 3    // Source value is invalid (i.e. NaN).
};

// This macro creates a RangeCheckResult from an upper and lower bound
// check by taking advantage of the fact that only NaN can be out of range in
// both directions at once.
#define BASE_NUMERIC_RANGE_CHECK_RESULT(is_in_upper_bound, is_in_lower_bound)                                          \
    RangeCheckResult(((is_in_upper_bound) ? 0 : TYPE_OVERFLOW) | ((is_in_lower_bound) ? 0 : TYPE_UNDERFLOW))

template <typename Dst,
          typename Src,
          DstSign IsDstSigned = std::numeric_limits<Dst>::is_signed ? DST_SIGNED : DST_UNSIGNED,
          SrcSign IsSrcSigned = std::numeric_limits<Src>::is_signed ? SRC_SIGNED : SRC_UNSIGNED,
          DstRange IsSrcRangeContained = StaticRangeCheck<Dst, Src>::value>
struct RangeCheckImpl
{
};

// The following templates are for ranges that must be verified at runtime. We
// split it into checks based on signedness to avoid confusing casts and
// compiler warnings on signed an unsigned comparisons.

// Dst range always contains the result: nothing to check.
template <typename Dst, typename Src, DstSign IsDstSigned, SrcSign IsSrcSigned>
struct RangeCheckImpl<Dst, Src, IsDstSigned, IsSrcSigned, CONTAINS_RANGE>
{
    static constexpr RangeCheckResult Check(Src /* value */) { return TYPE_VALID; }
};

// Signed to signed narrowing.
template <typename Dst, typename Src>
struct RangeCheckImpl<Dst, Src, DST_SIGNED, SRC_SIGNED, OVERLAPS_RANGE>
{
    static constexpr RangeCheckResult Check(Src value)
    {
        typedef std::numeric_limits<Dst> DstLimits;
        return DstLimits::is_iec559
                   ? BASE_NUMERIC_RANGE_CHECK_RESULT(value <= static_cast<Src>(utils::numericMax<Dst>()),
                                                     value >= static_cast<Src>(utils::numericMax<Dst>() * -1))
                   : BASE_NUMERIC_RANGE_CHECK_RESULT(value <= static_cast<Src>(utils::numericMax<Dst>()),
                                                     value >= static_cast<Src>(utils::numericMin<Dst>()));
    }
};

// Unsigned to unsigned narrowing.
template <typename Dst, typename Src>
struct RangeCheckImpl<Dst, Src, DST_UNSIGNED, SRC_UNSIGNED, OVERLAPS_RANGE>
{
    static constexpr RangeCheckResult Check(Src value)
    {
        return BASE_NUMERIC_RANGE_CHECK_RESULT(value <= static_cast<Src>(utils::numericMax<Dst>()), true);
    }
};

// Unsigned to signed.
template <typename Dst, typename Src>
struct RangeCheckImpl<Dst, Src, DST_SIGNED, SRC_UNSIGNED, OVERLAPS_RANGE>
{
    static constexpr RangeCheckResult Check(Src value)
    {
        return sizeof(Dst) > sizeof(Src)
                   ? TYPE_VALID
                   : BASE_NUMERIC_RANGE_CHECK_RESULT(value <= static_cast<Src>(utils::numericMax<Dst>()), true);
    }
};

// Signed to unsigned.
template <typename Dst, typename Src>
struct RangeCheckImpl<Dst, Src, DST_UNSIGNED, SRC_SIGNED, OVERLAPS_RANGE>
{
    typedef std::numeric_limits<Src> SrcLimits;
    // Compare based on max_exponent, which we must compute for integrals.
    static constexpr size_t DstMaxExponent() { return sizeof(Dst) * 8; }
    static constexpr size_t SrcMaxExponent()
    {
        return SrcLimits::is_iec559 ? SrcLimits::max_exponent : (sizeof(Src) * 8 - 1);
    }
    static constexpr RangeCheckResult Check(Src value)
    {
        return (DstMaxExponent() >= SrcMaxExponent())
                   ? BASE_NUMERIC_RANGE_CHECK_RESULT(true, value >= static_cast<Src>(0))
                   : BASE_NUMERIC_RANGE_CHECK_RESULT(value <= static_cast<Src>(utils::numericMax<Dst>()),
                                                     value >= static_cast<Src>(0));
    }
};

template <typename Dst, typename Src>
inline constexpr RangeCheckResult RangeCheck(Src value)
{
    static_assert(std::numeric_limits<Src>::is_specialized, "argument must be numeric");
    static_assert(std::numeric_limits<Dst>::is_specialized, "result must be numeric");
    return RangeCheckImpl<Dst, Src>::Check(value);
}
} // namespace detail

// Convenience function that returns true if the supplied value is in range
// for the destination type.
template <typename Dst, typename Src>
inline constexpr bool IsValueInRangeForNumericType(Src value)
{
    return detail::RangeCheck<Dst>(value) == detail::TYPE_VALID;
}

// checked_cast<> and dchecked_cast<> are analogous to static_cast<> for
// numeric types, except that they [D]CHECK that the specified numeric
// conversion will not overflow or underflow. NaN source will always trigger
// the [D]CHECK.
template <typename Dst, typename Src>
inline constexpr Dst checked_cast(Src value)
{
    // CXXKIT_CHECK(IsValueInRangeForNumericType<Dst>(value)); // TODO
    return static_cast<Dst>(value);
}
template <typename Dst, typename Src>
inline constexpr Dst dchecked_cast(Src value)
{
    // CXXKIT_DCHECK(IsValueInRangeForNumericType<Dst>(value)); // TODO
    return static_cast<Dst>(value);
}

// saturated_cast<> is analogous to static_cast<> for numeric types, except
// that the specified numeric conversion will saturate rather than overflow or
// underflow. NaN assignment to an integral will trigger a RTC_CHECK condition.
template <typename Dst, typename Src>
inline Dst saturated_cast(Src value)
{
    // Optimization for floating point values, which already saturate.
    if (std::numeric_limits<Dst>::is_iec559)
    {
        return static_cast<Dst>(value);
    }

    switch (detail::RangeCheck<Dst>(value))
    {
        case detail::TYPE_VALID: return static_cast<Dst>(value);
        case detail::TYPE_UNDERFLOW: return utils::numericMin<Dst>();
        case detail::TYPE_OVERFLOW:
            return utils::numericMax<Dst>();
            // Should fail only on attempting to assign NaN to a saturated integer.
        case detail::TYPE_INVALID: CXXKIT_CHECK_NOTREACHED();
    }
    CXXKIT_CHECK_NOTREACHED();
    return static_cast<Dst>(value);
}
} // namespace utils

CXXKIT_END_NAMESPACE

