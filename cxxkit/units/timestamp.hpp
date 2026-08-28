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

#pragma once

#include <cxxkit/units/units_global.hpp>

#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/units/unit_base.hpp>
#include <cxxkit/tools/checks.hpp>

CXXKIT_BEGIN_NAMESPACE

// Timestamp represents the time that has passed since some unspecified epoch.
// The epoch is assumed to be before any represented timestamps, this means that
// negative values are not valid. The most notable feature is that the
// difference of two Timestamps results in a TimeDelta.
class Timestamp final : public UnitBase<Timestamp>
{
public:
    // static Timestamp nowSteadyTime();
    // static Timestamp nowSystemTime();
    // static Timestamp untilSteadyTime(const TimeDelta delta) { return nowSteadyTime() + delta; }
    // static Timestamp untilSystemTime(const TimeDelta delta) { return nowSystemTime() + delta; }

    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Timestamp Seconds(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Timestamp Millis(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Timestamp Micros(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromValue(value);
    }

    Timestamp() = delete;

    template <typename Sink>
    friend void AbslStringify(Sink &sink, Timestamp value);

    template <typename T = int64_t>
    constexpr T seconds() const
    {
        return ToFraction<1000000, T>();
    }
    template <typename T = int64_t>
    constexpr T ms() const
    {
        return ToFraction<1000, T>();
    }
    template <typename T = int64_t>
    constexpr T us() const
    {
        return ToValue<T>();
    }

    constexpr int64_t seconds_or(int64_t fallback_value) const { return ToFractionOr<1000000>(fallback_value); }
    constexpr int64_t ms_or(int64_t fallback_value) const { return ToFractionOr<1000>(fallback_value); }
    constexpr int64_t us_or(int64_t fallback_value) const { return ToValueOr(fallback_value); }

    CXXKIT_CXX14_CONSTEXPR Timestamp operator+(const TimeDelta delta) const
    {
        if (IsPlusInfinity() || delta.IsPlusInfinity())
        {
            CXXKIT_DCHECK(!IsMinusInfinity());
            CXXKIT_DCHECK(!delta.IsMinusInfinity());
            return PlusInfinity();
        }
        else if (IsMinusInfinity() || delta.IsMinusInfinity())
        {
            CXXKIT_DCHECK(!IsPlusInfinity());
            CXXKIT_DCHECK(!delta.IsPlusInfinity());
            return MinusInfinity();
        }
        return Timestamp::Micros(us() + delta.us());
    }
    CXXKIT_CXX14_CONSTEXPR Timestamp operator-(const TimeDelta delta) const
    {
        if (IsPlusInfinity() || delta.IsMinusInfinity())
        {
            CXXKIT_DCHECK(!IsMinusInfinity());
            CXXKIT_DCHECK(!delta.IsPlusInfinity());
            return PlusInfinity();
        }
        else if (IsMinusInfinity() || delta.IsPlusInfinity())
        {
            CXXKIT_DCHECK(!IsPlusInfinity());
            CXXKIT_DCHECK(!delta.IsMinusInfinity());
            return MinusInfinity();
        }
        return Timestamp::Micros(us() - delta.us());
    }
    CXXKIT_CXX14_CONSTEXPR TimeDelta operator-(const Timestamp other) const
    {
        if (IsPlusInfinity() || other.IsMinusInfinity())
        {
            CXXKIT_DCHECK(!IsMinusInfinity());
            CXXKIT_DCHECK(!other.IsPlusInfinity());
            return TimeDelta::PlusInfinity();
        }
        else if (IsMinusInfinity() || other.IsPlusInfinity())
        {
            CXXKIT_DCHECK(!IsPlusInfinity());
            CXXKIT_DCHECK(!other.IsMinusInfinity());
            return TimeDelta::MinusInfinity();
        }
        return TimeDelta::Micros(us() - other.us());
    }
    CXXKIT_CXX14_CONSTEXPR Timestamp &operator-=(const TimeDelta delta)
    {
        *this = *this - delta;
        return *this;
    }
    CXXKIT_CXX14_CONSTEXPR Timestamp &operator+=(const TimeDelta delta)
    {
        *this = *this + delta;
        return *this;
    }

private:
    friend class UnitBase<Timestamp>;
    using UnitBase::UnitBase;
    static constexpr bool kOneSided = true;
};

CXXKIT_UNITS_API std::string to_string(Timestamp value);

template <typename Sink>
void AbslStringify(Sink &sink, Timestamp value)
{
    sink.Append(to_string(value));
}

CXXKIT_END_NAMESPACE

