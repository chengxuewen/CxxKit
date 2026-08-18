/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cxxkit/numerics/divide_round.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <cstdint>
#include <limits>

using namespace cxxkit;

#ifdef CXXKIT_OS_WIN
#    undef min
#    undef max
#    undef small
#endif

namespace
{

TEST(DivideRoundUpTest, CanBeUsedAsConstexpr)
{
    CXXKIT_CXX14_CONSTEXPR_ASSERT_X(DivideRoundUp(5, 1) == 5, "");
    CXXKIT_CXX14_CONSTEXPR_ASSERT_X(DivideRoundUp(5, 2) == 3, "");
}

TEST(DivideRoundUpTest, ReturnsZeroForZeroDividend)
{
    EXPECT_EQ(DivideRoundUp(uint8_t{0}, 1), 0);
    EXPECT_EQ(DivideRoundUp(uint8_t{0}, 3), 0);
    EXPECT_EQ(DivideRoundUp(int{0}, 1), 0);
    EXPECT_EQ(DivideRoundUp(int{0}, 3), 0);
}

TEST(DivideRoundUpTest, WorksForMaxDividend)
{
    EXPECT_EQ(DivideRoundUp(uint8_t{255}, 2), 128);
    EXPECT_EQ(DivideRoundUp(std::numeric_limits<int>::max(), 2),
              std::numeric_limits<int>::max() / 2 + (std::numeric_limits<int>::max() % 2));
}

TEST(DivideRoundToNearestTest, CanBeUsedAsConstexpr)
{
    static CXXKIT_CXX14_CONSTEXPR int kOne = DivideRoundToNearest(5, 4);
    static CXXKIT_CXX14_CONSTEXPR int kTwo = DivideRoundToNearest(7, 4);
    CXXKIT_CXX14_CONSTEXPR_ASSERT(kOne == 1);
    CXXKIT_CXX14_CONSTEXPR_ASSERT(kTwo == 2);
    CXXKIT_CXX14_CONSTEXPR_ASSERT(DivideRoundToNearest(-5, 4) == -1);
    CXXKIT_CXX14_CONSTEXPR_ASSERT(DivideRoundToNearest(-7, 4) == -2);
}

TEST(DivideRoundToNearestTest, DivideByOddNumber)
{
    EXPECT_EQ(DivideRoundToNearest(-5, 3), -2);
    EXPECT_EQ(DivideRoundToNearest(-4, 3), -1);
    EXPECT_EQ(DivideRoundToNearest(-3, 3), -1);
    EXPECT_EQ(DivideRoundToNearest(-2, 3), -1);
    EXPECT_EQ(DivideRoundToNearest(-1, 3), 0);
    EXPECT_EQ(DivideRoundToNearest(0, 3), 0);
    EXPECT_EQ(DivideRoundToNearest(1, 3), 0);
    EXPECT_EQ(DivideRoundToNearest(2, 3), 1);
    EXPECT_EQ(DivideRoundToNearest(3, 3), 1);
    EXPECT_EQ(DivideRoundToNearest(4, 3), 1);
    EXPECT_EQ(DivideRoundToNearest(5, 3), 2);
    EXPECT_EQ(DivideRoundToNearest(6, 3), 2);
}

TEST(DivideRoundToNearestTest, DivideByEvenNumberTieRoundsUp)
{
    EXPECT_EQ(DivideRoundToNearest(-7, 4), -2);
    EXPECT_EQ(DivideRoundToNearest(-6, 4), -1);
    EXPECT_EQ(DivideRoundToNearest(-5, 4), -1);
    EXPECT_EQ(DivideRoundToNearest(-4, 4), -1);
    EXPECT_EQ(DivideRoundToNearest(-3, 4), -1);
    EXPECT_EQ(DivideRoundToNearest(-2, 4), 0);
    EXPECT_EQ(DivideRoundToNearest(-1, 4), 0);
    EXPECT_EQ(DivideRoundToNearest(0, 4), 0);
    EXPECT_EQ(DivideRoundToNearest(1, 4), 0);
    EXPECT_EQ(DivideRoundToNearest(2, 4), 1);
    EXPECT_EQ(DivideRoundToNearest(3, 4), 1);
    EXPECT_EQ(DivideRoundToNearest(4, 4), 1);
    EXPECT_EQ(DivideRoundToNearest(5, 4), 1);
    EXPECT_EQ(DivideRoundToNearest(6, 4), 2);
    EXPECT_EQ(DivideRoundToNearest(7, 4), 2);
}

TEST(DivideRoundToNearestTest, LargeDivisor)
{
    EXPECT_EQ(DivideRoundToNearest(std::numeric_limits<int>::max() - 1, std::numeric_limits<int>::max()), 1);
    EXPECT_EQ(DivideRoundToNearest(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()), -1);
}

TEST(DivideRoundToNearestTest, DivideSmallTypeByLargeType)
{
    uint8_t small = 0xff;
    uint16_t large = 0xffff;
    EXPECT_EQ(DivideRoundToNearest(small, large), 0);
}

using IntegerTypes = ::testing::Types<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

template <typename T>
class DivideRoundTypedTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(DivideRoundTypedTest, IntegerTypes);

TYPED_TEST(DivideRoundTypedTest, RoundToNearestPreservesType)
{
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, int8_t{3})),
                                                         decltype(TypeParam{100} / int8_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, int16_t{3})),
                                                         decltype(TypeParam{100} / int16_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, int32_t{3})),
                                                         decltype(TypeParam{100} / int32_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, int64_t{3})),
                                                         decltype(TypeParam{100} / int64_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, uint8_t{3})),
                                                         decltype(TypeParam{100} / uint8_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, uint16_t{3})),
                                                         decltype(TypeParam{100} / uint16_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        // using Type = uint32_t;
        // const auto val0 = TypeParam{100};
        // const auto val1 = Type{3};
        // const auto val2 = TypeParam{100} / Type{3};
        // const auto val3 = DivideRoundToNearest(TypeParam{100}, Type{3});
        // const std::string val0Name = typeid(decltype(val0)).name();
        // const std::string val1Name = typeid(decltype(val1)).name();
        // const std::string val2Name = typeid(decltype(val2)).name();
        // const std::string val3Name = typeid(decltype(val3)).name();
        // const auto val0IsSigned = std::is_signed<decltype(val0)>::value;
        // const auto val1IsSigned = std::is_signed<decltype(val1)>::value;
        // const auto val2IsSigned = std::is_signed<decltype(val2)>::value;
        // const auto val3IsSigned = std::is_signed<decltype(val3)>::value;
        // const auto val0LessThanInt = sizeof(decltype(val0)) < sizeof(int);
        // const auto val1LessThanInt = sizeof(decltype(val1)) < sizeof(int);
        // const auto val2LessThanInt = sizeof(decltype(val2)) < sizeof(int);
        // const auto val3LessThanInt = sizeof(decltype(val3)) < sizeof(int);
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, uint32_t{3})),
                                                         decltype(TypeParam{100} / uint32_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundToNearest(TypeParam{100}, uint64_t{3})),
                                                         decltype(TypeParam{100} / uint64_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
}

TYPED_TEST(DivideRoundTypedTest, RoundUpPreservesType)
{
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, int8_t{3})),
                                                         decltype(TypeParam{100} / int8_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, int16_t{3})),
                                                         decltype(TypeParam{100} / int16_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, int32_t{3})),
                                                         decltype(TypeParam{100} / int32_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, int64_t{3})),
                                                         decltype(TypeParam{100} / int64_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, uint8_t{3})),
                                                         decltype(TypeParam{100} / uint8_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, uint16_t{3})),
                                                         decltype(TypeParam{100} / uint16_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, uint32_t{3})),
                                                         decltype(TypeParam{100} / uint32_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
    {
        CXXKIT_CXX14_CONSTEXPR auto value = std::is_same<decltype(DivideRoundUp(TypeParam{100}, uint64_t{3})),
                                                         decltype(TypeParam{100} / uint64_t{3})>::value;
        CXXKIT_CXX14_CONSTEXPR_ASSERT_X(value, "");
    }
}
} // namespace
