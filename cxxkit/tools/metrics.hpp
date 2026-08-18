/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
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

#pragma once

#include <cxxkit/text/string_utils.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/tools/checks.hpp>

#include <cstddef>
#include <atomic>
#include <string>
#include <memory>
#include <map>

#if defined(CXXKIT_DISABLE_METRICS)
#    define CXXKIT_METRICS_ENABLED 0
#else
#    define CXXKIT_METRICS_ENABLED 1
#endif

CXXKIT_BEGIN_NAMESPACE

namespace metrics
{
namespace detail
{
template <typename... Ts>
void NoOp(const Ts &...)
{
}
} // namespace detail

// Time that should have elapsed for stats that are gathered once per call.
constexpr int kMinRunTimeInSeconds = 10;

class Histogram;

// Functions for getting pointer to histogram (constructs or finds the named
// histogram).

// Get histogram for counters.
Histogram *HistogramFactoryGetCounts(StringView name, int min, int max, int bucket_count);

// Get histogram for counters with linear bucket spacing.
Histogram *HistogramFactoryGetCountsLinear(StringView name, int min, int max, int bucket_count);

// Get histogram for enumerators.
// `boundary` should be above the max enumerator sample.
Histogram *HistogramFactoryGetEnumeration(StringView name, int boundary);

// Get sparse histogram for enumerators.
// `boundary` should be above the max enumerator sample.
Histogram *SparseHistogramFactoryGetEnumeration(StringView name, int boundary);

// Function for adding a `sample` to a histogram.
void HistogramAdd(Histogram *histogram_pointer, int sample);

struct SampleInfo
{
    SampleInfo(StringView name, int min, int max, size_t bucket_count);
    ~SampleInfo();

    const std::string name;
    const int min;
    const int max;
    const size_t bucket_count;
    std::map<int, int> samples; // <value, # of events>
};

// Enables collection of samples.
// This method should be called before any other call into webrtc.
void Enable();

// Gets histograms and clears all samples.
void GetAndReset(std::map<std::string, std::unique_ptr<SampleInfo>, StringViewCmp> *histograms);

// Functions below are mainly for testing.

// Clears all samples.
void Reset();

// Returns the number of times the `sample` has been added to the histogram.
int NumEvents(StringView name, int sample);

// Returns the total number of added samples to the histogram.
int NumSamples(StringView name);

// Returns the minimum sample value (or -1 if the histogram has no samples).
int MinSample(StringView name);

// Returns a map with keys the samples with at least one event and values the
// number of events for that sample.
std::map<int, int> Samples(StringView name);
} // namespace metrics

#if CXXKIT_METRICS_ENABLED
#    define CXXKIT_METRIC_EXPECT_EQ(val1, val2)               EXPECT_EQ(val1, val2)
#    define CXXKIT_METRIC_EXPECT_EQ_WAIT(val1, val2, timeout) EXPECT_EQ_WAIT(val1, val2, timeout)
#    define CXXKIT_METRIC_EXPECT_GT(val1, val2)               EXPECT_GT(val1, val2)
#    define CXXKIT_METRIC_EXPECT_LE(val1, val2)               EXPECT_LE(val1, val2)
#    define CXXKIT_METRIC_EXPECT_TRUE(conditon)               EXPECT_TRUE(conditon)
#    define CXXKIT_METRIC_EXPECT_FALSE(conditon)              EXPECT_FALSE(conditon)
#    define CXXKIT_METRIC_EXPECT_THAT(value, matcher)         EXPECT_THAT(value, matcher)
#else
#    define CXXKIT_METRIC_EXPECT_EQ(val1, val2)               cxxkit::metrics::detail::NoOp(val1, val2)
#    define CXXKIT_METRIC_EXPECT_EQ_WAIT(val1, val2, timeout) cxxkit::metrics::detail::NoOp(val1, val2, timeout)
#    define CXXKIT_METRIC_EXPECT_GT(val1, val2)               cxxkit::metrics::detail::NoOp(val1, val2)
#    define CXXKIT_METRIC_EXPECT_LE(val1, val2)               cxxkit::metrics::detail::NoOp(val1, val2)
#    define CXXKIT_METRIC_EXPECT_TRUE(condition)              cxxkit::metrics::detail::NoOp(condition || true)
#    define CXXKIT_METRIC_EXPECT_FALSE(condition)             cxxkit::metrics::detail::NoOp(condition && false)
#    define CXXKIT_METRIC_EXPECT_THAT(value, matcher)         cxxkit::metrics::detail::NoOp(value, testing::_)
#endif

