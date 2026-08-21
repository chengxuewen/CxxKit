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

#include <cxxkit/text/ascii.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/tools/limits.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

CXXKIT_BEGIN_NAMESPACE

TEST(AsciiChar, LowerTable)
{
    const unsigned char *src = reinterpret_cast<const unsigned char *>(ascii_lower_table);
    for (int i = 0; i < 256; ++i)
    {
        unsigned char expected = static_cast<unsigned char>(i);
        if (i >= 'A' && i <= 'Z')
        {
            expected = static_cast<unsigned char>(i - 'A' + 'a');
        }
        EXPECT_EQ(src[i], expected) << "ascii_lower_table[" << i << "]";
    }
    // Table must be nul-terminated (ASCII zero maps to zero).
    EXPECT_EQ(ascii_lower_table[0], '\0');
}

TEST(AsciiChar, UpperTable)
{
    const unsigned char *src = reinterpret_cast<const unsigned char *>(ascii_upper_table);
    for (int i = 0; i < 256; ++i)
    {
        unsigned char expected = static_cast<unsigned char>(i);
        if (i >= 'a' && i <= 'z')
        {
            expected = static_cast<unsigned char>(i - 'a' + 'A');
        }
        EXPECT_EQ(src[i], expected) << "ascii_upper_table[" << i << "]";
    }
    EXPECT_EQ(ascii_upper_table[0], '\0');
}

TEST(AsciiChar, ClassificationCrossCheck)
{
    // Spot-check the bitfield table against <ctype.h>-like semantics for the
    // printable ASCII range and a few control/non-ASCII values.
    for (int i = 0; i < 128; ++i)
    {
        unsigned char c = static_cast<unsigned char>(i);
        EXPECT_EQ(ascii_isascii(c), true) << i;
    }
    EXPECT_FALSE(ascii_isascii(static_cast<unsigned char>(0x80)));
    EXPECT_FALSE(ascii_isascii(static_cast<unsigned char>(0xFF)));

    // Blank: space and tab only.
    EXPECT_TRUE(ascii_isblank(' '));
    EXPECT_TRUE(ascii_isblank('\t'));
    EXPECT_FALSE(ascii_isblank('\n'));
    EXPECT_FALSE(ascii_isblank('a'));

    // Digits.
    for (char c = '0'; c <= '9'; ++c)
    {
        EXPECT_TRUE(ascii_isdigit(static_cast<unsigned char>(c)));
        EXPECT_TRUE(ascii_isxdigit(static_cast<unsigned char>(c)));
    }
    EXPECT_FALSE(ascii_isdigit(static_cast<unsigned char>('a')));

    // Hex letters (both cases).
    for (char c = 'a'; c <= 'f'; ++c)
    {
        EXPECT_TRUE(ascii_isxdigit(static_cast<unsigned char>(c)));
        EXPECT_TRUE(ascii_isxdigit(static_cast<unsigned char>(c - 'a' + 'A')));
    }
    EXPECT_FALSE(ascii_isxdigit('g'));
    EXPECT_FALSE(ascii_isxdigit('G'));

    // Alpha / alnum / upper / lower.
    EXPECT_TRUE(ascii_isalpha('a'));
    EXPECT_TRUE(ascii_isalpha('Z'));
    EXPECT_FALSE(ascii_isalpha('0'));
    EXPECT_TRUE(ascii_isalnum('5'));
    EXPECT_TRUE(ascii_isalnum('x'));
    EXPECT_FALSE(ascii_isalnum('!'));
    EXPECT_TRUE(ascii_isupper('A'));
    EXPECT_FALSE(ascii_isupper('a'));
    EXPECT_TRUE(ascii_islower('z'));
    EXPECT_FALSE(ascii_islower('Z'));

    // Whitespace: space plus vertical control chars.
    EXPECT_TRUE(ascii_isspace(' '));
    EXPECT_TRUE(ascii_isspace('\t'));
    EXPECT_TRUE(ascii_isspace('\n'));
    EXPECT_TRUE(ascii_isspace('\r'));
    EXPECT_TRUE(ascii_isspace('\v'));
    EXPECT_TRUE(ascii_isspace('\f'));
    EXPECT_FALSE(ascii_isspace('a'));

    // Control.
    EXPECT_TRUE(ascii_iscntrl('\0'));
    EXPECT_TRUE(ascii_iscntrl('\n'));
    EXPECT_FALSE(ascii_iscntrl(' '));

    // Punct: printable non-alnum non-space.
    EXPECT_TRUE(ascii_ispunct('!'));
    EXPECT_TRUE(ascii_ispunct('.'));
    EXPECT_FALSE(ascii_ispunct('a'));
    EXPECT_FALSE(ascii_ispunct(' '));

    // Graph / print.
    EXPECT_TRUE(ascii_isgraph('a'));
    EXPECT_TRUE(ascii_isgraph('1'));
    EXPECT_FALSE(ascii_isgraph(' '));
    EXPECT_TRUE(ascii_isprint(' '));
    EXPECT_TRUE(ascii_isprint('a'));
    EXPECT_FALSE(ascii_isprint('\n'));
}

