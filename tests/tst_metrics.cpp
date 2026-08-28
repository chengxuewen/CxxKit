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

// cxxkit::tools metrics tests — coverage for metrics.cpp (histogram factories/add/reset).
#include <cxxkit/tools/metrics.hpp>

#include <gtest/gtest.h>

#include <map>
#include <memory>

using namespace cxxkit;

TEST(Metrics, HistogramCounts)
{
    metrics::enable();
    metrics::Histogram *hist = metrics::histogram_factory_get_counts("tst_counts", 0, 100, 10);
    ASSERT_TRUE(hist != nullptr);
    metrics::histogram_add(hist, 5);
    metrics::histogram_add(hist, 95);

    std::map<std::string, std::unique_ptr<metrics::SampleInfo>, cxxkit::StringViewCmp> histograms;
    metrics::get_and_reset(&histograms);
    EXPECT_FALSE(histograms.empty());
}

TEST(Metrics, HistogramCountsLinear)
{
    metrics::Histogram *hist = metrics::histogram_factory_get_counts_linear("tst_linear", 0, 100, 10);
    ASSERT_TRUE(hist != nullptr);
    metrics::histogram_add(hist, 10);
    metrics::reset();
}

TEST(Metrics, Enumeration)
{
    metrics::Histogram *hist = metrics::histogram_factory_get_enumeration("tst_enum", 5);
    ASSERT_TRUE(hist != nullptr);
    metrics::histogram_add(hist, 3);

    metrics::Histogram *sparse = metrics::sparse_histogram_factory_get_enumeration("tst_sparse", 8);
    ASSERT_TRUE(sparse != nullptr);
    metrics::histogram_add(sparse, 1);

    std::map<std::string, std::unique_ptr<metrics::SampleInfo>, cxxkit::StringViewCmp> histograms;
    metrics::get_and_reset(&histograms);
    EXPECT_FALSE(histograms.empty());
}