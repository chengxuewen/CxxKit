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

#pragma once

#include <cxxkit/tools/error.hpp>

#include <ostream>

CXXKIT_BEGIN_NAMESPACE

/**
 * @class Status
 * @brief A class representing the status of an operation, either success or failure with error information.
 *
 * The Status class is a wrapper around an Error object, providing a convenient way to represent
 * the result of an operation. It can be in either a success state or an error state.
 */
class Status
{
    struct Ok final
    {
    };

public:
    /**
     * @brief A constant representing a successful status.
     */
    static constexpr Ok ok{};

    /**
     * @brief Default constructor. Creates a successful status.
     */
    Status(const Ok &o = ok) { CXXKIT_UNUSED(o); }

    /**
     * @brief Copy constructor.
     * @param other The Status object to copy from.
     */
    Status(const Status &) = default;

    /**
     * @brief Move constructor.
     * @param other The Status object to move from.
     */
    Status(Status &&) = default;

    /**
     * @brief Constructs a Status with error information.
     * @param domain The error domain.
     * @param code The error code.
     * @param message The error message.
     * @param cause The cause of this error (optional).
     */
    Status(const Error::Domain &domain,
           ErrorId code,
           const StringView message,
           const Error::SharedDataPtr &cause = {})
        : mError(Error::create(domain, code, message, cause))
    {
    }

    /**
     * @brief Constructs a Status with a message. Uses the default error domain.
     * @param message The error message.
     * @param cause The cause of this error (optional).
     */
    Status(const char *message, const Error::SharedDataPtr &cause)
        : mError(Error::create(message, cause))
    {
    }

    /**
     * @brief Constructs a Status with a message. Uses the default error domain.
     * @param message The error message.
     * @param cause The cause of this error (optional).
     */
    Status(const StringView message, const Error::SharedDataPtr &cause)
        : mError(Error::create(message, cause))
    {
    }

    /**
     * @brief Constructs a Status with a message. Uses the default error domain.
     * @param message The error message.
     */
    Status(const char *message)
        : mError(Error::create(message))
    {
    }

    /**
     * @brief Constructs a Status with a message. Uses the default error domain.
     * @param message The error message.
     */
    Status(const StringView message)
        : mError(Error::create(message))
    {
    }

    /**
     * @brief Constructs a Status with a message. Uses the default error domain.
     * @param message The error message.
     */
    Status(const std::string &message)
        : mError(Error::create(message))
    {
    }

    /**
     * @brief Constructs a Status from an existing Error object.
     * @param error The Error object to wrap.
     */
    Status(const Error::SharedDataPtr &error)
        : mError(error)
    {
    }

    /**
     * @brief Constructs a Status from an existing Error object using move semantics.
     * @param error The Error object to wrap.
     */
    Status(Error::SharedDataPtr &&error)
        : mError(std::move(error))
    {
    }

    /**
     * @brief Destructor.
     */
    ~Status() = default;

    /**
     * @brief Copy assignment operator.
     * @param other The Status object to copy from.
     * @return A reference to this Status object.
     */
    Status &operator=(const Status &) = default;

    /**
     * @brief Move assignment operator.
     * @param other The Status object to move from.
     * @return A reference to this Status object.
     */
    Status &operator=(Status &&) = default;

    /**
     * @brief Equality operator.
     * @param other The Status object to compare with.
     * @return true if both Status objects represent the same state, false otherwise.
     */
    bool operator==(const Status &other) const { return mError == other.mError; }

    /**
     * @brief Inequality operator.
     * @param other The Status object to compare with.
     * @return true if Status objects represent different states, false otherwise.
     */
    bool operator!=(const Status &other) const { return mError != other.mError; }

    /**
     * @brief Assignment operator for StringView.
     * @param message The error message.
     * @return A reference to this Status object.
     */
    Status &operator=(const std::string &message)
    {
        mError = Error::create(message);
        return *this;
    }

    /**
     * @brief Assignment operator for StringView.
     * @param message The error message.
     * @return A reference to this Status object.
     */
    Status &operator=(const StringView message)
    {
        mError = Error::create(message);
        return *this;
    }

    /**
     * @brief Assignment operator for const char*.
     * @param message The error message.
     * @return A reference to this Status object.
     */
    Status &operator=(const char *message)
    {
        mError = Error::create(message);
        return *this;
    }

    /**
     * @brief Checks if the operation was successful.
     * @return true if successful, false otherwise.
     */
    bool isOk() const { return !mError.data(); }

    /**
     * @brief Converts to bool type. Returns true if the operation was successful.
     * @return true if successful, false otherwise.
     */
    operator bool() const { return isOk(); }

    /**
     * @brief Gets shared pointer to the stored error object if the operation failed.
     * @return Shared pointer to the stored error if present, otherwise an empty shared pointer.
     */
    const Error::SharedDataPtr error() const { return mError; }

    /**
     * @brief Gets the error code directly.
     * @return The error code, or Error::kInvalidId if successful.
     */
    ErrorId errorCode() const { return mError.data() ? mError.data()->code() : Error::kInvalidId; }

    /**
     * @brief Gets the error message directly.
     * @return The error message, or an empty string if successful.
     */
    std::string errorMessage() const { return mError.data() ? mError.data()->message() : ""; }

    /**
     * @brief Gets the full error string including domain, code, and message.
     * @return The formatted error string, or an empty string if successful.
     */
    std::string errorString() const { return mError.data() ? mError.data()->toString() : ""; }

private:
    Error::SharedDataPtr mError{nullptr}; ///< The error object, nullptr if successful.
};

/**
 * @brief Output stream operator for Status objects.
 * @param os The output stream.
 * @param status The Status object to output.
 * @return The output stream.
 */
inline std::ostream &operator<<(std::ostream &os, const Status &status)
{
    if (status.isOk())
    {
        os << "OK";
    }
    else
    {
        os << status.errorString();
    }
    return os;
}

CXXKIT_END_NAMESPACE
