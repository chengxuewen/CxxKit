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

#ifndef _CXXKIT_STRONG_ALIAS_HPP
#define _CXXKIT_STRONG_ALIAS_HPP

#include <cxxkit/base/global.hpp>

CXXKIT_BEGIN_NAMESPACE

// This is a copy of
// https://source.chromium.org/chromium/chromium/src/+/main:base/types/strong_alias.h
// as the API (and internals) are using type-safe integral identifiers, but this
// library can't depend on that file. The ostream operator has been removed
// per WebRTC library conventions, and the underlying type is exposed.

template <typename TagType, typename TheUnderlyingType>
class StrongAlias
{
public:
    using UnderlyingType = TheUnderlyingType;
    constexpr StrongAlias() = default;
    constexpr explicit StrongAlias(const UnderlyingType &v)
        : value_(v)
    {
    }
    constexpr explicit StrongAlias(UnderlyingType &&v) noexcept
        : value_(std::move(v))
    {
    }

    CXXKIT_CXX14_CONSTEXPR UnderlyingType *operator->() { return &value_; }
    constexpr const UnderlyingType *operator->() const { return &value_; }

    CXXKIT_CXX14_CONSTEXPR UnderlyingType &operator*() & { return value_; }
    constexpr const UnderlyingType &operator*() const & { return value_; }
    CXXKIT_CXX14_CONSTEXPR UnderlyingType &&operator*() && { return std::move(value_); }
    constexpr const UnderlyingType &&operator*() const && { return std::move(value_); }

    CXXKIT_CXX14_CONSTEXPR UnderlyingType &value() & { return value_; }
    constexpr const UnderlyingType &value() const & { return value_; }
    CXXKIT_CXX14_CONSTEXPR UnderlyingType &&value() && { return std::move(value_); }
    constexpr const UnderlyingType &&value() const && { return std::move(value_); }

    constexpr explicit operator const UnderlyingType &() const & { return value_; }

    constexpr bool operator==(const StrongAlias &other) const { return value_ == other.value_; }
    constexpr bool operator!=(const StrongAlias &other) const { return value_ != other.value_; }
    constexpr bool operator<(const StrongAlias &other) const { return value_ < other.value_; }
    constexpr bool operator<=(const StrongAlias &other) const { return value_ <= other.value_; }
    constexpr bool operator>(const StrongAlias &other) const { return value_ > other.value_; }
    constexpr bool operator>=(const StrongAlias &other) const { return value_ >= other.value_; }

protected:
    UnderlyingType value_;
};
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_STRONG_ALIAS_HPP
