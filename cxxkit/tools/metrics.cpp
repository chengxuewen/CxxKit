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
        : min_(min)
        , max_(max)
        , info_(name, min, max, bucket_count)
    {
        CXXKIT_DCHECK_GT(bucket_count, 0);
    }

    RtcHistogram(const RtcHistogram &) = delete;
    RtcHistogram &operator=(const RtcHistogram &) = delete;

    void Add(int sample)
    {
        sample = std::min(sample, max_);
        sample = std::max(sample, min_ - 1); // Underflow bucket.

        Mutex::Lock lock(mutex_);
        if (info_.samples.size() == kMaxSampleMapSize && info_.samples.find(sample) == info_.samples.end())
        {
            return;
        }
        ++info_.samples[sample];
    }

    // Returns a copy (or nullptr if there are no samples) and clears samples.
    std::unique_ptr<SampleInfo> GetAndReset()
    {
        Mutex::Lock lock(mutex_);
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
    void Reset()
    {
        Mutex::Lock lock(mutex_);
        info_.samples.clear();
    }

    int NumEvents(int sample) const
    {
        Mutex::Lock lock(mutex_);
        const auto it = info_.samples.find(sample);
        return (it == info_.samples.end()) ? 0 : it->second;
    }

    int NumSamples() const
    {
        int num_samples = 0;
        Mutex::Lock lock(mutex_);
        for (const auto &sample : info_.samples)
        {
            num_samples += sample.second;
        }
        return num_samples;
    }

    int MinSample() const
    {
        Mutex::Lock lock(mutex_);
        return (info_.samples.empty()) ? -1 : info_.samples.begin()->first;
    }

    std::map<int, int> Samples() const
    {
        Mutex::Lock lock(mutex_);
        return info_.samples;
    }

private:
    const int min_;
    const int max_;
    mutable Mutex mutex_;
    SampleInfo info_ CXXKIT_ATTRIBUTE_GUARDED_BY(mutex_);
};

class RtcHistogramMap
{
public:
    RtcHistogramMap() { }
    ~RtcHistogramMap() { }

    RtcHistogramMap(const RtcHistogramMap &) = delete;
    RtcHistogramMap &operator=(const RtcHistogramMap &) = delete;

    Histogram *GetCountsHistogram(StringView name, int min, int max, int bucket_count)
    {
        Mutex::Lock lock(mutex_);
        const auto &it = map_.find(name.data());
        if (it != map_.end())
        {
            return reinterpret_cast<Histogram *>(it->second.get());
        }

        RtcHistogram *hist = new RtcHistogram(name, min, max, bucket_count);
        map_.emplace(name, hist);
        return reinterpret_cast<Histogram *>(hist);
    }

    Histogram *GetEnumerationHistogram(StringView name, int boundary)
    {
        Mutex::Lock lock(mutex_);
        const auto &it = map_.find(name.data());
        if (it != map_.end())
        {
            return reinterpret_cast<Histogram *>(it->second.get());
        }

        RtcHistogram *hist = new RtcHistogram(name, 1, boundary, boundary + 1);
        map_.emplace(name, hist);
        return reinterpret_cast<Histogram *>(hist);
    }

    void GetAndReset(std::map<std::string, std::unique_ptr<SampleInfo>, StringViewCmp> *histograms)
    {
        Mutex::Lock lock(mutex_);
        for (const auto &kv : map_)
        {
            std::unique_ptr<SampleInfo> info = kv.second->GetAndReset();
            if (info)
            {
                histograms->insert(std::make_pair(kv.first, std::move(info)));
            }
        }
    }

    // Functions only for testing.
    void Reset()
    {
        Mutex::Lock lock(mutex_);
        for (const auto &kv : map_)
        {
            kv.second->Reset();
        }
    }

    int NumEvents(StringView name, int sample) const
    {
        Mutex::Lock lock(mutex_);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? 0 : it->second->NumEvents(sample);
    }

    int NumSamples(StringView name) const
    {
        Mutex::Lock lock(mutex_);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? 0 : it->second->NumSamples();
    }

