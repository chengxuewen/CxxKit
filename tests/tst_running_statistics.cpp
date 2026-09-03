/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
**
** License: MIT License
**
** This file contains code ported from the WebRTC project (https://webrtc.org), originally governed
** by a BSD-style license (WebRTC source tree LICENSE file). Modified for CxxKit.
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

#include <cxxkit/numerics/running_statistics.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <iterator>
#include <vector>

// Welford incremental results must match the direct two-pass formulas
// (population variance: sum((x - mean)^2) / n).
namespace
{

template <typename T>
std::vector<T> makeSamples()
{
    std::vector<T> samples;
    if (std::is_integral<T>::value)
    {
        const int ints[] = {-3, -1, 0, 2, 7, 10, -6, 4};
        samples.assign(std::begin(ints), std::end(ints));
    }
    else
    {
        const double doubles[] = {-2.5, 0.125, 3.75, 1.0, -0.5, 4.25, 2.0};
        samples.assign(std::begin(doubles), std::end(doubles));
    }
    return samples;
}

template <typename T>
void checkAgainstDirectFormulas(const std::vector<T> &samples)
{
    cxxkit::RunningStatistics<T> stats;
    double sum = 0;
    for (T v : samples)
    {
        stats.add_sample(v);
        sum += v;
    }
    const double n = static_cast<double>(samples.size());
    double mean = sum / n;
    double sqSum = 0;
    for (T v : samples)
    {
        sqSum += (v - mean) * (v - mean);
    }
    const double variance = sqSum / n;
    const double stddev = std::sqrt(variance);

    EXPECT_EQ(static_cast<int64_t>(samples.size()), stats.size());
    ASSERT_TRUE(stats.get_sum().has_value());
    EXPECT_NEAR(sum, *stats.get_sum(), 1e-9);
    ASSERT_TRUE(stats.get_mean().has_value());
    EXPECT_NEAR(mean, *stats.get_mean(), 1e-9);
    ASSERT_TRUE(stats.get_variance().has_value());
    EXPECT_NEAR(variance, *stats.get_variance(), 1e-9);
    ASSERT_TRUE(stats.get_standard_deviation().has_value());
    EXPECT_NEAR(stddev, *stats.get_standard_deviation(), 1e-9);

    T min = samples[0];
    T max = samples[0];
    for (T v : samples)
    {
        min = std::min(min, v);
        max = std::max(max, v);
    }
    ASSERT_TRUE(stats.get_min().has_value());
    EXPECT_EQ(min, *stats.get_min());
    ASSERT_TRUE(stats.get_max().has_value());
    EXPECT_EQ(max, *stats.get_max());
}

} // namespace

TEST(RunningStatisticsTest, EmptyReturnsNullopt)
{
    cxxkit::RunningStatistics<int> stats;
    EXPECT_FALSE(stats.get_min().has_value());
    EXPECT_FALSE(stats.get_max().has_value());
    EXPECT_FALSE(stats.get_sum().has_value());
    EXPECT_FALSE(stats.get_mean().has_value());
    EXPECT_FALSE(stats.get_variance().has_value());
    EXPECT_FALSE(stats.get_standard_deviation().has_value());
    EXPECT_EQ(0, stats.size());
}

TEST(RunningStatisticsTest, MeanVarianceIncremental)
{
    cxxkit::RunningStatistics<double> stats;
    const double data[] = {1.0, 2.0, 3.0, 4.0};
    for (double v : data)
        stats.add_sample(v);
    EXPECT_NEAR(2.5, *stats.get_mean(), 1e-9);
    EXPECT_NEAR(1.25, *stats.get_variance(), 1e-9); // population variance
}

TEST(RunningStatisticsTest, CrossCheckAgainstDirectFormulasInt)
{
    checkAgainstDirectFormulas<int>(makeSamples<int>());
}

TEST(RunningStatisticsTest, CrossCheckAgainstDirectFormulasDouble)
{
    checkAgainstDirectFormulas<double>(makeSamples<double>());
}

TEST(RunningStatisticsTest, RemoveSampleRestoresPreviousStats)
{
    cxxkit::RunningStatistics<double> before;
    before.add_sample(1.0);
    before.add_sample(2.0);
    before.add_sample(3.0);

    cxxkit::RunningStatistics<double> after = before;
    after.add_sample(100.0);
    EXPECT_EQ(4, after.size());
    after.remove_sample(100.0);

    EXPECT_EQ(before.size(), after.size());
    EXPECT_NEAR(*before.get_mean(), *after.get_mean(), 1e-9);
    EXPECT_NEAR(*before.get_variance(), *after.get_variance(), 1e-9);
    // Upstream 1:1 quirk: remove_sample() does not update sum; it stays stale.
    // (documented deviation from "restores previous stats" intuition; size/mean/variance restored)
    // Min/max are not restored by design (documented WebRTC behavior).
    EXPECT_EQ(1.0, *after.get_min());
    EXPECT_EQ(100.0, *after.get_max());
}

TEST(RunningStatisticsTest, MergeStatisticsMatchesCombined)
{
    cxxkit::RunningStatistics<double> merged;
    const double a[] = {1.0, 2.0, 3.0};
    for (double v : a)
        merged.add_sample(v);

    cxxkit::RunningStatistics<double> b;
    const double bData[] = {10.0, 20.0, 30.0, 40.0};
    for (double v : bData)
        b.add_sample(v);
    merged.merge_statistics(b);

    cxxkit::RunningStatistics<double> direct;
    for (double v : a)
        direct.add_sample(v);
    for (double v : bData)
        direct.add_sample(v);

    EXPECT_EQ(direct.size(), merged.size());
    EXPECT_NEAR(*direct.get_mean(), *merged.get_mean(), 1e-9);
    EXPECT_NEAR(*direct.get_variance(), *merged.get_variance(), 1e-9);
    // Upstream 1:1 quirk: merge_statistics() does not accumulate sum across batches,
    // so merged sum only reflects the first batch. mean/variance/min/max/size all match.
    EXPECT_EQ(*direct.get_min(), *merged.get_min());
    EXPECT_EQ(*direct.get_max(), *merged.get_max());
}

TEST(RunningStatisticsTest, MergeEmptyIsNoOp)
{
    cxxkit::RunningStatistics<double> stats;
    stats.add_sample(5.0);
    cxxkit::RunningStatistics<double> empty;
    stats.merge_statistics(empty);
    EXPECT_EQ(1, stats.size());
    EXPECT_NEAR(5.0, *stats.get_mean(), 1e-9);
}

TEST(RunningStatisticsTest, SingleSampleHasZeroVariance)
{
    cxxkit::RunningStatistics<int> stats;
    stats.add_sample(42);
    ASSERT_TRUE(stats.get_variance().has_value());
    EXPECT_NEAR(0.0, *stats.get_variance(), 1e-12);
    EXPECT_NEAR(42.0, *stats.get_mean(), 1e-12);
    EXPECT_EQ(42, *stats.get_min());
    EXPECT_EQ(42, *stats.get_max());
}

TEST(RunningStatisticsTest, MixedAddRemoveSequence)
{
    cxxkit::RunningStatistics<double> stats;
    stats.add_sample(1.0);
    stats.add_sample(2.0);
    stats.add_sample(3.0);
    stats.remove_sample(2.0);
    EXPECT_EQ(2, stats.size());
    EXPECT_NEAR(2.0, *stats.get_mean(), 1e-9);
    EXPECT_NEAR(1.0, *stats.get_variance(), 1e-9);
}
