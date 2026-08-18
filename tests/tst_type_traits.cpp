/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
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

#include "cxxkit/tools/type_traits.hpp"
#include "cxxkit/memory/memory.hpp"

#include <thread>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
template <typename... Args> std::string StrCat(Args &&...args)
{
    std::string result;
    for (const auto &elem : {std::forward<Args>(args)...})
    {
        result += elem;
    }
    return result;
}

int Sink(std::unique_ptr<int> p) { return *p; }

std::unique_ptr<int> Factory(int n) { return utils::make_unique<int>(n); }

void NoOp() { }

struct Functor
{
    void operator()(int) const { }
    int operator()(double, char) { return 0; }
};

struct ConstFunctor
{
    int operator()(int) const { return 0; }
    int operator()(int a, int b) const { return a - b; }
};

struct MutableFunctor
{
    int operator()(int a, int b) { return a - b; }
};

struct EphemeralFunctor
{
    int operator()(int a, int b) && { return a - b; }
};

struct OverloadedFunctor
{
    template <typename... Args> std::string operator()(const Args &...args) & { return StrCat("&", args...); }
    template <typename... Args> std::string operator()(const Args &...args) const &
    {
        return StrCat("const&", args...);
    }
    template <typename... Args> std::string operator()(const Args &...args) && { return StrCat("&&", args...); }
};

struct FlipFlop
{
    int ConstMethod() const { return member; }
    FlipFlop operator*() const { return {-member}; }

    int member;
};

// CallMaybeWithArg(f) resolves either to invoke(f) or invoke(f, 42), depending on which one is valid.
template <typename F> decltype(traits::invoke(std::declval<const F &>())) CallMaybeWithArg(const F &f)
{
    return traits::invoke(f);
}

template <typename F> decltype(traits::invoke(std::declval<const F &>(), 42)) CallMaybeWithArg(const F &f)
{
    return traits::invoke(f, 42);
}

int Function(int a, int b) { return a - b; }

int FreeFunction(int, double) { return 0; }

void VoidFunction(int &a, int &b)
{
    a += b;
    b = a - b;
    a -= b;
}

int ZeroArgFunction() { return -1937; }

struct Class
{
    int Method(int a, int b) { return a - b; }
    int ConstMethod(int a, int b) const { return a - b; }
    int RefMethod(int a, int b) & { return a - b; }
    int RefRefMethod(int a, int b) && { return a - b; }
    int NoExceptMethod(int a, int b) noexcept { return a - b; }
    int VolatileMethod(int a, int b) volatile { return a - b; }

    int member;
};

auto Lambda = [](int, double) -> int { return 0; };
auto MutableLambda = [](int, double) mutable -> int { return 0; };

using StdFunction = std::function<int(int, double)>;

} // namespace

TEST(InvokeTest, Function)
{
    EXPECT_EQ(1, traits::invoke(Function, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Function, 3, 2));
}

TEST(InvokeTest, NonCopyableArgument) { EXPECT_EQ(42, traits::invoke(Sink, utils::make_unique<int>(42))); }

TEST(InvokeTest, NonCopyableResult) { EXPECT_EQ(*traits::invoke(Factory, 42).get(), 42); }

TEST(InvokeTest, VoidResult) { traits::invoke(NoOp); }

TEST(InvokeTest, ConstFunctor) { EXPECT_EQ(1, traits::invoke(ConstFunctor(), 3, 2)); }

TEST(InvokeTest, MutableFunctor)
{
    MutableFunctor f;
    EXPECT_EQ(1, traits::invoke(f, 3, 2));
    EXPECT_EQ(1, traits::invoke(MutableFunctor(), 3, 2));
}

TEST(InvokeTest, EphemeralFunctor)
{
    EphemeralFunctor f;
    EXPECT_EQ(1, traits::invoke(std::move(f), 3, 2));
    EXPECT_EQ(1, traits::invoke(EphemeralFunctor(), 3, 2));
}

TEST(InvokeTest, OverloadedFunctor)
{
    OverloadedFunctor f;
    const OverloadedFunctor &cf = f;

    EXPECT_EQ("&", traits::invoke(f));
    EXPECT_EQ("& 42", traits::invoke(f, " 42"));

    EXPECT_EQ("const&", traits::invoke(cf));
    EXPECT_EQ("const& 42", traits::invoke(cf, " 42"));

    EXPECT_EQ("&&", traits::invoke(std::move(f)));

    OverloadedFunctor f2;
    EXPECT_EQ("&& 42", traits::invoke(std::move(f2), " 42"));
}

TEST(InvokeTest, ReferenceWrapper)
{
    ConstFunctor cf;
    MutableFunctor mf;
    EXPECT_EQ(1, traits::invoke(std::cref(cf), 3, 2));
    EXPECT_EQ(1, traits::invoke(std::ref(cf), 3, 2));
    EXPECT_EQ(1, traits::invoke(std::ref(mf), 3, 2));
}

