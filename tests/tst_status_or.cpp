#include <cxxkit/tools/status_or.hpp>

#include <gtest/gtest.h>

#include <string>

TEST(StatusOr, OkValue)
{
    cxxkit::StatusOr<int> sor = 42;
    EXPECT_TRUE(sor.ok());
    EXPECT_EQ(sor.value(), 42);
}

TEST(StatusOr, ErrorStatus)
{
    cxxkit::StatusOr<int> sor = cxxkit::Status("not found");
    EXPECT_FALSE(sor.ok());
    EXPECT_EQ(sor.status().errorMessage(), "not found");
}

TEST(StatusOr, ValueOr)
{
    cxxkit::StatusOr<int> ok = 42;
    cxxkit::StatusOr<int> err = cxxkit::Status("error");
    EXPECT_EQ(ok.value_or(0), 42);
    EXPECT_EQ(err.value_or(0), 0);
}

TEST(StatusOr, OrDie)
{
    cxxkit::StatusOr<int> ok = 42;
    EXPECT_EQ(ok.OrDie(), 42);
    // err.OrDie() would trigger CXXKIT_CHECK failure (fatal)
}

TEST(StatusOr, MakeStatusOr)
{
    auto sor = cxxkit::MakeStatusOr(42);
    EXPECT_TRUE(sor.ok());
    EXPECT_EQ(sor.value(), 42);
}

TEST(StatusOr, CopyConstruct)
{
    cxxkit::StatusOr<int> a = 42;
    cxxkit::StatusOr<int> b = a;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), 42);
}

TEST(StatusOr, MoveConstruct)
{
    cxxkit::StatusOr<std::string> a = std::string("hello");
    cxxkit::StatusOr<std::string> b = std::move(a);
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), "hello");
}

TEST(StatusOr, CopyAssign)
{
    cxxkit::StatusOr<int> a = 42;
    cxxkit::StatusOr<int> b = cxxkit::Status("error");
    b = a;
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), 42);
}

TEST(StatusOr, MoveAssign)
{
    cxxkit::StatusOr<std::string> a = std::string("world");
    cxxkit::StatusOr<std::string> b = cxxkit::Status("error");
    b = std::move(a);
    EXPECT_TRUE(b.ok());
    EXPECT_EQ(b.value(), "world");
}

TEST(StatusOr, CopyConstructError)
{
    cxxkit::StatusOr<int> a = cxxkit::Status("original error");
    cxxkit::StatusOr<int> b = a;
    EXPECT_FALSE(b.ok());
    EXPECT_EQ(b.status().errorMessage(), "original error");
}
