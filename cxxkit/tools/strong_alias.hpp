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

#include <cxxkit/base/global.hpp>

CXXKIT_BEGIN_NAMESPACE

/**
 * @defgroup cxxkit_tools_strong_alias Strong Alias (strong-typedef)
 * @{
 *
 * @brief Type-safe wrapper around an underlying type, preventing implicit
 *        assignment between different tagged types even when they share
 *        the same underlying representation.
 *
 * @details Based on Chromium's `base::StrongAlias` (source.chromium.org).
 *          Prevents accidental mixing of semantically distinct but
 *          identically-represented values (e.g., file handles, socket IDs,
 *          resource identifiers). The ostream operator was removed per
 *          library conventions.
 *
 * @code
 * // Two "int" values that the compiler treats as distinct types
 * StrongAlias<class UserIdTag, int> UserId;
 * StrongAlias<class FileIdTag, int> FileId;
 *
 * UserId uid(1);
 * FileId fid(1);
 * // uid == fid   // compilation error — different types
 * @endcode
 */
template <typename TagType, typename TheUnderlyingType>
class StrongAlias
{
public:
    /** @brief The underlying value type (e.g., int, std::string). */
    using UnderlyingType = TheUnderlyingType;

    /**
     * @brief Default constructor. Leaves @p mValue default-initialized.
     */
    constexpr StrongAlias() = default;

    /**
     * @brief Constructs from an lvalue underlying.
     * @param v The underlying value to wrap.
     */
    constexpr explicit StrongAlias(const UnderlyingType &v)
        : mValue(v)
    {
    }

    /**
     * @brief Constructs from an rvalue underlying.
     * @param v The underlying value to move-construct into.
     */
    constexpr explicit StrongAlias(UnderlyingType &&v) noexcept
        : mValue(std::move(v))
    {
    }

    /** @brief Dereference to mutable underlying pointer. */
    CXXKIT_CXX14_CONSTEXPR UnderlyingType *operator->() { return &mValue; }
    /** @brief Dereference to const underlying pointer. */
    constexpr const UnderlyingType *operator->() const { return &mValue; }

    /** @brief Dereference to mutable underlying reference. */
    CXXKIT_CXX14_CONSTEXPR UnderlyingType &operator*() & { return mValue; }
    /** @brief Dereference to const underlying reference (lvalue). */
    constexpr const UnderlyingType &operator*() const & { return mValue; }
    /** @brief Dereference to rvalue underlying (move). */
    CXXKIT_CXX14_CONSTEXPR UnderlyingType &&operator*() && { return std::move(mValue); }
    /** @brief Dereference to const rvalue underlying (move). */
    constexpr const UnderlyingType &&operator*() const && { return std::move(mValue); }

    /** @brief Access mutable underlying value. */
    CXXKIT_CXX14_CONSTEXPR UnderlyingType &value() & { return mValue; }
    /** @brief Access const underlying value (lvalue). */
    constexpr const UnderlyingType &value() const & { return mValue; }
    /** @brief Move-construct underlying value from this. */
    CXXKIT_CXX14_CONSTEXPR UnderlyingType &&value() && { return std::move(mValue); }
    /** @brief Move-construct const underlying from this. */
    constexpr const UnderlyingType &&value() const && { return std::move(mValue); }

    /** @brief Explicit conversion to const underlying reference. */
    constexpr explicit operator const UnderlyingType &() const & { return mValue; }

    /** @brief Equality comparison between two StrongAlias of the same tag/underlying. */
    constexpr bool operator==(const StrongAlias &other) const { return mValue == other.mValue; }
    /** @brief Inequality comparison. */
    constexpr bool operator!=(const StrongAlias &other) const { return mValue != other.mValue; }
    /** @brief Less-than ordering. */
    constexpr bool operator<(const StrongAlias &other) const { return mValue < other.mValue; }
    /** @brief Less-than-or-equal ordering. */
    constexpr bool operator<=(const StrongAlias &other) const { return mValue <= other.mValue; }
    /** @brief Greater-than ordering. */
    constexpr bool operator>(const StrongAlias &other) const { return mValue > other.mValue; }
    /** @brief Greater-than-or-equal ordering. */
    constexpr bool operator>=(const StrongAlias &other) const { return mValue >= other.mValue; }

protected:
    UnderlyingType mValue; ///< The wrapped underlying value.
};

/** @} */ // end of cxxkit_tools_strong_alias

CXXKIT_END_NAMESPACE