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

#include <cxxkit/tools/enum_flags.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
enum class enum_testFlag
{
    value_0 = 0,
    value_1 = 1,
    value_2 = 2,
    value_3 = 4,
    value_4 = 8,

    value_14 = value_1 | value_4
};
CXXKIT_DECLARE_ENUM_FLAGS(enum_testFlags, enum_testFlag)
CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(enum_testFlags)

template <uint32_t N, typename T>
bool verifyConstExpr(T n)
{
    return n == N;
}
CXXKIT_CONSTEXPR enum_testFlags testRelaxedConstExpr()
{
    enum_testFlags value;
    value = enum_testFlag::value_1 | enum_testFlag::value_2;
    value |= enum_testFlag::value_3;
    value &= ~(int)enum_testFlag::value_1;
    value ^= enum_testFlag::value_2;
    return value;
}

enum class MockMouseButton
{
    NoButton = 0,
    LeftButton = 1 << 0,
    RightButton = 1 << 1,
    MiddleButton = 1 << 2
};
CXXKIT_DECLARE_ENUM_FLAGS(MockMouseButtons, MockMouseButton)
CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(MockMouseButtons)

enum class MockWindowFlag
{
    Window = 1,
    Dialog = 1 << 1 | Window
};
CXXKIT_DECLARE_ENUM_FLAGS(MockWindowFlags, MockWindowFlag)
CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(MockWindowFlags)

enum class MockAlignmentFlag
{
    AlignLeft = 0x0001,
    AlignTop = 0x0020
};
CXXKIT_DECLARE_ENUM_FLAGS(MockAlignment, MockAlignmentFlag)
CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(MockAlignment)

enum class MyStrictEnum
{
    StrictZero,
    StrictOne,
    StrictTwo,
    StrictFour = 4
};
CXXKIT_DECLARE_ENUM_FLAGS(MyStrictFlags, MyStrictEnum)
CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(MyStrictFlags)

static_assert(!TypeInfo<MyStrictFlags>::isStatic, "");
static_assert(!TypeInfo<MyStrictFlags>::isComplex, "");
static_assert(!TypeInfo<MyStrictFlags>::isPointer, "");

enum class MyStrictNoOpEnum
{
    StrictZero,
    StrictOne,
    StrictTwo,
    StrictFour = 4
};
CXXKIT_DECLARE_ENUM_FLAGS(MyStrictNoOpFlags, MyStrictNoOpEnum)
} // namespace


TEST(EnumFlagsTest, DefaultConstructor)
{
    // Test with enum_testFlags
    enum_testFlags f1;
    enum_testFlags f2;
    EXPECT_EQ(f1, f2);
    EXPECT_TRUE(!f1);
}

TEST(EnumFlagsTest, EnumConstructor)
{
    // Test with enum_testFlags
    enum_testFlags f1(enum_testFlag::value_1);
    enum_testFlags f2(enum_testFlag::value_1);
    EXPECT_EQ(f1, f2);
}

TEST(EnumFlagsTest, CopyConstructor)
{
    // Test with enum_testFlags
    enum_testFlags f1(enum_testFlag::value_1);
    enum_testFlags f2(f1);
    EXPECT_EQ(f1, f2);
}

TEST(EnumFlagsTest, TestFlagOperatorAnd)
{
    {
        // return reference
        enum_testFlags f1 = enum_testFlag::value_1 | enum_testFlag::value_2;
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_1));
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_2));
        EXPECT_FALSE(f1.test_flag(enum_testFlag::value_3));

        f1 &= uint32_t(2);
        EXPECT_EQ(f1, 2);

        f1 &= enum_testFlag::value_3;
        EXPECT_EQ(f1, 0);
    }
    {
        // return value
        enum_testFlags f1(enum_testFlag::value_1 | enum_testFlag::value_2);
        enum_testFlags f2 = f1 & enum_testFlag::value_2;

        EXPECT_FALSE(f2.test_flag(enum_testFlag::value_1));
        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_2));

        f2 = f1 & uint32_t(2);
        EXPECT_EQ(f2, 2);

        f2 = f1 & enum_testFlag::value_3;
        EXPECT_EQ(f2, 0);
    }
}

