/***********************************************************************************************************************
**
** Library: cxxkit
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

#include <cxxkit/tools/type_traits.hpp>

#include <cmath>
#include <cstdlib>
#include <numeric>

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
/***********************************************************************************************************************
  * wrap std::abs for unsigned available
***********************************************************************************************************************/
template <typename T>
constexpr auto abs(T v) -> typename std::enable_if<!std::is_signed<T>::value, T>::type
{
    return v;
}
template <typename T>
constexpr auto abs(T v) -> typename std::enable_if<std::is_signed<T>::value, T>::type
{
    return std::abs(v);
}

/***********************************************************************************************************************
  * like std::gcd, greatest common divisor
***********************************************************************************************************************/
template <typename T>
CXXKIT_CXX14_CONSTEXPR T gcd(T a, T b)
{
    static_assert(std::is_integral<T>(), "");
    if (b == 0)
    {
        return a;
    }
    return utils::abs(utils::gcd(b, a % b));
}
template <typename T>
CXXKIT_CXX14_CONSTEXPR T gcd_iterative(T a, T b)
{
    static_assert(std::is_integral<T>(), "");
    while (b != 0)
    {
        T temp = b;
        b = a % b;
        a = temp;
    }
    return utils::abs(a);
}

/***********************************************************************************************************************
  * like std::lcm, least common multiple
***********************************************************************************************************************/
template <typename M, typename N>
CXXKIT_CXX14_CONSTEXPR auto lcm(M m, N n) -> typename std::common_type<M, N>::type
{
    static_assert(std::is_integral<M>(), "");
    static_assert(std::is_integral<N>(), "");
    using CommonType = typename std::common_type<M, N>::type;
    if (m == 0 || n == 0)
    {
        return 0;
    }
    CommonType a = utils::abs(static_cast<CommonType>(m));
    CommonType b = utils::abs(static_cast<CommonType>(n));
    return (a / utils::gcd(a, b)) * b;
}

/***********************************************************************************************************************
  * Given two numbers `x` and `y` such that x >= y, computes the difference
  * x - y without causing undefined behavior due to signed overflow.
***********************************************************************************************************************/
// Given two numbers `x` and `y` such that x >= y, computes the difference
// x - y without causing undefined behavior due to signed overflow.
template <typename T>
typename std::make_unsigned<T>::type unsigned_difference(T x, T y)
{
    static_assert(std::is_signed<T>::value, "Function unsigned_difference is only meaningful for signed types.");
    CXXKIT_DCHECK_GE(x, y);
    using unsigned_type = typename std::make_unsigned<T>::type;
    // int -> unsigned conversion repeatedly adds UINT_MAX + 1 until the number
    // can be represented as an unsigned. Since we know that the actual
    // difference x - y can be represented as an unsigned, it is sufficient to
    // compute the difference modulo UINT_MAX + 1, i.e using unsigned arithmetic.
    return static_cast<unsigned_type>(x) - static_cast<unsigned_type>(y);
}
}; // namespace utils

CXXKIT_END_NAMESPACE
