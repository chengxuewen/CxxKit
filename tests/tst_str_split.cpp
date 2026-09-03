/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2017 The Abseil Authors.
**
** License: MIT License
**
** This file contains tests for text/str_split.hpp, a simplified reimplementation of string splitting
** (API-compatible subset of abseil-cpp `absl/strings/str_split.h`, originally governed by the
** Apache License 2.0). Modified for CxxKit.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

// Tests for str_split (text/str_split.hpp), a trimmed port of abseil `absl::StrSplit`.

#include <cxxkit/text/str_split.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

using cxxkit::StringView;

std::vector<std::string> to_strings(const std::vector<StringView> &tokens)
{
    std::vector<std::string> out;
    out.reserve(tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        out.push_back(std::string(tokens[i]));
    }
    return out;
}

} // namespace

TEST(StrSplitTest, SplitBySingleChar)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b", "c"}), to_strings(cxxkit::str_split("a,b,c", ',')));
    EXPECT_EQ(std::vector<std::string>({"a", "b", "c"}), to_strings(cxxkit::str_split("a-b-c", '-')));
}

TEST(StrSplitTest, SplitBySingleCharNoDelimiter)
{
    // No delimiter present: the whole text is the single token.
    EXPECT_EQ(std::vector<std::string>({"abc"}), to_strings(cxxkit::str_split("abc", ',')));
}

TEST(StrSplitTest, SplitBySingleCharTrailingDelimiter)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b", ""}), to_strings(cxxkit::str_split("a,b,", ',')));
}

TEST(StrSplitTest, SplitBySingleCharAdjacentDelimiters)
{
    EXPECT_EQ(std::vector<std::string>({"a", "", "b"}), to_strings(cxxkit::str_split("a,,b", ',')));
}

TEST(StrSplitTest, SplitBySingleCharEmptyText)
{
    // abseil semantics: empty text yields exactly one empty token.
    EXPECT_EQ(std::vector<std::string>({""}), to_strings(cxxkit::str_split("", ',')));
}

TEST(StrSplitTest, SplitBySingleCharAllDelimiters)
{
    EXPECT_EQ(std::vector<std::string>({"", "", "", ""}), to_strings(cxxkit::str_split(",,,", ',')));
    EXPECT_EQ(std::vector<std::string>(), to_strings(cxxkit::str_split(",,,", ',', cxxkit::skip_empty())));
}

TEST(StrSplitTest, SplitByAnyOf)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b", "c", "d"}),
              to_strings(cxxkit::str_split("a,b;c d", cxxkit::by_any_of(",; "))));
}

TEST(StrSplitTest, SplitByAnyOfSingleDelimiter)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b"}), to_strings(cxxkit::str_split("a,b", cxxkit::by_any_of(","))));
}

TEST(StrSplitTest, SplitByAnyOfNoDelimiter)
{
    EXPECT_EQ(std::vector<std::string>({"abc"}), to_strings(cxxkit::str_split("abc", cxxkit::by_any_of(",; "))));
}

TEST(StrSplitTest, SplitByAnyOfAdjacentDelimiters)
{
    EXPECT_EQ(std::vector<std::string>({"a", "", "b"}),
              to_strings(cxxkit::str_split("a,;b", cxxkit::by_any_of(",; "))));
}

TEST(StrSplitTest, SplitByAnyOfEmptyText)
{
    EXPECT_EQ(std::vector<std::string>({""}), to_strings(cxxkit::str_split("", cxxkit::by_any_of(",; "))));
}

TEST(StrSplitTest, SplitByString)
{
    EXPECT_EQ(std::vector<std::string>({"", "a", "b", ""}),
              to_strings(cxxkit::str_split("--a--b--", cxxkit::by_string("--"))));
}

TEST(StrSplitTest, SplitByStringNoMatch)
{
    // No delimiter occurrence: the whole text is one token.
    EXPECT_EQ(std::vector<std::string>({"a.b.c"}), to_strings(cxxkit::str_split("a.b.c", cxxkit::by_string("--"))));
}