TEST(EnumFlagsTest, TestFlagOperatorOr)
{
    {
        // return reference
        enum_testFlags f1(enum_testFlag::value_1 | enum_testFlag::value_2);

        f1 |= enum_testFlag::value_3;

        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_1));
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_2));
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_3));

        enum_testFlags f2(enum_testFlag::value_4);
        f1 |= f2;
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_4));
    }
    {
        // return value
        enum_testFlags f1(enum_testFlag::value_1 | enum_testFlag::value_2);

        enum_testFlags f2 = f1 | enum_testFlag::value_3;

        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_1));
        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_2));
        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_3));

        enum_testFlags f3(enum_testFlag::value_4);
        f2 = f1 | f3;

        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_4));
    }
}


TEST(EnumFlagsTest, TestFlagOperatorXor)
{
    {
        // return reference
        enum_testFlags f1(enum_testFlag::value_1 | enum_testFlag::value_2);

        f1 ^= enum_testFlag::value_3;

        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_1));
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_2));
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_3));

        f1 ^= enum_testFlag::value_2;

        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_1));
        EXPECT_FALSE(f1.test_flag(enum_testFlag::value_2));
        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_3));

        enum_testFlags f3(enum_testFlag::value_3);
        f1 ^= f3;

        EXPECT_TRUE(f1.test_flag(enum_testFlag::value_1));
        EXPECT_FALSE(f1.test_flag(enum_testFlag::value_2));
        EXPECT_FALSE(f1.test_flag(enum_testFlag::value_3));
    }
    {
        // return value
        enum_testFlags f1(enum_testFlag::value_1 | enum_testFlag::value_2);

        enum_testFlags f2 = f1 ^ enum_testFlag::value_3;

        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_1));
        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_2));
        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_3));

        f2 = f2 ^ enum_testFlag::value_2;

        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_1));
        EXPECT_FALSE(f2.test_flag(enum_testFlag::value_2));
        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_3));

        enum_testFlags f3(enum_testFlag::value_3);
        f2 = f2 ^ f3;

        EXPECT_TRUE(f2.test_flag(enum_testFlag::value_1));
        EXPECT_FALSE(f2.test_flag(enum_testFlag::value_2));
        EXPECT_FALSE(f2.test_flag(enum_testFlag::value_3));
    }
}

TEST(EnumFlagsTest, TestFlagOperatorNot)
{
    enum_testFlags f1 = enum_testFlag::value_1 | enum_testFlag::value_2;

    enum_testFlags f2 = ~f1;

    EXPECT_FALSE(f2.test_flag(enum_testFlag::value_1));
    EXPECT_FALSE(f2.test_flag(enum_testFlag::value_2));
    EXPECT_TRUE(f2.test_flag(enum_testFlag::value_3));
    EXPECT_TRUE(f2.test_flag(enum_testFlag::value_4));
}

TEST(EnumFlagsTest, TestFlagZeroFlag)
{
    {
        enum_testFlags f = enum_testFlag::value_1 | enum_testFlag::value_2;
        /* enum_testFlag::value_0 has the value 0. */

        EXPECT_FALSE(f.test_flag(enum_testFlag::value_0));
    }

    {
        /* A zero enum set should test true with zero. */
        EXPECT_TRUE(enum_testFlags().test_flag(enum_testFlag::value_0));
    }

    {
        enum_testFlags f = enum_testFlag::value_0;
        EXPECT_TRUE(f.test_flag(enum_testFlag::value_0));
    }
}

