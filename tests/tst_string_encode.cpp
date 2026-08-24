// cxxkit::text string_encode tests — coverage for string_encode.cpp (hex/tokenize/FromString).
#include <cxxkit/text/string_encode.hpp>
#include <cxxkit/containers/array_view.hpp>

#include <gtest/gtest.h>

using namespace cxxkit;
using namespace cxxkit::utils;

TEST(StringEncode, HexEncodeDecode)
{
    std::string enc = hex_encode(StringView("AB"));
    EXPECT_EQ(enc, "4142");

    char out[16] = {0};
    ArrayView<char> view(out, sizeof(out));
    const size_t n = hex_decode(view, StringView("4142"));
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(out[0], 'A');
    EXPECT_EQ(out[1], 'B');
}

TEST(StringEncode, HexWithDelimiter)
{
    std::string enc = hex_encode_with_delimiter(StringView("AB"), ':');
    EXPECT_EQ(enc, "41:42");

    char out[16] = {0};
    ArrayView<char> view(out, sizeof(out));
    const size_t n = hex_decode_with_delimiter(view, StringView("41:42"), ':');
    EXPECT_EQ(n, 2u);
    EXPECT_EQ(out[0], 'A');
    EXPECT_EQ(out[1], 'B');
}

TEST(StringEncode, Tokenize)
{
    std::vector<std::string> fields;
    const size_t n = tokenize(StringView("a,b,c"), ',', &fields);
    EXPECT_EQ(n, 3u);
    ASSERT_EQ(fields.size(), 3u);
    EXPECT_EQ(fields[0], "a");
    EXPECT_EQ(fields[1], "b");
    EXPECT_EQ(fields[2], "c");
}

TEST(StringEncode, TokenizeFirst)
{
    std::string token, rest;
    EXPECT_TRUE(tokenize_first(StringView("a,b,c"), ',', &token, &rest));
    EXPECT_EQ(token, "a");
    EXPECT_EQ(rest, "b,c");
}

TEST(StringEncode, FromString)
{
    bool b = false;
    EXPECT_TRUE(FromString(StringView("true"), &b));
    EXPECT_TRUE(b);
    EXPECT_TRUE(FromString(StringView("false"), &b));
    EXPECT_FALSE(b);
}