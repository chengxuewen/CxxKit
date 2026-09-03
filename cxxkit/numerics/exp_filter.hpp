/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
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
 * @brief Exponential filter, ported from WebRTC `rtc_base/numerics/exp_filter.h`.
 */

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/numerics/numerics_global.hpp>

CXXKIT_BEGIN_NAMESPACE

/** @brief Exponential filter for smoothing noisy signals (e.g. bandwidth and packet loss estimation).
 *
 * This class can be used, for example, for smoothing the result of bandwidth estimation and packet
 * loss estimation. Applies: y(k) = min(alpha^exp * y(k-1) + (1 - alpha^exp) * sample, max).
 */
class CXXKIT_NUMERICS_API ExpFilter
{
public:
    /** @brief Sentinel meaning "no cap / undefined filter output".
     */
    static constexpr float kValueUndefined = -1.0f;

    /** @brief Construct the filter with factor base `alpha` and optional upper cap `max`.
     */
    explicit ExpFilter(float alpha, float max = kValueUndefined)
        : mMax(max)
    {
        reset(alpha);
    }

    /** @brief Resets the filter to its initial state, and resets filter factor base to
     * the given value `alpha`.
     */
    void reset(float alpha);

    /** @brief Applies the filter with a given exponent on the provided sample:
     * y(k) = min(alpha^exp * y(k-1) + (1 - alpha^exp) * sample, max).
     */
    float apply(float exp, float sample);

    /** @brief Returns current filtered value.
     */
    float filtered() const { return mFiltered; }

    /** @brief Changes the filter factor base to the given value `alpha`.
     */
    void update_base(float alpha);

private:
    float mAlpha;     ///< Filter factor base.
    float mFiltered;  ///< Current filter output.
    const float mMax; ///< Upper cap on filtered value.
};

CXXKIT_END_NAMESPACE