#if CXXKIT_METRICS_ENABLED
// Macros for allowing apps (e.g. Chrome) to gather and aggregate statistics.
//
// Histogram for counters.
// CXXKIT_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count);
//
// Histogram for enumerators.
// The boundary should be above the max enumerator sample.
// CXXKIT_HISTOGRAM_ENUMERATION(name, sample, boundary);
//
//
// The macros use the methods HistogramFactoryGetCounts,
// HistogramFactoryGetEnumeration and HistogramAdd.
//
// By default kit provides implementations of the aforementioned methods
// that can be found in system_wrappers/source/metrics.cc. If clients want to
// provide a custom version, they will have to:
//
// 1. Compile kit defining the preprocessor macro
//    WEBOCTK_EXCLUDE_METRICS_DEFAULT (if GN is used this can be achieved
//    by setting the GN arg rtc_exclude_metrics_default to true).
// 2. Provide implementations of:
//    Histogram* cxxkit::metrics::HistogramFactoryGetCounts(StringView name, int sample, int min, int max, int bucket_count);
//    Histogram* cxxkit::metrics::HistogramFactoryGetEnumeration(StringView name, int sample, int boundary);
//    void cxxkit::metrics::HistogramAdd(Histogram* histogram_pointer, StringView name, int sample);
//
// Example usage:
//
// CXXKIT_HISTOGRAM_COUNTS("RTC.Video.NacksSent", nacks_sent, 1, 100000, 100);
//
// enum Types {
//   kTypeX,
//   kTypeY,
//   kBoundary,
// };
//
// CXXKIT_HISTOGRAM_ENUMERATION("RTC.Types", kTypeX, kBoundary);
//
// NOTE: It is recommended to do the Chromium review for modifications to
// histograms.xml before new metrics are committed to WebRTC.

// Macros for adding samples to a named histogram.

// Histogram for counters (exponentially spaced buckets).
#    define CXXKIT_HISTOGRAM_COUNTS_100(name, sample)    CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 100, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_200(name, sample)    CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 200, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_500(name, sample)    CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 500, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_1000(name, sample)   CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 1000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_10000(name, sample)  CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 10000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_100000(name, sample) CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 100000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_1M(name, sample)     CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 1000000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_1G(name, sample)     CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 1000000000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count)                                                \
        CXXKIT_HISTOGRAM_COMMON_BLOCK(name,                                                                              \
                                    sample,                                                                            \
                                    cxxkit::metrics::HistogramFactoryGetCounts(name, min, max, bucket_count))

#    define CXXKIT_HISTOGRAM_COUNTS_LINEAR(name, sample, min, max, bucket_count)                                         \
        CXXKIT_HISTOGRAM_COMMON_BLOCK(name,                                                                              \
                                    sample,                                                                            \
                                    cxxkit::metrics::HistogramFactoryGetCountsLinear(name, min, max, bucket_count))

// Slow metrics: pointer to metric is acquired at each call and is not cached.
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_100(name, sample)    CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 100, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_200(name, sample)    CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 200, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_500(name, sample)    CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 500, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_1000(name, sample)   CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 1000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_10000(name, sample)  CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 10000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_100000(name, sample) CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 100000, 50)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, min, max, bucket_count)                                         \
        CXXKIT_HISTOGRAM_COMMON_BLOCK_SLOW(name,                                                                         \
                                         sample,                                                                       \
                                         cxxkit::metrics::HistogramFactoryGetCounts(name, min, max, bucket_count))

// Histogram for percentage (evenly spaced buckets).
#    define CXXKIT_HISTOGRAM_PERCENTAGE_SPARSE(name, sample) CXXKIT_HISTOGRAM_ENUMERATION_SPARSE(name, sample, 101)

