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

#include <cxxkit/tools/limits.hpp>
#include <cxxkit/tools/assert.hpp>
#include <cxxkit/numerics/divide_round.hpp>
#include <cxxkit/numerics/safe_conversions.hpp>

#include <cmath>

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief UnitBase is a base class for implementing custom value types with a specific unit.
 * It provides type safety and commonly useful operations.
 * The underlying storage is always an int64_t, it's up to the unit implementation to choose what scale it represents.
 *
 * It's used like:
 * class MyUnit: public UnitBase<MyUnit> {...};
 *
 * @tparam Unit_T The subclass representing the specific unit.
 */
template <typename Unit_T>
class UnitBase
{
public:
    UnitBase() = delete;
    static constexpr Unit_T Zero() { return Unit_T(0); }
    static constexpr Unit_T plus_infinity() { return Unit_T(plus_infinity_val()); }
    static constexpr Unit_T minus_infinity() { return Unit_T(minus_infinity_val()); }

    constexpr bool is_zero() const { return mValue == 0; }
    constexpr bool is_finite() const { return !is_infinite(); }
    constexpr bool is_infinite() const { return mValue == plus_infinity_val() || mValue == minus_infinity_val(); }
    constexpr bool is_plus_infinity() const { return mValue == plus_infinity_val(); }
    constexpr bool is_minus_infinity() const { return mValue == minus_infinity_val(); }