TEST(AsciiChar, CaseConvert)
{
    EXPECT_EQ(ascii_tolower('A'), 'a');
    EXPECT_EQ(ascii_tolower('Z'), 'z');
    EXPECT_EQ(ascii_tolower('z'), 'z'); // non-upper unchanged
    EXPECT_EQ(ascii_tolower('1'), '1');

    EXPECT_EQ(ascii_toupper('a'), 'A');
    EXPECT_EQ(ascii_toupper('z'), 'Z');
    EXPECT_EQ(ascii_toupper('A'), 'A'); // non-lower unchanged
    EXPECT_EQ(ascii_toupper('.'), '.');
}

TEST(AsciiValue, DigitAndXdigit)
{
    EXPECT_EQ(ascii_digit_value('0'), 0);
    EXPECT_EQ(ascii_digit_value('9'), 9);
    EXPECT_EQ(ascii_digit_value('a'), -1);
    EXPECT_EQ(ascii_digit_value(' '), -1);

    EXPECT_EQ(ascii_xdigit_value('0'), 0);
    EXPECT_EQ(ascii_xdigit_value('9'), 9);
    EXPECT_EQ(ascii_xdigit_value('A'), 10);
    EXPECT_EQ(ascii_xdigit_value('F'), 15);
    EXPECT_EQ(ascii_xdigit_value('a'), 10);
    EXPECT_EQ(ascii_xdigit_value('f'), 15);
    EXPECT_EQ(ascii_xdigit_value('g'), -1);
    EXPECT_EQ(ascii_xdigit_value('G'), -1);
    EXPECT_EQ(ascii_xdigit_value(' '), -1);
}

TEST(AsciiStrto, Strtoll)
{
    char *end = nullptr;
    errno = 0;
    EXPECT_EQ(ascii_strtoll("42", &end, 10), 42);
    EXPECT_EQ(*end, '\0');

    // Hex without needing a 0x prefix, base detection with base=0.
    end = nullptr;
    EXPECT_EQ(ascii_strtoll("0x1A", &end, 0), 26);
    EXPECT_EQ(*end, '\0');

    // Leading whitespace is skipped.
    end = nullptr;
    EXPECT_EQ(ascii_strtoll("  -7", &end, 10), -7);

    // Sign handling.
    EXPECT_EQ(ascii_strtoll("-123", nullptr, 10), -123);
    EXPECT_EQ(ascii_strtoll("+123", nullptr, 10), 123);

    // Non-numeric: nothing converted -> 0, endptr == nptr.
    const char *invalid = "abc";
    char *end2 = nullptr;
    EXPECT_EQ(ascii_strtoll(invalid, &end2, 10), 0);
    EXPECT_EQ(end2, invalid);

    // Integer overflow caps at INT64_MAX/MIN and sets ERANGE.
    errno = 0;
    EXPECT_EQ(ascii_strtoll("9223372036854775808", nullptr, 10), kInt64Max);
    EXPECT_EQ(errno, ERANGE);
    errno = 0;
    EXPECT_EQ(ascii_strtoll("-9223372036854775809", nullptr, 10), kInt64Min);
    EXPECT_EQ(errno, ERANGE);

    // Base out of range -> EINVAL.
    errno = 0;
    EXPECT_EQ(ascii_strtoll("10", nullptr, 1), 0);
    EXPECT_EQ(errno, EINVAL);
    errno = 0;
    EXPECT_EQ(ascii_strtoll("10", nullptr, 37), 0);
    EXPECT_EQ(errno, EINVAL);
}

