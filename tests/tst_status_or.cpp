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

#include <cxxkit/tools/status_or.hpp>

#include <gtest/gtest.h>

#include <string>

TEST(StatusOr, OkValue)
{
    cxxkit::StatusOr<int> sor = 42;
    EXPECT_TRUE(sor.ok());
    EXPECT_EQ(sor.value(), 42);
}

TEST(StatusOr, ErrorStatus)
{
    cxxkit::StatusOr<int> sor = cxxkit::Status("not found");
    EXPECT_FALSE(sor.ok());
    EXPECT_EQ(sor.status().error_message(), "not found");
}

TEST(StatusOr, ValueOr)
{
    cxxkit::StatusOr<int> ok = 42;
    cxxkit::StatusOr<int> err = cxxkit::Status("error");
    EXPECT_EQ(ok.value_or(0), 42);
    EXPECT_EQ(err.value_or(0), 0);
}

TEST(StatusOr, or_die)
{
    cxxkit::StatusOr<int> ok = 42;
    EXPECT_EQ(ok.or_die(), 42);
    // err.or_die() would trigger CXXKIT_CHECK failure (fatal)
}

TEST(StatusOr, make_status_or)
{
    auto sor = cxxkit::make_status_or(42);
    EXPECT_TRUE(sor.ok());
    EXPECT_EQ(sor.value(), 42);
}

TEST(StatusOr, CopyConstruct)
{
    cxxkit::StatusOr<int> a = 42;
    cxxkit::StatusOr<int> b = a;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), 42);
}

TEST(StatusOr, MoveConstruct)
{
    cxxkit::StatusOr<std::string> a = std::string("hello");
    cxxkit::StatusOr<std::string> b = std::move(a);
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), "hello");
}

TEST(StatusOr, CopyAssign)
{
    cxxkit::StatusOr<int> a = 42;
    cxxkit::StatusOr<int> b = cxxkit::Status("error");
    b = a;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), 42);
}

TEST(StatusOr, MoveAssign)
{
    cxxkit::StatusOr<std::string> a = std::string("world");
    cxxkit::StatusOr<std::string> b = cxxkit::Status("error");
    b = std::move(a);
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), "world");
}

TEST(StatusOr, CopyConstructError)
{
    cxxkit::StatusOr<int> a = cxxkit::Status("original error");
    cxxkit::StatusOr<int> b = a;
    EXPECT_FALSE(b.ok());
    EXPECT_EQ(b.status().error_message(), "original error");
}