// Histogram for booleans.
#    define CXXKIT_HISTOGRAM_BOOLEAN_SPARSE(name, sample) CXXKIT_HISTOGRAM_ENUMERATION_SPARSE(name, sample, 2)

// Histogram for enumerators (evenly spaced buckets).
// `boundary` should be above the max enumerator sample.
//
// TODO(qingsi): Refactor the default implementation given by RtcHistogram,
// which is already sparse, and remove the boundary argument from the macro.
#    define CXXKIT_HISTOGRAM_ENUMERATION_SPARSE(name, sample, boundary)                                                  \
        CXXKIT_HISTOGRAM_COMMON_BLOCK_SLOW(name,                                                                         \
                                         sample,                                                                       \
                                         cxxkit::metrics::SparseHistogramFactoryGetEnumeration(name, boundary))

// Histogram for percentage (evenly spaced buckets).
#    define CXXKIT_HISTOGRAM_PERCENTAGE(name, sample) CXXKIT_HISTOGRAM_ENUMERATION(name, sample, 101)

// Histogram for booleans.
#    define CXXKIT_HISTOGRAM_BOOLEAN(name, sample) CXXKIT_HISTOGRAM_ENUMERATION(name, sample, 2)

// Histogram for enumerators (evenly spaced buckets).
// `boundary` should be above the max enumerator sample.
#    define CXXKIT_HISTOGRAM_ENUMERATION(name, sample, boundary)                                                         \
        CXXKIT_HISTOGRAM_COMMON_BLOCK_SLOW(name, sample, cxxkit::metrics::HistogramFactoryGetEnumeration(name, boundary))

// The name of the histogram should not vary.
#    define CXXKIT_HISTOGRAM_COMMON_BLOCK(constant_name, sample, factory_get_invocation)                                 \
        do                                                                                                             \
        {                                                                                                              \
            static std::atomic<cxxkit::metrics::Histogram *> atomic_histogram_pointer(nullptr);                          \
            cxxkit::metrics::Histogram *histogram_pointer = atomic_histogram_pointer.load(std::memory_order_acquire);    \
            if (!histogram_pointer)                                                                                    \
            {                                                                                                          \
                histogram_pointer = factory_get_invocation;                                                            \
                cxxkit::metrics::Histogram *null_histogram = nullptr;                                                    \
                atomic_histogram_pointer.compare_exchange_strong(null_histogram, histogram_pointer);                   \
            }                                                                                                          \
            if (histogram_pointer)                                                                                     \
            {                                                                                                          \
                cxxkit::metrics::HistogramAdd(histogram_pointer, sample);                                                \
            }                                                                                                          \
        } while (0)

// The histogram is constructed/found for each call.
// May be used for histograms with infrequent updates.`
#    define CXXKIT_HISTOGRAM_COMMON_BLOCK_SLOW(name, sample, factory_get_invocation)                                     \
        do                                                                                                             \
        {                                                                                                              \
            cxxkit::metrics::Histogram *histogram_pointer = factory_get_invocation;                                      \
            if (histogram_pointer)                                                                                     \
            {                                                                                                          \
                cxxkit::metrics::HistogramAdd(histogram_pointer, sample);                                                \
            }                                                                                                          \
        } while (0)

// Helper macros.
// Macros for calling a histogram with varying name (e.g. when using a metric
// in different modes such as real-time vs screenshare). Fast, because pointer
// is cached. `index` should be different for different names. Allowed `index`
// values are 0, 1, and 2.
#    define CXXKIT_HISTOGRAMS_COUNTS_100(index, name, sample)                                                            \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 100, 50))

#    define CXXKIT_HISTOGRAMS_COUNTS_200(index, name, sample)                                                            \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 200, 50))

#    define CXXKIT_HISTOGRAMS_COUNTS_500(index, name, sample)                                                            \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 500, 50))

#    define CXXKIT_HISTOGRAMS_COUNTS_1000(index, name, sample)                                                           \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 1000, 50))

#    define CXXKIT_HISTOGRAMS_COUNTS_10000(index, name, sample)                                                          \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 10000, 50))

