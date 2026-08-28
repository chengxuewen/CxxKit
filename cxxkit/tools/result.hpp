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

#include <cxxkit/tools/type_traits.hpp>
#include <cxxkit/tools/optional.hpp>
#include <cxxkit/tools/variant.hpp>
#include <cxxkit/tools/status.hpp>
#include <cxxkit/tools/error.hpp>

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Represents a result that can be either a value (success) or an error (failure).
 *
 * The Result class provides a type-safe way to handle operations that can either succeed with a value
 * or fail with an error message. It uses a variant to efficiently store either the value or the error,
 * ensuring that only one of them is present at any time.
 *
 * @tparam T The type of the value returned on success.
 */
template <typename T>
class Result
{
    template <typename U>
    friend class Result;

public:
    /**
     * @brief The type of the value stored in the Result on success.
     */
    using Value = T;

    /**
     * @brief Default constructor. Creates an empty Result.
     *
     * The Result is initialized in an empty state, containing neither a value nor an error.
     */
    Result() noexcept
        : mData(VariantMonostate{})
    {
    }

    /**
     * @brief copy constructor. Creates a Result by copying another Result.
     *
     * @param other The Result to copy from.
     */
    Result(const Result &other) = default;

    /**
     * @brief Move constructor. Creates a Result by moving from another Result.
     *
     * @param other The Result to move from.
     */
    Result(Result &&other) noexcept
        : mData(std::move(other.mData))
    {
    }

    /**
     * @brief copy constructor from a Result of a convertible type.
     *
     * @tparam U The type of the other Result's value, which must be convertible to T.
     * @param other The Result to copy from.
     */
    template <typename U, typename = traits::enable_if_t<traits::is_convertible<U, T>::value>>
    Result(const Result<U> &other)
    {
        if (other.ok())
        {
            mData = T(other.value());
        }
        else
        {
            mData = utils::get<Error::SharedDataPtr>(other.mData);
        }
    }

    /**
     * @brief Move constructor from a Result of a convertible type.
     *
     * @tparam U The type of the other Result's value, which must be convertible to T.
     * @param other The Result to move from.
     */
    template <typename U, typename = traits::enable_if_t<traits::is_convertible<U, T>::value>>
    Result(Result<U> &&other)
    {
        if (other.ok())
        {
            mData = T(std::move(other.value()));
        }
        else
        {
            mData = std::move(utils::get<Error::SharedDataPtr>(other.mData));
        }
    }

    /**
     * @brief Constructs a Result with a value (success).
     *
     * @param value The value to store in the Result.
     */
    Result(const T &value)
        : mData(value)
    {
    }

    /**
     * @brief Constructs a Result with a moved value (success).
     *
     * @param value The value to move into the Result.
     */
    Result(T &&value) noexcept
        : mData(std::move(value))
    {
    }

    /**
     * @brief Constructs a Result with an error message (failure).
     *
     * @param domain The error domain.
     * @param code The error code.
     * @param message The error message.
     * @param cause Optional cause of the error.
     */
    Result(const Error::Domain &domain, ErrorId code, const StringView message, const Error::SharedDataPtr &cause = {})
        : mData(Error::create(domain, code, message, cause))
    {
    }

    /**
     * @brief Constructs a Result with an existing error (failure).
     *
     * @param error The shared error pointer to store.
     */
    Result(const Error::SharedDataPtr &error)
        : mData(error)
    {
    }

    /**
     * @brief Constructs a Result with a moved error (failure).
     *
     * @param error The shared error pointer to move into the Result.
     */
    Result(Error::SharedDataPtr &&error) noexcept
        : mData(std::move(error))
    {
    }

    /**
     * @brief copy assignment operator.
     *
     * @param other The Result to copy from.
     * @return Reference to this Result.
     */
    Result &operator=(const Result &other) = default;

