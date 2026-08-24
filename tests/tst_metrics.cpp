// cxxkit::tools metrics tests — coverage for metrics.cpp (histogram factories/add/reset).
#include <cxxkit/tools/metrics.hpp>

#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace cxxkit;

TEST(Metrics, HistogramCounts)
{
    metrics::Enable();
    metrics::Histogram *hist = metrics::HistogramFactoryGetCounts("tst_counts", 0, 100, 10);
    ASSERT_TRUE(hist != nullptr);
    metrics::HistogramAdd(hist, 5);
    metrics::HistogramAdd(hist, 95);

    std::map<std::string, std::unique_ptr<metrics::SampleInfo>, cxxkit::StringViewCmp> histograms;
    metrics::GetAndReset(&histograms);
    EXPECT_FALSE(histograms.empty());
}

TEST(Metrics, HistogramCountsLinear)
{
    metrics::Histogram *hist = metrics::HistogramFactoryGetCountsLinear("tst_linear", 0, 100, 10);
    ASSERT_TRUE(hist != nullptr);
    metrics::HistogramAdd(hist, 10);
    metrics::Reset();
}

TEST(Metrics, Enumeration)
{
    metrics::Histogram *hist = metrics::HistogramFactoryGetEnumeration("tst_enum", 5);
    ASSERT_TRUE(hist != nullptr);
    metrics::HistogramAdd(hist, 3);

    metrics::Histogram *sparse = metrics::SparseHistogramFactoryGetEnumeration("tst_sparse", 8);
    ASSERT_TRUE(sparse != nullptr);
    metrics::HistogramAdd(sparse, 1);

    std::map<std::string, std::unique_ptr<metrics::SampleInfo>, cxxkit::StringViewCmp> histograms;
    metrics::GetAndReset(&histograms);
    EXPECT_FALSE(histograms.empty());
}