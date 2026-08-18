// Copyright 2021 The Abseil Authors.
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

#include "cxxkit/tools/scope_guard.hpp"
#include "cxxkit/tools/utility.hpp"

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

// `FunctorClass` is a type used for testing `ScopeGuard`. It is intended to
// represent users that make their own move-only callback types outside of
// `std::function` and lambda literals.
class FunctorClass
{
    using Callback = std::function<void()>;

public:
    explicit FunctorClass(Callback callback) : callback_(std::move(callback)) {}

    FunctorClass(FunctorClass &&other) : callback_(utils::exchange(other.callback_, Callback())) {}

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
struct CleanupTest : public ::testing::Test {};
TYPED_TEST_SUITE(CleanupTest, CleanupTestParams);

bool fn_ptr_called = false;
void FnPtrFunction() { fn_ptr_called = true; }

TYPED_TEST(CleanupTest, FactoryProducesCorrectType)
{
    {
        auto callback = TypeParam::AsCallback([] {});
        auto scopeGuard = utils::makeScopeGuard(std::move(callback));
        static_assert(IsSame<ScopeGuard<decltype(callback)>, decltype(scopeGuard)>(), "");
    }
    {
        auto scopeGuard = utils::makeScopeGuard(&FnPtrFunction);
        static_assert(IsSame<ScopeGuard<void (*)()>, decltype(scopeGuard)>(), "");
    }
    {
        auto scopeGuard = utils::makeScopeGuard(FnPtrFunction);
        static_assert(IsSame<ScopeGuard<void (*)()>, decltype(scopeGuard)>(), "");
    }
}

#if CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
TYPED_TEST(CleanupTest, CTADProducesCorrectType)
{
    {
        auto callback = TypeParam::AsCallback([] {});
        ScopeGuard scopeGuard = std::move(callback);
        static_assert(IsSame<ScopeGuard<decltype(callback)>, decltype(scopeGuard)>(), "");
    }
    {
        ScopeGuard scopeGuard = &FnPtrFunction;
        static_assert(IsSame<ScopeGuard<void (*)()>, decltype(scopeGuard)>(), "");
    }
    {
        ScopeGuard scopeGuard = FnPtrFunction;
        static_assert(IsSame<ScopeGuard<void (*)()>, decltype(scopeGuard)>(), "");
    }
}

TYPED_TEST(CleanupTest, FactoryAndCTADProduceSameType)
{
    {
        auto callback = IdentityFactory::AsCallback([] {});
        auto factory_cleanup = utils::makeScopeGuard(callback);
        ScopeGuard deduction_cleanup = callback;
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::makeScopeGuard(FunctorClassFactory::AsCallback([] {}));
        ScopeGuard deduction_cleanup = FunctorClassFactory::AsCallback([] {});
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::makeScopeGuard(StdFunctionFactory::AsCallback([] {}));
        ScopeGuard deduction_cleanup = StdFunctionFactory::AsCallback([] {});
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::makeScopeGuard(&FnPtrFunction);
        ScopeGuard deduction_cleanup = &FnPtrFunction;
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
    {
        auto factory_cleanup = utils::makeScopeGuard(FnPtrFunction);
        ScopeGuard deduction_cleanup = FnPtrFunction;
        static_assert(IsSame<decltype(factory_cleanup), decltype(deduction_cleanup)>(), "");
    }
}
#endif  // CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION

TYPED_TEST(CleanupTest, BasicUsage)
{
    bool called = false;

    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback
    }

    EXPECT_TRUE(called);  // Destructor should invoke the callback
}

TYPED_TEST(CleanupTest, BasicUsageWithFunctionPointer)
{
    fn_ptr_called = false;

    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback(&FnPtrFunction));
        EXPECT_FALSE(fn_ptr_called);  // Constructor shouldn't invoke the callback
    }

    EXPECT_TRUE(fn_ptr_called);  // Destructor should invoke the callback
}

TYPED_TEST(CleanupTest, Cancel)
{
    bool called = false;

    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback

        std::move(scopeGuard).cancel();
        EXPECT_FALSE(called);  // Cancel shouldn't invoke the callback
    }

    EXPECT_FALSE(called);  // Destructor shouldn't invoke the callback
}

TYPED_TEST(CleanupTest, Invoke)
{
    bool called = false;

    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback

        std::move(scopeGuard).invoke();
        EXPECT_TRUE(called);  // Invoke should invoke the callback

        called = false;  // Reset tracker before destructor runs
    }

    EXPECT_FALSE(called);  // Destructor shouldn't invoke the callback
}

TYPED_TEST(CleanupTest, Move)
{
    bool called = false;

    {
        auto moved_from_cleanup = utils::makeScopeGuard(TypeParam::AsCallback([&called] { called = true; }));
        EXPECT_FALSE(called);  // Constructor shouldn't invoke the callback

        {
            auto moved_to_cleanup = std::move(moved_from_cleanup);
            EXPECT_FALSE(called);  // Move shouldn't invoke the callback
        }

        EXPECT_TRUE(called);  // Destructor should invoke the callback

        called = false;  // Reset tracker before destructor runs
    }

    EXPECT_FALSE(called);  // Destructor shouldn't invoke the callback
}

int DestructionCount = 0;

struct DestructionCounter
{
    void operator()() {}

    ~DestructionCounter() { ++DestructionCount; }
};

TYPED_TEST(CleanupTest, DestructorDestroys)
{
    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback(DestructionCounter()));
        DestructionCount = 0;
    }

    EXPECT_EQ(DestructionCount, 1);  // Engaged scopeGuard destroys
}

TYPED_TEST(CleanupTest, CancelDestroys)
{
    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback(DestructionCounter()));
        DestructionCount = 0;

        std::move(scopeGuard).cancel();
        EXPECT_EQ(DestructionCount, 1);  // Cancel destroys
    }

    EXPECT_EQ(DestructionCount, 1);  // Canceled scopeGuard does not double destroy
}

TYPED_TEST(CleanupTest, InvokeDestroys)
{
    {
        auto scopeGuard = utils::makeScopeGuard(TypeParam::AsCallback(DestructionCounter()));
        DestructionCount = 0;

        std::move(scopeGuard).invoke();
        EXPECT_EQ(DestructionCount, 1);  // Invoke destroys
    }

    EXPECT_EQ(DestructionCount, 1);  // Invoked scopeGuard does not double destroy
}
}  // namespace
