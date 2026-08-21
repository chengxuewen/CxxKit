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

#pragma once

#include <cxxkit/base/global.hpp>

#include <cxxkit/3rdparty/mpark/variant.hpp>

CXXKIT_BEGIN_NAMESPACE

/**
 * @defgroup cxxkit_tools_variant Variant (type-safe discriminated union)
 * @{
 *
 * @brief Exception-free, vendor-free variant type wrapping mpark::variant.
 *
 * Provides C++17-style `std::variant` semantics for C++11 builds. The variant
 * holds exactly one of its alternative types at a time, identified by an index.
 *
 * @details All accessors are exception-safe where possible. Use @c get_if for
 *          non-throwing access; use @c get only when the index is known to be
 *          valid, since it throws @c BadVariantAccess otherwise.
 * @see https://github.com/mpark/variant for upstream reference.
 */

/**
 * @brief Sentinel alternative representing an "empty" variant.
 *
 * Analogous to `std::monostate`. Useful for the initial state of a variant
 * that will later hold a meaningful alternative.
 * @code
 * Variant<int, VariantMonostate> v;   // holds monostate
 * @endcode
 */
using VariantMonostate = mpark::monostate;

/**
 * @brief Exception thrown by @c get<T>() when the variant holds a different type.
 *
 * Derived from @c std::exception. Never thrown by non-throwing accessors
 * such as @c holds_alternative or @c get_if.
 */
using BadVariantAccess = mpark::bad_variant_access;

/**
 * @brief Type-safe discriminated union holding one of @c Ts...
 *
 * Template parameter pack must contain at least one type and no duplicates.
 * The variant holds exactly one alternative at any time. Default-constructed
 * variants hold the first alternative (which must be default-constructible).
 * @tparam Ts... Alternative types the variant may hold.
 *
 * @code
 * using V = Variant<int, std::string, double>;
 * V v = 42;              // holds int
 * v = std::string("hi"); // holds string now
 * @endcode
 */
template <typename... Ts>
using Variant = mpark::variant<Ts...>;

/**
 * @brief Compile-time count of alternatives in a Variant.
 *
 * Yields an `integral_constant<size_t, N>` where N is the number of types
 * in the variant's parameter pack.
 * @tparam T A `Variant<...>` type.
 */
template <typename T>
using VariantSize = mpark::variant_size<T>;

/**
 * @brief Type of the I-th alternative of a Variant.
 *
 * @tparam I Zero-based index into the variant's alternatives.
 * @tparam T A `Variant<...>` type.
 */
template <std::size_t I, typename T>
using VariantAlternative = mpark::variant_alternative<I, T>;

namespace utils
{
/**
 * @brief Retrieves the held value of type @c T from a variant.
 *
 * @tparam T The expected alternative type.
 * @param v The variant to access.
 * @return Reference to the held value of type @c T.
 * @throws BadVariantAccess if the variant does not hold @c T.
 */
using mpark::get;

/**
 * @brief Non-throwing pointer access to the held value of type @c T.
 *
 * @tparam T The expected alternative type.
 * @param p Pointer to the variant.
 * @return Pointer to the held value, or @c nullptr if the variant does not hold @c T.
 */
using mpark::get_if;

/**
 * @brief Applies a visitor to the currently held alternative.
 *
 * The visitor may be a callable, function pointer, or overload set.
 * Supports heterogeneous visitors that return a common type.
 * @tparam Visitor Type of the visitor (callable object or function).
 * @param vis The visitor to invoke.
 * @param v The variant.
 * @return The result of invoking @p vis with the held value.
 */
using mpark::visit;

/**
 * @brief Checks whether a variant currently holds a given alternative type.
 *
 * @tparam T The alternative type to check for.
 * @param v The variant.
 * @return @c true if the variant holds @c T, @c false otherwise.
 */
using mpark::holds_alternative;
} // namespace utils

/** @} */ // end of cxxkit_tools_variant

CXXKIT_END_NAMESPACE