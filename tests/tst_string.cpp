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

#include <cxxkit/text/string.hpp>
#include <cxxkit/text/string_view.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <string>

CXXKIT_BEGIN_NAMESPACE

TEST(String, DefaultAndBuffer)
{
    String s;
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.length(), 0u);
    EXPECT_EQ(s.c_str()[0], '\0');
    EXPECT_FALSE(s.is_dynamic());
}

TEST(String, ShortStringIsInlined)
{
    String s("hello");
    EXPECT_EQ(s.size(), 5u);
    EXPECT_STREQ(s.c_str(), "hello");
    EXPECT_FALSE(s.is_dynamic()); // fits in the 48-byte inline buffer
}

TEST(String, LongStringIsDynamic)
{
    std::string longStr(100, 'x');
    String s(longStr);
    EXPECT_EQ(s.size(), longStr.size());
    EXPECT_FALSE(s.is_dynamic() == false); // longer than inline buffer -> dynamic
    EXPECT_EQ(std::string(s.c_str(), s.size()), longStr);
}

TEST(String, ConstructFromStdStringAndView)
{
    String a(std::string("alpha"));
    EXPECT_STREQ(a.c_str(), "alpha");
    EXPECT_EQ(a.size(), 5u);

    StringView view("bravo");
    String b(view);
    EXPECT_STREQ(b.c_str(), "bravo");
    EXPECT_EQ(b.size(), 5u);

    // Explicit len shorter than strlen.
    String c("foobar", 3);
    EXPECT_STREQ(c.c_str(), "foo");
    EXPECT_EQ(c.size(), 3u);
}

TEST(String, CopyAndMove)
{
    String a("copy me");
    String b(a);
    EXPECT_STREQ(b.c_str(), "copy me");
    EXPECT_EQ(b.size(), a.size());

    String c(std::move(a));
    EXPECT_STREQ(c.c_str(), "copy me");
    // Moved-from is valid but unspecified; just ensure no crash and dtor is fine.
}

TEST(String, Assignment)
{
    String s;
    s = std::string("first");
    EXPECT_STREQ(s.c_str(), "first");

    String other("second");
    s = other;
    EXPECT_STREQ(s.c_str(), "second");

    String moved("third");
    s = std::move(moved);
    EXPECT_STREQ(s.c_str(), "third");
}

TEST(String, StdStringRoundTrip)
{
    String s("xyz");
    EXPECT_EQ(s.std_string(), std::string("xyz"));
    EXPECT_EQ(String::to_std_string(s), std::string("xyz"));
    EXPECT_STREQ(s.c_string(), "xyz");
}

TEST(String, ComparisonOperators)
{
    String a("abc");
    String b("abd");
    String c("abc");
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(a < c);
}

TEST(String, StrDupAndStrNdup)
{
    char *dup = String::strdup("hello");
    ASSERT_NE(dup, nullptr);
    EXPECT_STREQ(dup, "hello");
    std::free(dup);

    char *ndup = String::strndup("hello", 3);
    ASSERT_NE(ndup, nullptr);
    EXPECT_STREQ(ndup, "hel");
    std::free(ndup);

    // Shorter source than n is padded with nuls and nul-terminated.
    char *padded = String::strndup("hi", 5);
    ASSERT_NE(padded, nullptr);
    EXPECT_EQ(padded[0], 'h');
    EXPECT_EQ(padded[1], 'i');
    EXPECT_EQ(padded[2], '\0');
    EXPECT_EQ(padded[4], '\0');
    std::free(padded);

    // NULL input returns NULL.
    EXPECT_EQ(String::strdup(nullptr), nullptr);
    EXPECT_EQ(String::strndup(nullptr, 4), nullptr);
}

TEST(String, StrncpyS)
{
    char dst[8] = {0};
    // count < capacity -> copied and nul-terminated.
    EXPECT_EQ(String::strncpy_s(dst, sizeof(dst), "abcd", 4), 0);
    EXPECT_STREQ(dst, "abcd");

    // count < nElements -> truncated copy succeeds and nul-terminates.
    char dst2[4] = {0}; // fits 3 chars + nul
    EXPECT_EQ(String::strncpy_s(dst2, sizeof(dst2), "abcdef", 3), 0);
    EXPECT_STREQ(dst2, "abc");

    // count == 0 -> no-op success.
    EXPECT_EQ(String::strncpy_s(dst, sizeof(dst), "aaaa", 0), 0);

    // Invalid arguments -> -1.
    EXPECT_EQ(String::strncpy_s(nullptr, 4, "abc", 3), -1);
    EXPECT_EQ(String::strncpy_s(dst, 4, nullptr, 3), -1);
    EXPECT_EQ(String::strncpy_s(dst, 0, "abc", 3), -1);

    // count < nElements (partial copy) leaves a trailing nul.
    char dst3[6] = {0};
    EXPECT_EQ(String::strncpy_s(dst3, 6, "ABCDEFGH", 5), 0);
    EXPECT_STREQ(dst3, "ABCDE");
}

CXXKIT_END_NAMESPACE
