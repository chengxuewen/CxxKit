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

/** @file
 * @brief Sequence-number wraparound utilities (forward/reverse/min diff, ahead comparisons,
 * ascending/descending comparators), ported from WebRTC `sequence_number_util.h` and `mod_ops.h`.
 */

#pragma once

#include <cxxkit/base/global.hpp>

#include <algorithm>
#include <limits>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE

/** @brief Forward difference between two wrapping sequence numbers.
 * How far `b` is ahead of `a` when moving forward through the sequence.
 * Example (uint8_t): forward_diff(253, 2) == 5; forward_diff(2, 253) == 251.
 * If `M > 0` wrapping occurs at `M`, if `M == 0` at the largest value representable by `T`.
 *
 * @tparam T Unsigned integer type.
 * @tparam M Sequence modulus; 0 means full range of `T`.
 */
template <typename T, T M>
inline typename std::enable_if<(M > 0), T>::type forward_diff(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    return a <= b ? b - a : M - (a - b);
}

/** @overload forward_diff(T a, T b)
 * Modulus `M == 0`: wrap at the largest value representable by `T`.
 */
template <typename T, T M>
inline typename std::enable_if<(M == 0), T>::type forward_diff(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    return b - a;
}

/** @brief Forward difference with default (full-range) modulus. */
template <typename T>
inline T forward_diff(T a, T b)
{
    return forward_diff<T, 0>(a, b);
}

/** @brief Reverse difference between two wrapping sequence numbers.
 * How far `a` is ahead of `b` when moving backward through the sequence.
 * Example (uint8_t): reverse_diff(2, 253) == 5; reverse_diff(253, 2) == 251.
 *
 * @tparam T Unsigned integer type.
 * @tparam M Sequence modulus; 0 means full range of `T`.
 */
template <typename T, T M>
inline typename std::enable_if<(M > 0), T>::type reverse_diff(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    return b <= a ? a - b : M - (b - a);
}

/** @overload reverse_diff(T a, T b)
 * Modulus `M == 0`: wrap at the largest value representable by `T`.
 */
template <typename T, T M>
inline typename std::enable_if<(M == 0), T>::type reverse_diff(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    return a - b;
}

/** @brief Reverse difference with default (full-range) modulus. */
template <typename T>
inline T reverse_diff(T a, T b)
{
    return reverse_diff<T, 0>(a, b);
}

/** @brief Minimum distance between two wrapping sequence numbers:
 * min(forward_diff(a, b), reverse_diff(a, b)).
 */
template <typename T, T M = 0>
inline T min_diff(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    return std::min(forward_diff<T, M>(a, b), reverse_diff<T, M>(a, b));
}

/** @brief Test if the sequence number `a` is ahead or at sequence number `b`.
 * If `M` is an even number and the two sequence numbers are at max distance from each other,
 * then the sequence number with the highest value is considered to be ahead.
 */
template <typename T, T M>
inline typename std::enable_if<(M > 0), bool>::type ahead_or_at(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    const T max_dist = M / 2;
    if (!(M & 1) && min_diff<T, M>(a, b) == max_dist)
    {
        return b < a;
    }
    return forward_diff<T, M>(b, a) <= max_dist;
}

/** @overload ahead_or_at(T a, T b)
 * Modulus `M == 0`: wrap at the largest value representable by `T`.
 */
template <typename T, T M>
inline typename std::enable_if<(M == 0), bool>::type ahead_or_at(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    const T max_dist = std::numeric_limits<T>::max() / 2 + T(1);
    if (a - b == max_dist)
    {
        return b < a;
    }
    return forward_diff(b, a) < max_dist;
}

/** @brief ahead_or_at with default (full-range) modulus. */
template <typename T>
inline bool ahead_or_at(T a, T b)
{
    return ahead_or_at<T, 0>(a, b);
}

/** @brief Test if the sequence number `a` is ahead of (not equal to) sequence number `b`.
 * If `M` is an even number and the two sequence numbers are at max distance from each other,
 * then the sequence number with the highest value is considered to be ahead.
 */
template <typename T, T M = 0>
inline bool ahead_of(T a, T b)
{
    static_assert(std::is_unsigned<T>::value, "Type must be an unsigned integer.");
    return a != b && ahead_or_at<T, M>(a, b);
}

/** @brief Comparator comparing sequence numbers in ascending (continuous) order.
 * WARNING! If used to sort sequence numbers of length `M` then the interval covered by the
 * sequence numbers may not be larger than floor(M/2).
 */
template <typename T, T M = 0>
struct AscendingSeqNumComp
{
    bool operator()(T a, T b) const { return ahead_of<T, M>(a, b); }
};

/** @brief Comparator comparing sequence numbers in descending (continuous) order.
 * WARNING! If used to sort sequence numbers of length `M` then the interval covered by the
 * sequence numbers may not be larger than floor(M/2).
 */
template <typename T, T M = 0>
struct DescendingSeqNumComp
{
    bool operator()(T a, T b) const { return ahead_of<T, M>(b, a); }
};

CXXKIT_END_NAMESPACE
