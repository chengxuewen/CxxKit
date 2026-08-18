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

#ifndef _CXXKIT_SAFE_COMPARE_HPP
#define _CXXKIT_SAFE_COMPARE_HPP

#include "cxxkit/tools/type_traits.hpp"

#include <stddef.h>
#include <stdint.h>

CXXKIT_BEGIN_NAMESPACE

namespace safe_cmp_impl
{

template <size_t N> struct LargerIntImpl : std::false_type
{
};
template <> struct LargerIntImpl<sizeof(int8_t)> : std::true_type
{
    using type = int16_t;
};
template <> struct LargerIntImpl<sizeof(int16_t)> : std::true_type
{
    using type = int32_t;
};
template <> struct LargerIntImpl<sizeof(int32_t)> : std::true_type
{
    using type = int64_t;
};

// LargerInt<T1, T2>::value is true iff there's a signed type that's larger
// than T1 (and no larger than the larger of T2 and int*, for performance
// reasons); and if there is such a type, LargerInt<T1, T2>::type is an alias
// for it.
template <typename T1, typename T2>
struct LargerInt : LargerIntImpl<sizeof(T1) < sizeof(T2) || sizeof(T1) < sizeof(int *) ? sizeof(T1) : 0>
{
};

template <typename T> constexpr typename std::make_unsigned<T>::type makeUnsigned(T a)
{
    return static_cast<typename std::make_unsigned<T>::type>(a);
}

// Overload for when both T1 and T2 have the same signedness.
template <typename Op,
          typename T1,
          typename T2,
    typename std::enable_if<std::is_signed<T1>::value == std::is_signed<T2>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b)
{
    return Op::Op(a, b);
}

// Overload for signed - unsigned comparison that can be promoted to a bigger
// signed type.
template <typename Op,
          typename T1,
          typename T2,
    typename std::enable_if<std::is_signed<T1>::value && std::is_unsigned<T2>::value &&
                            LargerInt<T2, T1>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b)
{
    return Op::Op(a, static_cast<typename LargerInt<T2, T1>::type>(b));
}

// Overload for unsigned - signed comparison that can be promoted to a bigger
// signed type.
template <typename Op,
          typename T1,
          typename T2,
    typename std::enable_if<std::is_unsigned<T1>::value && std::is_signed<T2>::value &&
                            LargerInt<T1, T2>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b)
{
    return Op::Op(static_cast<typename LargerInt<T1, T2>::type>(a), b);
}

// Overload for signed - unsigned comparison that can't be promoted to a bigger
// signed type.
template <typename Op,
          typename T1,
          typename T2,
    typename std::enable_if<std::is_signed<T1>::value && std::is_unsigned<T2>::value &&
                            !LargerInt<T2, T1>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b)
{
    return a < 0 ? Op::Op(-1, 0) : Op::Op(safe_cmp_impl::makeUnsigned(a), b);
}

// Overload for unsigned - signed comparison that can't be promoted to a bigger
// signed type.
template <typename Op,
          typename T1,
          typename T2,
    typename std::enable_if<std::is_unsigned<T1>::value && std::is_signed<T2>::value &&
                            !LargerInt<T1, T2>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b)
{
    return b < 0 ? Op::Op(0, -1) : Op::Op(a, safe_cmp_impl::makeUnsigned(b));
}

#define CXXKIT_SAFECMP_MAKE_OP(name, op)                                                                                 \
    struct name                                                                                                        \
    {                                                                                                                  \
        template <typename T1, typename T2> static constexpr bool Op(T1 a, T2 b) { return a op b; }                    \
    };

CXXKIT_SAFECMP_MAKE_OP(EqOp, ==)

CXXKIT_SAFECMP_MAKE_OP(NeOp, !=)

CXXKIT_SAFECMP_MAKE_OP(LtOp, <)

CXXKIT_SAFECMP_MAKE_OP(LeOp, <=)

CXXKIT_SAFECMP_MAKE_OP(GtOp, >)

CXXKIT_SAFECMP_MAKE_OP(GeOp, >=)

#undef CXXKIT_SAFECMP_MAKE_OP
} // namespace safe_cmp_impl

#define CXXKIT_SAFECMP_MAKE_FUN(name)                                                                                    \
    template <typename T1, typename T2>                                                                                \
    constexpr typename std::enable_if<IsIntLike<T1>::value && IsIntLike<T2>::value, bool>::type Safe##name(T1 a, T2 b) \
    {                                                                                                                  \
        /* Unary plus here turns enums into real integral types. */                                                    \
        return safe_cmp_impl::cmp<safe_cmp_impl::name##Op>(+a, +b);                                                    \
    }                                                                                                                  \
    template <typename T1, typename T2>                                                                                \
    constexpr typename std::enable_if<!IsIntLike<T1>::value || !IsIntLike<T2>::value, bool>::type Safe##name(          \
        const T1 &a, const T2 &b)                                                                                      \
    {                                                                                                                  \
        return safe_cmp_impl::name##Op::Op(a, b);                                                                      \
    }
CXXKIT_SAFECMP_MAKE_FUN(Eq)
CXXKIT_SAFECMP_MAKE_FUN(Ne)
CXXKIT_SAFECMP_MAKE_FUN(Lt)
CXXKIT_SAFECMP_MAKE_FUN(Le)
CXXKIT_SAFECMP_MAKE_FUN(Gt)
CXXKIT_SAFECMP_MAKE_FUN(Ge)
#undef CXXKIT_SAFECMP_MAKE_FUN
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_SAFE_COMPARE_HPP
