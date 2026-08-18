/***********************************************************************************************************************
**
** Library: cxxkit
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

#include "cxxkit/base/macros.hpp"

#include <memory>
#include <utility>
#include <functional>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE

namespace traits
{
/***********************************************************************************************************************
 * like cxx17 std::remove_*_t
***********************************************************************************************************************/
#if CXXKIT_CC_CPP14_OR_GREATER
using std::remove_cv_t;
using std::remove_const_t;
using std::remove_pointer_t;
using std::remove_volatile_t;
using std::remove_reference_t;
#else
template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;
template <typename T>
using remove_const_t = typename std::remove_const<T>::type;
template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;
template <typename T>
using remove_volatile_t = typename std::remove_volatile<T>::type;
template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;
#endif

/***********************************************************************************************************************
 * like cxx17 std::add_*_t
***********************************************************************************************************************/
#if CXXKIT_CC_CPP14_OR_GREATER
using std::add_const_t;
using std::add_pointer_t;
using std::add_lvalue_reference_t;
#else
template <typename T>
using add_const_t = typename std::add_const<T>::type;
template <typename T>
using add_pointer_t = typename std::add_pointer<T>::type;
template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;
#endif

/***********************************************************************************************************************
 * like cxx14 std::enable_if_t
***********************************************************************************************************************/
template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

/***********************************************************************************************************************
* lis_nullptr_t
***********************************************************************************************************************/
template <typename T>
using is_nullptr_t = std::is_same<T, std::nullptr_t>;

/***********************************************************************************************************************
* like cxx17 std::is_void_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_void_v;
#else
template <typename T>
constexpr bool is_void_v = std::is_void<T>::value;
#endif

/***********************************************************************************************************************
* like cxx17 std::is_same_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_same_v;
#else
template <typename T, typename U>
constexpr bool is_same_v = std::is_same<T, U>::value;
#endif

/***********************************************************************************************************************
* like cxx17 std::is_trivially_copyable_v std::is_trivially_destructible
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_trivially_copyable_v;
using std::is_trivially_destructible;
using std::is_trivially_destructible_v;
#else
template <typename T>
constexpr bool is_trivially_copyable_v = std::is_trivially_copyable<T>::value;
template <typename T>
struct is_trivially_destructible
    : public std::integral_constant<bool,
                                    std::is_destructible<T>::value &&
                                        (std::is_trivially_copyable<T>::value || std::is_trivial<T>::value)>
{
};
template <typename T>
constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
#endif

/***********************************************************************************************************************
 * is_relocatable(is_trivially_copyable && is_trivially_destructible)
***********************************************************************************************************************/
template <typename T>
struct is_relocatable : public std::integral_constant<bool,
#if defined(CXXKIT_CC_CLANG) || !defined(CXXKIT_CC_GNU) || CXXKIT_CC_GNU >= 501
                                                      is_trivially_copyable_v<T> && is_trivially_destructible_v<T>
#else
                                                      std::is_enum<T>::value || std::is_integral<T>::value
#endif
                                                      >
{
};
template <typename T>
constexpr bool is_relocatable_v = is_relocatable<T>::value;

/***********************************************************************************************************************
 * like cxx17 std::is_convertible_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_convertible_v;
#else
template <typename F, typename T>
constexpr bool is_convertible_v = std::is_convertible<F, T>::value;
#endif

/***********************************************************************************************************************
 * like cxx17 std::is_function_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_function_v;
#else
template <typename T>
constexpr bool is_function_v = std::is_function<T>::value;
#endif

/***********************************************************************************************************************
 * like cxx17 std::is_pointer_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_pointer_v;
#else
template <typename T>
constexpr bool is_pointer_v = std::is_pointer<T>::value;
#endif

/***********************************************************************************************************************
 * like cxx17 std::is_base_of_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_base_of_v;
#else
template <typename B, typename D>
constexpr bool is_base_of_v = std::is_base_of<B, D>::value;
#endif

/***********************************************************************************************************************
 * like cxx17 std::is_member_function_pointer_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_member_function_pointer_v;
#else
template <typename T>
constexpr bool is_member_function_pointer_v = std::is_member_function_pointer<T>::value;
#endif

/***********************************************************************************************************************
 * is_function_pointer is_function_pointer_v
***********************************************************************************************************************/
template <typename T>
struct is_function_pointer
    : std::integral_constant<bool, std::is_pointer<T>::value && std::is_function<remove_pointer_t<T>>::value>
{
};
template <typename T>
constexpr bool is_function_pointer_v = is_function_pointer<T>::value;