TEST(StrSplitTest, SplitByStringSingleOccurrence)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b"}), to_strings(cxxkit::str_split("a--b", cxxkit::by_string("--"))));
}

TEST(StrSplitTest, SplitByStringTrailingEmpty)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b", ""}),
              to_strings(cxxkit::str_split("a--b--", cxxkit::by_string("--"))));
}

TEST(StrSplitTest, SplitByStringEmptyText)
{
    EXPECT_EQ(std::vector<std::string>({""}), to_strings(cxxkit::str_split("", cxxkit::by_string("--"))));
}

TEST(StrSplitTest, SplitByStringAllDelimiters)
{
    // "------" = delim,x2,delim,end -> tokens "", "", "", "".
    EXPECT_EQ(std::vector<std::string>({"", "", "", ""}),
              to_strings(cxxkit::str_split("------", cxxkit::by_string("--"))));
}

TEST(StrSplitTest, SkipEmptySingleChar)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b"}),
              to_strings(cxxkit::str_split(",,a,,b,,", ',', cxxkit::skip_empty())));
}

TEST(StrSplitTest, SkipEmptyByAnyOf)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b"}),
              to_strings(cxxkit::str_split(" ,a ,b ", cxxkit::by_any_of(", "), cxxkit::skip_empty())));
}

TEST(StrSplitTest, SkipEmptyByString)
{
    EXPECT_EQ(std::vector<std::string>({"a", "b"}),
              to_strings(cxxkit::str_split("a--b--", cxxkit::by_string("--"), cxxkit::skip_empty())));
}

TEST(StrSplitTest, SkipEmptyAllEmptyTokens)
{
    EXPECT_EQ(std::vector<std::string>(),
              to_strings(cxxkit::str_split("", cxxkit::by_any_of(","), cxxkit::skip_empty())));
    EXPECT_EQ(std::vector<std::string>(), to_strings(cxxkit::str_split("---", '-', cxxkit::skip_empty())));
}

TEST(StrSplitTest, SkipEmptyKeepsNonEmpty)
{
    EXPECT_EQ(std::vector<std::string>({"one", "two"}),
              to_strings(cxxkit::str_split("one--two", cxxkit::by_string("--"), cxxkit::skip_empty())));
}

TEST(StrSplitTest, AllowEmptyPreservesTokens)
{
    EXPECT_EQ(std::vector<std::string>({"a", "", "b"}),
              to_strings(cxxkit::str_split("a,,b", ',', cxxkit::allow_empty())));
    EXPECT_EQ(std::vector<std::string>({"", "a", "b", ""}),
              to_strings(cxxkit::str_split("--a--b--", cxxkit::by_string("--"), cxxkit::allow_empty())));
    EXPECT_EQ(std::vector<std::string>({"a", "", "b"}),
              to_strings(cxxkit::str_split("a,,b", cxxkit::by_any_of(","), cxxkit::allow_empty())));
}

TEST(StrSplitTest, TokensReferenceSourceBuffer)
{
    // Non-owning contract: result elements point into the source buffer.
    char buffer[] = "x,y";
    const std::vector<StringView> tokens = cxxkit::str_split(buffer, ',');
    ASSERT_EQ(2u, tokens.size());
    EXPECT_EQ(tokens[0].data(), buffer + 0);
    EXPECT_EQ(tokens[1].data(), buffer + 2);
    EXPECT_EQ("x", std::string(tokens[0]));
    EXPECT_EQ("y", std::string(tokens[1]));
    buffer[0] = 'z'; // views see the mutation
    EXPECT_EQ("z", std::string(tokens[0]));
}

TEST(StrSplitTest, SplitByStringDelimiterIsView)
{
    // The delimiter itself is a non-owning view: it must outlive the call, the tokens reference `text`.
    const char *text_buf = "p--q";
    const std::vector<StringView> tokens = cxxkit::str_split(StringView(text_buf), cxxkit::by_string("--"));
    EXPECT_EQ(std::vector<std::string>({"p", "q"}), to_strings(tokens));
}
