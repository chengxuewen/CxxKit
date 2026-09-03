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

#include <cxxkit/numerics/exp_filter.hpp>

#include <cmath>

CXXKIT_BEGIN_NAMESPACE

constexpr float ExpFilter::kValueUndefined; // ODR definition (C++11/14, needed if odr-used)

void ExpFilter::reset(float alpha)
{
    mAlpha = alpha;
    mFiltered = kValueUndefined;
}

float ExpFilter::apply(float exp, float sample)
{
    if (mFiltered == kValueUndefined)
    {
        // Initialize filtered value.
        mFiltered = sample;
    }
    else if (exp == 1.0f)
    {
        mFiltered = mAlpha * mFiltered + (1 - mAlpha) * sample;
    }
    else
    {
        float alpha = std::pow(mAlpha, exp);
        mFiltered = alpha * mFiltered + (1 - alpha) * sample;
    }
    if (mMax != kValueUndefined && mFiltered > mMax)
    {
        mFiltered = mMax;
    }
    return mFiltered;
}

void ExpFilter::update_base(float alpha)
{
    mAlpha = alpha;
}

CXXKIT_END_NAMESPACE