/***********************************************************************************************************************
 * like cxx17 std::void_t
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::void_t;
#else
namespace detail
{
template <typename... Ts>
struct Void
{
    using type = void;
};
} // namespace detail
template <typename... Ts>
using void_t = typename detail::Void<Ts...>::type;
#endif

/***********************************************************************************************************************
 * like cxx14 std::conditional_t
***********************************************************************************************************************/
using std::conditional;
#if CXXKIT_CC_CPP14_OR_GREATER
using std::conditional_t;
#else
template <bool Bp, class IfRes, class ElseRes>
using conditional_t = typename std::conditional<Bp, IfRes, ElseRes>::type;
#endif

/***********************************************************************************************************************
  * like cxx17 std::invoke_result_t std::invoke
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::invoke_result_t;
using std::invoke;
#else
namespace detail
{
/**
 * The five classes below each implement one of the clauses from the definition of INVOKE.
 * The inner class template Accept<F, Args...> checks whether the clause is applicable;
 * static function template Invoke(f, args...) does the invocation.
 *
 * By separating the clause selection logic from invocation we make sure that Invoke() does exactly
 * what the standard says.
 */
template <typename Derived>
struct StrippedAccept
{
    template <typename... Args>
    struct Accept
        : Derived::template AcceptImpl<typename std::remove_cv<typename std::remove_reference<Args>::type>::type...>
    {
    };
};
/**
 * (t1.*f)(t2, ..., tN) when f is a pointer to a member function of a class T and t1 is an object of type T or a
 * reference to an object of type T or a reference to an object of a type derived from T.
 */
struct MemFunAndRef : StrippedAccept<MemFunAndRef>
{
    template <typename... Args>
    struct AcceptImpl : std::false_type
    {
    };

    template <typename MemFunType, typename C, typename Obj, typename... Args>
    struct AcceptImpl<MemFunType C::*, Obj, Args...>
        : std::integral_constant<bool, std::is_base_of<C, Obj>::value && std::is_function<MemFunType>::value>
    {
    };

    template <typename MemFun, typename Obj, typename... Args>
    static decltype((std::declval<Obj>().*std::declval<MemFun>())(std::declval<Args>()...)) Invoke(MemFun &&mem_fun,
                                                                                                   Obj &&obj,
                                                                                                   Args &&...args)
    {
        CXXKIT_WARNING_PUSH
        CXXKIT_WARNING_DISABLE_GCC("-Warray-bounds")
        CXXKIT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized")
        return (std::forward<Obj>(obj).*std::forward<MemFun>(mem_fun))(std::forward<Args>(args)...);
        CXXKIT_WARNING_POP
    }
};
/**
 * ((*t1).*f)(t2, ..., tN) when f is a pointer to a member function of a
 * class T and t1 is not one of the types described in the previous item.
 */
struct MemFunAndPtr : StrippedAccept<MemFunAndPtr>
{
    template <typename... Args>
    struct AcceptImpl : std::false_type
    {
    };

    template <typename MemFunType, typename C, typename Ptr, typename... Args>
    struct AcceptImpl<MemFunType C::*, Ptr, Args...>
        : std::integral_constant<bool, !std::is_base_of<C, Ptr>::value && std::is_function<MemFunType>::value>
    {
    };

    template <typename MemFun, typename Ptr, typename... Args>
    static decltype(((*std::declval<Ptr>()).*std::declval<MemFun>())(std::declval<Args>()...)) Invoke(MemFun &&mem_fun,
                                                                                                      Ptr &&ptr,
                                                                                                      Args &&...args)
    {
        return ((*std::forward<Ptr>(ptr)).*std::forward<MemFun>(mem_fun))(std::forward<Args>(args)...);
    }
};
/**
 * t1.*f when N == 1 and f is a pointer to member data of a class T and t1 is
 * an object of type T or a reference to an object of type T or a reference
 * to an object of a type derived from T.
 */
struct DataMemAndRef : StrippedAccept<DataMemAndRef>
{
    template <typename... Args>
    struct AcceptImpl : std::false_type
    {
    };

    template <typename R, typename C, typename Obj>
    struct AcceptImpl<R C::*, Obj>
        : std::integral_constant<bool, std::is_base_of<C, Obj>::value && !std::is_function<R>::value>
    {
    };

