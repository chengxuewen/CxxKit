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

#include <cxxkit/tools/metrics.hpp>
#include <cxxkit/thread/mutex.hpp>

#include <algorithm>

CXXKIT_BEGIN_NAMESPACE

namespace metrics
{
class Histogram;

namespace
{
// Limit for the maximum number of sample values that can be stored.
// TODO(asapersson): Consider using bucket count (and set up
// linearly/exponentially spaced buckets) if samples are logged more frequently.
const int kMaxSampleMapSize = 300;

class RtcHistogram
{
public:
    RtcHistogram(StringView name, int min, int max, int bucket_count)
        : mMin(min)
        , mMax(max)
        , info_(name, min, max, bucket_count)
    {
        CXXKIT_DCHECK_GT(bucket_count, 0);
    }

    RtcHistogram(const RtcHistogram &) = delete;
    RtcHistogram &operator=(const RtcHistogram &) = delete;

    void add(int sample)
    {
        sample = std::min(sample, mMax);
        sample = std::max(sample, mMin - 1); // Underflow bucket.

        Mutex::Lock lock(mMutex);
        if (info_.samples.size() == kMaxSampleMapSize && info_.samples.find(sample) == info_.samples.end())
        {
            return;
        }
        ++info_.samples[sample];
    }

    // Returns a copy (or nullptr if there are no samples) and clears samples.
    std::unique_ptr<SampleInfo> get_and_reset()
    {
        Mutex::Lock lock(mMutex);
        if (info_.samples.empty())
        {
            return nullptr;
        }

        SampleInfo *copy = new SampleInfo(info_.name, info_.min, info_.max, info_.bucket_count);

        std::swap(info_.samples, copy->samples);

        return std::unique_ptr<SampleInfo>(copy);
    }

    const std::string &name() const { return info_.name; }

    // Functions only for testing.
    void reset()
    {
        Mutex::Lock lock(mMutex);
        info_.samples.clear();
    }

    int num_events(int sample) const
    {
        Mutex::Lock lock(mMutex);
        const auto it = info_.samples.find(sample);
        return (it == info_.samples.end()) ? 0 : it->second;
    }

    int num_samples() const
    {
        int num_samples = 0;
        Mutex::Lock lock(mMutex);
        for (const auto &sample : info_.samples)
        {
            num_samples += sample.second;
        }
        return num_samples;
    }

    int min_sample() const
    {
        Mutex::Lock lock(mMutex);
        return (info_.samples.empty()) ? -1 : info_.samples.begin()->first;
    }

    std::map<int, int> samples() const
    {
        Mutex::Lock lock(mMutex);
        return info_.samples;
    }

private:
    const int mMin;
    const int mMax;
    mutable Mutex mMutex;
    SampleInfo info_ CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex);
};

class RtcHistogramMap
{
public:
    RtcHistogramMap() { }
    ~RtcHistogramMap() { }

    RtcHistogramMap(const RtcHistogramMap &) = delete;
    RtcHistogramMap &operator=(const RtcHistogramMap &) = delete;

    Histogram *get_counts_histogram(StringView name, int min, int max, int bucket_count)
    {
        Mutex::Lock lock(mMutex);
        const auto &it = map_.find(name.data());
        if (it != map_.end())
        {
            return reinterpret_cast<Histogram *>(it->second.get());
        }

        RtcHistogram *hist = new RtcHistogram(name, min, max, bucket_count);
        map_.emplace(name, hist);
        return reinterpret_cast<Histogram *>(hist);
    }

    Histogram *get_enumeration_histogram(StringView name, int boundary)
    {
        Mutex::Lock lock(mMutex);
        const auto &it = map_.find(name.data());
        if (it != map_.end())
        {
            return reinterpret_cast<Histogram *>(it->second.get());
        }

        RtcHistogram *hist = new RtcHistogram(name, 1, boundary, boundary + 1);
        map_.emplace(name, hist);
        return reinterpret_cast<Histogram *>(hist);
    }

    void get_and_reset(std::map<std::string, std::unique_ptr<SampleInfo>, StringViewCmp> *histograms)
    {
        Mutex::Lock lock(mMutex);
        for (const auto &kv : map_)
        {
            std::unique_ptr<SampleInfo> info = kv.second->get_and_reset();
            if (info)
            {
                histograms->insert(std::make_pair(kv.first, std::move(info)));
            }
        }
    }

    // Functions only for testing.
    void reset()
    {
        Mutex::Lock lock(mMutex);
        for (const auto &kv : map_)
        {
            kv.second->reset();
        }
    }

    int num_events(StringView name, int sample) const
    {
        Mutex::Lock lock(mMutex);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? 0 : it->second->num_events(sample);
    }

    int num_samples(StringView name) const
    {
        Mutex::Lock lock(mMutex);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? 0 : it->second->num_samples();
    }

