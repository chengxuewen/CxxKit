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

#include <cxxkit/units/unit_base.hpp>

CXXKIT_BEGIN_NAMESPACE

// TimeDelta represents the difference between two timestamps. Commonly this can
// be a duration. However since two Timestamps are not guaranteed to have the
// same epoch (they might come from different computers, making exact
// synchronisation infeasible), the duration covered by a TimeDelta can be
// undefined. To simplify usage, it can be constructed and converted to
// different units, specifically seconds (s), milliseconds (ms) and
// microseconds (us).
class CXXKIT_UNITS_API TimeDelta final : public RelativeUnit<TimeDelta>
{
public:
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR TimeDelta Minutes(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return Seconds(value * 60);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR TimeDelta Seconds(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR TimeDelta Millis(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromFraction(1000, value);
    }
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR TimeDelta Micros(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromValue(value);
    }

    constexpr TimeDelta() = default;

    template <typename Sink>
    friend void AbslStringify(Sink &sink, TimeDelta value);

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
    template <typename T = int64_t>
    constexpr T ns() const
    {
        return ToMultiple<1000, T>();
    }

    constexpr int64_t seconds_or(int64_t fallback_value) const { return ToFractionOr<1000000>(fallback_value); }
    constexpr int64_t ms_or(int64_t fallback_value) const { return ToFractionOr<1000>(fallback_value); }
    constexpr int64_t us_or(int64_t fallback_value) const { return ToValueOr(fallback_value); }

    constexpr TimeDelta Abs() const { return us() < 0 ? TimeDelta::Micros(-us()) : *this; }


private:
    friend class UnitBase<TimeDelta>;

    using RelativeUnit::RelativeUnit;
    static constexpr bool kOneSided = false;
};

namespace utils
{
CXXKIT_UNITS_API std::string toString(TimeDelta value);

// template <typename Sink>
// void stringify(Sink &sink, TimeDelta value)
// {
//     sink.Append(toString(value));
// }
} // namespace utils

CXXKIT_END_NAMESPACE