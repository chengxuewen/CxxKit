/***********************************************************************************************************************
**
** Library: CxxKit
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
    EXPECT_TRUE(utils::string_starts_with(a, a));
    EXPECT_TRUE(utils::string_starts_with(a, "foo"));
    EXPECT_TRUE(utils::string_starts_with(a, e));
    EXPECT_TRUE(utils::string_starts_with(b, s1));
    EXPECT_TRUE(utils::string_starts_with(b, b));
    EXPECT_TRUE(utils::string_starts_with(b, e));
    EXPECT_TRUE(utils::string_starts_with(e, ""));
    EXPECT_FALSE(utils::string_starts_with(a, b));
    EXPECT_FALSE(utils::string_starts_with(b, a));
    EXPECT_FALSE(utils::string_starts_with(e, a));
}

TEST(MatchTest, EndsWith)
{
    const std::string s1("123\0abc", 7);
    const cxxkit::StringView a("foobar");
    const cxxkit::StringView b(s1);
    const cxxkit::StringView e;
    EXPECT_TRUE(utils::string_ends_with(a, a));
    EXPECT_TRUE(utils::string_ends_with(a, "bar"));
    EXPECT_TRUE(utils::string_ends_with(a, e));
    EXPECT_TRUE(utils::string_ends_with(b, s1));
    EXPECT_TRUE(utils::string_ends_with(b, b));
    EXPECT_TRUE(utils::string_ends_with(b, e));
    EXPECT_TRUE(utils::string_ends_with(e, ""));
    EXPECT_FALSE(utils::string_ends_with(a, b));
    EXPECT_FALSE(utils::string_ends_with(b, a));
    EXPECT_FALSE(utils::string_ends_with(e, a));
}

TEST(MatchTest, Contains)
{
    cxxkit::StringView a("abcdefg");
    cxxkit::StringView b("abcd");
    cxxkit::StringView c("efg");
    cxxkit::StringView d("gh");
    EXPECT_TRUE(utils::string_contains(a, a));
    EXPECT_TRUE(utils::string_contains(a, b));
    EXPECT_TRUE(utils::string_contains(a, c));
    EXPECT_FALSE(utils::string_contains(a, d));
    EXPECT_TRUE(utils::string_contains("", ""));
    EXPECT_TRUE(utils::string_contains("abc", ""));
    EXPECT_FALSE(utils::string_contains("", "a"));
}

TEST(MatchTest, ContainsChar)
{
    cxxkit::StringView a("abcdefg");
    cxxkit::StringView b("abcd");
    EXPECT_TRUE(utils::string_contains(a, 'a'));
    EXPECT_TRUE(utils::string_contains(a, 'b'));
    EXPECT_TRUE(utils::string_contains(a, 'e'));
    EXPECT_FALSE(utils::string_contains(a, 'h'));

    EXPECT_TRUE(utils::string_contains(b, 'a'));
    EXPECT_TRUE(utils::string_contains(b, 'b'));
    EXPECT_FALSE(utils::string_contains(b, 'e'));
    EXPECT_FALSE(utils::string_contains(b, 'h'));

    EXPECT_FALSE(utils::string_contains("", 'a'));
    EXPECT_FALSE(utils::string_contains("", 'a'));
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
    EXPECT_TRUE(utils::string_ends_with(s, sv));
    EXPECT_TRUE(utils::string_starts_with(cs, sv));
    EXPECT_TRUE(utils::string_contains(cs, sv));
    EXPECT_FALSE(utils::string_contains(cs, sv2));
}

TEST(MatchTest, EqualsIgnoreCase)
{
    std::string text = "the";
    cxxkit::StringView data(text);

    EXPECT_TRUE(utils::string_equals_ignore_case(data, "The"));
    EXPECT_TRUE(utils::string_equals_ignore_case(data, "THE"));
    EXPECT_TRUE(utils::string_equals_ignore_case(data, "the"));
    EXPECT_TRUE(utils::string_equals_ignore_case(data, std::string("the")));
    EXPECT_FALSE(utils::string_equals_ignore_case(data, "Quick"));
    EXPECT_FALSE(utils::string_equals_ignore_case(data, "then"));
    EXPECT_FALSE(utils::string_equals_ignore_case(data, std::string("then")));
}

TEST(MatchTest, StartsWithIgnoreCase)
{
    EXPECT_TRUE(utils::string_starts_with_ignore_case("foo", "foo"));
    EXPECT_TRUE(utils::string_starts_with_ignore_case("foo", "Fo"));
    EXPECT_TRUE(utils::string_starts_with_ignore_case("foo", ""));
    EXPECT_FALSE(utils::string_starts_with_ignore_case("foo", "fooo"));
    EXPECT_FALSE(utils::string_starts_with_ignore_case("", "fo"));
}

TEST(MatchTest, EndsWithIgnoreCase)
{
    EXPECT_TRUE(utils::string_ends_with_ignore_case("foo", "foo"));
    EXPECT_TRUE(utils::string_ends_with_ignore_case("foo", "Oo"));
    EXPECT_TRUE(utils::string_ends_with_ignore_case("foo", ""));
    EXPECT_FALSE(utils::string_ends_with_ignore_case("foo", "fooo"));
    EXPECT_FALSE(utils::string_ends_with_ignore_case("", "fo"));
}

TEST(MatchTest, ContainsIgnoreCase)
{
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("foo", "foo"));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("FOO", "Foo"));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("--FOO", "Foo"));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("FOO--", "Foo"));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case("BAR", "Foo"));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case("BAR", "Foo"));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("123456", "123456"));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("123456", "234"));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("", ""));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case("abc", ""));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case("", "a"));
}

TEST(MatchTest, ContainsCharIgnoreCase)
{
    cxxkit::StringView a("AaBCdefg!");
    cxxkit::StringView b("AaBCd!");
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, 'a'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, 'A'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, 'b'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, 'B'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, 'e'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, 'E'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(a, 'h'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(a, 'H'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(a, '!'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(a, '?'));

    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(b, 'a'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(b, 'A'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(b, 'b'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(b, 'B'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(b, 'e'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(b, 'E'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(b, 'h'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(b, 'H'));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(b, '!'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(b, '?'));

    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case("", 'a'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case("", 'A'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case("", '0'));
}

TEST(StringFormatTest, Empty)
{
    EXPECT_EQ("", cxxkit::utils::string_format("%s", ""));
}

TEST(StringFormatTest, Misc)
{
    EXPECT_EQ("123hello w", cxxkit::utils::string_format("%3d%2s %1c", 123, "hello", 'w'));
    EXPECT_EQ("3 = three", cxxkit::utils::string_format("%d = %s", 1 + 2, "three"));
}

TEST(StringFormatTest, MaxSizeShouldWork)
{
    const int kSrcLen = 512;
    char str[kSrcLen];
    std::fill_n(str, kSrcLen, 'A');
    str[kSrcLen - 1] = 0;
    EXPECT_EQ(str, cxxkit::utils::string_format("%s", str));
}

// test that formating a string using `StringView` works as expected
// whe using `%.*s`.
TEST(StringFormatTest, FormatStringView)
{
    const std::string main_string("This is a substring test.");
    std::vector<cxxkit::StringView> string_views = cxxkit::utils::string_split(main_string, ' ');
    ASSERT_EQ(string_views.size(), 5u);

    const StringView &sv = string_views[3];
    std::string formatted = cxxkit::utils::string_format("We have a %.*s.", static_cast<int>(sv.size()), sv.data());
    EXPECT_EQ(formatted.compare("We have a substring."), 0);
}

CXXKIT_END_NAMESPACE

// Coverage for string_utils.cpp implementation functions (string_utils.hpp alone had thin tests).
TEST(StringUtils, ExtractFileName)
{
    EXPECT_STREQ(cxxkit::utils::extract_file_name("/a/b/file.cpp"), "file.cpp");
    EXPECT_STREQ(cxxkit::utils::extract_file_name("plain.cpp"), "plain.cpp");
}

TEST(StringUtils, ExtractFunctionName)
{
    // Actual behavior: returns the segment after the last space before '(' up to '('
    // (leading space preserved) plus the suffix.
    const std::string fn = cxxkit::utils::extract_function_name("void foo(int x)");
    EXPECT_EQ(fn, " foo");
}

TEST(StringUtils, StringCompare)
{
    // PIT-28 fixed: identical bytes now compare equal; case-folded equal also true.
    EXPECT_TRUE(cxxkit::utils::string_compare("abc", "abc", 3, false));
    EXPECT_TRUE(cxxkit::utils::string_compare("ABC", "abc", 3, true));
    EXPECT_FALSE(cxxkit::utils::string_compare("abc", "abd", 3, false));
    EXPECT_FALSE(cxxkit::utils::string_compare("ABC", "abd", 3, true));
}

TEST(StringUtils, StringCaseCmp)
{
    EXPECT_EQ(cxxkit::utils::string_case_cmp("AbC", "aBc", 3), 0);
    EXPECT_NE(cxxkit::utils::string_case_cmp("abc", "abd", 3), 0);
}

TEST(StringUtils, ContainsIgnoreCase)
{
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(cxxkit::StringView("Hello World"), cxxkit::StringView("WORLD")));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(cxxkit::StringView("Hello"), cxxkit::StringView("xyz")));
    EXPECT_TRUE(cxxkit::utils::string_contains_ignore_case(cxxkit::StringView("abc"), 'b'));
    EXPECT_FALSE(cxxkit::utils::string_contains_ignore_case(cxxkit::StringView("abc"), 'z'));
}

TEST(StringUtils, StringSplit)
{
    const std::vector<cxxkit::StringView> parts = cxxkit::utils::string_split(cxxkit::StringView("a,b,c"), ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], cxxkit::StringView("a"));
    EXPECT_EQ(parts[1], cxxkit::StringView("b"));
    EXPECT_EQ(parts[2], "c");
}

TEST(StringUtils, StringFormat)
{
    std::string s = cxxkit::utils::string_format("%d-%s", 7, "x");
    EXPECT_EQ(s, "7-x");
}