    template <typename DataMem, typename Ref>
    static decltype(std::declval<Ref>().*std::declval<DataMem>()) Invoke(DataMem &&data_mem, Ref &&ref)
    {
        return std::forward<Ref>(ref).*std::forward<DataMem>(data_mem);
    }
};
/**
 * (*t1).*f when N == 1 and f is a pointer to member data of a class T and t1
 * is not one of the types described in the previous item.
 */
struct DataMemAndPtr : StrippedAccept<DataMemAndPtr>
{
    template <typename... Args>
    struct AcceptImpl : std::false_type
    {
    };

    template <typename R, typename C, typename Ptr>
    struct AcceptImpl<R C::*, Ptr>
        : std::integral_constant<bool, !std::is_base_of<C, Ptr>::value && !std::is_function<R>::value>
    {
    };

    template <typename DataMem, typename Ptr>
    static decltype((*std::declval<Ptr>()).*std::declval<DataMem>()) Invoke(DataMem &&data_mem, Ptr &&ptr)
    {
        return (*std::forward<Ptr>(ptr)).*std::forward<DataMem>(data_mem);
    }
};
/**
 * f(t1, t2, ..., tN) in all other cases.
 */
struct Callable
{
    /**
     * Callable doesn't have Accept because it's the last clause that gets picked
     * when none of the previous clauses are applicable.
     */
    template <typename F, typename... Args>
    static decltype(std::declval<F>()(std::declval<Args>()...)) Invoke(F &&f, Args &&...args)
    {
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }
};
/**
 * Resolves to the first matching clause.
 */
template <typename... Args>
struct Invoker
{
    typedef typename std::conditional<
        MemFunAndRef::Accept<Args...>::value,
        MemFunAndRef,
        typename std::conditional<
            MemFunAndPtr::Accept<Args...>::value,
            MemFunAndPtr,
            typename std::conditional<
                DataMemAndRef::Accept<Args...>::value,
                DataMemAndRef,
                typename std::conditional<DataMemAndPtr::Accept<Args...>::value, DataMemAndPtr, Callable>::type>::
                type>::type>::type type;
};
} // namespace detail
// The result type of Invoke<F, Args...>.
template <typename F, typename... Args>
using invoke_result_t = decltype(detail::Invoker<F, Args...>::type::Invoke(std::declval<F>(), std::declval<Args>()...));
// Invoke(f, args...) is an implementation of INVOKE(f, args...) from section [func.require] of the C++ standard.
template <typename F, typename... Args>
invoke_result_t<F, Args...> invoke(F &&f, Args &&...args)
{
    return detail::Invoker<F, Args...>::type::Invoke(std::forward<F>(f), std::forward<Args>(args)...);
}
#endif

/***********************************************************************************************************************
 * like cxx17 std::is_invocable_r std::is_invocable_r_t
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
using std::is_invocable;
using std::is_invocable_v;
using std::is_invocable_r;
using std::is_invocable_r_v;
#else
namespace detail
{
template <typename AlwaysVoid, typename, typename, typename...>
struct IsInvocableRImpl : std::false_type
{
};
template <typename R, typename F, typename... Args>
struct IsInvocableRImpl<void_t<invoke_result_t<F, Args...>>, R, F, Args...>
    : std::integral_constant<bool, std::is_convertible<invoke_result_t<F, Args...>, R>::value || std::is_void<R>::value>
{
};
} // namespace detail
template <typename F, typename... Args>
using is_invocable = detail::IsInvocableRImpl<void, void, F, Args...>;
template <typename F, typename... Args>
constexpr bool is_invocable_v = is_invocable<F, Args...>::value;
// Type trait whose member `value` is true if invoking `F` with `Args` is valid,
// and either the return type is convertible to `R`, or `R` is void.
// C++11-compatible version of `std::is_invocable_r`.
template <typename R, typename F, typename... Args>
using is_invocable_r = detail::IsInvocableRImpl<void, R, F, Args...>;
template <typename R, typename F, typename... Args>
constexpr bool is_invocable_r_v = is_invocable_r<R, F, Args...>::value;
#endif

/***********************************************************************************************************************
  * is_weak_ptr is_weak_ptr_compatible
***********************************************************************************************************************/
namespace detail
{
template <typename T>
std::weak_ptr<T> toWeakPtr(const std::shared_ptr<T> &ptr)
{
    return ptr;
}
template <typename T>
std::weak_ptr<T> toWeakPtr(const std::weak_ptr<T> &ptr)
{
    return ptr;
}
} // namespace detail
template <typename T, typename = void>
struct is_weak_ptr : std::false_type
{
};
template <typename T>
struct is_weak_ptr<T,
                   void_t<decltype(std::declval<T>().expired()),
                          decltype(std::declval<T>().lock()),
                          decltype(std::declval<T>().reset())>> : std::true_type
{
};
template <typename T>
constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