    /**
     * @brief Move assignment operator.
     *
     * @param other The Result to move from.
     * @return Reference to this Result.
     */
    Result &operator=(Result &&other) noexcept
    {
        if (this != &other)
        {
            mData = std::move(other.mData);
        }
        return *this;
    }

    /**
     * @brief copy assignment from a Result of a convertible type.
     *
     * @tparam U The type of the other Result's value, which must be convertible to T.
     * @param other The Result to copy from.
     * @return Reference to this Result.
     */
    template <typename U, typename = traits::enable_if_t<traits::is_convertible<U, T>::value>>
    Result &operator=(const Result<U> &other)
    {
        if (other.ok())
        {
            mData = T(other.value());
        }
        else
        {
            mData = utils::get<Error::SharedDataPtr>(other.mData);
        }
        return *this;
    }

    /**
     * @brief Move assignment from a Result of a convertible type.
     *
     * @tparam U The type of the other Result's value, which must be convertible to T.
     * @param other The Result to move from.
     * @return Reference to this Result.
     */
    template <typename U, typename = traits::enable_if_t<traits::is_convertible<U, T>::value>>
    Result &operator=(Result<U> &&other)
    {
        if (other.ok())
        {
            mData = T(std::move(other.value()));
        }
        else
        {
            mData = std::move(utils::get<Error::SharedDataPtr>(other.mData));
        }
        return *this;
    }

    /**
     * @brief Checks if the Result contains a value (success).
     *
     * @return true if the Result contains a value, false otherwise.
     */
    CXXKIT_CONSTEXPR bool ok() const { return utils::holds_alternative<T>(mData); }

    /**
     * @brief Checks if the Result contains a value (success).
     *
     * Alias for ok().
     * @return true if the Result contains a value, false otherwise.
     */
    CXXKIT_CONSTEXPR bool is_ok() const { return this->ok(); }

    /**
     * @brief Checks if the Result contains a value (success).
     *
     * Alias for ok().
     * @return true if the Result contains a value, false otherwise.
     */
    CXXKIT_CONSTEXPR bool success() const { return this->ok(); }

    /**
     * @brief Checks if the Result contains a value (success).
     *
     * Alias for ok().
     * @return true if the Result contains a value, false otherwise.
     */
    CXXKIT_CONSTEXPR bool is_success() const { return this->ok(); }

    /**
     * @brief Converts the Result to a boolean value.
     *
     * @return true if the Result contains a value, false otherwise.
     */
    CXXKIT_CONSTEXPR operator bool() const { return this->ok(); }

    /**
     * @brief Gets a reference to the stored value.
     *
     * @pre The Result must contain a value (ok() must return true).
     * @return Reference to the stored value.
     * @throw BadVariantAccess if the Result does not contain a value.
     */
    CXXKIT_CONSTEXPR T &value() & { return utils::get<T>(mData); }

    /**
     * @brief Gets a const reference to the stored value.
     *
     * @pre The Result must contain a value (ok() must return true).
     * @return Const reference to the stored value.
     * @throw BadVariantAccess if the Result does not contain a value.
     */
    CXXKIT_CONSTEXPR const T &value() const & { return utils::get<T>(mData); }

    /**
     * @brief Gets an rvalue reference to the stored value.
     *
     * @pre The Result must contain a value (ok() must return true).
     * @return Rvalue reference to the stored value.
     * @throw BadVariantAccess if the Result does not contain a value.
     */
    CXXKIT_CONSTEXPR T &&value() && { return std::move(utils::get<T>(mData)); }

    /**
     * @brief Gets a const rvalue reference to the stored value.
     *
     * @pre The Result must contain a value (ok() must return true).
     * @return Const rvalue reference to the stored value.
     * @throw BadVariantAccess if the Result does not contain a value.
     */
    CXXKIT_CONSTEXPR const T &&value() const && { return std::move(utils::get<const T>(mData)); }

