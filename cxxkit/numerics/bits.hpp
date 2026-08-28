/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright 2020 The Abseil Authors
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

#include <cxxkit/tools/type_traits.hpp>

#include <cstdint>
#include <limits>

#if (defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L) ||                                                  \
    (defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L)
#    include <bit>
#endif

// Clang on Windows has __builtin_clzll; otherwise we need to use the windows intrinsic functions.
#if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
#endif


#if defined(__GNUC__) && !defined(__clang__)
// GCC
#    define CXXKIT__BITS_HAS_BUILTIN_OR_GCC(x) 1
#else
#    define CXXKIT__BITS_HAS_BUILTIN_OR_GCC(x) CXXKIT_CC_HAS_BUILTIN(x)
#endif

#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_popcountl) && CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_popcountll)
#    define CXXKIT__BITS_CONSTEXPR_POPCOUNT     constexpr
#    define CXXKIT__BITS_HAS_CONSTEXPR_POPCOUNT 1
#else
#    define CXXKIT__BITS_CONSTEXPR_POPCOUNT
#    define CXXKIT__BITS_HAS_CONSTEXPR_POPCOUNT 0
#endif

#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_clz) && CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_clzll)
#    define CXXKIT__BITS_CONSTEXPR_CLZ     constexpr
#    define CXXKIT__BITS_HAS_CONSTEXPR_CLZ 1
#else
#    define CXXKIT__BITS_CONSTEXPR_CLZ
#    define CXXKIT__BITS_HAS_CONSTEXPR_CLZ 0
#endif

#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_ctz) && CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_ctzll)
#    define CXXKIT__BITS_CONSTEXPR_CTZ     constexpr
#    define CXXKIT__BITS_HAS_CONSTEXPR_CTZ 1
#else
#    define CXXKIT__BITS_CONSTEXPR_CTZ
#    define CXXKIT__BITS_HAS_CONSTEXPR_CTZ 0
#endif

/** @brief Bit-manipulation primitives (`cxxkit::utils`).
 * All functions require unsigned `T` with a power-of-2 bit-width; `constexpr` when compiler intrinsics support it.
 * @see Rotations: `rotl`/`rotr`, popcount: `popcount`, leading/trailing zero counts, `has_single_bit`/`bit_width`/`bit_floor`/`bit_ceil`
 */
CXXKIT_BEGIN_NAMESPACE

