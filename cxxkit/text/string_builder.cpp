/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License + BSD-3 (ported from webrtc rtc_base/strings/string_builder.h)
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

/*
 *  Copyright 2018 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cxxkit/text/string_builder.hpp>
#include <cxxkit/numerics/safe_minmax.hpp>
#include <cxxkit/tools/checks.hpp>

#include <stdarg.h>

#include <cstdio>
#include <cstring>

CXXKIT_BEGIN_NAMESPACE

SimpleStringBuilder::SimpleStringBuilder(ArrayView<char> buffer)
    : mBuffer(buffer)
{
    mBuffer[0] = '\0';
    CXXKIT_DCHECK(is_consistent());
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(char ch)
{
    return operator<<(StringView(&ch, 1));
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(StringView str)
{
    CXXKIT_DCHECK_LT(mSize + str.length(), mBuffer.size()) << "Buffer size was insufficient";
    const size_t chars_added = safe_min(str.length(), mBuffer.size() - mSize - 1);
    memcpy(&mBuffer[mSize], str.data(), chars_added);
    mSize += chars_added;
    mBuffer[mSize] = '\0';
    CXXKIT_DCHECK(is_consistent());
    return *this;
}

// Numeric conversion routines.
//
// We use std::[v]snprintf instead of std::to_string because:
// * std::to_string relies on the current locale for formatting purposes,
//   and therefore concurrent calls to std::to_string from multiple threads
//   may result in partial serialization of calls
// * snprintf allows us to print the number directly into our buffer.
// * avoid allocating a std::string (potential heap alloc).
// TODO(tommi): Switch to std::to_chars in C++17.

SimpleStringBuilder &SimpleStringBuilder::operator<<(int i)
{
    return append_format("%d", i);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(unsigned i)
{
    return append_format("%u", i);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(long i)
{ // NOLINT
    return append_format("%ld", i);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(long long i)
{ // NOLINT
    return append_format("%lld", i);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(unsigned long i)
{ // NOLINT
    return append_format("%lu", i);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(unsigned long long i)
{ // NOLINT
    return append_format("%llu", i);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(float f)
{
    return append_format("%g", f);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(double f)
{
    return append_format("%g", f);
}

SimpleStringBuilder &SimpleStringBuilder::operator<<(long double f)
{
    return append_format("%Lg", f);
}

SimpleStringBuilder &SimpleStringBuilder::append_format(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int len = std::vsnprintf(&mBuffer[mSize], mBuffer.size() - mSize, fmt, args);
    if (len >= 0)
    {
        const size_t chars_added = safe_min(len, mBuffer.size() - 1 - mSize);
        mSize += chars_added;
        CXXKIT_DCHECK_EQ(len, chars_added) << "Buffer size was insufficient";
    }
    else
    {
        // This should never happen, but we're paranoid, so re-write the
        // terminator in case vsnprintf() overwrote it.
        CXXKIT_DCHECK_NOTREACHED();
        mBuffer[mSize] = '\0';
    }
    va_end(args);
    CXXKIT_DCHECK(is_consistent());
    return *this;
}

StringBuilder &StringBuilder::append_format(const char *fmt, ...)
{
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    const int predicted_length = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    CXXKIT_DCHECK_GE(predicted_length, 0);
    if (predicted_length > 0)
    {
        const size_t size = mString.size();
        mString.resize(size + predicted_length);
        // Pass "+ 1" to vsnprintf to include space for the '\0'.
        const int actual_length = std::vsnprintf(&mString[size], predicted_length + 1, fmt, args);
        CXXKIT_DCHECK_GE(actual_length, 0);
    }
    va_end(args);
    return *this;
}

CXXKIT_END_NAMESPACE