TEST(EnumFlagsTest, TestFlagMultiBits)
{
    {
        const enum_testFlags f(enum_testFlag::value_1);
        EXPECT_FALSE(f.test_flag(enum_testFlag::value_4));
    }

    {
        const enum_testFlags f(enum_testFlag::value_14);
        EXPECT_TRUE(f.test_flag(enum_testFlag::value_4));
    }
}

TEST(EnumFlagsTest, TestFlag)
{
    enum_testFlags f = enum_testFlag::value_1 | enum_testFlag::value_2;


    EXPECT_TRUE(f.test_flag(enum_testFlag::value_1));
    EXPECT_TRUE(f.test_flag(enum_testFlag::value_2));
    EXPECT_FALSE(f.test_flag(enum_testFlag::value_3));
    EXPECT_FALSE(f.test_flag(enum_testFlag::value_4));

    f = 0;
    // negative
    EXPECT_FALSE(f.test_flag(enum_testFlag::value_1));
    EXPECT_FALSE(f.test_flag(enum_testFlag::value_2));
}

TEST(EnumFlagsTest, ConstExpr)
{
    MockMouseButtons btn = MockMouseButton::LeftButton | MockMouseButton::RightButton;
    switch (btn)
    {
        case (MockMouseButtons::Value)MockMouseButton::LeftButton: EXPECT_TRUE(false); break;
        case (MockMouseButtons::Value)MockMouseButton::RightButton: EXPECT_TRUE(false); break;
        case (MockMouseButtons::Value)(MockMouseButton::LeftButton | MockMouseButton::RightButton):
            EXPECT_TRUE(true);
            break;
        default: EXPECT_TRUE(false);
    }

    EXPECT_TRUE(
        verifyConstExpr<1>((MockMouseButton::LeftButton | MockMouseButton::RightButton) & MockMouseButton::LeftButton));
    EXPECT_TRUE(verifyConstExpr<0>((MockMouseButton::LeftButton | MockMouseButton::RightButton) &
                                   MockMouseButton::MiddleButton));
    EXPECT_TRUE(verifyConstExpr<7>((MockMouseButton::LeftButton | MockMouseButton::RightButton) |
                                   MockMouseButton::MiddleButton));
    EXPECT_TRUE(verifyConstExpr<(uint32_t)~3>(~(MockMouseButton::LeftButton | MockMouseButton::RightButton)));
    EXPECT_TRUE(verifyConstExpr<3>(MockMouseButtons(MockMouseButton::LeftButton) ^ MockMouseButton::RightButton));
    EXPECT_TRUE(verifyConstExpr<0>(MockMouseButtons(0)));
    EXPECT_TRUE(verifyConstExpr<2>(MockMouseButtons(MockMouseButton::RightButton) & 0xff));
    EXPECT_TRUE(verifyConstExpr<0xff>(MockMouseButtons(MockMouseButton::RightButton) | 0xff));

    EXPECT_FALSE(verifyConstExpr<2>(~MockMouseButtons(MockMouseButton::LeftButton)));

#if defined(__cpp_constexpr) && __cpp_constexpr - 0 >= 201304
    EXPECT_TRUE(verifyConstExpr<4>(testRelaxedConstExpr()));
#endif
}

TEST(EnumFlagsTest, Signedness)
{
    // Check that the relative signedness of the types matches
    static_assert((std::is_unsigned<typename std::underlying_type<MockMouseButton>::type>::value ==
                   std::is_unsigned<MockMouseButtons::Value>::value),
                  "Signedness mismatch for MockMouseButtons");

    static_assert((std::is_unsigned<typename std::underlying_type<MockAlignmentFlag>::type>::value ==
                   std::is_unsigned<MockAlignment::Value>::value),
                  "Signedness mismatch for MockAlignment");
}

