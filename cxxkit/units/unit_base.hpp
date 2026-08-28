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
    static constexpr Unit_T PlusInfinity() { return Unit_T(PlusInfinityVal()); }
    static constexpr Unit_T MinusInfinity() { return Unit_T(MinusInfinityVal()); }

    constexpr bool IsZero() const { return mValue == 0; }
    constexpr bool IsFinite() const { return !IsInfinite(); }
    constexpr bool IsInfinite() const { return mValue == PlusInfinityVal() || mValue == MinusInfinityVal(); }
    constexpr bool IsPlusInfinity() const { return mValue == PlusInfinityVal(); }
    constexpr bool IsMinusInfinity() const { return mValue == MinusInfinityVal(); }

    constexpr bool operator==(const UnitBase<Unit_T> &other) const { return mValue == other.mValue; }
    constexpr bool operator!=(const UnitBase<Unit_T> &other) const { return mValue != other.mValue; }
    constexpr bool operator<=(const UnitBase<Unit_T> &other) const { return mValue <= other.mValue; }
    constexpr bool operator>=(const UnitBase<Unit_T> &other) const { return mValue >= other.mValue; }
    constexpr bool operator>(const UnitBase<Unit_T> &other) const { return mValue > other.mValue; }
    constexpr bool operator<(const UnitBase<Unit_T> &other) const { return mValue < other.mValue; }
    CXXKIT_CXX14_CONSTEXPR Unit_T RoundTo(const Unit_T &resolution) const
    {
        CXXKIT_DCHECK(IsFinite());
        CXXKIT_DCHECK(resolution.IsFinite());
        CXXKIT_DCHECK_GT(resolution.mValue, 0);
        return Unit_T((mValue + resolution.mValue / 2) / resolution.mValue) * resolution.mValue;
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T RoundUpTo(const Unit_T &resolution) const
    {
        CXXKIT_DCHECK(IsFinite());
        CXXKIT_DCHECK(resolution.IsFinite());
        CXXKIT_DCHECK(resolution.IsFinite());
        CXXKIT_DCHECK_GT(resolution.mValue, 0);
        return Unit_T((mValue + resolution.mValue - 1) / resolution.mValue) * resolution.mValue;
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T RoundDownTo(const Unit_T &resolution) const
    {
        CXXKIT_DCHECK(IsFinite());
        CXXKIT_DCHECK(resolution.IsFinite());
        CXXKIT_DCHECK_GT(resolution.mValue, 0);
        return Unit_T(mValue / resolution.mValue) * resolution.mValue;
    }

protected:
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    static CXXKIT_CXX14_CONSTEXPR Unit_T FromValue(T value)
    {
        if (Unit_T::kOneSided)
        {
            CXXKIT_DCHECK_GE(value, 0);
        }
        CXXKIT_DCHECK_GT(value, MinusInfinityVal());
        CXXKIT_DCHECK_LT(value, PlusInfinityVal());
        return Unit_T(utils::dchecked_cast<int64_t>(value));
    }

    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    static CXXKIT_CXX14_CONSTEXPR Unit_T FromValue(T value)
    {
        if (value == std::numeric_limits<T>::infinity())
        {
            return PlusInfinity();
        }
        else if (value == -std::numeric_limits<T>::infinity())
        {
            return MinusInfinity();
        }
        else
        {
            return FromValue(utils::dchecked_cast<int64_t>(value));
        }
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    static CXXKIT_CXX14_CONSTEXPR Unit_T FromFraction(int64_t denominator, T value)
    {
        if (Unit_T::kOneSided)
        {
            CXXKIT_DCHECK_GE(value, 0);
        }
        CXXKIT_DCHECK_GT(value, MinusInfinityVal() / denominator);
        CXXKIT_DCHECK_LT(value, PlusInfinityVal() / denominator);
        return Unit_T(utils::dchecked_cast<int64_t>(value * denominator));
    }
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    static constexpr Unit_T FromFraction(int64_t denominator, T value)
    {
        return FromValue(value * denominator);
    }

    template <typename T = int64_t>
    CXXKIT_CXX14_CONSTEXPR typename std::enable_if<std::is_integral<T>::value, T>::type ToValue() const
    {
        return utils::dchecked_cast<T>(mValue);
    }
    template <typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type ToValue() const
    {
        return IsPlusInfinity()    ? std::numeric_limits<T>::infinity()
               : IsMinusInfinity() ? -std::numeric_limits<T>::infinity()
                                   : mValue;
    }
    template <typename T>
    constexpr T ToValueOr(T fallbackValue) const
    {
        return IsFinite() ? mValue : fallbackValue;
    }

    template <int64_t Denominator, typename T = int64_t>
    CXXKIT_CXX14_CONSTEXPR typename std::enable_if<std::is_integral<T>::value, T>::type ToFraction() const
    {
        CXXKIT_DCHECK(IsFinite());
        return utils::dchecked_cast<T>(DivideRoundToNearest(mValue, Denominator));
    }
    template <int64_t Denominator, typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type ToFraction() const
    {
        return ToValue<T>() * (1 / static_cast<T>(Denominator));
    }

    template <int64_t Denominator>
    constexpr int64_t ToFractionOr(int64_t fallbackValue) const
    {
        return IsFinite() ? DivideRoundToNearest(mValue, Denominator) : fallbackValue;
    }

    template <int64_t Factor, typename T = int64_t>
    CXXKIT_CXX14_CONSTEXPR typename std::enable_if<std::is_integral<T>::value, T>::type ToMultiple() const
    {
        CXXKIT_DCHECK_GE(ToValue(), utils::numeric_min<T>() / Factor);
        CXXKIT_DCHECK_LE(ToValue(), utils::numeric_max<T>() / Factor);
        return utils::dchecked_cast<T>(ToValue() * Factor);
    }
    template <int64_t Factor, typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type ToMultiple() const
    {
        return ToValue<T>() * Factor;
    }

    explicit constexpr UnitBase(int64_t value)
        : mValue(value)
    {
    }

private:
    template <class RelativeUnit_T>
    friend class RelativeUnit;

    static inline constexpr int64_t PlusInfinityVal() { return utils::numeric_max<int64_t>(); }
    static inline constexpr int64_t MinusInfinityVal() { return utils::numeric_min<int64_t>(); }

    CXXKIT_CXX14_CONSTEXPR Unit_T &AsSubClassRef() { return static_cast<Unit_T &>(*this); }
    CXXKIT_CXX14_CONSTEXPR const Unit_T &AsSubClassRef() const { return static_cast<const Unit_T &>(*this); }

    int64_t mValue;
};

// Extends UnitBase to provide operations for relative units, that is, units
// that have a meaningful relation between values such that a += b is a
// sensible thing to do. For a,b <- same unit.
template <class Unit_T>
class RelativeUnit : public UnitBase<Unit_T>
{
public:
    constexpr Unit_T Clamped(Unit_T min_value, Unit_T max_value) const
    {
        return utils::math_max(min_value, utils::math_min(UnitBase<Unit_T>::AsSubClassRef(), max_value));
    }
    CXXKIT_CXX14_CONSTEXPR void Clamp(Unit_T min_value, Unit_T max_value) { *this = Clamped(min_value, max_value); }
    CXXKIT_CXX14_CONSTEXPR Unit_T operator+(const Unit_T other) const
    {
        if (this->IsPlusInfinity() || other.IsPlusInfinity())
        {
            CXXKIT_DCHECK(!this->IsMinusInfinity());
            CXXKIT_DCHECK(!other.IsMinusInfinity());
            return this->PlusInfinity();
        }
        else if (this->IsMinusInfinity() || other.IsMinusInfinity())
        {
            CXXKIT_DCHECK(!this->IsPlusInfinity());
            CXXKIT_DCHECK(!other.IsPlusInfinity());
            return this->MinusInfinity();
        }
        return UnitBase<Unit_T>::FromValue(this->ToValue() + other.ToValue());
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T operator-(const Unit_T other) const
    {
        if (this->IsPlusInfinity() || other.IsMinusInfinity())
        {
            CXXKIT_DCHECK(!this->IsMinusInfinity());
            CXXKIT_DCHECK(!other.IsPlusInfinity());
            return this->PlusInfinity();
        }
        else if (this->IsMinusInfinity() || other.IsPlusInfinity())
        {
            CXXKIT_DCHECK(!this->IsPlusInfinity());
            CXXKIT_DCHECK(!other.IsMinusInfinity());
            return this->MinusInfinity();
        }
        return UnitBase<Unit_T>::FromValue(this->ToValue() - other.ToValue());
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T &operator+=(const Unit_T other)
    {
        *this = *this + other;
        return this->AsSubClassRef();
    }
    CXXKIT_CXX14_CONSTEXPR Unit_T &operator-=(const Unit_T other)
    {
        *this = *this - other;
        return this->AsSubClassRef();
    }
    constexpr double operator/(const Unit_T other) const
    {
        return UnitBase<Unit_T>::template ToValue<double>() / other.template ToValue<double>();
    }
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    constexpr Unit_T operator/(T scalar) const
    {
        return UnitBase<Unit_T>::FromValue(std::llround(this->ToValue() / scalar));
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    constexpr Unit_T operator/(T scalar) const
    {
        return UnitBase<Unit_T>::FromValue(this->ToValue() / scalar);
    }
    constexpr Unit_T operator*(double scalar) const
    {
        return UnitBase<Unit_T>::FromValue(std::llround(this->ToValue() * scalar));
    }
    constexpr Unit_T operator*(int64_t scalar) const { return UnitBase<Unit_T>::FromValue(this->ToValue() * scalar); }
    constexpr Unit_T operator*(int32_t scalar) const { return UnitBase<Unit_T>::FromValue(this->ToValue() * scalar); }
    constexpr Unit_T operator*(size_t scalar) const { return UnitBase<Unit_T>::FromValue(this->ToValue() * scalar); }

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
    if (other.IsPlusInfinity())
    {
        return UnitBase<Unit_T>::MinusInfinity();
    }
    if (other.IsMinusInfinity())
    {
        return UnitBase<Unit_T>::PlusInfinity();
    }
    return -1 * other;
}

CXXKIT_END_NAMESPACE

