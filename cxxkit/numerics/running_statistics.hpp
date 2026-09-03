/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
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
 * @brief Running (online) statistics via Welford's method, ported from WebRTC `RunningStatistics`.
 */

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/optional.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
// Provide neutral element with respect to min().
// Typically used as an initial value for running minimum.
template <typename T, typename std::enable_if<std::numeric_limits<T>::has_infinity>::type * = nullptr>
constexpr T infinity_or_max()
{
    return std::numeric_limits<T>::infinity();
}

template <typename T, typename std::enable_if<!std::numeric_limits<T>::has_infinity>::type * = nullptr>
constexpr T infinity_or_max()
{
    // Fallback to max().
    return std::numeric_limits<T>::max();
}

// Provide neutral element with respect to max().
// Typically used as an initial value for running maximum.
template <typename T, typename std::enable_if<std::numeric_limits<T>::has_infinity>::type * = nullptr>
constexpr T minus_infinity_or_min()
{
    static_assert(std::is_signed<T>::value, "Unsupported. Please open a bug.");
    return -std::numeric_limits<T>::infinity();
}

template <typename T, typename std::enable_if<!std::numeric_limits<T>::has_infinity>::type * = nullptr>
constexpr T minus_infinity_or_min()
{
    // Fallback to min().
    return std::numeric_limits<T>::min();
}
} // namespace detail

/** @brief Running (online) statistics using Welford's method.
 * The go-to class for min, max, mean, variance and standard deviation over a stream of samples.
 * All measures return `cxxkit::utils::nullopt` when no samples were fed (`size() == 0`);
 * otherwise the returned optional is guaranteed to contain a value.
 *
 * Note: remove_sample() does not affect min and max. For a moving window over the last N samples,
 * use a rolling accumulator instead.
 *
 * Reference: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
 *
 * @tparam T Scalar sample type; must be convertible to double. Rationale: measures are computed
 * with greater precision than the samples themselves.
 */
template <typename T>
class RunningStatistics
{
public:
    // Update stats ////////////////////////////////////////////

    /** @brief Add a value participating in the statistics in O(1) time.
     */
    void add_sample(T sample)
    {
        mMax = std::max(mMax, sample);
        mMin = std::min(mMin, sample);
        mSum += sample;
        ++mSize;
        // Welford's incremental update.
        const double delta = sample - mMean;
        mMean += delta / mSize;
        const double delta2 = sample - mMean;
        mCumul += delta * delta2;
    }

    /** @brief Remove a previously added value in O(1) time. Nb: this doesn't affect min or max.
     * Calling removeSample() when size() == 0 is incorrect.
     */
    void remove_sample(T sample)
    {
        CXXKIT_DCHECK_GT(size(), 0);
        // In production, just saturate at 0.
        if (mSize == 0)
        {
            return;
        }
        // Since samples order doesn't matter, this is the
        // exact reciprocal of Welford's incremental update.
        --mSize;
        const double delta = sample - mMean;
        mMean -= delta / mSize;
        const double delta2 = sample - mMean;
        mCumul -= delta * delta2;
    }

    /** @brief Merge other stats, as if samples were added one by one, but in O(1).
     */
    void merge_statistics(const RunningStatistics &other)
    {
        if (other.mSize == 0)
        {
            return;
        }
        mMax = std::max(mMax, other.mMax);
        mMin = std::min(mMin, other.mMin);
        const int64_t newSize = mSize + other.mSize;
        const double newMean = (mMean * mSize + other.mMean * other.mSize) / newSize;
        // Each cumulant must be corrected.
        //   * from: sum((x_i - mMean)^2)
        //   * to:   sum((x - newMean)^2)
        struct Delta
        {
            double newMean;
            double operator()(const RunningStatistics &stats) const
            {
                return stats.mSize * (newMean * (newMean - 2 * stats.mMean) + stats.mMean * stats.mMean);
            }
        } delta{newMean};
        mCumul = mCumul + delta(*this) + other.mCumul + delta(other);
        mMean = newMean;
        mSize = newSize;
    }

    // Get measures ////////////////////////////////////////////

    /** @brief Returns number of samples involved via add_sample() or merge_statistics(),
     * minus number of times remove_sample() was called.
     */
    int64_t size() const { return mSize; }

    /** @brief Returns minimum among all seen samples, in O(1) time.
     * This isn't affected by remove_sample().
     */
    Optional<T> get_min() const
    {
        if (mSize == 0)
        {
            return utils::nullopt;
        }
        return mMin;
    }

    /** @brief Returns maximum among all seen samples, in O(1) time.
     * This isn't affected by remove_sample().
     */
    Optional<T> get_max() const
    {
        if (mSize == 0)
        {
            return utils::nullopt;
        }
        return mMax;
    }

    /** @brief Returns sum in O(1) time.
     */
    Optional<double> get_sum() const
    {
        if (mSize == 0)
        {
            return utils::nullopt;
        }
        return mSum;
    }

    /** @brief Returns mean in O(1) time.
     */
    Optional<double> get_mean() const
    {
        if (mSize == 0)
        {
            return utils::nullopt;
        }
        return mMean;
    }

    /** @brief Returns unbiased sample variance in O(1) time.
     */
    Optional<double> get_variance() const
    {
        if (mSize == 0)
        {
            return utils::nullopt;
        }
        return mCumul / mSize;
    }

    /** @brief Returns unbiased standard deviation in O(1) time.
     */
    Optional<double> get_standard_deviation() const
    {
        if (mSize == 0)
        {
            return utils::nullopt;
        }
        return std::sqrt(*get_variance());
    }

private:
    int64_t mSize = 0;                           ///< Samples seen.
    T mMin = detail::infinity_or_max<T>();       ///< Running minimum (init: +inf or max).
    T mMax = detail::minus_infinity_or_min<T>(); ///< Running maximum (init: -inf or min).
    double mMean = 0;
    double mCumul = 0; ///< Variance * size, sometimes noted m2.
    double mSum = 0;
};

CXXKIT_END_NAMESPACE
