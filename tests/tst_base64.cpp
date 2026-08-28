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
    Base64::encode_from_array(input, sizeof(input) - 1, &encoded);
    EXPECT_EQ(encoded, "aGVsbG8gd29ybGQ=");

    std::string decoded = Base64::decode(StringView(encoded), Base64::DO_STRICT);
    EXPECT_EQ(decoded, std::string(input, sizeof(input) - 1));
}

TEST(Base64, EncodeEmpty)
{
    std::string encoded;
    Base64::encode_from_array(nullptr, 0, &encoded);
    EXPECT_TRUE(encoded.empty());
}

TEST(Base64, is_base64_char)
{
    EXPECT_TRUE(Base64::is_base64_char('a'));
    EXPECT_TRUE(Base64::is_base64_char('Z'));
    EXPECT_TRUE(Base64::is_base64_char('0'));
    EXPECT_TRUE(Base64::is_base64_char('+'));
    EXPECT_TRUE(Base64::is_base64_char('/'));
    EXPECT_FALSE(Base64::is_base64_char('!'));
    EXPECT_FALSE(Base64::is_base64_char(' '));
}

TEST(Base64, get_next_base64_char)
{
    char next = 0;
    EXPECT_TRUE(Base64::get_next_base64_char('a', &next));
    EXPECT_EQ(next, 'b');
    EXPECT_FALSE(Base64::get_next_base64_char('!', &next));
}

TEST(Base64, is_base64_encoded)
{
    EXPECT_TRUE(Base64::is_base64_encoded(StringView("aGVsbG8"))); // no padding allowed
    EXPECT_FALSE(Base64::is_base64_encoded(StringView("not base64!")));
}