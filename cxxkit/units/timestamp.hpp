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
    static CXXKIT_CXX14_CONSTEXPR Timestamp seconds(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return from_fraction(1000000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Timestamp millis(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return from_fraction(1000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR Timestamp micros(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return from_value(value);
    }

    Timestamp() = delete;

    template <typename Sink>
    friend void absl_stringify(Sink &sink, Timestamp value);

    template <typename T = int64_t>
    constexpr T seconds() const
    {
        return to_fraction<1000000, T>();
    }
    template <typename T = int64_t>
    constexpr T ms() const
    {
        return to_fraction<1000, T>();
    }
    template <typename T = int64_t>
    constexpr T us() const
    {
        return to_value<T>();
    }

    constexpr int64_t seconds_or(int64_t fallback_value) const { return to_fraction_or<1000000>(fallback_value); }
    constexpr int64_t ms_or(int64_t fallback_value) const { return to_fraction_or<1000>(fallback_value); }
    constexpr int64_t us_or(int64_t fallback_value) const { return to_value_or(fallback_value); }

    CXXKIT_CXX14_CONSTEXPR Timestamp operator+(const TimeDelta delta) const
    {
        if (is_plus_infinity() || delta.is_plus_infinity())
        {
            CXXKIT_DCHECK(!is_minus_infinity());
            CXXKIT_DCHECK(!delta.is_minus_infinity());
            return plus_infinity();
        }
        else if (is_minus_infinity() || delta.is_minus_infinity())
        {
            CXXKIT_DCHECK(!is_plus_infinity());
            CXXKIT_DCHECK(!delta.is_plus_infinity());
            return minus_infinity();
        }
        return Timestamp::micros(us() + delta.us());
    }
    CXXKIT_CXX14_CONSTEXPR Timestamp operator-(const TimeDelta delta) const
    {
        if (is_plus_infinity() || delta.is_minus_infinity())
        {
            CXXKIT_DCHECK(!is_minus_infinity());
            CXXKIT_DCHECK(!delta.is_plus_infinity());
            return plus_infinity();
        }
        else if (is_minus_infinity() || delta.is_plus_infinity())
        {
            CXXKIT_DCHECK(!is_plus_infinity());
            CXXKIT_DCHECK(!delta.is_minus_infinity());
            return minus_infinity();
        }
        return Timestamp::micros(us() - delta.us());
    }
    CXXKIT_CXX14_CONSTEXPR TimeDelta operator-(const Timestamp other) const
    {
        if (is_plus_infinity() || other.is_minus_infinity())
        {
            CXXKIT_DCHECK(!is_minus_infinity());
            CXXKIT_DCHECK(!other.is_plus_infinity());
            return TimeDelta::plus_infinity();
        }
        else if (is_minus_infinity() || other.is_plus_infinity())
        {
            CXXKIT_DCHECK(!is_plus_infinity());
            CXXKIT_DCHECK(!other.is_minus_infinity());
            return TimeDelta::minus_infinity();
        }
        return TimeDelta::micros(us() - other.us());
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
void absl_stringify(Sink &sink, Timestamp value)
{
    sink.append(to_string(value));
}

CXXKIT_END_NAMESPACE