#    define CXXKIT_HISTOGRAMS_COUNTS_100000(index, name, sample)                                                         \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_COUNTS(name, sample, 1, 100000, 50))

#    define CXXKIT_HISTOGRAMS_ENUMERATION(index, name, sample, boundary)                                                 \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_ENUMERATION(name, sample, boundary))

#    define CXXKIT_HISTOGRAMS_PERCENTAGE(index, name, sample)                                                            \
        CXXKIT_HISTOGRAMS_COMMON(index, name, sample, CXXKIT_HISTOGRAM_PERCENTAGE(name, sample))

#    define CXXKIT_HISTOGRAMS_COMMON(index, name, sample, macro_invocation)                                              \
        do                                                                                                             \
        {                                                                                                              \
            switch (index)                                                                                             \
            {                                                                                                          \
                case 0: macro_invocation; break;                                                                       \
                case 1: macro_invocation; break;                                                                       \
                case 2: macro_invocation; break;                                                                       \
                default: CXXKIT_DCHECK_NOTREACHED();                                                                     \
            }                                                                                                          \
        } while (0)

#else

// This section defines no-op alternatives to the metrics macros when CXXKIT_METRICS_ENABLED is defined.
#    define CXXKIT_HISTOGRAM_COUNTS_100(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_200(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_500(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_1000(name, sample)   cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_10000(name, sample)  cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_100000(name, sample) cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_1M(name, sample)     cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_1G(name, sample)     cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count)                                                \
        cxxkit::metrics::detail::NoOp(name, sample, min, max, bucket_count)
#    define CXXKIT_HISTOGRAM_COUNTS_LINEAR(name, sample, min, max, bucket_count)                                         \
        cxxkit::metrics::detail::NoOp(name, sample, min, max, bucket_count)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_100(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_200(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_500(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_1000(name, sample)   cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_10000(name, sample)  cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE_100000(name, sample) cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_COUNTS_SPARSE(name, sample, min, max, bucket_count)                                         \
        cxxkit::metrics::detail::NoOp(name, sample, min, max, bucket_count)
#    define CXXKIT_HISTOGRAM_PERCENTAGE_SPARSE(name, sample) cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_BOOLEAN_SPARSE(name, sample)    cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_ENUMERATION_SPARSE(name, sample, boundary)                                                  \
        cxxkit::metrics::detail::NoOp(name, sample, boundary)
#    define CXXKIT_HISTOGRAM_PERCENTAGE(name, sample)            cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_BOOLEAN(name, sample)               cxxkit::metrics::detail::NoOp(name, sample)
#    define CXXKIT_HISTOGRAM_ENUMERATION(name, sample, boundary) cxxkit::metrics::detail::NoOp(name, sample, boundary)
#    define CXXKIT_HISTOGRAM_COMMON_BLOCK(constant_name, sample, factory_get_invocation)                                 \
        cxxkit::metrics::detail::NoOp(constant_name, sample, factory_get_invocation)
#    define CXXKIT_HISTOGRAM_COMMON_BLOCK_SLOW(name, sample, factory_get_invocation)                                     \
        cxxkit::metrics::detail::NoOp(name, sample, factory_get_invocation)
#    define CXXKIT_HISTOGRAMS_COUNTS_100(index, name, sample)    cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_COUNTS_200(index, name, sample)    cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_COUNTS_500(index, name, sample)    cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_COUNTS_1000(index, name, sample)   cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_COUNTS_10000(index, name, sample)  cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_COUNTS_100000(index, name, sample) cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_ENUMERATION(index, name, sample, boundary)                                                 \
        cxxkit::metrics::detail::NoOp(index, name, sample, boundary)
#    define CXXKIT_HISTOGRAMS_PERCENTAGE(index, name, sample) cxxkit::metrics::detail::NoOp(index, name, sample)
#    define CXXKIT_HISTOGRAMS_COMMON(index, name, sample, macro_invocation)                                              \
        cxxkit::metrics::detail::NoOp(index, name, sample, macro_invocation)
#endif // CXXKIT_METRICS_ENABLED

CXXKIT_END_NAMESPACE