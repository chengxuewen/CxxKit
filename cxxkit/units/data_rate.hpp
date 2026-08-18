/***********************************************************************************************************************
**
** Library: cxxkit
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

#ifndef _CXXKIT_DATA_RATE_HPP
#define _CXXKIT_DATA_RATE_HPP

#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/units/data_size.hpp>
#include <cxxkit/units/frequency.hpp>

#include <cstdint>
#include <limits>
#include <string>

CXXKIT_BEGIN_NAMESPACE

// DataRate is a class that represents a given data rate. This can be used to
// represent bandwidth, encoding bitrate, etc. The internal storage is bits per
// second (bps).
class DataRate final : public RelativeUnit<DataRate>
{
public:
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR DataRate BitsPerSec(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromValue(value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR DataRate BytesPerSec(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(8, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR DataRate KilobitsPerSec(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000, value);
    }
    static constexpr DataRate Infinity() { return PlusInfinity(); }

    constexpr DataRate() = default;

    template <typename Sink>
    friend void AbslStringify(Sink &sink, DataRate value);

    template <typename T = int64_t>
    constexpr T bps() const
    {
        return ToValue<T>();
    }
    template <typename T = int64_t>
    constexpr T bytes_per_sec() const
    {
        return ToFraction<8, T>();
    }
    template <typename T = int64_t>
    constexpr T kbps() const
    {
        return ToFraction<1000, T>();
    }
    constexpr int64_t bps_or(int64_t fallback_value) const { return ToValueOr(fallback_value); }
    constexpr int64_t kbps_or(int64_t fallback_value) const { return ToFractionOr<1000>(fallback_value); }

private:
    // Bits per second used internally to simplify debugging by making the value
    // more recognizable.
    friend class UnitBase<DataRate>;

    using RelativeUnit::RelativeUnit;
    static constexpr bool kOneSided = true;
};

namespace data_rate_impl
{
inline CXXKIT_CXX14_CONSTEXPR int64_t Microbits(const DataSize &size)
{
    constexpr int64_t kMaxBeforeConversion = std::numeric_limits<int64_t>::max() / 8000000;
    CXXKIT_DCHECK_LE(size.bytes(), kMaxBeforeConversion) << "size is too large to be expressed in microbits";
    return size.bytes() * 8000000;
}

inline CXXKIT_CXX14_CONSTEXPR int64_t MillibytePerSec(const DataRate &size)
{
    constexpr int64_t kMaxBeforeConversion = std::numeric_limits<int64_t>::max() / (1000 / 8);
    CXXKIT_DCHECK_LE(size.bps(), kMaxBeforeConversion) << "rate is too large to be expressed in microbytes per second";
    return size.bps() * (1000 / 8);
}
} // namespace data_rate_impl

inline CXXKIT_CXX14_CONSTEXPR DataRate operator/(const DataSize size, const TimeDelta duration)
{
    return DataRate::BitsPerSec(data_rate_impl::Microbits(size) / duration.us());
}
inline CXXKIT_CXX14_CONSTEXPR TimeDelta operator/(const DataSize size, const DataRate rate)
{
    return TimeDelta::Micros(data_rate_impl::Microbits(size) / rate.bps());
}
inline CXXKIT_CXX14_CONSTEXPR DataSize operator*(const DataRate rate, const TimeDelta duration)
{
    int64_t microbits = rate.bps() * duration.us();
    return DataSize::Bytes((microbits + 4000000) / 8000000);
}
inline constexpr DataSize operator*(const TimeDelta duration, const DataRate rate)
{
    return rate * duration;
}

inline CXXKIT_CXX14_CONSTEXPR DataSize operator/(const DataRate rate, const Frequency frequency)
{
    int64_t millihertz = frequency.millihertz<int64_t>();
    // Note that the value is truncated here reather than rounded, potentially
    // introducing an error of .5 bytes if rounding were expected.
    return DataSize::Bytes(data_rate_impl::MillibytePerSec(rate) / millihertz);
}
inline CXXKIT_CXX14_CONSTEXPR Frequency operator/(const DataRate rate, const DataSize size)
{
    return Frequency::MilliHertz(data_rate_impl::MillibytePerSec(rate) / size.bytes());
}
inline CXXKIT_CXX14_CONSTEXPR DataRate operator*(const DataSize size, const Frequency frequency)
{
    CXXKIT_DCHECK(frequency.IsZero() ||
                  size.bytes() <= std::numeric_limits<int64_t>::max() / 8 / frequency.millihertz<int64_t>());
    int64_t millibits_per_second = size.bytes() * 8 * frequency.millihertz<int64_t>();
    return DataRate::BitsPerSec((millibits_per_second + 500) / 1000);
}
inline constexpr DataRate operator*(const Frequency frequency, const DataSize size)
{
    return size * frequency;
}

namespace utils
{
CXXKIT_CORE_API std::string toString(DataRate value);

// template <typename Sink>
// void stringify(Sink &sink, DataRate value)
// {
//     sink.Append(toString(value));
// }
} // namespace utils

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_DATA_RATE_HPP