TEST(EnumFlagsTest, InitializerLists)
{
    // Use explicit OR instead of initializer list
    MockMouseButtons bts = {MockMouseButton::LeftButton, MockMouseButton::RightButton};
    EXPECT_TRUE(bts.test_flag(MockMouseButton::LeftButton));
    EXPECT_TRUE(bts.test_flag(MockMouseButton::RightButton));
    EXPECT_FALSE(bts.test_flag(MockMouseButton::MiddleButton));

    MyStrictNoOpFlags flags = {MyStrictNoOpEnum::StrictOne, MyStrictNoOpEnum::StrictFour};
    EXPECT_TRUE(flags.test_flag(MyStrictNoOpEnum::StrictOne));
    EXPECT_TRUE(flags.test_flag(MyStrictNoOpEnum::StrictFour));
    EXPECT_FALSE(flags.test_flag(MyStrictNoOpEnum::StrictTwo));
}

TEST(EnumFlagsTest, ClassEnum)
{
    // The main aim of the test is making sure it compiles
    // The EXPECT_EQ are there as an extra
    MyStrictEnum e1 = MyStrictEnum::StrictOne;
    MyStrictEnum e2 = MyStrictEnum::StrictTwo;

    MyStrictFlags f1(MyStrictEnum::StrictOne);
    EXPECT_EQ(f1, 1);

    MyStrictFlags f2(e2);
    EXPECT_EQ(f2, 2);

    MyStrictFlags f0;
    EXPECT_EQ(f0, 0);

    MyStrictFlags f3(e2 | e1);
    EXPECT_EQ(f3, 3);

    EXPECT_TRUE(f3.test_flag(MyStrictEnum::StrictOne));
    EXPECT_TRUE(!f1.test_flag(MyStrictEnum::StrictTwo));

    EXPECT_TRUE(!f0);

    EXPECT_EQ(f3 & int(1), 1);
    EXPECT_EQ(f3 & uint(1), 1);
    EXPECT_EQ(f3 & MyStrictEnum::StrictOne, 1);

    MyStrictFlags aux;
    aux = f3;
    aux &= int(1);
    EXPECT_EQ(aux, 1);

    aux = f3;
    aux &= uint(1);
    EXPECT_EQ(aux, 1);

    aux = f3;
    aux &= MyStrictEnum::StrictOne;
    EXPECT_EQ(aux, 1);

    aux = f3;
    aux &= f1;
    EXPECT_EQ(aux, 1);

    aux = f3 ^ f3;
    EXPECT_EQ(aux, 0);

    aux = f3 ^ f1;
    EXPECT_EQ(aux, 2);

    aux = f3 ^ f0;
    EXPECT_EQ(aux, 3);

    aux = f3 ^ MyStrictEnum::StrictOne;
    EXPECT_EQ(aux, 2);

    aux = f3 ^ MyStrictEnum::StrictZero;
    EXPECT_EQ(aux, 3);

    aux = f3;
    aux ^= f3;
    EXPECT_EQ(aux, 0);

    aux = f3;
    aux ^= f1;
    EXPECT_EQ(aux, 2);

    aux = f3;
    aux ^= f0;
    EXPECT_EQ(aux, 3);

    aux = f3;
    aux ^= MyStrictEnum::StrictOne;
    EXPECT_EQ(aux, 2);

    aux = f3;
    aux ^= MyStrictEnum::StrictZero;
    EXPECT_EQ(aux, 3);

    aux = f1 | f2;
    EXPECT_EQ(aux, 3);

    aux = MyStrictEnum::StrictOne | MyStrictEnum::StrictTwo;
    EXPECT_EQ(aux, 3);

    aux = f1;
    aux |= f2;
    EXPECT_EQ(aux, 3);

    aux = MyStrictEnum::StrictOne;
    aux |= MyStrictEnum::StrictTwo;
    EXPECT_EQ(aux, 3);

    aux = ~f1;
    EXPECT_EQ(aux, -2);

    // Just to make sure it compiles
    EXPECT_TRUE(true) << f3;
}

CXXKIT_END_NAMESPACE
