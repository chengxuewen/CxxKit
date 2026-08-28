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
// Created by cxw on 25-8-8.
//

#include <cxxkit/tools/source_location.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace cxxkit;

namespace
{

// This is a typical use: taking SourceLocation::Current as a default parameter.
// So even though this looks contrived, it confirms that such usage works as
// expected.
SourceLocation WhereAmI(const SourceLocation &location = SourceLocation::current())
{
    return location;
}
} // namespace

TEST(LocationTest, CurrentYieldsCorrectValue)
{
    [[maybe_unused]] int previous_line = __LINE__;
    SourceLocation here = WhereAmI();
    const char *const fileName = "tst_source_location.cpp";
    const char *const functionName = "TestBody";
    EXPECT_THAT(here.filePath(), ::testing::EndsWith(fileName));
    EXPECT_EQ(here.fileName(), std::string(fileName));
    EXPECT_EQ(here.fileLine(), std::string(fileName) + ":" + std::to_string(previous_line + 1));
    EXPECT_EQ(here.lineNumber(), previous_line + 1);
    EXPECT_EQ(here.toString(),
              std::string(functionName) + "@" + std::string(fileName) + ":" + std::to_string(previous_line + 1));
    EXPECT_STREQ(functionName, here.functionName());
}