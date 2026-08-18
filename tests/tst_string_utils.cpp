/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright 2020 The WebRTC Project Authors. All rights reserved.
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

#include <cxxkit/text/string_utils.hpp>
#include <cxxkit/text/string_view.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

CXXKIT_BEGIN_NAMESPACE

TEST(MatchTest, StartsWith)
{
    const std::string s1("123\0abc", 7);
    const cxxkit::StringView a("foobar");
    const cxxkit::StringView b(s1);
    const cxxkit::StringView e;
    EXPECT_TRUE(utils::stringStartsWith(a, a));
    EXPECT_TRUE(utils::stringStartsWith(a, "foo"));
    EXPECT_TRUE(utils::stringStartsWith(a, e));
    EXPECT_TRUE(utils::stringStartsWith(b, s1));
    EXPECT_TRUE(utils::stringStartsWith(b, b));
    EXPECT_TRUE(utils::stringStartsWith(b, e));
    EXPECT_TRUE(utils::stringStartsWith(e, ""));
    EXPECT_FALSE(utils::stringStartsWith(a, b));
    EXPECT_FALSE(utils::stringStartsWith(b, a));
    EXPECT_FALSE(utils::stringStartsWith(e, a));
}

TEST(MatchTest, EndsWith)
{
    const std::string s1("123\0abc", 7);
    const cxxkit::StringView a("foobar");
    const cxxkit::StringView b(s1);
    const cxxkit::StringView e;
    EXPECT_TRUE(utils::stringEndsWith(a, a));
    EXPECT_TRUE(utils::stringEndsWith(a, "bar"));
    EXPECT_TRUE(utils::stringEndsWith(a, e));
    EXPECT_TRUE(utils::stringEndsWith(b, s1));
    EXPECT_TRUE(utils::stringEndsWith(b, b));
    EXPECT_TRUE(utils::stringEndsWith(b, e));
    EXPECT_TRUE(utils::stringEndsWith(e, ""));
    EXPECT_FALSE(utils::stringEndsWith(a, b));
    EXPECT_FALSE(utils::stringEndsWith(b, a));
    EXPECT_FALSE(utils::stringEndsWith(e, a));
}

TEST(MatchTest, Contains)
{
    cxxkit::StringView a("abcdefg");
    cxxkit::StringView b("abcd");
    cxxkit::StringView c("efg");
    cxxkit::StringView d("gh");
    EXPECT_TRUE(utils::stringContains(a, a));
    EXPECT_TRUE(utils::stringContains(a, b));
    EXPECT_TRUE(utils::stringContains(a, c));
    EXPECT_FALSE(utils::stringContains(a, d));
    EXPECT_TRUE(utils::stringContains("", ""));
    EXPECT_TRUE(utils::stringContains("abc", ""));
    EXPECT_FALSE(utils::stringContains("", "a"));
}

TEST(MatchTest, ContainsChar)
{
    cxxkit::StringView a("abcdefg");
    cxxkit::StringView b("abcd");
    EXPECT_TRUE(utils::stringContains(a, 'a'));
    EXPECT_TRUE(utils::stringContains(a, 'b'));
    EXPECT_TRUE(utils::stringContains(a, 'e'));
    EXPECT_FALSE(utils::stringContains(a, 'h'));

    EXPECT_TRUE(utils::stringContains(b, 'a'));
    EXPECT_TRUE(utils::stringContains(b, 'b'));
    EXPECT_FALSE(utils::stringContains(b, 'e'));
    EXPECT_FALSE(utils::stringContains(b, 'h'));

    EXPECT_FALSE(utils::stringContains("", 'a'));
    EXPECT_FALSE(utils::stringContains("", 'a'));
}

TEST(MatchTest, ContainsNull)
{
    const std::string s = "foo";
    const char *cs = "foo";
    const cxxkit::StringView sv("foo");
    const cxxkit::StringView sv2("foo\0bar", 4);
    EXPECT_EQ(s, "foo");
    EXPECT_EQ(sv, "foo");
    EXPECT_NE(sv2, "foo");
    EXPECT_TRUE(utils::stringEndsWith(s, sv));
    EXPECT_TRUE(utils::stringStartsWith(cs, sv));
    EXPECT_TRUE(utils::stringContains(cs, sv));
    EXPECT_FALSE(utils::stringContains(cs, sv2));
}

