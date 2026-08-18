/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#include "cxxkit/tools/type_traits.hpp"

CXXKIT_BEGIN_NAMESPACE

template <typename T>
class SizeBase
{
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");

public:
    constexpr SizeBase() noexcept
        : mWidth(T())
        , mHeight(T())
    {
    }
    constexpr SizeBase(T width, T height) noexcept
        : mWidth(width)
        , mHeight(height)
    {
    }

    // 访问器
    constexpr T width() const noexcept { return mWidth; }
    constexpr T height() const noexcept { return mHeight; }

    // 设置器
    constexpr void setWidth(T width) noexcept { mWidth = width; }
    constexpr void setHeight(T height) noexcept { mHeight = height; }

    void transpose() noexcept { std::swap(mWidth, mHeight); }
    [[nodiscard]] constexpr SizeBase transposed() const noexcept { return SizeBase(mHeight, mWidth); }

    // 辅助方法
    constexpr T area() const noexcept { return mWidth * mHeight; }
    constexpr T pixelCount() const noexcept { return mWidth * mHeight; }

    constexpr bool isNull() const noexcept { return mWidth == 0 && mHeight == 0; }
    constexpr bool isValid() const noexcept { return mWidth > T() && mHeight > T(); }
    constexpr bool isEmpty() const noexcept { return mWidth <= T() || mHeight <= T(); }

    // 转换为标准库的 pair
    constexpr std::pair<T, T> toPair() const noexcept { return std::make_pair(mWidth, mHeight); }

    // 比较运算符
    friend constexpr bool operator==(const SizeBase &lhs, const SizeBase &rhs) noexcept
    {
        return lhs.mWidth == rhs.mWidth && lhs.mHeight == rhs.mHeight;
    }

    friend constexpr bool operator!=(const SizeBase &lhs, const SizeBase &rhs) noexcept { return !(lhs == rhs); }

    // 算术运算符（可选）
    friend constexpr SizeBase operator*(const SizeBase &lhs, T scalar) noexcept
    {
        return SizeBase(lhs.mWidth * scalar, lhs.mHeight * scalar);
    }

    friend constexpr SizeBase operator/(const SizeBase &lhs, T scalar) noexcept
    {
        return SizeBase(lhs.mWidth / scalar, lhs.mHeight / scalar);
    }

protected:
    T mWidth;
    T mHeight;
};

using Size = SizeBase<int>;
using SizeF = SizeBase<float>;
using Resolution = SizeBase<int>;

CXXKIT_END_NAMESPACE