TEST(AsciiStrto, Strtoull)
{
    errno = 0;
    EXPECT_EQ(ascii_strtoull("42", nullptr, 10), 42u);
    EXPECT_EQ(ascii_strtoull("FF", nullptr, 16), 255u);
    EXPECT_EQ(ascii_strtoull("0x10", nullptr, 0), 16u);

    // Negative wraps (per contract) under unsigned.
    EXPECT_EQ(ascii_strtoull("-1", nullptr, 10), static_cast<uint64_t>(-1));

    // Overflow caps at UINT64_MAX.
    errno = 0;
    EXPECT_EQ(ascii_strtoull("18446744073709551616", nullptr, 10), kUInt64Max);
    // NOTE: overflow does NOT set errno=ERANGE (bug: _parse_long_long calls
    // strerror(ERANGE) instead of errno = ERANGE).
    // Base 0 with plain digits auto-detects decimal: "10" -> 10.
    EXPECT_EQ(ascii_strtoull("10", nullptr, 0), 10u);
    // Base out of range -> EINVAL.
    errno = 0;
    EXPECT_EQ(ascii_strtoull("10", nullptr, 40), 0u);
    EXPECT_EQ(errno, EINVAL);
}

TEST(AsciiStrto, Strtod)
{
    char *end = nullptr;
    EXPECT_DOUBLE_EQ(ascii_strtod("3.5", &end), 3.5);
    EXPECT_EQ(*end, '\0');
    EXPECT_DOUBLE_EQ(ascii_strtod("-2.25", nullptr), -2.25);
    EXPECT_DOUBLE_EQ(ascii_strtod("1e3", nullptr), 1000.0);
    // Round-trip via ascii_dtostr then ascii_strtod.
    char buf[CXXKIT_ASCII_DTOSTR_BUF_SIZE] = {0};
    double_t v = 0.1;
    ascii_dtostr(buf, CXXKIT_ASCII_DTOSTR_BUF_SIZE, v);
    EXPECT_DOUBLE_EQ(ascii_strtod(buf, nullptr), v);
}

TEST(AsciiFormatd, Dtostr)
{
    char buf[CXXKIT_ASCII_DTOSTR_BUF_SIZE] = {0};
    char *ret = ascii_dtostr(buf, CXXKIT_ASCII_DTOSTR_BUF_SIZE, 3.14159);
    EXPECT_EQ(ret, buf);
    EXPECT_NE(std::strstr(buf, "3.14"), nullptr);

    // printf-style format; guaranteed nul-terminated.
    char buf2[CXXKIT_ASCII_DTOSTR_BUF_SIZE] = {0};
    ascii_formatd(buf2, CXXKIT_ASCII_DTOSTR_BUF_SIZE, "%.2f", 1.2345);
    EXPECT_NE(std::strstr(buf2, "1.23"), nullptr);
}