TEST(InvokeTest, MemberFunction)
{
    std::unique_ptr<Class> p(new Class);
    std::unique_ptr<const Class> cp(new Class);
    std::unique_ptr<volatile Class> vp(new Class);

    EXPECT_EQ(1, traits::invoke(&Class::Method, p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::Method, p.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::Method, *p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::RefMethod, p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::RefMethod, p.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::RefMethod, *p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::RefRefMethod, std::move(*p), 3,
                                     2)); // NOLINT
    EXPECT_EQ(1, traits::invoke(&Class::NoExceptMethod, p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::NoExceptMethod, p.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::NoExceptMethod, *p, 3, 2));

    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, p.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, *p, 3, 2));

    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, cp, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, cp.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, *cp, 3, 2));

    EXPECT_EQ(1, traits::invoke(&Class::VolatileMethod, p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::VolatileMethod, p.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::VolatileMethod, *p, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::VolatileMethod, vp, 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::VolatileMethod, vp.get(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::VolatileMethod, *vp, 3, 2));

    EXPECT_EQ(1, traits::invoke(&Class::Method, utils::make_unique<Class>(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, utils::make_unique<Class>(), 3, 2));
    EXPECT_EQ(1, traits::invoke(&Class::ConstMethod, utils::make_unique<const Class>(), 3, 2));
}

TEST(InvokeTest, DataMember)
{
    std::unique_ptr<Class> p(new Class{42});
    std::unique_ptr<const Class> cp(new Class{42});
    EXPECT_EQ(42, traits::invoke(&Class::member, p));
    EXPECT_EQ(42, traits::invoke(&Class::member, *p));
    EXPECT_EQ(42, traits::invoke(&Class::member, p.get()));

    traits::invoke(&Class::member, p) = 42;
    traits::invoke(&Class::member, p.get()) = 42;

    EXPECT_EQ(42, traits::invoke(&Class::member, cp));
    EXPECT_EQ(42, traits::invoke(&Class::member, *cp));
    EXPECT_EQ(42, traits::invoke(&Class::member, cp.get()));
}

TEST(InvokeTest, FlipFlop)
{
    FlipFlop obj = {42};
    // This call could resolve to (obj.*&FlipFlop::ConstMethod)() or
    // ((*obj).*&FlipFlop::ConstMethod)(). We verify that it's the former.
    EXPECT_EQ(42, traits::invoke(&FlipFlop::ConstMethod, obj));
    EXPECT_EQ(42, traits::invoke(&FlipFlop::member, obj));
}

TEST(InvokeTest, SfinaeFriendly)
{
    CallMaybeWithArg(NoOp);
    EXPECT_EQ(*CallMaybeWithArg(Factory).get(), 42);
}


TEST(IsInvocableTest, FreeFunctionExactMatch)
{
    static_assert(traits::is_invocable<decltype(FreeFunction), int, double>::value,
                  "Should be true for exact match on a free function");
}

TEST(IsInvocableTest, FreeFunctionArgumentConversion)
{
    static_assert(traits::is_invocable<decltype(FreeFunction), short, float>::value,
                  "Should be true for convertible argument types");
}

TEST(IsInvocableTest, FreeFunctionVoidReturn)
{
    static_assert(traits::is_invocable<decltype(VoidFunction), int &, int &>::value,
                  "Should be true for void return type");
}

TEST(IsInvocableTest, FreeFunctionArgumentTypeMismatch)
{
    static_assert(!traits::is_invocable<decltype(FreeFunction), std::string, double>::value,
                  "Should be false for first argument type mismatch");
    static_assert(!traits::is_invocable<decltype(FreeFunction), int, std::string>::value,
                  "Should be false for second argument type mismatch");
}

TEST(IsInvocableTest, FreeFunctionArgumentCountMismatch)
{
    static_assert(!traits::is_invocable<decltype(FreeFunction), int>::value,
                  "Should be false for too few arguments");
    static_assert(!traits::is_invocable<decltype(FreeFunction), int, double, char>::value,
                  "Should be false for too many arguments");
}

TEST(IsInvocableTest, FreeFunctionZeroArgs)
{
    static_assert(traits::is_invocable<decltype(ZeroArgFunction)>::value,
                  "Should be true for zero-arg free function");
}

TEST(IsInvocableTest, FunctorExactMatch)
{
    static_assert(traits::is_invocable<Functor, int, double>::value,
                  "Should be true for exact match on a functor");
}

TEST(IsInvocableTest, ConstFunctorExactMatch)
{
    static_assert(traits::is_invocable<ConstFunctor, int, double>::value,
                  "Should be true for exact match on a const functor");
    static_assert(traits::is_invocable<const ConstFunctor, int, double>::value,
                  "Should be true for const object of const functor");
}

TEST(IsInvocableTest, FunctorArgumentConversion)
{
    static_assert(traits::is_invocable<Functor, short, float>::value,
                  "Should be true for convertible arguments to functor");
}

TEST(IsInvocableTest, LambdaExactMatch)
{
    static_assert(traits::is_invocable<decltype(Lambda), int, double>::value,
                  "Should be true for exact match on a lambda");
}

TEST(IsInvocableTest, MutableLambdaExactMatch)
{
    static_assert(traits::is_invocable<decltype(MutableLambda), int, double>::value,
                  "Should be true for exact match on a mutable lambda");
}

TEST(IsInvocableTest, LambdaArgumentConversion)
{
    static_assert(traits::is_invocable<decltype(Lambda), short, float>::value,
                  "Should be true for convertible arguments to lambda");
}

TEST(IsInvocableTest, StdFunctionExactMatch)
{
    StdFunction func = [](int, double) { return 0; };
    static_assert(traits::is_invocable<StdFunction, int, double>::value,
                  "Should be true for exact match on std::function");
}

TEST(IsInvocableTest, StdFunctionArgumentConversion)
{
    static_assert(traits::is_invocable<StdFunction, short, float>::value,
                  "Should be true for convertible arguments to std::function");
}

TEST(IsInvocableTest, MemberFunctionWithReference)
{
    static_assert(traits::is_invocable<decltype(&Class::Method), Class &, int, double>::value,
                  "Should be true for member function with class reference");
}

TEST(IsInvocableTest, MemberFunctionWithPointer)
{
    static_assert(traits::is_invocable<decltype(&Class::Method), Class *, int, double>::value,
                  "Should be true for member function with class pointer");
}

TEST(IsInvocableTest, ConstMemberFunctionWithConstReference)
{
    static_assert(traits::is_invocable<decltype(&Class::ConstMethod), const Class &, int, double>::value,
                  "Should be true for const member function with const reference");
}

TEST(IsInvocableTest, MemberFunctionArgumentConversion)
{
    static_assert(traits::is_invocable<decltype(&Class::Method), Class &, short, float>::value,
                  "Should be true for convertible arguments to member function");
}

TEST(IsInvocableTest, MemberFunctionObjectTypeMismatch)
{
    static_assert(!traits::is_invocable<decltype(&Class::Method), int, int, double>::value,
                  "Should be false for wrong object type");
    static_assert(!traits::is_invocable<decltype(&Class::Method), const Class &, int, double>::value,
                  "Should be false for const reference to non-const method");
}

TEST(IsInvocableTest, DataMemberWithReference)
{
    static_assert(traits::is_invocable<decltype(&Class::member), Class &>::value,
                  "Should be true for data member with class reference");
}

TEST(IsInvocableTest, DataMemberWithPointer)
{
    static_assert(traits::is_invocable<decltype(&Class::member), Class *>::value,
                  "Should be true for data member with class pointer");
}

TEST(IsInvocableTest, DataMemberReturnsReference)
{
    static_assert(traits::is_invocable<decltype(&Class::member), Class &>::value,
                  "Data member access should be invocable");
}

TEST(IsInvocableTest, NonCallableTypes)
{
    static_assert(!traits::is_invocable<int>::value, "Should be false for int");
    static_assert(!traits::is_invocable<double, int>::value, "Should be false for double with argument");
    static_assert(!traits::is_invocable<std::string, char>::value,
                  "Should be false for std::string with argument");
}

TEST(IsInvocableTest, PointerToNonCallable)
{
    static_assert(!traits::is_invocable<int *, int>::value, "Should be false for pointer to non-callable type");
}

TEST(IsInvocableTest, LvalueReferenceArguments)
{
    auto takes_lvalue_ref = [](int &) { };
    int x = 0;
    static_assert(traits::is_invocable<decltype(takes_lvalue_ref), int &>::value,
                  "Should be true for lvalue reference argument");
    static_assert(!traits::is_invocable<decltype(takes_lvalue_ref), const int &>::value,
                  "Should be false for const lvalue ref to non-const param");
    static_assert(!traits::is_invocable<decltype(takes_lvalue_ref), int>::value,
                  "Should be false for rvalue to lvalue ref param");
}

TEST(IsInvocableTest, ConstLvalueReferenceArguments)
{
    auto takes_const_ref = [](const int &) { };
    static_assert(traits::is_invocable<decltype(takes_const_ref), const int &>::value,
                  "Should be true for const lvalue ref to const param");
    static_assert(traits::is_invocable<decltype(takes_const_ref), int &>::value,
                  "Should be true for lvalue ref to const param");
    static_assert(traits::is_invocable<decltype(takes_const_ref), int>::value,
                  "Should be true for rvalue to const param");
}

TEST(IsInvocableTest, RvalueReferenceArguments)
{
    auto takes_rvalue_ref = [](int &&) { };
    static_assert(traits::is_invocable<decltype(takes_rvalue_ref), int>::value,
                  "Should be true for rvalue to rvalue ref param");
    static_assert(!traits::is_invocable<decltype(takes_rvalue_ref), int &>::value,
                  "Should be false for lvalue ref to rvalue ref param");
}

TEST(IsInvocableTest, VariadicFunction)
{
    auto variadic_func = [](auto &&...) { };
    static_assert(traits::is_invocable<decltype(variadic_func)>::value,
                  "Should be true for variadic with zero args");
    static_assert(traits::is_invocable<decltype(variadic_func), int>::value,
                  "Should be true for variadic with one arg");
    static_assert(traits::is_invocable<decltype(variadic_func), int, double, std::string>::value,
                  "Should be true for variadic with multiple args");
}

TEST(IsInvocableTest, MoveOnlyArguments)
{
    auto takes_unique_ptr = [](std::unique_ptr<int>) { };
    static_assert(traits::is_invocable<decltype(takes_unique_ptr), std::unique_ptr<int>>::value,
                  "Should be true for move-only type argument");
    static_assert(!traits::is_invocable<decltype(takes_unique_ptr), std::unique_ptr<int> &>::value,
                  "Should be false for lvalue ref to move-only param");
}

TEST(IsInvocableRTest, CallableExactMatch)
{
    static_assert(traits::is_invocable_r<int, decltype(Function), int, int>::value,
                  "Should be true for exact match of types on a free function");
}

TEST(IsInvocableRTest, CallableArgumentConversionMatch)
{
    static_assert(traits::is_invocable_r<int, decltype(Function), char, int>::value,
                  "Should be true for convertible argument type");
}

TEST(IsInvocableRTest, CallableReturnConversionMatch)
{
    static_assert(traits::is_invocable_r<double, decltype(Function), int, int>::value,
                  "Should be true for convertible return type");
}

TEST(IsInvocableRTest, CallableReturnVoid)
{
    static_assert(traits::is_invocable_r<void, decltype(VoidFunction), int &, int &>::value,
                  "Should be true for void expected and actual return types");
    static_assert(traits::is_invocable_r<void, decltype(Function), int, int>::value,
                  "Should be true for void expected and non-void actual return types");
}

TEST(IsInvocableRTest, CallableRefQualifierMismatch)
{
    static_assert(!traits::is_invocable_r<void, decltype(VoidFunction), int &, const int &>::value,
                  "Should be false for reference constness mismatch");
    static_assert(!traits::is_invocable_r<void, decltype(VoidFunction), int &&, int &>::value,
                  "Should be false for reference value category mismatch");
}

TEST(IsInvocableRTest, CallableArgumentTypeMismatch)
{
    static_assert(!traits::is_invocable_r<int, decltype(Function), std::string, int>::value,
                  "Should be false for argument type mismatch");
}

TEST(IsInvocableRTest, CallableReturnTypeMismatch)
{
    static_assert(!traits::is_invocable_r<std::string, decltype(Function), int, int>::value,
                  "Should be false for return type mismatch");
}

TEST(IsInvocableRTest, CallableTooFewArgs)
{
    static_assert(!traits::is_invocable_r<int, decltype(Function), int>::value,
                  "Should be false for too few arguments");
}

TEST(IsInvocableRTest, CallableTooManyArgs)
{
    static_assert(!traits::is_invocable_r<int, decltype(Function), int, int, int>::value,
                  "Should be false for too many arguments");
}

TEST(IsInvocableRTest, MemberFunctionAndReference)
{
    static_assert(traits::is_invocable_r<int, decltype(&Class::Method), Class &, int, int>::value,
                  "Should be true for exact match of types on a member function "
                  "and class reference");
}

TEST(IsInvocableRTest, MemberFunctionAndPointer)
{
    static_assert(traits::is_invocable_r<int, decltype(&Class::Method), Class *, int, int>::value,
                  "Should be true for exact match of types on a member function "
                  "and class pointer");
}

TEST(IsInvocableRTest, DataMemberAndReference)
{
    static_assert(traits::is_invocable_r<int, decltype(&Class::member), Class &>::value,
                  "Should be true for exact match of types on a data member and "
                  "class reference");
}

TEST(IsInvocableRTest, DataMemberAndPointer)
{
    static_assert(traits::is_invocable_r<int, decltype(&Class::member), Class *>::value,
                  "Should be true for exact match of types on a data member and "
                  "class pointer");
}

TEST(IsInvocableRTest, CallableZeroArgs)
{
    static_assert(traits::is_invocable_r<int, decltype(ZeroArgFunction)>::value,
                  "Should be true for exact match for a zero-arg free function");
}

CXXKIT_END_NAMESPACE
