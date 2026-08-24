/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
template <typename T, typename... Ts>
struct TypeListMaxSizeImpl;

//! @cond INTERNAL  // template specialisations (implementation detail)
template <typename T>
struct TypeListMaxSizeImpl<T>
{
    static CXXKIT_CONSTEXPR_OR_CONST size_t value = sizeof(T);
};

template <typename T1, typename T2, typename... U>
struct TypeListMaxSizeImpl<T1, T2, U...>
{
    static CXXKIT_CONSTEXPR_OR_CONST std::size_t value =
        TypeListMaxSizeImpl<traits::conditional_t<sizeof(T1) >= sizeof(T2), T1, T2>, U...>::value;
};

template <template <class...> class List, class... Ts>
struct TypeListMaxSizeImpl<List<Ts...>>
{
    static CXXKIT_CONSTEXPR_OR_CONST std::size_t value = TypeListMaxSizeImpl<Ts...>::value;
};
} // namespace detail
/**
 * Returns the maximum sizeof value from all given types use it like this:
 *  TypeListMaxSize<int, bool, double>::value => 8
 */
template <typename... Args>
using TypeListMaxSize = std::integral_constant<std::size_t, detail::TypeListMaxSizeImpl<Args...>::value>;


namespace detail
{
template <typename T, typename... Ts>
struct TypeListMaxAlignImpl;

template <typename T>
struct TypeListMaxAlignImpl<T>
{
    static CXXKIT_CONSTEXPR_OR_CONST size_t value = std::alignment_of<T>::value;
};

template <typename T1, typename T2, typename... U>
struct TypeListMaxAlignImpl<T1, T2, U...>
{
    static CXXKIT_CONSTEXPR_OR_CONST std::size_t value = TypeListMaxAlignImpl<
        traits::conditional_t<std::alignment_of<T1>::value >= std::alignment_of<T2>::value, T1, T2>,
        U...>::value;
};

template <template <class...> class List, class... Ts>
struct TypeListMaxAlignImpl<List<Ts...>>
{
    static CXXKIT_CONSTEXPR_OR_CONST std::size_t value = TypeListMaxAlignImpl<Ts...>::value;
};
} // namespace detail
/**
 * Returns the maximum sizeof value from all given types use it like this:
 *  TypeListMaxAlign<int, bool, double>::value => 8
 */
template <typename... Ts>
using TypeListMaxAlign = std::integral_constant<std::size_t, detail::TypeListMaxAlignImpl<Ts...>::value>;


template <typename HeadArg, typename... TailArgs>
struct Types
{
    using Head = HeadArg;
    using Tail = Types<TailArgs...>;
};

template <typename HeadArg>
struct Types<HeadArg>
{
    using Head = HeadArg;
    using Tail = None;
};

template <typename... Args>
struct TypeList
{
    static CXXKIT_CONSTEXPR_OR_CONST auto size = sizeof...(Args);
    using type = Types<Args...>;
    using Type = type;
};
template <typename... Args>
using type_list = TypeList<Args...>;

//! @endcond

CXXKIT_END_NAMESPACE