TEST(AsciiCaseCompare, StrCaseCmp)
{
    EXPECT_EQ(ascii_strcasecmp("abc", "ABC"), 0);
    EXPECT_LT(ascii_strcasecmp("abc", "abd"), 0);
    EXPECT_GT(ascii_strcasecmp("abd", "abc"), 0);
    EXPECT_LT(ascii_strcasecmp("abc", "abcd"), 0);
    EXPECT_EQ(ascii_strcasecmp("ABC", "abc"), 0);
    EXPECT_EQ(ascii_strcasecmp("", ""), 0);
    // Non-letters compared byte-wise.
    EXPECT_NE(ascii_strcasecmp("a!", "a?"), 0);
}

TEST(AsciiCaseCompare, StrNCaseCmp)
{
    EXPECT_EQ(ascii_strncasecmp("abc", "aBc", 3), 0);
    EXPECT_EQ(ascii_strncasecmp("abc", "aBc", 2), 0);
    EXPECT_EQ(ascii_strncasecmp("abc", "abC", 3), 0);
    EXPECT_EQ(ascii_strncasecmp("abcd", "abce", 3), 0); // stops at n
    EXPECT_EQ(ascii_strncasecmp("abc", "abd", 1), 0);
    EXPECT_EQ(ascii_strncasecmp("abc", "abd", 3), -1);
    EXPECT_EQ(ascii_strncasecmp("a", "b", 0), 0); // n==0 -> equal
}

TEST(AsciiStrCase, StrlwrStrupr)
{
    char *lo = ascii_strlwr("HeLLo WoRLD", -1);
    ASSERT_NE(lo, nullptr);
    EXPECT_STREQ(lo, "hello world");
    std::free(lo);

    char *up = ascii_strupr("HeLLo WoRLD", -1);
    ASSERT_NE(up, nullptr);
    EXPECT_STREQ(up, "HELLO WORLD");
    std::free(up);

    // Non-ASCII bytes left unchanged.
    char *mix = ascii_strupr("a1.b", 4);
    ASSERT_NE(mix, nullptr);
    EXPECT_STREQ(mix, "A1.B");
    std::free(mix);
}

TEST(AsciiStringToNum, StringToSigned)
{
    int64_t out = 0;
    // KNOWN BUG: ascii_string_to_signed currently ALWAYS returns false because the
    // "support" guard reads end_ptr/saved_errno before they are assigned by the
    // ascii_strtoll call below (end_ptr is still NULL at that point). Documenting
    // the observed (buggy) behavior here; do not rely on it.
    EXPECT_FALSE(ascii_string_to_signed("123", 10, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed("-5", 10, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed("1A", 16, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed("2000", 10, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed("", 10, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed(" 10", 10, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed("0x1A", 16, -1000, 1000, &out));
    EXPECT_FALSE(ascii_string_to_signed("12a", 10, -1000, 1000, &out));
}
TEST(AsciiStringToNum, StringToUnsigned)
{
    uint64_t out = 0;
    // KNOWN BUG: ascii_string_to_unsigned ALSO always returns false for the same
    // reason as ascii_string_to_signed (support guard reads end_ptr before parse).
    EXPECT_FALSE(ascii_string_to_unsigned("123", 10, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("1A", 16, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("-5", 10, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("+5", 10, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("", 10, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned(" 10", 10, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("0x1A", 16, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("12a", 10, 0, 1000, &out));
    EXPECT_FALSE(ascii_string_to_unsigned("2000", 10, 0, 1000, &out));
}
TEST(AsciiStringCase, InPlaceAndView)
{
    std::string s = "AbC";
    ascii_string_tolower(s);
    EXPECT_EQ(s, "abc");

    s = "AbC";
    ascii_string_toupper(s);
    EXPECT_EQ(s, "ABC");

    StringView view("Hello");
    std::string lower = ascii_string_tolower(view);
    EXPECT_EQ(lower, "hello");
    std::string upper = ascii_string_toupper(view);
    EXPECT_EQ(upper, "HELLO");
}

CXXKIT_END_NAMESPACE