template <typename T, typename = void>
struct is_weak_ptr_compatible : std::false_type
{
};
template <typename T>
struct is_weak_ptr_compatible<T, void_t<decltype(detail::toWeakPtr(std::declval<T>()))>>
    : is_weak_ptr<decltype(detail::toWeakPtr(std::declval<T>()))>
{
};
template <typename T>
constexpr bool is_weak_ptr_compatible_v = is_weak_ptr_compatible<T>::value;

/***********************************************************************************************************************
  * has_call_operator has_call_operator_v
***********************************************************************************************************************/
#if CXXKIT_CC_CPP17_OR_GREATER
template <typename T>
struct has_call_operator
{
    static constexpr bool value = std::is_invocable_v<T>;
};
#else
template <typename, typename = void>
struct has_call_operator : std::false_type
{
};
template <typename F>
struct has_call_operator<F, void_t<decltype(&std::remove_reference<F>::type::operator())>> : std::true_type
{
};
#endif
template <typename T>
constexpr bool has_call_operator_v = has_call_operator<T>::value;

/***********************************************************************************************************************
 * A slightly different decay than in the standard, the extent of arrays are not removed.
 * Applies lvalue-to-rvalue, function-to-pointer implicit conversions to the type T and removes cv-qualifiers.
 * @code
 *  decay_except_array<int&>::type → int
 *  decay_except_array<const int>::type → int
 *  decay_except_array<void(int)>::type → void(*)(int)
 *  decay_except_array<int[5]>::type → int[5]  // Preserve array dimensions
 * @endcode
***********************************************************************************************************************/
template <typename T>
struct decay_except_array
{
    using Tp = remove_reference_t<T>;
    using type = conditional_t<std::is_function<Tp>::value, add_pointer_t<Tp>, remove_cv_t<Tp>>;
};
template <typename T>
using decay_except_array_t = typename decay_except_array<T>::type;

/***********************************************************************************************************************
 * This trait will removes cv-qualifiers, pointers and reference from type T.
***********************************************************************************************************************/
template <typename T, typename Enable = void>
struct raw_type
{
    using type = remove_cv_t<T>;
};
template <typename T>
struct raw_type<T, enable_if_t<std::is_pointer<T>::value && !is_function_pointer<T>::value>>
{
    using type = typename raw_type<remove_pointer_t<T>>::type;
};
template <typename T>
struct raw_type<T, enable_if_t<std::is_reference<T>::value>>
{
    using type = typename raw_type<remove_reference_t<T>>::type;
};
template <typename T>
using raw_type_t = typename raw_type<T>::type;

/***********************************************************************************************************************
 * this trait will remove the cv-qualifier, pointer types, reference type and also the array dimension
***********************************************************************************************************************/
template <typename T, typename Enable = void>
struct raw_array_type
{
    using type = raw_type_t<T>;
};
namespace detail
{
template <typename T>
struct raw_array_type_impl;

template <typename T, std::size_t N>
struct raw_array_type_impl<T[N]>
{
    using type = typename raw_array_type<T>::type;
};
} // namespace detail
template <typename T>
struct raw_array_type<T, typename std::enable_if<std::is_array<T>::value>::type>
{
    using type = typename detail::raw_array_type_impl<remove_cv_t<T>>::type;
};
template <typename T>
using raw_array_type_t = typename raw_array_type<T>::type;

/***********************************************************************************************************************
 * is_one_dim_char_array<T>::value Returns true if the given type is a char array
 * e.g. is_one_dim_char_array<char[10][10]>::value => false
 * e.g. is_one_dim_char_array<char[10]>::value => true
 *      is_one_dim_char_array<int[10]>::value => false
 *      is_one_dim_char_array<char>::value => false
***********************************************************************************************************************/
template <typename T>
using is_one_dim_char_array = std::integral_constant<
    bool,
    std::is_array<T>::value && std::is_same<char, raw_array_type_t<T>>::value && (std::rank<T>::value == 1)>;
} // namespace traits


