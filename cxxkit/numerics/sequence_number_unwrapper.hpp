/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright (c) 2022 The WebRTC project authors. All Rights Reserved.
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
 * @brief Sequence-number unwrapper mapping wrapping sequence numbers to a monotonically
 * increasing int64_t sequence, ported from WebRTC `sequence_number_unwrapper.h`.
 */

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/numerics/sequence_number_util.hpp>
#include <cxxkit/tools/optional.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
/** @brief Signed delta between two wrapping sequence numbers, adjusted to be negative
 * when `new_value` is actually behind `last_value` in the wrapping order.
 */
template <typename T, T M>
int64_t unwrapper_delta(T last_value, T new_value)
{
    const int64_t backward_adjustment = M == 0 ? int64_t{std::numeric_limits<T>::max()} + 1 : int64_t{M};
    int64_t result = forward_diff<T, M>(last_value, new_value);
    if (!ahead_or_at<T, M>(new_value, last_value))
    {
        result -= backward_adjustment;
    }
    return result;
}
} // namespace detail

/** @brief A sequence number unwrapper where the first unwrapped value equals the first value
 * being unwrapped.
 *
 * @tparam T Unsigned integer type, smaller than int64_t.
 * @tparam M Sequence modulus; 0 means full range of `T`.
 */
template <typename T, T M = 0>
class SeqNumUnwrapper
{
    static_assert(std::is_unsigned<T>::value && std::numeric_limits<T>::max() < std::numeric_limits<int64_t>::max(),
                  "Type unwrapped must be an unsigned integer smaller than int64_t.");

public:
    /** @brief Unwraps `value` and updates the internal state of the unwrapper. */
    int64_t unwrap(T value)
    {
        if (!mLastValue)
        {
            mLastUnwrapped = int64_t{value};
        }
        else
        {
            mLastUnwrapped += detail::unwrapper_delta<T, M>(*mLastValue, value);
        }

        mLastValue = value;
        return mLastUnwrapped;
    }

    /** @brief Returns the `value` without updating the internal state of the unwrapper. */
    int64_t peek_unwrap(T value) const
    {
        if (!mLastValue)
        {
            return value;
        }
        return mLastUnwrapped + detail::unwrapper_delta<T, M>(*mLastValue, value);
    }

    /** @brief Resets the unwrapper to its initial state. Unwrapped sequence numbers will
     * begin at 0 after resetting (the next unwrap() re-anchors to the given value).
     */
    void reset()
    {
        mLastUnwrapped = 0;
        mLastValue.reset();
    }

private:
    int64_t mLastUnwrapped = 0;
    Optional<T> mLastValue;
};

/** @brief Unwrapper for 32-bit RTP timestamps. */
typedef SeqNumUnwrapper<uint32_t> rtp_timestamp_unwrapper;

/** @brief Unwrapper for 16-bit RTP sequence numbers. */
typedef SeqNumUnwrapper<uint16_t> rtp_sequence_number_unwrapper;

CXXKIT_END_NAMESPACE
