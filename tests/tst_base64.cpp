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