template <typename... Ts>
using VoidType = traits::void_t<Ts...>; //TODO::DEL

/***********************************************************************************************************************
  * like cxx17 std::conjunction
***********************************************************************************************************************/
template <typename...>
struct Conjunction : std::true_type
{
};
template <typename Arg>
struct Conjunction<Arg> : Arg
{
};
template <typename Arg, typename... Args>
struct Conjunction<Arg, Args...> : std::conditional<!bool(Arg::value), Arg, Conjunction<Args...>>::type
{
};

/***********************************************************************************************************************
 * ReturnsVoid
***********************************************************************************************************************/
template <typename F>
struct ReturnsVoid
    : std::conditional<std::is_same<traits::invoke_result_t<F>, void>::value, std::true_type, std::false_type>::type
{
};

/***********************************************************************************************************************
  * Determines if the given class has zero-argument .data() and .size() methods
  * whose return values are convertible to T* and size_t, respectively.
***********************************************************************************************************************/
template <typename DS, typename T>
class HasDataAndSize
{
private:
    template <typename C,
              typename std::enable_if<std::is_convertible<decltype(std::declval<C>().data()), T *>::value &&
                                      std::is_convertible<decltype(std::declval<C>().size()), std::size_t>::value>::type
                  * = nullptr>
    static int Test(int);

    template <typename>
    static char Test(...);

public:
    static constexpr bool value = std::is_same<decltype(Test<DS>(0)), int>::value;
};

namespace test_has_data_and_size
{

template <typename DR, typename SR>
struct Test1
{
    DR data();
    SR size();
};

static_assert(HasDataAndSize<Test1<int *, int>, int>::value, "");
static_assert(HasDataAndSize<Test1<int *, int>, const int>::value, "");
static_assert(HasDataAndSize<Test1<const int *, int>, const int>::value, "");
static_assert(!HasDataAndSize<Test1<const int *, int>, int>::value, "implicit cast of const int* to int*");
static_assert(!HasDataAndSize<Test1<char *, size_t>, int>::value, "implicit cast of char* to int*");

struct Test2
{
    int *data;
    size_t size;
};
static_assert(!HasDataAndSize<Test2, int>::value, ".data and .size aren't functions");

struct Test3
{
    int *data();
};

static_assert(!HasDataAndSize<Test3, int>::value, ".size() is missing");

class Test4
{
    int *data();
    size_t size();
};

static_assert(!HasDataAndSize<Test4, int>::value, ".data() and .size() are private");
} // namespace test_has_data_and_size

namespace detail
{

// Determines if the given type is an enum that converts implicitly to an integral type.
template <typename T>
struct IsIntEnum
{
private:
    // This overload is used if the type is an enum, and unary plus
    // compiles and turns it into an integral type.
    template <typename X,
              typename std::enable_if<std::is_enum<X>::value &&
                                      std::is_integral<decltype(+std::declval<X>())>::value>::type * = nullptr>
    static int Test(int);

    // Otherwise, this overload is used.
    template <typename>
    static char Test(...);

public:
    static constexpr bool value = std::is_same<decltype(Test<typename std::remove_reference<T>::type>(0)), int>::value;
};
} // namespace detail
// Determines if the given type is integral, or an enum that converts implicitly to an integral type.
template <typename T>
struct IsIntLike
{
private:
    using X = typename std::remove_reference<T>::type;

public:
    static constexpr bool value = std::is_integral<X>::value || detail::IsIntEnum<X>::value;
};
namespace test_enum_intlike
{

enum E1
{
    e1
};
enum
{
    e2
};
enum class E3
{
    e3
};
struct S
{
};

static_assert(detail::IsIntEnum<E1>::value, "");
static_assert(detail::IsIntEnum<decltype(e2)>::value, "");
static_assert(!detail::IsIntEnum<E3>::value, "");
static_assert(!detail::IsIntEnum<int>::value, "");
static_assert(!detail::IsIntEnum<float>::value, "");
static_assert(!detail::IsIntEnum<S>::value, "");

static_assert(IsIntLike<E1>::value, "");
static_assert(IsIntLike<decltype(e2)>::value, "");
static_assert(!IsIntLike<E3>::value, "");
static_assert(IsIntLike<int>::value, "");
static_assert(!IsIntLike<float>::value, "");
static_assert(!IsIntLike<S>::value, "");
} // namespace test_enum_intlike

CXXKIT_END_NAMESPACE