TEST(MatchTest, EqualsIgnoreCase)
{
    std::string text = "the";
    cxxkit::StringView data(text);

    EXPECT_TRUE(utils::stringEqualsIgnoreCase(data, "The"));
    EXPECT_TRUE(utils::stringEqualsIgnoreCase(data, "THE"));
    EXPECT_TRUE(utils::stringEqualsIgnoreCase(data, "the"));
    EXPECT_TRUE(utils::stringEqualsIgnoreCase(data, std::string("the")));
    EXPECT_FALSE(utils::stringEqualsIgnoreCase(data, "Quick"));
    EXPECT_FALSE(utils::stringEqualsIgnoreCase(data, "then"));
    EXPECT_FALSE(utils::stringEqualsIgnoreCase(data, std::string("then")));
}

TEST(MatchTest, StartsWithIgnoreCase)
{
    EXPECT_TRUE(utils::stringStartsWithIgnoreCase("foo", "foo"));
    EXPECT_TRUE(utils::stringStartsWithIgnoreCase("foo", "Fo"));
    EXPECT_TRUE(utils::stringStartsWithIgnoreCase("foo", ""));
    EXPECT_FALSE(utils::stringStartsWithIgnoreCase("foo", "fooo"));
    EXPECT_FALSE(utils::stringStartsWithIgnoreCase("", "fo"));
}

TEST(MatchTest, EndsWithIgnoreCase)
{
    EXPECT_TRUE(utils::stringEndsWithIgnoreCase("foo", "foo"));
    EXPECT_TRUE(utils::stringEndsWithIgnoreCase("foo", "Oo"));
    EXPECT_TRUE(utils::stringEndsWithIgnoreCase("foo", ""));
    EXPECT_FALSE(utils::stringEndsWithIgnoreCase("foo", "fooo"));
    EXPECT_FALSE(utils::stringEndsWithIgnoreCase("", "fo"));
}

TEST(MatchTest, ContainsIgnoreCase)
{
    EXPECT_TRUE(utils::stringContainsIgnoreCase("foo", "foo"));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("FOO", "Foo"));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("--FOO", "Foo"));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("FOO--", "Foo"));
    EXPECT_FALSE(utils::stringContainsIgnoreCase("BAR", "Foo"));
    EXPECT_FALSE(utils::stringContainsIgnoreCase("BAR", "Foo"));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("123456", "123456"));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("123456", "234"));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("", ""));
    EXPECT_TRUE(utils::stringContainsIgnoreCase("abc", ""));
    EXPECT_FALSE(utils::stringContainsIgnoreCase("", "a"));
}

TEST(MatchTest, ContainsCharIgnoreCase)
{
    cxxkit::StringView a("AaBCdefg!");
    cxxkit::StringView b("AaBCd!");
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, 'a'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, 'A'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, 'b'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, 'B'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, 'e'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, 'E'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(a, 'h'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(a, 'H'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(a, '!'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(a, '?'));

    EXPECT_TRUE(utils::stringContainsIgnoreCase(b, 'a'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(b, 'A'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(b, 'b'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(b, 'B'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(b, 'e'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(b, 'E'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(b, 'h'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(b, 'H'));
    EXPECT_TRUE(utils::stringContainsIgnoreCase(b, '!'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase(b, '?'));

    EXPECT_FALSE(utils::stringContainsIgnoreCase("", 'a'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase("", 'A'));
    EXPECT_FALSE(utils::stringContainsIgnoreCase("", '0'));
}

TEST(StringFormatTest, Empty)
{
    EXPECT_EQ("", utils::stringFormat("%s", ""));
}

TEST(StringFormatTest, Misc)
{
    EXPECT_EQ("123hello w", utils::stringFormat("%3d%2s %1c", 123, "hello", 'w'));
    EXPECT_EQ("3 = three", utils::stringFormat("%d = %s", 1 + 2, "three"));
}

TEST(StringFormatTest, MaxSizeShouldWork)
{
    const int kSrcLen = 512;
    char str[kSrcLen];
    std::fill_n(str, kSrcLen, 'A');
    str[kSrcLen - 1] = 0;
    EXPECT_EQ(str, utils::stringFormat("%s", str));
}

// Test that formating a string using `StringView` works as expected
// whe using `%.*s`.
TEST(StringFormatTest, FormatStringView)
{
    const std::string main_string("This is a substring test.");
    std::vector<StringView> string_views = utils::stringSplit(main_string, ' ');
    ASSERT_EQ(string_views.size(), 5u);

    const StringView &sv = string_views[3];
    std::string formatted = utils::stringFormat("We have a %.*s.", static_cast<int>(sv.size()), sv.data());
    EXPECT_EQ(formatted.compare("We have a substring."), 0);
}

CXXKIT_END_NAMESPACE
