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

#include <cxxkit/functional/unique_function.hpp>
#include <cxxkit/functional/function_view.hpp>
#include <cxxkit/memory/memory.hpp>

#include <memory>
#include <utility>

#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
static int call(FunctionView<int()> fun)
{
    return fun();
}

static UniqueFunction<void()> moveUniqueFunction()
{
    // remove the commented dummy capture to be compilable
    UniqueFunction<void()> func = [i = std::vector<std::vector<std::unique_ptr<int>>>{}]()
    {
        // ...
    };
    return std::move(func);
}
} // namespace

TEST(UniqueFunctionTest, CanMoveNonowningNoncopyableView)
{
    UniqueFunction<int()> fun = []() mutable { return 12345; };
    int result = call(fun);
    ASSERT_EQ(result, 12345);
}

TEST(UniqueFunctionTest, CanAssignNonowningNoncopyableView)
{
    UniqueFunction<int()> fun = []() mutable { return 12345; };
    FunctionView<int()> fv;
    fv = fun;
    int result = fv();
    ASSERT_EQ(result, 12345);
}

TEST(UniqueFunctionTest, CanSelfContaining)
{
    std::function<bool()> first = [] { return true; };
    UniqueFunction<bool()> second = first;
    EXPECT_TRUE(first() && second());
}

TEST(UniqueFunctionTest, CanBeStoredInVector)
{
    using fun_t = UniqueFunction<int(int)>;

    std::vector<fun_t> v;
    v.reserve(1);
    fun_t f{[](int i) { return 2 * i; }};
    fun_t f2{[](int i) { return 2 * i; }};
    v.emplace_back(std::move(f));
    v.emplace_back(std::move(f2));

    auto const res = v[0](7);
    ASSERT_EQ(res, 14);
}

TEST(UniqueFunctionTest, CanCopyAssignableAndConstructible)
{
    using fun_t = UniqueFunction<int(int)>;
    ASSERT_FALSE(std::is_copy_assignable<fun_t>::value);
    ASSERT_FALSE(std::is_copy_constructible<fun_t>::value);
}

CXXKIT_END_NAMESPACE
