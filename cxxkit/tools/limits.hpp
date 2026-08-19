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

#ifndef _CXXKIT_LIMITS_HPP
#define _CXXKIT_LIMITS_HPP

#include <cxxkit/base/global.hpp>

#include <limits>
#include <algorithm>

#ifdef min
#    undef min
#endif
#ifdef max
#    undef max
#endif

CXXKIT_BEGIN_NAMESPACE

CXXKIT_STATIC_CONSTANT_NUMBER(kInt8Min, std::numeric_limits<int8_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt8Max, std::numeric_limits<int8_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt16Min, std::numeric_limits<int16_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt16Max, std::numeric_limits<int16_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt32Min, std::numeric_limits<int32_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt32Max, std::numeric_limits<int32_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt64Min, std::numeric_limits<int64_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kInt64Max, std::numeric_limits<int64_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt8Min, std::numeric_limits<uint8_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt8Max, std::numeric_limits<uint8_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt16Min, std::numeric_limits<uint16_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt16Max, std::numeric_limits<uint16_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt32Min, std::numeric_limits<uint32_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt32Max, std::numeric_limits<uint32_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt64Min, std::numeric_limits<uint64_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUInt64Max, std::numeric_limits<uint64_t>::max())

CXXKIT_STATIC_CONSTANT_NUMBER(kSizeMin, std::numeric_limits<size_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kSizeMax, std::numeric_limits<size_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kSSizeMin, std::numeric_limits<size_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kSSizeMax, std::numeric_limits<size_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kByteMin, std::numeric_limits<byte_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kByteMax, std::numeric_limits<byte_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUCharMin, std::numeric_limits<uchar_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUCharMax, std::numeric_limits<uchar_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUShortMin, std::numeric_limits<ushort_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUShortMax, std::numeric_limits<ushort_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kUIntMin, std::numeric_limits<uint_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kUIntMax, std::numeric_limits<uint_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kULongMin, std::numeric_limits<ulong_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kULongMax, std::numeric_limits<ulong_t>::max())
CXXKIT_STATIC_CONSTANT_NUMBER(kULongLongMin, std::numeric_limits<ulonglong_t>::min())
CXXKIT_STATIC_CONSTANT_NUMBER(kULongLongMax, std::numeric_limits<ulonglong_t>::max())

namespace utils
{
template <typename T>
static constexpr T numericMin() noexcept
{
    return std::numeric_limits<T>::min();
}
template <typename T>
static constexpr T mathMin(const T &left, const T &right) noexcept
{
    return std::min<T>(left, right);
}
template <typename T, typename P>
static constexpr T mathMin(const T &left, const T &right, P pred) noexcept
{
    return std::min<T, P>(left, right, pred);
}

template <typename T>
static constexpr T numericMax() noexcept
{
    return std::numeric_limits<T>::max();
}
template <typename T>
static constexpr T mathMax(const T &left, const T &right) noexcept
{
    return std::max<T>(left, right);
}
template <typename T, typename P>
static constexpr T mathMax(const T &left, const T &right, P pred) noexcept
{
    return std::max<T, P>(left, right, pred);
}
}; // namespace utils
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_LIMITS_HPP