    constexpr bool operator==(const UnitBase<Unit_T> &other) const { return mValue == other.mValue; }
    constexpr bool operator!=(const UnitBase<Unit_T> &other) const { return mValue != other.mValue; }
    constexpr bool operator<=(const UnitBase<Unit_T> &other) const { return mValue <= other.mValue; }
    constexpr bool operator>=(const UnitBase<Unit_T> &other) const { return mValue >= other.mValue; }
    constexpr bool operator>(const UnitBase<Unit_T> &other) const { return mValue > other.mValue; }
    constexpr bool operator<(const UnitBase<Unit_T> &other) const { return mValue < other.mValue; }
    CXXKIT_CXX14_CONSTEXPR Unit_T round_to(const Unit_T &resolution) const
    {
        CXXKIT_DCHECK(is_finite());
        CXXKIT_DCHECK(resolution.is_finite());
        CXXKIT_DCHECK_GT(resolution.mValue, 0);
        return Unit_T((mValue + resolution.mValue / 2) / resolution.mValue) * resolution.mValue;
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T round_up_to(const Unit_T &resolution) const
    {
        CXXKIT_DCHECK(is_finite());
        CXXKIT_DCHECK(resolution.is_finite());
        CXXKIT_DCHECK(resolution.is_finite());
        CXXKIT_DCHECK_GT(resolution.mValue, 0);
        return Unit_T((mValue + resolution.mValue - 1) / resolution.mValue) * resolution.mValue;
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T round_down_to(const Unit_T &resolution) const
    {
        CXXKIT_DCHECK(is_finite());
        CXXKIT_DCHECK(resolution.is_finite());
        CXXKIT_DCHECK_GT(resolution.mValue, 0);
        return Unit_T(mValue / resolution.mValue) * resolution.mValue;
    }

protected:
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    static CXXKIT_CXX14_CONSTEXPR Unit_T from_value(T value)
    {
        if (Unit_T::kOneSided)
        {
            CXXKIT_DCHECK_GE(value, 0);
        }
        CXXKIT_DCHECK_GT(value, minus_infinity_val());
        CXXKIT_DCHECK_LT(value, plus_infinity_val());
        return Unit_T(utils::dchecked_cast<int64_t>(value));
    }

    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    static CXXKIT_CXX14_CONSTEXPR Unit_T from_value(T value)
    {
        if (value == std::numeric_limits<T>::infinity())
        {
            return plus_infinity();
        }
        else if (value == -std::numeric_limits<T>::infinity())
        {
            return minus_infinity();
        }
        else
        {
            return from_value(utils::dchecked_cast<int64_t>(value));
        }
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    static CXXKIT_CXX14_CONSTEXPR Unit_T from_fraction(int64_t denominator, T value)
    {
        if (Unit_T::kOneSided)
        {
            CXXKIT_DCHECK_GE(value, 0);
        }
        CXXKIT_DCHECK_GT(value, minus_infinity_val() / denominator);
        CXXKIT_DCHECK_LT(value, plus_infinity_val() / denominator);
        return Unit_T(utils::dchecked_cast<int64_t>(value * denominator));
    }
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    static constexpr Unit_T from_fraction(int64_t denominator, T value)
    {
        return from_value(value * denominator);
    }

    template <typename T = int64_t>
    CXXKIT_CXX14_CONSTEXPR typename std::enable_if<std::is_integral<T>::value, T>::type to_value() const
    {
        return utils::dchecked_cast<T>(mValue);
    }
    template <typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type to_value() const
    {
        return is_plus_infinity()    ? std::numeric_limits<T>::infinity()
               : is_minus_infinity() ? -std::numeric_limits<T>::infinity()
                                     : mValue;
    }
    template <typename T>
    constexpr T to_value_or(T fallbackValue) const
    {
        return is_finite() ? mValue : fallbackValue;
    }

    template <int64_t Denominator, typename T = int64_t>
    CXXKIT_CXX14_CONSTEXPR typename std::enable_if<std::is_integral<T>::value, T>::type to_fraction() const
    {
        CXXKIT_DCHECK(is_finite());
        return utils::dchecked_cast<T>(divide_round_to_nearest(mValue, Denominator));
    }
    template <int64_t Denominator, typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type to_fraction() const
    {
        return to_value<T>() * (1 / static_cast<T>(Denominator));
    }

    template <int64_t Denominator>
    constexpr int64_t to_fraction_or(int64_t fallbackValue) const
    {
        return is_finite() ? divide_round_to_nearest(mValue, Denominator) : fallbackValue;
    }

    template <int64_t Factor, typename T = int64_t>
    CXXKIT_CXX14_CONSTEXPR typename std::enable_if<std::is_integral<T>::value, T>::type to_multiple() const
    {
        CXXKIT_DCHECK_GE(to_value(), utils::numeric_min<T>() / Factor);
        CXXKIT_DCHECK_LE(to_value(), utils::numeric_max<T>() / Factor);
        return utils::dchecked_cast<T>(to_value() * Factor);
    }
    template <int64_t Factor, typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type to_multiple() const
    {
        return to_value<T>() * Factor;
    }

    explicit constexpr UnitBase(int64_t value)
        : mValue(value)
    {
    }

private:
    template <class RelativeUnit_T>
    friend class RelativeUnit;

    static inline constexpr int64_t plus_infinity_val() { return utils::numeric_max<int64_t>(); }
    static inline constexpr int64_t minus_infinity_val() { return utils::numeric_min<int64_t>(); }

    CXXKIT_CXX14_CONSTEXPR Unit_T &as_sub_class_ref() { return static_cast<Unit_T &>(*this); }
    CXXKIT_CXX14_CONSTEXPR const Unit_T &as_sub_class_ref() const { return static_cast<const Unit_T &>(*this); }

    int64_t mValue;
};

// Extends UnitBase to provide operations for relative units, that is, units
// that have a meaningful relation between values such that a += b is a
// sensible thing to do. For a,b <- same unit.
template <class Unit_T>
class RelativeUnit : public UnitBase<Unit_T>
{
public:
    constexpr Unit_T clamped(Unit_T min_value, Unit_T max_value) const
    {
        return utils::math_max(min_value, utils::math_min(UnitBase<Unit_T>::as_sub_class_ref(), max_value));
    }
    CXXKIT_CXX14_CONSTEXPR void clamp(Unit_T min_value, Unit_T max_value) { *this = clamped(min_value, max_value); }
    CXXKIT_CXX14_CONSTEXPR Unit_T operator+(const Unit_T other) const
    {
        if (this->is_plus_infinity() || other.is_plus_infinity())
        {
            CXXKIT_DCHECK(!this->is_minus_infinity());
            CXXKIT_DCHECK(!other.is_minus_infinity());
            return this->plus_infinity();
        }
        else if (this->is_minus_infinity() || other.is_minus_infinity())
        {
            CXXKIT_DCHECK(!this->is_plus_infinity());
            CXXKIT_DCHECK(!other.is_plus_infinity());
            return this->minus_infinity();
        }
        return UnitBase<Unit_T>::from_value(this->to_value() + other.to_value());
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T operator-(const Unit_T other) const
    {
        if (this->is_plus_infinity() || other.is_minus_infinity())
        {
            CXXKIT_DCHECK(!this->is_minus_infinity());
            CXXKIT_DCHECK(!other.is_plus_infinity());
            return this->plus_infinity();
        }
        else if (this->is_minus_infinity() || other.is_plus_infinity())
        {
            CXXKIT_DCHECK(!this->is_plus_infinity());
            CXXKIT_DCHECK(!other.is_minus_infinity());
            return this->minus_infinity();
        }
        return UnitBase<Unit_T>::from_value(this->to_value() - other.to_value());
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T &operator+=(const Unit_T other)
    {
        *this = *this + other;
        return this->as_sub_class_ref();
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T &operator-=(const Unit_T other)
    {
        *this = *this - other;
        return this->as_sub_class_ref();
    }
    constexpr double operator/(const Unit_T other) const
    {
        return UnitBase<Unit_T>::template to_value<double>() / other.template to_value<double>();
    }
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    constexpr Unit_T operator/(T scalar) const
    {
        return UnitBase<Unit_T>::from_value(std::llround(this->to_value() / scalar));
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    constexpr Unit_T operator/(T scalar) const
    {
        return UnitBase<Unit_T>::from_value(this->to_value() / scalar);
    }
    constexpr Unit_T operator*(double scalar) const
    {
        return UnitBase<Unit_T>::from_value(std::llround(this->to_value() * scalar));
    }
    constexpr Unit_T operator*(int64_t scalar) const { return UnitBase<Unit_T>::from_value(this->to_value() * scalar); }
    constexpr Unit_T operator*(int32_t scalar) const { return UnitBase<Unit_T>::from_value(this->to_value() * scalar); }
    constexpr Unit_T operator*(size_t scalar) const { return UnitBase<Unit_T>::from_value(this->to_value() * scalar); }

protected:
    using UnitBase<Unit_T>::UnitBase;
    constexpr RelativeUnit()
        : UnitBase<Unit_T>(0)
    {
    }
};

template <class Unit_T>
inline constexpr Unit_T operator*(double scalar, RelativeUnit<Unit_T> other)
{
    return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(int64_t scalar, RelativeUnit<Unit_T> other)
{
    return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(int32_t scalar, RelativeUnit<Unit_T> other)
{
    return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(size_t scalar, RelativeUnit<Unit_T> other)
{
    return other * scalar;
}

template <class Unit_T>
inline CXXKIT_CXX14_CONSTEXPR Unit_T operator-(RelativeUnit<Unit_T> other)
{
    if (other.is_plus_infinity())
    {
        return UnitBase<Unit_T>::minus_infinity();
    }
    if (other.is_minus_infinity())
    {
        return UnitBase<Unit_T>::plus_infinity();
    }
    return -1 * other;
}

CXXKIT_END_NAMESPACE
