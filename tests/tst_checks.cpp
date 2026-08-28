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

#include <cxxkit/tools/checks.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using ::testing::HasSubstr;
using ::testing::Not;

TEST(ChecksTest, ExpressionNotEvaluatedWhenCheckPassing)
{
    int i = 0;
    CXXKIT_CHECK(true) << "i=" << ++i;
    CXXKIT_CHECK_EQ(i, 0) << "Previous check passed, but i was incremented!";
}

#if GTEST_HAS_DEATH_TEST && !defined(CXXKIT_ANDROID)

TEST(ChecksDeathTest, Checks)
{
    return; //TODO
    EXPECT_DEATH(CXXKIT_FATAL() << "message",
                 "\n\n#\n"
                 "# Fatal error in: \\S+, line \\w+\n"
                 "# last system error: \\w+\n"
                 "# Check failed: FATAL\\(\\)\n"
                 "# message");

    int a = 1, b = 2;
    EXPECT_DEATH(CXXKIT_CHECK_EQ(a, b) << 1 << 2u,
                 "\n\n#\n"
                 "# Fatal error in: \\S+, line \\w+\n"
                 "# last system error: \\w+\n"
                 "# Check failed: a == b \\(1 vs. 2\\)\n"
                 "# 12");
    CXXKIT_CHECK_EQ(5, 5);

    CXXKIT_CHECK(true) << "Shouldn't crash" << 1;
    EXPECT_DEATH(CXXKIT_CHECK(false) << "Hi there!",
                 "\n\n#\n"
                 "# Fatal error in: \\S+, line \\w+\n"
                 "# last system error: \\w+\n"
                 "# Check failed: false\n"
                 "# Hi there!");

    //    StructWithStringfy t;
    //    EXPECT_DEATH(CXXKIT_CHECK(false) << t, HasSubstr("absl-stringify"));
}
#endif // GTEST_HAS_DEATH_TEST && !defined(CXXKIT_ANDROID)

} // namespace
