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

#ifndef _CXXKIT_NULLABILITY_HPP
#define _CXXKIT_NULLABILITY_HPP

#include <cxxkit/tools/type_traits.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
// `IsNullabilityCompatible` checks whether its first argument is a class
// explicitly tagged as supporting nullability annotations. The tag is the type
// declaration `nullability_compatible`.
template <typename, typename = void>
struct IsNullabilityCompatible : std::false_type
{
};

template <typename T>
struct IsNullabilityCompatible<T, VoidType<typename T::nullability_compatible>> : std::true_type
{
};

template <typename T>
struct IsSupportedType : std::conditional<IsNullabilityCompatible<T>::value, std::true_type, std::false_type>
{
};

template <typename T>
struct IsSupportedType<T *> : std::true_type
{
};

template <typename T, typename U>
struct IsSupportedType<T U::*> : std::true_type
{
};

template <typename T, typename... Deleter>
struct IsSupportedType<std::unique_ptr<T, Deleter...>> : std::true_type
{
};

template <typename T>
struct IsSupportedType<std::shared_ptr<T>> : std::true_type
{
};

template <typename T>
struct EnableNullable
{
    static_assert(IsSupportedType<typename std::remove_cv<T>::type>::value,
                  "Template argument must be a raw or supported smart pointer "
                  "type. See absl/base/nullability.h.");
    using type = T;
};

template <typename T>
struct EnableNonnull
{
    static_assert(IsSupportedType<typename std::remove_cv<T>::type>::value,
                  "Template argument must be a raw or supported smart pointer "
                  "type. See absl/base/nullability.h.");
    using type = T;
};

template <typename T>
struct EnableNullabilityUnknown
{
    static_assert(IsSupportedType<typename std::remove_cv<T>::type>::value,
                  "Template argument must be a raw or supported smart pointer "
                  "type. See absl/base/nullability.h.");
    using type = T;
};

// Note: we do not apply Clang nullability attributes (e.g. _Nullable).  These
// only support raw pointers, and conditionally enabling them only for raw
// pointers inhibits template arg deduction.  Ideally, they would support all
// pointer-like types.

template <typename T, typename = typename EnableNonnull<T>::type>
using NonnullImpl CXXKIT_CPP_ATTRIBUTE_CLANG_ANNOTATE("Nonnull") = T;

template <typename T, typename = typename EnableNullable<T>::type>
using NullableImpl CXXKIT_CPP_ATTRIBUTE_CLANG_ANNOTATE("Nullable") = T;

template <typename T, typename = typename EnableNullabilityUnknown<T>::type>
using NullabilityUnknownImpl CXXKIT_CPP_ATTRIBUTE_CLANG_ANNOTATE("Nullability_Unspecified") = T;
} // namespace detail

// absl::Nonnull
//
// The indicated pointer is never null. It is the responsibility of the provider
// of this pointer across an API boundary to ensure that the pointer is never
// set to null. Consumers of this pointer across an API boundary may safely
// dereference the pointer.
//
// Example:
//
// // `employee` is designated as not null.
// void PaySalary(absl::Nonnull<Employee *> employee) {
//   pay(*employee);  // OK to dereference
// }
template <typename T>
using Nonnull = detail::NonnullImpl<T>;

// absl::Nullable
//
// The indicated pointer may, by design, be either null or non-null. Consumers
// of this pointer across an API boundary should perform a `nullptr` check
// before performing any operation using the pointer.
//
// Example:
//
// // `employee` may  be null.
// void PaySalary(absl::Nullable<Employee *> employee) {
//   if (employee != nullptr) {
//     Pay(*employee);  // OK to dereference
//   }
// }
template <typename T>
using Nullable = detail::NullableImpl<T>;

// absl::NullabilityUnknown (default)
//
// The indicated pointer has not yet been determined to be definitively
// "non-null" or "nullable." Providers of such pointers across API boundaries
// should, over time, annotate such pointers as either "non-null" or "nullable."
// Consumers of these pointers across an API boundary should treat such pointers
// with the same caution they treat currently unannotated pointers. Most
// existing code will have "unknown"  pointers, which should eventually be
// migrated into one of the above two nullability states: `Nonnull<T>` or
//  `Nullable<T>`.
//
// NOTE: Because this annotation is the global default state, unannotated
// pointers are assumed to have "unknown" semantics. This assumption is designed
// to minimize churn and reduce clutter within the codebase.
//
// Example:
//
// // `employee`s nullability state is unknown.
// void PaySalary(absl::NullabilityUnknown<Employee *> employee) {
//   Pay(*employee); // Potentially dangerous. API provider should investigate.
// }
//
// Note that a pointer without an annotation, by default, is assumed to have the
// annotation `NullabilityUnknown`.
//
// // `employee`s nullability state is unknown.
// void PaySalary(Employee* employee) {
//   Pay(*employee); // Potentially dangerous. API provider should investigate.
// }
template <typename T>
using NullabilityUnknown = detail::NullabilityUnknownImpl<T>;
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_NULLABILITY_HPP
