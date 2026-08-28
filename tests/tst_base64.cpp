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

// cxxkit::text Base64 tests — coverage for base64.cpp.
#include <cxxkit/text/base64.hpp>

#include <gtest/gtest.h>

using namespace cxxkit;

TEST(Base64, EncodeDecodeRoundTrip)
{
    const char input[] = "hello world";
    std::string encoded;
    Base64::EncodeFromArray(input, sizeof(input) - 1, &encoded);
    EXPECT_EQ(encoded, "aGVsbG8gd29ybGQ=");

    std::string decoded = Base64::Decode(StringView(encoded), Base64::DO_STRICT);
    EXPECT_EQ(decoded, std::string(input, sizeof(input) - 1));
}

TEST(Base64, EncodeEmpty)
{
    std::string encoded;
    Base64::EncodeFromArray(nullptr, 0, &encoded);
    EXPECT_TRUE(encoded.empty());
}

TEST(Base64, IsBase64Char)
{
    EXPECT_TRUE(Base64::IsBase64Char('a'));
    EXPECT_TRUE(Base64::IsBase64Char('Z'));
    EXPECT_TRUE(Base64::IsBase64Char('0'));
    EXPECT_TRUE(Base64::IsBase64Char('+'));
    EXPECT_TRUE(Base64::IsBase64Char('/'));
    EXPECT_FALSE(Base64::IsBase64Char('!'));
    EXPECT_FALSE(Base64::IsBase64Char(' '));
}

TEST(Base64, GetNextBase64Char)
{
    char next = 0;
    EXPECT_TRUE(Base64::GetNextBase64Char('a', &next));
    EXPECT_EQ(next, 'b');
    EXPECT_FALSE(Base64::GetNextBase64Char('!', &next));
}

TEST(Base64, IsBase64Encoded)
{
    EXPECT_TRUE(Base64::IsBase64Encoded(StringView("aGVsbG8"))); // no padding allowed
    EXPECT_FALSE(Base64::IsBase64Encoded(StringView("not base64!")));
}