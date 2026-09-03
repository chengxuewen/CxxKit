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
 * @brief Percentile filter over a multiset of observations, ported from WebRTC `PercentileFilter`.
 */

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/checks.hpp>

#include <cstdint>
#include <set>
#include <iterator>

CXXKIT_BEGIN_NAMESPACE

/** @brief Class to efficiently get the percentile value from a group of observations.
 * The percentile is the value below which a given percentage of the observations fall.
 *
 * @tparam T Observation type; must support `<` comparison.
 */
template <typename T>
class PercentileFilter
{
public:
    /** @brief Construct filter. `percentile` should be between 0 and 1.
     */
    explicit PercentileFilter(float percentile)
        : mPercentile(percentile)
        , mPercentileIt(mSet.begin())
        , mPercentileIndex(0)
    {
        CXXKIT_DCHECK_GE(percentile, 0.0f);
        CXXKIT_DCHECK_LE(percentile, 1.0f);
    }

    /** @brief Insert one observation. The complexity of this operation is logarithmic in
     * the size of the container.
     */
    void insert(const T &value)
    {
        // Insert element at the upper bound.
        mSet.insert(value);
        if (mSet.size() == 1u)
        {
            // First element inserted - initialize percentile iterator and index.
            mPercentileIt = mSet.begin();
            mPercentileIndex = 0;
        }
        else if (value < *mPercentileIt)
        {
            // If new element is before us, increment `mPercentileIndex`.
            ++mPercentileIndex;
        }
        update_percentile_iterator();
    }

    /** @brief Remove one observation or return false if `value` doesn't exist in the
     * container. The complexity of this operation is logarithmic in the size of the
     * container.
     */
    bool erase(const T &value)
    {
        typename std::multiset<T>::const_iterator it = mSet.lower_bound(value);
        // Ignore erase operation if the element is not present in the current set.
        if (it == mSet.end() || *it != value)
        {
            return false;
        }
        if (it == mPercentileIt)
        {
            // If same iterator, update to the following element. Index is not affected.
            mPercentileIt = mSet.erase(it);
        }
        else
        {
            mSet.erase(it);
            // If erased element was before us, decrement `mPercentileIndex`.
            if (value <= *mPercentileIt)
            {
                --mPercentileIndex;
            }
        }
        update_percentile_iterator();
        return true;
    }

    /** @brief Get the percentile value. The complexity of this operation is constant.
     * Returns a default-constructed `T()` when the filter is empty.
     */
    T get_percentile_value() const { return mSet.empty() ? T() : *mPercentileIt; }

    /** @brief Removes all the stored observations.
     */
    void reset()
    {
        mSet.clear();
        mPercentileIt = mSet.begin();
        mPercentileIndex = 0;
    }

private:
    /** @brief Update iterator and index to point at target percentile value.
     */
    void update_percentile_iterator()
    {
        if (mSet.empty())
        {
            return;
        }
        const int64_t index = static_cast<int64_t>(mPercentile * (mSet.size() - 1));
        std::advance(mPercentileIt, index - mPercentileIndex);
        mPercentileIndex = index;
    }

    const float mPercentile;
    std::multiset<T> mSet;
    // Maintain iterator and index of current target percentile value.
    typename std::multiset<T>::iterator mPercentileIt;
    int64_t mPercentileIndex;
};

CXXKIT_END_NAMESPACE
