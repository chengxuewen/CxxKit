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

#include <cxxkit/tools/assert.hpp>
#include <cxxkit/tools/logging.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>

CXXKIT_BEGIN_NAMESPACE


TEST(Assert, TrueConditionsAreNoops)
{
    int x = 1;
    CXXKIT_ASSERT(x == 1);
    CXXKIT_ASSERT_X(x == 1, "eq", "x is one");
    CXXKIT_STATIC_ASSERT(1 == 1);
    CXXKIT_STATIC_ASSERT_X(1 == 1, "one equals one");
    CXXKIT_HARDENING_ASSERT(x == 1);
    CXXKIT_HARDENING_ASSERT(2 + 2 == 4);
    // Expression side effect is evaluated for the regular assert.
    int calls = 0;
    CXXKIT_ASSERT((++calls) >= 0);
    EXPECT_EQ(calls, 1);
    EXPECT_TRUE(true);
}

TEST(Assert, StaticAssertsCompile)
{
    CXXKIT_STATIC_ASSERT(sizeof(char) == 1);
    CXXKIT_STATIC_ASSERT_X(sizeof(int) >= 2, "int is at least 2 bytes");
}

TEST(Assert, CxxKitAssertAborts)
{
    // assert logs at Fatal then aborts; we only verify the process terminates.
    EXPECT_DEATH(cxxkit_assert("boom-assert", __FILE__, __LINE__), "");
}

TEST(Assert, CxxKitAssertXAborts)
{
    EXPECT_DEATH(cxxkit_assert_x("where-clause", "boom-assert-x", __FILE__, __LINE__), "");
}

TEST(Assert, MacroFailureAborts)
{
    EXPECT_DEATH(CXXKIT_ASSERT(1 == 2), "");
    EXPECT_DEATH(CXXKIT_ASSERT_X(1 == 2, "w", "what"), "");
}

CXXKIT_END_NAMESPACE
