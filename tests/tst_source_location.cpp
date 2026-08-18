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
}  // namespace

TEST(LocationTest, CurrentYieldsCorrectValue)
{
    [[maybe_unused]] int previous_line = __LINE__;
    SourceLocation here = WhereAmI();
    const char *const fileName = "tst_source_location.cpp";
    const char *const functionName = "TestBody";
    EXPECT_THAT(here.filePath(), ::testing::EndsWith(fileName));
    EXPECT_EQ(here.fileName(), std::string(fileName));
    EXPECT_EQ(here.fileLine(), std::string(fileName) +":" + std::to_string(previous_line + 1));
    EXPECT_EQ(here.lineNumber(), previous_line + 1);
    EXPECT_EQ(here.toString(),
              std::string(functionName) +"@" + std::string(fileName) + ":" + std::to_string(previous_line + 1));
    EXPECT_STREQ(functionName, here.functionName());
}