namespace utils
{
namespace detail
{

constexpr bool is_power_of2(unsigned int x) noexcept
{
    return x != 0 && (x & (x - 1)) == 0;
}

template <class T>
CXXKIT_ATTRIBUTE_MUST_USE_RESULT CXXKIT_FORCE_INLINE constexpr T rotate_right(T x, int s) noexcept
{
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(is_power_of2(std::numeric_limits<T>::digits), "T must have a power-of-2 size");

    return static_cast<T>(x >> (s & (std::numeric_limits<T>::digits - 1))) |
           static_cast<T>(x << ((-s) & (std::numeric_limits<T>::digits - 1)));
}

template <class T>
CXXKIT_ATTRIBUTE_MUST_USE_RESULT CXXKIT_FORCE_INLINE constexpr T rotate_left(T x, int s) noexcept
{
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(is_power_of2(std::numeric_limits<T>::digits), "T must have a power-of-2 size");

    return static_cast<T>(x << (s & (std::numeric_limits<T>::digits - 1))) |
           static_cast<T>(x >> ((-s) & (std::numeric_limits<T>::digits - 1)));
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_POPCOUNT int popcount32(uint32_t x) noexcept
{
#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_popcount)
    static_assert(sizeof(unsigned int) == sizeof(x), "__builtin_popcount does not take 32-bit arg");
    return __builtin_popcount(x);
#else
    x -= ((x >> 1) & 0x55555555);
    x = ((x >> 2) & 0x33333333) + (x & 0x33333333);
    return static_cast<int>((((x + (x >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24);
#endif
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_POPCOUNT int popcount64(uint64_t x) noexcept
{
#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_popcountll)
    static_assert(sizeof(unsigned long long) == sizeof(x), // NOLINT(runtime/int)
                  "__builtin_popcount does not take 64-bit arg");
    return __builtin_popcountll(x);
#else
    x -= (x >> 1) & 0x5555555555555555ULL;
    x = ((x >> 2) & 0x3333333333333333ULL) + (x & 0x3333333333333333ULL);
    return static_cast<int>((((x + (x >> 4)) & 0xF0F0F0F0F0F0F0FULL) * 0x101010101010101ULL) >> 56);
#endif
}

template <class T>
CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_POPCOUNT int popcount(T x) noexcept
{
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(is_power_of2(std::numeric_limits<T>::digits), "T must have a power-of-2 size");
    static_assert(sizeof(x) <= sizeof(uint64_t), "T is too large");
    return sizeof(x) <= sizeof(uint32_t) ? popcount32(x) : popcount64(x);
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CLZ int count_leading_zeroes32(uint32_t x)
{
#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_clz)
    // Use __builtin_clz, which uses the following instructions:
    //  x86: bsr, lzcnt
    //  ARM64: clz
    //  PPC: cntlzd

    static_assert(sizeof(unsigned int) == sizeof(x), "__builtin_clz does not take 32-bit arg");
    // Handle 0 as a special case because __builtin_clz(0) is undefined.
    return x == 0 ? 32 : __builtin_clz(x);
#elif defined(_MSC_VER) && !defined(__clang__)
    unsigned long result = 0; // NOLINT(runtime/int)
    if (_BitScanReverse(&result, x))
    {
        return 31 - result;
    }
    return 32;
#else
    int zeroes = 28;
    if (x >> 16)
    {
        zeroes -= 16;
        x >>= 16;
    }
    if (x >> 8)
    {
        zeroes -= 8;
        x >>= 8;
    }
    if (x >> 4)
    {
        zeroes -= 4;
        x >>= 4;
    }
    return "\4\3\2\2\1\1\1\1\0\0\0\0\0\0\0"[x] + zeroes;
#endif
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CLZ int count_leading_zeroes16(uint16_t x)
{
#if CXXKIT_CC_HAS_BUILTIN(__builtin_clzs)
    static_assert(sizeof(unsigned short) == sizeof(x), // NOLINT(runtime/int)
                  "__builtin_clzs does not take 16-bit arg");
    return x == 0 ? 16 : __builtin_clzs(x);
#else
    return count_leading_zeroes32(x) - 16;
#endif
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CLZ int count_leading_zeroes64(uint64_t x)
{
#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_clzll)
    // Use __builtin_clzll, which uses the following instructions:
    //  x86: bsr, lzcnt
    //  ARM64: clz
    //  PPC: cntlzd
    static_assert(sizeof(unsigned long long) == sizeof(x), // NOLINT(runtime/int)
                  "__builtin_clzll does not take 64-bit arg");

    // Handle 0 as a special case because __builtin_clzll(0) is undefined.
    return x == 0 ? 64 : __builtin_clzll(x);
#elif defined(_MSC_VER) && !defined(__clang__) && (defined(_M_X64) || defined(_M_ARM64))
    // MSVC does not have __buitin_clzll. Use _BitScanReverse64.
    unsigned long result = 0; // NOLINT(runtime/int)
    if (_BitScanReverse64(&result, x))
    {
        return 63 - result;
    }
    return 64;
#elif defined(_MSC_VER) && !defined(__clang__)
    // MSVC does not have __buitin_clzll. Compose two calls to _BitScanReverse
    unsigned long result = 0; // NOLINT(runtime/int)
    if ((x >> 32) && _BitScanReverse(&result, static_cast<unsigned long>(x >> 32)))
    {
        return 31 - result;
    }
    if (_BitScanReverse(&result, static_cast<unsigned long>(x)))
    {
        return 63 - result;
    }
    return 64;
#else
    int zeroes = 60;
    if (x >> 32)
    {
        zeroes -= 32;
        x >>= 32;
    }
    if (x >> 16)
    {
        zeroes -= 16;
        x >>= 16;
    }
    if (x >> 8)
    {
        zeroes -= 8;
        x >>= 8;
    }
    if (x >> 4)
    {
        zeroes -= 4;
        x >>= 4;
    }
    return "\4\3\2\2\1\1\1\1\0\0\0\0\0\0\0"[x] + zeroes;
#endif
}

template <typename T>
CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CLZ int count_leading_zeroes(T x)
{
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(is_power_of2(std::numeric_limits<T>::digits), "T must have a power-of-2 size");
    static_assert(sizeof(T) <= sizeof(uint64_t), "T too large");
    return sizeof(T) <= sizeof(uint16_t)
               ? count_leading_zeroes16(static_cast<uint16_t>(x)) -
                     (std::numeric_limits<uint16_t>::digits - std::numeric_limits<T>::digits)
               : (sizeof(T) <= sizeof(uint32_t)
                      ? count_leading_zeroes32(static_cast<uint32_t>(x)) -
                            (std::numeric_limits<uint32_t>::digits - std::numeric_limits<T>::digits)
                      : count_leading_zeroes64(x));
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CTZ int count_trailing_zeroes_nonzero32(uint32_t x)
{
#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_ctz)
    static_assert(sizeof(unsigned int) == sizeof(x), "__builtin_ctz does not take 32-bit arg");
    return __builtin_ctz(x);
#elif defined(_MSC_VER) && !defined(__clang__)
    unsigned long result = 0; // NOLINT(runtime/int)
    _BitScanForward(&result, x);
    return result;
#else
    int c = 31;
    x &= ~x + 1;
    if (x & 0x0000FFFF)
        c -= 16;
    if (x & 0x00FF00FF)
        c -= 8;
    if (x & 0x0F0F0F0F)
        c -= 4;
    if (x & 0x33333333)
        c -= 2;
    if (x & 0x55555555)
        c -= 1;
    return c;
#endif
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CTZ int count_trailing_zeroes_nonzero64(uint64_t x)
{
#if CXXKIT__BITS_HAS_BUILTIN_OR_GCC(__builtin_ctzll)
    static_assert(sizeof(unsigned long long) == sizeof(x), // NOLINT(runtime/int)
                  "__builtin_ctzll does not take 64-bit arg");
    return __builtin_ctzll(x);
#elif defined(_MSC_VER) && !defined(__clang__) && (defined(_M_X64) || defined(_M_ARM64))
    unsigned long result = 0; // NOLINT(runtime/int)
    _BitScanForward64(&result, x);
    return result;
#elif defined(_MSC_VER) && !defined(__clang__)
    unsigned long result = 0; // NOLINT(runtime/int)
    if (static_cast<uint32_t>(x) == 0)
    {
        _BitScanForward(&result, static_cast<unsigned long>(x >> 32));
        return result + 32;
    }
    _BitScanForward(&result, static_cast<unsigned long>(x));
    return result;
#else
    int c = 63;
    x &= ~x + 1;
    if (x & 0x00000000FFFFFFFF)
        c -= 32;
    if (x & 0x0000FFFF0000FFFF)
        c -= 16;
    if (x & 0x00FF00FF00FF00FF)
        c -= 8;
    if (x & 0x0F0F0F0F0F0F0F0F)
        c -= 4;
    if (x & 0x3333333333333333)
        c -= 2;
    if (x & 0x5555555555555555)
        c -= 1;
    return c;
#endif
}

CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CTZ int count_trailing_zeroes_nonzero16(uint16_t x)
{
#if CXXKIT_CC_HAS_BUILTIN(__builtin_ctzs)
    static_assert(sizeof(unsigned short) == sizeof(x), // NOLINT(runtime/int)
                  "__builtin_ctzs does not take 16-bit arg");
    return __builtin_ctzs(x);
#else
    return count_trailing_zeroes_nonzero32(x);
#endif
}

template <class T>
CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CTZ int count_trailing_zeroes(T x) noexcept
{
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(is_power_of2(std::numeric_limits<T>::digits), "T must have a power-of-2 size");
    static_assert(sizeof(T) <= sizeof(uint64_t), "T too large");
    return x == 0 ? std::numeric_limits<T>::digits
                  : (sizeof(T) <= sizeof(uint16_t)
                         ? count_trailing_zeroes_nonzero16(static_cast<uint16_t>(x))
                         : (sizeof(T) <= sizeof(uint32_t) ? count_trailing_zeroes_nonzero32(static_cast<uint32_t>(x))
                                                          : count_trailing_zeroes_nonzero64(x)));
}

// If T is narrower than unsigned, T{1} << bit_width will be promoted.  We
// want to force it to wraparound so that bit_ceil of an invalid value are not
// core constant expressions.
template <class T>
CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CLZ typename std::enable_if<std::is_unsigned<T>::value, T>::type
bit_ceil_promotion_helper(T x, T promotion)
{
    return (T{1} << (x + promotion)) >> promotion;
}

template <class T>
CXXKIT_FORCE_INLINE CXXKIT__BITS_CONSTEXPR_CLZ typename std::enable_if<std::is_unsigned<T>::value, T>::type
bit_ceil_non_power_of2(T x)
{
    // If T is narrower than unsigned, it undergoes promotion to unsigned when we
    // shift.  We calculate the number of bits added by the wider type.
    return bit_ceil_promotion_helper(
        static_cast<T>(std::numeric_limits<T>::digits - count_leading_zeroes(x)),
        T{sizeof(T) >= sizeof(unsigned) ? 0 : std::numeric_limits<unsigned>::digits - std::numeric_limits<T>::digits});
}
} // namespace detail

/** @brief rotate `x` left by `s` bits. `T` must be unsigned with a power-of-2 size.
 * @tparam T Unsigned integer type.
 * @param x Value to rotate.
 * @param s Rotation amount (any signed int; negative wraps right).
 * @return Rotated value.
 */
#if !(defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L)
// rotating
template <class T>
CXXKIT_ATTRIBUTE_MUST_USE_RESULT constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type rotl(
    T x,
    int s) noexcept
{
    return detail::rotate_left(x, s);
}

/** @brief rotate `x` right by `s` bits. `T` must be unsigned with a power-of-2 size.
 * @tparam T Unsigned integer type.
 * @param x Value to rotate.
 * @param s Rotation amount.
 * @return Rotated value.
 */
template <class T>
CXXKIT_ATTRIBUTE_MUST_USE_RESULT constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type rotr(
    T x,
    int s) noexcept
{
    return detail::rotate_right(x, s);
}

// Counting functions
//
// While these functions are typically constexpr, on some platforms, they may
// not be marked as constexpr due to constraints of the compiler/available
// intrinsics.
/** @brief Count leading zero bits of `x`. `T` must be unsigned, <= 64 bits.
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Leading zero count (equals `T::digits` when `x == 0`).
 */
template <class T>
CXXKIT__BITS_CONSTEXPR_CLZ inline typename std::enable_if<std::is_unsigned<T>::value, int>::type countl_zero(
    T x) noexcept
{
    return detail::count_leading_zeroes(x);
}

/** @brief Count leading one bits of `x` (alias of `countl_zero(~x)`).
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Leading one count.
 */
template <class T>
CXXKIT__BITS_CONSTEXPR_CLZ inline typename std::enable_if<std::is_unsigned<T>::value, int>::type countl_one(
    T x) noexcept
{
    // Avoid integer promotion to a wider type
    return countl_zero(static_cast<T>(~x));
}

/** @brief Count trailing zero bits of `x`.
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Trailing zero count (equals `T::digits` when `x == 0`).
 */
template <class T>
CXXKIT__BITS_CONSTEXPR_CTZ inline typename std::enable_if<std::is_unsigned<T>::value, int>::type countr_zero(
    T x) noexcept
{
    return detail::count_trailing_zeroes(x);
}

/** @brief Count trailing one bits of `x` (alias of `countr_zero(~x)`).
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Trailing one count.
 */
template <class T>
CXXKIT__BITS_CONSTEXPR_CTZ inline typename std::enable_if<std::is_unsigned<T>::value, int>::type countr_one(
    T x) noexcept
{
    // Avoid integer promotion to a wider type
    return countr_zero(static_cast<T>(~x));
}

/** @brief popcount (number of set bits) of `x`. `T` must be unsigned, <= 64 bits.
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Number of set bits.
 */
template <class T>
CXXKIT__BITS_CONSTEXPR_POPCOUNT inline typename std::enable_if<std::is_unsigned<T>::value, int>::type popcount(
    T x) noexcept
{
    return detail::popcount(x);
}
#else // defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L

using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::popcount;
using std::rotl;
using std::rotr;

#endif

#if !(defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L)
/** @brief True iff `x` has exactly one bit set (i.e. `x` is a power of two).
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return `true` if `x != 0 && (x & (x-1)) == 0`.
 */
// Returns: true if x is an integral power of two; false otherwise.
template <class T>
constexpr inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type has_single_bit(T x) noexcept
{
    return x != 0 && (x & (x - 1)) == 0;
}
/** @brief `bit_width(x)`: smallest integer `k` such that `(1<<k) > x`; equals 0 when `x == 0`.
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Bit width.
 */

// Returns: If x == 0, 0; otherwise one plus the base-2 logarithm of x, with any
// fractional part discarded.
template <class T>
CXXKIT__BITS_CONSTEXPR_CLZ inline typename std::enable_if<std::is_unsigned<T>::value, T>::type bit_width(T x) noexcept
{
    return std::numeric_limits<T>::digits - static_cast<unsigned int>(countl_zero(x));
}
/** @brief `bit_floor(x)`: largest power of two <= `x`; 0 when `x == 0`.
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Largest power of two <= `x`.
 */

// Returns: If x == 0, 0; otherwise the maximal value y such that
// has_single_bit(y) is true and y <= x.
template <class T>
CXXKIT__BITS_CONSTEXPR_CLZ inline typename std::enable_if<std::is_unsigned<T>::value, T>::type bit_floor(T x) noexcept
{
    return x == 0 ? 0 : T{1} << (bit_width(x) - 1);
}
/** @brief `bit_ceil(x)`: smallest power of two >= `x`. Precondition: result must be representable as `T`.
 * @tparam T Unsigned integer type.
 * @param x Value.
 * @return Smallest power of two >= `x`.
 */

// Returns: N, where N is the smallest power of 2 greater than or equal to x.
//
// Preconditions: N is representable as a value of type T.
template <class T>
CXXKIT__BITS_CONSTEXPR_CLZ inline typename std::enable_if<std::is_unsigned<T>::value, T>::type bit_ceil(T x)
{
    // If T is narrower than unsigned, T{1} << bit_width will be promoted.  We
    // want to force it to wraparound so that bit_ceil of an invalid value are not
    // core constant expressions.
    //
    // bit_ceil_non_power_of2 triggers an overflow in constexpr contexts if we would
    // undergo promotion to unsigned but not fit the result into T without
    // truncation.
    return has_single_bit(x) ? T{1} << (bit_width(x) - 1) : detail::bit_ceil_non_power_of2(x);
}
#else // defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L

using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::has_single_bit;

#endif

} // namespace utils

CXXKIT_END_NAMESPACE
