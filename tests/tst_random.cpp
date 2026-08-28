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

// cxxkit::tools Random tests — coverage for random.cpp (seedable RNG, distributions).
#include <cxxkit/tools/random.hpp>

#include <gtest/gtest.h>

using namespace cxxkit;

TEST(Random, DeterministicSeed)
{
    Random r1(12345);
    Random r2(12345);
    EXPECT_EQ(r1.Rand(1000), r2.Rand(1000));
    EXPECT_EQ(r1.Rand(10, 20), r2.Rand(10, 20));
}

TEST(Random, RandRange)
{
    Random rng(42);
    for (int i = 0; i < 100; ++i)
    {
        const uint32_t v = rng.Rand(5);
        EXPECT_LE(v, 5u); // Rand(t) is uniform on [0, t]
        const uint32_t lo = rng.Rand(10, 20);
        EXPECT_GE(lo, 10u);
        EXPECT_LE(lo, 20u); // high bound inclusive
    }
}

TEST(Random, Distributions)
{
    Random rng(7);
    const double g = rng.Gaussian(0.0, 1.0);
    EXPECT_TRUE(g > -10.0 && g < 10.0);

    const double e = rng.Exponential(1.0);
    EXPECT_GE(e, 0.0);

    const double u = rng.Rand<double>();
    EXPECT_GE(u, 0.0);
    EXPECT_LT(u, 1.0);

    const float uf = rng.Rand<float>();
    EXPECT_GE(uf, 0.0f);
    EXPECT_LT(uf, 1.0f);

    const double again = rng.Rand<double>();
    EXPECT_GE(again, 0.0);
    EXPECT_LT(again, 1.0);
}
TEST(Random, RandInt32)
{
    Random rng(99);
    const int32_t lo = rng.Rand(-10, 10);
    EXPECT_GE(lo, -10);
    EXPECT_LE(lo, 10);
    const int32_t same = rng.Rand(7, 7);
    EXPECT_EQ(same, 7);
}

TEST(Random, RandBool)
{
    Random rng(3);
    bool sawTrue = false;
    bool sawFalse = false;
    for (int i = 0; i < 64; ++i)
    {
        if (rng.Rand<bool>())
            sawTrue = true;
        else
            sawFalse = true;
    }
    EXPECT_TRUE(sawTrue);
    EXPECT_TRUE(sawFalse);
}

TEST(Random, UtilsCreateRandomString)
{
    const std::string s = utils::CreateRandomString(16);
    EXPECT_EQ(s.size(), 16u);

    std::string out;
    EXPECT_TRUE(utils::CreateRandomString(8, &out));
    EXPECT_EQ(out.size(), 8u);

    std::string t;
    EXPECT_TRUE(utils::CreateRandomString(8, StringView("ab"), &t));
    EXPECT_EQ(t.size(), 8u);
    for (char c : t)
    {
        EXPECT_TRUE(c == 'a' || c == 'b');
    }
}

TEST(Random, UtilsSeedInit)
{
    EXPECT_TRUE(utils::InitRandom(42));
    // Deterministic re-seeding must be repeatable.
    std::string a = utils::CreateRandomString(8);
    utils::InitRandom(42);
    std::string b = utils::CreateRandomString(8);
    EXPECT_EQ(a, b);

    utils::SetRandomTestMode(true);
    utils::SetDefaultRandomGenerator();
}