    /**
     * @brief Gets the stored value if present, otherwise returns a default value.
     *
     * @tparam U Type of the default value, which must be convertible to T.
     * @param defaultValue The default value to return if the Result does not contain a value.
     * @return The stored value if present, otherwise defaultValue.
     */
    template <typename U>
    T value_or(U &&defaultValue) const &
    {
        if (this->ok())
        {
            return this->value();
        }
        return std::forward<U>(defaultValue);
    }

    /**
     * @brief Gets the stored value if present, otherwise returns a default value (rvalue overload).
     *
     * @tparam U Type of the default value, which must be convertible to T.
     * @param defaultValue The default value to return if the Result does not contain a value.
     * @return The stored value if present, otherwise defaultValue.
     */
    template <typename U>
    T value_or(U &&defaultValue) &&
    {
        if (this->ok())
        {
            return std::move(this->value());
        }
        return std::forward<U>(defaultValue);
    }

    /**
     * @brief Gets the stored value if present, otherwise invokes a function to produce a value.
     *
     * @tparam F Type of the function, which must return a value convertible to T.
     * @param f The function to invoke if the Result does not contain a value.
     * @return The stored value if present, otherwise the result of invoking f().
     */
    template <typename F>
    T value_or_else(F &&f) const &
    {
        if (this->ok())
        {
            return this->value();
        }
        return std::forward<F>(f)();
    }

    /**
     * @brief Gets the stored value if present, otherwise invokes a function to produce a value (rvalue overload).
     *
     * @tparam F Type of the function, which must return a value convertible to T.
     * @param f The function to invoke if the Result does not contain a value.
     * @return The stored value if present, otherwise the result of invoking f().
     */
    template <typename F>
    T value_or_else(F &&f) &&
    {
        if (this->ok())
        {
            return std::move(this->value());
        }
        return std::forward<F>(f)();
    }

    /**
     * @brief Gets a shared pointer to the stored error.
     *
     * @return Shared pointer to the stored error if present, otherwise an empty shared pointer.
     */
    Error::SharedDataPtr error()
    {
        if (utils::holds_alternative<Error::SharedDataPtr>(mData))
        {
            return utils::get<Error::SharedDataPtr>(mData);
        }
        return Error::SharedDataPtr();
    }

    /**
     * @brief Gets a shared pointer to the stored error (const version).
     *
     * @return Shared pointer to the stored error if present, otherwise an empty shared pointer.
     */
    const Error::SharedDataPtr error() const
    {
        if (utils::holds_alternative<Error::SharedDataPtr>(mData))
        {
            return utils::get<Error::SharedDataPtr>(mData);
        }
        return Error::SharedDataPtr();
    }

    /**
     * @brief Gets a string representation of the error.
     *
     * @return String representation of the error if present, otherwise an empty string.
     */
    std::string error_string() const
    {
        if (const Error *err = this->error())
        {
            return err->to_string();
        }
        return "";
    }

    /**
     * @brief Converts the Result to a Status object.
     *
     * @return A Status object representing the Result.
     */
    Status status() const
    {
        const auto error = this->error();
        return error.data() ? Status(error) : Status::ok;
    }

    /**
     * @brief Swaps the contents of this Result with another Result.
     *
     * @param other The Result to swap with.
     */
    void swap(Result &other) noexcept { std::swap(mData, other.mData); }

    /**
     * @brief Swaps the contents of two Result objects.
     *
     * @param lhs The first Result to swap.
     * @param rhs The second Result to swap.
     */
    friend void swap(Result &lhs, Result &rhs) noexcept { lhs.swap(rhs); }

private:
    /**
     * @brief Stores either a value, an error, or nothing.
     *
     * The variant can hold one of three types:
     * - VariantMonostate: Empty state
     * - T: Value on success
     * - Error::SharedDataPtr: Error on failure
     */
    Variant<VariantMonostate, T, Error::SharedDataPtr> mData;
};

CXXKIT_END_NAMESPACE
