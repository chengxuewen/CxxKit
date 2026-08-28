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

#include <cxxkit/tools/status.hpp>
#include <cxxkit/tools/checks.hpp>

#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

CXXKIT_BEGIN_NAMESPACE

/**
 * @class StatusOr
 * @brief Holds either a value of type T or a Status error.
 *
 * Port from absl::StatusOr, adapted to CxxKit Status. StatusOr<T> is a
 * discriminated union that holds either a value T or a Status error.
 * The caller must check ok() before accessing the value.
 *
 * @tparam T The value type.
 */
template <class T>
class StatusOr
{
public:
    /// @brief Construct with a value.
    StatusOr(const T &value) : mHasValue(true)
    {
        new (&mStorage) T(value);
    }

    /// @brief Construct with a value (move).
    StatusOr(T &&value) : mHasValue(true)
    {
        new (&mStorage) T(std::move(value));
    }

    /// @brief Construct with an error Status (must not be ok).
    StatusOr(const Status &status) : mHasValue(false), mStatus(status)
    {
        CXXKIT_CHECK(!mStatus.is_ok()) << "StatusOr constructed with ok Status";
    }

    /// @brief Construct with an error Status (move, must not be ok).
    StatusOr(Status &&status) : mHasValue(false), mStatus(std::move(status))
    {
        CXXKIT_CHECK(!mStatus.is_ok()) << "StatusOr constructed with ok Status";
    }

    /// @brief Copy constructor.
    StatusOr(const StatusOr &other) : mHasValue(other.mHasValue), mStatus(other.mStatus)
    {
        if (mHasValue)
        {
            new (&mStorage) T(other.value_ref());
        }
    }

    /// @brief Move constructor.
    StatusOr(StatusOr &&other) : mHasValue(other.mHasValue), mStatus(std::move(other.mStatus))
    {
        if (mHasValue)
        {
            new (&mStorage) T(std::move(other.value_ref()));
        }
    }

    /// @brief Destructor.
    ~StatusOr()
    {
        if (mHasValue)
        {
            value_ref().~T();
        }
    }

    /// @brief Copy assignment.
    StatusOr &operator=(const StatusOr &other)
    {
        if (this == &other)
        {
            return *this;
        }
        if (mHasValue)
        {
            value_ref().~T();
        }
        mHasValue = other.mHasValue;
        mStatus = other.mStatus;
        if (mHasValue)
        {
            new (&mStorage) T(other.value_ref());
        }
        return *this;
    }

    /// @brief Move assignment.
    StatusOr &operator=(StatusOr &&other)
    {
        if (this == &other)
        {
            return *this;
        }
        if (mHasValue)
        {
            value_ref().~T();
        }
        mHasValue = other.mHasValue;
        mStatus = std::move(other.mStatus);
        if (mHasValue)
        {
            new (&mStorage) T(std::move(other.value_ref()));
        }
        return *this;
    }

    /// @brief Returns true if the StatusOr holds a value.
    bool ok() const { return mHasValue; }

    /// @brief Returns the status (ok if value is held).
    const Status &status() const { return mStatus; }

    /// @brief Returns the value (throws if error).
    T &value()
    {
        CXXKIT_CHECK(mHasValue) << "StatusOr::value() called on error";
        return value_ref();
    }
    const T &value() const
    {
        CXXKIT_CHECK(mHasValue) << "StatusOr::value() called on error";
        return value_ref();
    }

    /// @brief Returns the value or the provided default.
    T value_or(T default_value) const { return mHasValue ? value_ref() : default_value; }

    /// @brief Returns the value; aborts via CXXKIT_CHECK if error.
    T &OrDie()
    {
        CXXKIT_CHECK(mHasValue) << "StatusOr::OrDie(): " << mStatus.error_message();
        return value_ref();
    }
    const T &OrDie() const
    {
        CXXKIT_CHECK(mHasValue) << "StatusOr::OrDie(): " << mStatus.error_message();
        return value_ref();
    }

private:
    bool mHasValue;
    Status mStatus;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type mStorage;

    T &value_ref() { return *reinterpret_cast<T *>(&mStorage); }
    const T &value_ref() const { return *reinterpret_cast<const T *>(&mStorage); }
};

/// @brief Factory function: creates a StatusOr<T> holding a value.
template <class T>
StatusOr<T> MakeStatusOr(T value)
{
    return StatusOr<T>(std::move(value));
}

CXXKIT_END_NAMESPACE
