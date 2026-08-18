/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2018 The WebRTC Project Authors. All rights reserved.
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

#include "cxxkit/text/string_builder.hpp"

#include <string.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

TEST(SimpleStringBuilder, Limit)
{
    char sb_buf[10];
    SimpleStringBuilder sb(sb_buf);
    EXPECT_EQ(0u, strlen(sb.str()));

    // Test that for a SSB with a buffer size of 10, that we can write 9 chars
    // into it.
    sb << "012345678"; // 9 characters + '\0'.
    EXPECT_EQ(0, strcmp(sb.str(), "012345678"));
}

TEST(SimpleStringBuilder, NumbersAndChars)
{
    char sb_buf[100];
    SimpleStringBuilder sb(sb_buf);
    sb << 1 << ':' << 2.1 << ":" << 2.2f << ':' << 78187493520ll << ':' << 78187493520ul;
    EXPECT_EQ(0, strcmp(sb.str(), "1:2.1:2.2:78187493520:78187493520"));
}

TEST(SimpleStringBuilder, Format)
{
    char sb_buf[100];
    SimpleStringBuilder sb(sb_buf);
    sb << "Here we go - ";
    sb.AppendFormat("This is a hex formatted value: 0x%08llx", 3735928559ULL);
    EXPECT_EQ(0, strcmp(sb.str(), "Here we go - This is a hex formatted value: 0xdeadbeef"));
}

TEST(SimpleStringBuilder, StdString)
{
    char sb_buf[100];
    SimpleStringBuilder sb(sb_buf);
    std::string str = "does this work?";
    sb << str;
    EXPECT_EQ(str, sb.str());
}

// These tests are safe to run if we have death test support or if DCHECKs are
// off.
#if (GTEST_HAS_DEATH_TEST && !defined(WEBRTC_ANDROID)) || !CXXKIT_DCHECK_IS_ON

TEST(SimpleStringBuilderDeathTest, BufferOverrunConstCharP)
{
    char sb_buf[4];
    SimpleStringBuilder sb(sb_buf);
    const char *const msg = "This is just too much";
#    if CXXKIT_DCHECK_IS_ON
    EXPECT_DEATH(sb << msg, "");
#    else
    sb << msg;
    EXPECT_THAT(sb.str(), ::testing::StrEq("Thi"));
#    endif
}

TEST(SimpleStringBuilderDeathTest, BufferOverrunStdString)
{
    char sb_buf[4];
    SimpleStringBuilder sb(sb_buf);
    sb << 12;
    const std::string msg = "Aw, come on!";
#    if CXXKIT_DCHECK_IS_ON
    EXPECT_DEATH(sb << msg, "");
#    else
    sb << msg;
    EXPECT_THAT(sb.str(), ::testing::StrEq("12A"));
#    endif
}

TEST(SimpleStringBuilderDeathTest, BufferOverrunInt)
{
    char sb_buf[4];
    SimpleStringBuilder sb(sb_buf);
    constexpr int num = -12345;
#    if CXXKIT_DCHECK_IS_ON
    EXPECT_DEATH(sb << num, "");
#    else
    sb << num;
    // If we run into the end of the buffer, resonable results are either that
    // the append has no effect or that it's truncated at the point where the
    // buffer ends.
    EXPECT_THAT(sb.str(), ::testing::AnyOf(::testing::StrEq(""), ::testing::StrEq("-12")));
#    endif
}

TEST(SimpleStringBuilderDeathTest, BufferOverrunDouble)
{
    char sb_buf[5];
    SimpleStringBuilder sb(sb_buf);
    constexpr double num = 123.456;
#    if CXXKIT_DCHECK_IS_ON
    EXPECT_DEATH(sb << num, "");
#    else
    sb << num;
    EXPECT_THAT(sb.str(), ::testing::AnyOf(::testing::StrEq(""), ::testing::StrEq("123.")));
#    endif
}

TEST(SimpleStringBuilderDeathTest, BufferOverrunConstCharPAlreadyFull)
{
    char sb_buf[4];
    SimpleStringBuilder sb(sb_buf);
    sb << 123;
    const char *const msg = "This is just too much";
#    if CXXKIT_DCHECK_IS_ON
    EXPECT_DEATH(sb << msg, "");
#    else
    sb << msg;
    EXPECT_THAT(sb.str(), ::testing::StrEq("123"));
#    endif
}

TEST(SimpleStringBuilderDeathTest, BufferOverrunIntAlreadyFull)
{
    char sb_buf[4];
    SimpleStringBuilder sb(sb_buf);
    sb << "xyz";
    constexpr int num = -12345;
#    if CXXKIT_DCHECK_IS_ON
    EXPECT_DEATH(sb << num, "");
#    else
    sb << num;
    EXPECT_THAT(sb.str(), ::testing::StrEq("xyz"));
#    endif
}

#endif

////////////////////////////////////////////////////////////////////////////////
// StringBuilder.

TEST(StringBuilder, Limit)
{
    StringBuilder sb;
    EXPECT_EQ(0u, sb.str().size());

    sb << "012345678";
    EXPECT_EQ(sb.str(), "012345678");
}

TEST(StringBuilder, NumbersAndChars)
{
    StringBuilder sb;
    sb << 1 << ":" << 2.1 << ":" << 2.2f << ":" << 78187493520ll << ":" << 78187493520ul;
    EXPECT_THAT(sb.str(), ::testing::MatchesRegex("1:2.10*:2.20*:78187493520:78187493520"));
}

TEST(StringBuilder, Format)
{
    StringBuilder sb;
    sb << "Here we go - ";
    sb.AppendFormat("This is a hex formatted value: 0x%08llx", 3735928559ULL);
    EXPECT_EQ(sb.str(), "Here we go - This is a hex formatted value: 0xdeadbeef");
}

TEST(StringBuilder, StdString)
{
    StringBuilder sb;
    std::string str = "does this work?";
    sb << str;
    EXPECT_EQ(str, sb.str());
}

TEST(StringBuilder, Release)
{
    StringBuilder sb;
    std::string str = "This string has to be of a moderate length, or we might "
                      "run into problems with small object optimizations.";
    EXPECT_LT(sizeof(str), str.size());
    sb << str;
    EXPECT_EQ(str, sb.str());
    const char *original_buffer = sb.str().c_str();
    std::string moved = sb.Release();
    EXPECT_TRUE(sb.str().empty());
    EXPECT_EQ(str, moved);
    EXPECT_EQ(original_buffer, moved.c_str());
}

TEST(StringBuilder, Reset)
{
    StringBuilder sb("abc");
    sb << "def";
    EXPECT_EQ("abcdef", sb.str());
    sb.Clear();
    EXPECT_TRUE(sb.str().empty());
    sb << 123 << "!";
    EXPECT_EQ("123!", sb.str());
}

CXXKIT_END_NAMESPACE