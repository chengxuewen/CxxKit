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

#ifndef _CXXKIT_FREQUENCY_HPP
#define _CXXKIT_FREQUENCY_HPP

#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/tools/checks.hpp>

#include <type_traits>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>

CXXKIT_BEGIN_NAMESPACE

class Frequency final : public RelativeUnit<Frequency>
{
public:
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Frequency MilliHertz(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromValue(value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Frequency Hertz(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Frequency KiloHertz(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000000, value);
    }

    constexpr Frequency() = default;

    template <typename Sink>
    friend void AbslStringify(Sink &sink, Frequency value);

    template <typename T = int64_t>
    constexpr T hertz() const
    {
        return ToFraction<1000, T>();
    }
    template <typename T = int64_t>
    constexpr T millihertz() const
    {
        return ToValue<T>();
    }

private:
    friend class UnitBase<Frequency>;

    using RelativeUnit::RelativeUnit;
    static constexpr bool kOneSided = true;
};

inline CXXKIT_CXX14_CONSTEXPR Frequency operator/(int64_t nominator,
                                                const TimeDelta &interval)
{
    constexpr int64_t kKiloPerMicro = 1000 * 1000000;
    CXXKIT_DCHECK_LE(nominator, std::numeric_limits<int64_t>::max() / kKiloPerMicro);
    CXXKIT_CHECK(interval.IsFinite());
    CXXKIT_CHECK(!interval.IsZero());
    return Frequency::MilliHertz(nominator * kKiloPerMicro / interval.us());
}

inline CXXKIT_CXX14_CONSTEXPR TimeDelta operator/(int64_t nominator,
                                                const Frequency &frequency)
{
    constexpr int64_t kMegaPerMilli = 1000000 * 1000;
    CXXKIT_DCHECK_LE(nominator, std::numeric_limits<int64_t>::max() / kMegaPerMilli);
    CXXKIT_CHECK(frequency.IsFinite());
    CXXKIT_CHECK(!frequency.IsZero());
    return TimeDelta::Micros(nominator * kMegaPerMilli / frequency.millihertz());
}

inline constexpr double operator*(Frequency frequency, TimeDelta time_delta)
{
    return frequency.hertz<double>() * time_delta.seconds<double>();
}
inline constexpr double operator*(TimeDelta time_delta, Frequency frequency)
{
    return frequency * time_delta;
}

CXXKIT_CORE_API std::string toString(Frequency value);

template <typename Sink>
void AbslStringify(Sink &sink, Frequency value)
{
    sink.Append(toString(value));
}
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_FREQUENCY_HPP
