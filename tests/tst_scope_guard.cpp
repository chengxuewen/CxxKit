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

//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cxxkit/tools/scope_guard.hpp>
#include <cxxkit/tools/utility.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <functional>
#include <utility>

using namespace cxxkit;

namespace
{

// using Tag = absl::cleanup_internal::Tag;

template <typename Type1, typename Type2>
constexpr bool IsSame()
{
    return (std::is_same<Type1, Type2>::value);
}

struct IdentityFactory
{
    template <typename Callback>
    static Callback AsCallback(Callback callback)
    {
        return Callback(std::move(callback));
    }
};

// `FunctorClass` is a type used for testing `scope_guard`. It is intended to
// represent users that make their own move-only callback types outside of
// `std::function` and lambda literals.
class FunctorClass
{
    using Callback = std::function<void()>;

public:
    explicit FunctorClass(Callback callback)
        : callback_(std::move(callback))
    {
    }

    FunctorClass(FunctorClass &&other)
        : callback_(utils::exchange(other.callback_, Callback()))
    {
    }

    FunctorClass(const FunctorClass &) = delete;

    FunctorClass &operator=(const FunctorClass &) = delete;

    FunctorClass &operator=(FunctorClass &&) = delete;

    void operator()() const & = delete;

    void operator()() &&
    {
        ASSERT_TRUE(callback_);
        callback_();
        callback_ = nullptr;
    }

private:
    Callback callback_;
};

struct FunctorClassFactory
{
    template <typename Callback>
    static FunctorClass AsCallback(Callback callback)
    {
        return FunctorClass(std::move(callback));
    }
};

struct StdFunctionFactory
{
    template <typename Callback>
    static std::function<void()> AsCallback(Callback callback)
    {
        return std::function<void()>(std::move(callback));
    }
};

using CleanupTestParams = ::testing::Types<IdentityFactory, FunctorClassFactory, StdFunctionFactory>;
template <typename>
struct CleanupTest : public ::testing::Test
{
};
TYPED_TEST_SUITE(CleanupTest, CleanupTestParams);

bool fn_ptr_called = false;
void FnPtrFunction()
{
    fn_ptr_called = true;
}

TYPED_TEST(CleanupTest, FactoryProducesCorrectType)
{
    {
        auto callback = TypeParam::AsCallback([] {});
        auto scopeGuard = utils::make_scope_guard(std::move(callback));
        static_assert(IsSame<scope_guard<decltype(callback)>, decltype(scopeGuard)>(), "");
    }
    {
        auto scopeGuard = utils::make_scope_guard(&FnPtrFunction);
        static_assert(IsSame<scope_guard<void (*)()>, decltype(scopeGuard)>(), "");
    }
    {
        auto scopeGuard = utils::make_scope_guard(FnPtrFunction);
        static_assert(IsSame<scope_guard<void (*)()>, decltype(scopeGuard)>(), "");
    }
}

#if CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
TYPED_TEST(CleanupTest, CTADProducesCorrectType)
{
    {
        auto callback = TypeParam::AsCallback([] {});
        scope_guard scopeGuard = std::move(callback);
        static_assert(IsSame<scope_guard<decltype(callback)>, decltype(scopeGuard)>(), "");
    }
    {
        scope_guard scopeGuard = &FnPtrFunction;
        static_assert(IsSame<scope_guard<void (*)()>, decltype(scopeGuard)>(), "");
    }
    {
        scope_guard scopeGuard = FnPtrFunction;
        static_assert(IsSame<scope_guard<void (*)()>, decltype(scopeGuard)>(), "");
    }
}

TYPED_TEST(CleanupTest, FactoryAndCTADProduceSameType)
{
    {
        auto callback = IdentityFactory::AsCallback([] {});
        auto factory_cleanup = utils::make_scope_guard(callback);
        scope_guard deduction_cleanup = callback;
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::make_scope_guard(FunctorClassFactory::AsCallback([] {}));
        scope_guard deduction_cleanup = FunctorClassFactory::AsCallback([] {});
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::make_scope_guard(StdFunctionFactory::AsCallback([] {}));
        scope_guard deduction_cleanup = StdFunctionFactory::AsCallback([] {});
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::make_scope_guard(&FnPtrFunction);
        scope_guard deduction_cleanup = &FnPtrFunction;
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::make_scope_guard(FnPtrFunction);
        scope_guard deduction_cleanup = FnPtrFunction;
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
}
#endif // CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION

TYPED_TEST(CleanupTest, BasicUsage)
{
    bool called = false;

    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called); // Constructor shouldn't invoke the callback
    }

    EXPECT_TRUE(called); // Destructor should invoke the callback
}

TYPED_TEST(CleanupTest, BasicUsageWithFunctionPointer)
{
    fn_ptr_called = false;

    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback(&FnPtrFunction));
        EXPECT_FALSE(fn_ptr_called); // Constructor shouldn't invoke the callback
    }

    EXPECT_TRUE(fn_ptr_called); // Destructor should invoke the callback
}

TYPED_TEST(CleanupTest, Cancel)
{
    bool called = false;

    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called); // Constructor shouldn't invoke the callback

        std::move(scopeGuard).cancel();
        EXPECT_FALSE(called); // Cancel shouldn't invoke the callback
    }

    EXPECT_FALSE(called); // Destructor shouldn't invoke the callback
}

TYPED_TEST(CleanupTest, invoke)
{
    bool called = false;

    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called); // Constructor shouldn't invoke the callback

        std::move(scopeGuard).invoke();
        EXPECT_TRUE(called); // invoke should invoke the callback

        called = false; // reset tracker before destructor runs
    }

    EXPECT_FALSE(called); // Destructor shouldn't invoke the callback
}

TYPED_TEST(CleanupTest, Move)
{
    bool called = false;

    {
        auto moved_from_cleanup = utils::make_scope_guard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called); // Constructor shouldn't invoke the callback

        {
            auto moved_to_cleanup = std::move(moved_from_cleanup);
            EXPECT_FALSE(called); // Move shouldn't invoke the callback
        }

        EXPECT_TRUE(called); // Destructor should invoke the callback

        called = false; // reset tracker before destructor runs
    }

    EXPECT_FALSE(called); // Destructor shouldn't invoke the callback
}

int DestructionCount = 0;

struct DestructionCounter
{
    void operator()() { }

    ~DestructionCounter() { ++DestructionCount; }
};

TYPED_TEST(CleanupTest, DestructorDestroys)
{
    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback(DestructionCounter()));
        DestructionCount = 0;
    }

    EXPECT_EQ(DestructionCount, 1); // Engaged scopeGuard destroys
}

TYPED_TEST(CleanupTest, CancelDestroys)
{
    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback(DestructionCounter()));
        DestructionCount = 0;

        std::move(scopeGuard).cancel();
        EXPECT_EQ(DestructionCount, 1); // Cancel destroys
    }

    EXPECT_EQ(DestructionCount, 1); // Canceled scopeGuard does not double destroy
}

TYPED_TEST(CleanupTest, InvokeDestroys)
{
    {
        auto scopeGuard = utils::make_scope_guard(TypeParam::AsCallback(DestructionCounter()));
        DestructionCount = 0;

        std::move(scopeGuard).invoke();
        EXPECT_EQ(DestructionCount, 1); // invoke destroys
    }

    EXPECT_EQ(DestructionCount, 1); // Invoked scopeGuard does not double destroy
}
} // namespace