    int MinSample(StringView name) const
    {
        Mutex::Lock lock(mutex_);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? -1 : it->second->MinSample();
    }

    std::map<int, int> Samples(StringView name) const
    {
        Mutex::Lock lock(mutex_);
        const auto &it = map_.find(name.data());
        return (it == map_.end()) ? std::map<int, int>() : it->second->Samples();
    }

private:
    mutable Mutex mutex_;
    std::map<std::string, std::unique_ptr<RtcHistogram>, StringViewCmp> map_ CXXKIT_ATTRIBUTE_GUARDED_BY(mutex_);
};

// RtcHistogramMap is allocated upon call to Enable().
// The histogram getter functions, which return pointer values to the histograms
// in the map, are cached in WebRTC. Therefore, this memory is not freed by the
// application (the memory will be reclaimed by the OS).
static std::atomic<RtcHistogramMap *> g_rtc_histogram_map(nullptr);

void CreateMap()
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

// Set the first time we start using histograms. Used to make sure Enable() is
// not called thereafter.
#if CXXKIT_DCHECK_IS_ON
static std::atomic<int> g_rtc_histogram_called(0);
#endif

// Gets the map (or nullptr).
RtcHistogramMap *GetMap()
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
Histogram *HistogramFactoryGetCounts(StringView name, int min, int max, int bucket_count)
{
    // TODO(asapersson): Alternative implementation will be needed if this
    // histogram type should be truly exponential.
    return HistogramFactoryGetCountsLinear(name, min, max, bucket_count);
}

// Histogram with linearly spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
Histogram *HistogramFactoryGetCountsLinear(StringView name, int min, int max, int bucket_count)
{
    RtcHistogramMap *map = GetMap();
    if (!map)
    {
        return nullptr;
    }

    return map->GetCountsHistogram(name, min, max, bucket_count);
}

// Histogram with linearly spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
Histogram *HistogramFactoryGetEnumeration(StringView name, int boundary)
{
    RtcHistogramMap *map = GetMap();
    if (!map)
    {
        return nullptr;
    }

    return map->GetEnumerationHistogram(name, boundary);
}

// Our default implementation reuses the non-sparse histogram.
Histogram *SparseHistogramFactoryGetEnumeration(StringView name, int boundary)
{
    return HistogramFactoryGetEnumeration(name, boundary);
}

// Fast path. Adds `sample` to cached `histogram_pointer`.
void HistogramAdd(Histogram *histogram_pointer, int sample)
{
    RtcHistogram *ptr = reinterpret_cast<RtcHistogram *>(histogram_pointer);
    ptr->Add(sample);
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
void Enable()
{
    CXXKIT_DCHECK(g_rtc_histogram_map.load() == nullptr);
#if CXXKIT_DCHECK_IS_ON
    CXXKIT_DCHECK_EQ(0, g_rtc_histogram_called.load(std::memory_order_acquire));
#endif
    CreateMap();
}

void GetAndReset(std::map<std::string, std::unique_ptr<SampleInfo>, StringViewCmp> *histograms)
{
    histograms->clear();
    RtcHistogramMap *map = GetMap();
    if (map)
    {
        map->GetAndReset(histograms);
    }
}

void Reset()
{
    RtcHistogramMap *map = GetMap();
    if (map)
    {
        map->Reset();
    }
}

int NumEvents(StringView name, int sample)
{
    RtcHistogramMap *map = GetMap();
    return map ? map->NumEvents(name, sample) : 0;
}

int NumSamples(StringView name)
{
    RtcHistogramMap *map = GetMap();
    return map ? map->NumSamples(name) : 0;
}

int MinSample(StringView name)
{
    RtcHistogramMap *map = GetMap();
    return map ? map->MinSample(name) : -1;
}

std::map<int, int> Samples(StringView name)
{
    RtcHistogramMap *map = GetMap();
    return map ? map->Samples(name) : std::map<int, int>();
}
} // namespace metrics

CXXKIT_END_NAMESPACE