    int min_sample(StringView name) const
    {
        Mutex::Lock lock(mMutex);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? -1 : it->second->min_sample();
    }

    std::map<int, int> samples(StringView name) const
    {
        Mutex::Lock lock(mMutex);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? std::map<int, int>() : it->second->samples();
    }

private:
    mutable Mutex mMutex;
    std::map<std::string, std::unique_ptr<RtcHistogram>, StringViewCmp> map_ CXXKIT_ATTRIBUTE_GUARDED_BY(mMutex);
};

// RtcHistogramMap is allocated upon call to enable().
// The histogram getter functions, which return pointer values to the histograms
// in the map, are cached in WebRTC. Therefore, this memory is not freed by the
// application (the memory will be reclaimed by the OS).
static std::atomic<RtcHistogramMap *> g_rtc_histogram_map(nullptr);

void create_map()
{
    RtcHistogramMap *map = g_rtc_histogram_map.load(std::memory_order_acquire);
    if (map == nullptr)
    {
        RtcHistogramMap *new_map = new RtcHistogramMap();
        if (!g_rtc_histogram_map.compare_exchange_strong(map, new_map))
        {
            delete new_map;
        }
    }
}

// set the first time we start using histograms. Used to make sure enable() is
// not called thereafter.
#if CXXKIT_DCHECK_IS_ON
static std::atomic<int> g_rtc_histogram_called(0);
#endif

// Gets the map (or nullptr).
RtcHistogramMap *get_map()
{
#if CXXKIT_DCHECK_IS_ON
    g_rtc_histogram_called.store(1, std::memory_order_release);
#endif
    return g_rtc_histogram_map.load();
}
} // namespace

#ifndef CXXKIT_EXCLUDE_METRICS_DEFAULT
// Implementation of histogram methods in
// webrtc/system_wrappers/interface/metrics.h.

// Histogram with exponentially spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
Histogram *histogram_factory_get_counts(StringView name, int min, int max, int bucket_count)
{
    // TODO(asapersson): Alternative implementation will be needed if this
    // histogram type should be truly exponential.
    return histogram_factory_get_counts_linear(name, min, max, bucket_count);
}

// Histogram with linearly spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
Histogram *histogram_factory_get_counts_linear(StringView name, int min, int max, int bucket_count)
{
    RtcHistogramMap *map = get_map();
    if (!map)
    {
        return nullptr;
    }

    return map->get_counts_histogram(name, min, max, bucket_count);
}

// Histogram with linearly spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
Histogram *histogram_factory_get_enumeration(StringView name, int boundary)
{
    RtcHistogramMap *map = get_map();
    if (!map)
    {
        return nullptr;
    }

    return map->get_enumeration_histogram(name, boundary);
}

// Our default implementation reuses the non-sparse histogram.
Histogram *sparse_histogram_factory_get_enumeration(StringView name, int boundary)
{
    return histogram_factory_get_enumeration(name, boundary);
}

// Fast path. Adds `sample` to cached `histogram_pointer`.
void histogram_add(Histogram *histogram_pointer, int sample)
{
    RtcHistogram *ptr = reinterpret_cast<RtcHistogram *>(histogram_pointer);
    ptr->add(sample);
}

#endif // CXXKIT_EXCLUDE_METRICS_DEFAULT

SampleInfo::SampleInfo(StringView name, int min, int max, size_t bucket_count)
    : name(name)
    , min(min)
    , max(max)
    , bucket_count(bucket_count)
{
}

SampleInfo::~SampleInfo()
{
}

// Implementation of global functions in metrics.h.
void enable()
{
    CXXKIT_DCHECK(g_rtc_histogram_map.load() == nullptr);
#if CXXKIT_DCHECK_IS_ON
    CXXKIT_DCHECK_EQ(0, g_rtc_histogram_called.load(std::memory_order_acquire));
#endif
    create_map();
}

void get_and_reset(std::map<std::string, std::unique_ptr<SampleInfo>, StringViewCmp> *histograms)
{
    histograms->clear();
    RtcHistogramMap *map = get_map();
    if (map)
    {
        map->get_and_reset(histograms);
    }
}

void reset()
{
    RtcHistogramMap *map = get_map();
    if (map)
    {
        map->reset();
    }
}

int num_events(StringView name, int sample)
{
    RtcHistogramMap *map = get_map();
    return map ? map->num_events(name, sample) : 0;
}

int num_samples(StringView name)
{
    RtcHistogramMap *map = get_map();
    return map ? map->num_samples(name) : 0;
}

int min_sample(StringView name)
{
    RtcHistogramMap *map = get_map();
    return map ? map->min_sample(name) : -1;
}

std::map<int, int> samples(StringView name)
{
    RtcHistogramMap *map = get_map();
    return map ? map->samples(name) : std::map<int, int>();
}
} // namespace metrics

CXXKIT_END_NAMESPACE
