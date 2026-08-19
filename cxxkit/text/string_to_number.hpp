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

#include <cxxkit/tools/optional.hpp>
#include <cxxkit/text/string_view.hpp>

#include <limits>

CXXKIT_BEGIN_NAMESPACE


// This file declares a family of functions to parse integers from strings.
// The standard C library functions either fail to indicate errors (atoi, etc.)
// or are a hassle to work with (strtol, sscanf, etc.). The standard C++ library
// functions (std::stoi, etc.) indicate errors by throwing exceptions, which
// are disabled in WebRTC.
//
// Integers are parsed using:
//   Optional<int-type> stringToNumber(StringView str,
//                                           int base = 10);
//
// These functions parse a value from the beginning of a string into one of the
// fundamental integer types, or returns an empty Optional if parsing
// failed. Values outside of the range supported by the type will be
// rejected. The strings must begin with a digit or a minus sign. No leading
// space nor trailing contents are allowed.
// By setting base to 0, one of octal, decimal or hexadecimal will be
// detected from the string's prefix (0, nothing or 0x, respectively).
// If non-zero, base can be set to a value between 2 and 36 inclusively.

namespace utils
{
namespace detail
{
// These must be (unsigned) long long, to match the signature of strto(u)ll.
using unsigned_type = unsigned long long; // NOLINT(runtime/int)
using signed_type = long long;            // NOLINT(runtime/int)

static Optional<signed_type> ParseSigned(StringView str, int base)
{
    if (str.empty())
    {
        return utils::nullopt;
    }

    if (isdigit(static_cast<unsigned char>(str[0])) || str[0] == '-')
    {
        std::string str_str(str);
        char *end = nullptr;
        errno = 0;
        const signed_type value = std::strtoll(str_str.c_str(), &end, base);
        // Check for errors and also make sure that there were no embedded nuls in
        // the input string.
        if (end == str_str.c_str() + str_str.size() && errno == 0)
        {
            return value;
        }
    }
    return utils::nullopt;
}
static Optional<unsigned_type> ParseUnsigned(StringView str, int base)
{
    if (str.empty())
    {
        return utils::nullopt;
    }

    if (isdigit(static_cast<unsigned char>(str[0])) || str[0] == '-')
    {
        std::string str_str(str);
        // Explicitly discard negative values. std::strtoull parsing causes unsigned
        // wraparound. We cannot just reject values that start with -, though, since
        // -0 is perfectly fine, as is -0000000000000000000000000000000.
        const bool is_negative = str[0] == '-';
        char *end = nullptr;
        errno = 0;
        const unsigned_type value = std::strtoull(str_str.c_str(), &end, base);
        // Check for errors and also make sure that there were no embedded nuls in
        // the input string.
        if (end == str_str.c_str() + str_str.size() && errno == 0 && (value == 0 || !is_negative))
        {
            return value;
        }
    }
    return utils::nullopt;
}

template <typename T>
T StrToT(const char *str, char **str_end);

template <>
inline float StrToT(const char *str, char **str_end)
{
    return std::strtof(str, str_end);
}

template <>
inline double StrToT(const char *str, char **str_end)
{
    return std::strtod(str, str_end);
}

template <>
inline long double StrToT(const char *str, char **str_end)
{
    return std::strtold(str, str_end);
}

template <typename T>
Optional<T> ParseFloatingPoint(StringView str)
{
    if (str.empty())
    {
        return utils::nullopt;
    }

    if (str[0] == '\0')
    {
        return utils::nullopt;
    }
    std::string str_str(str);
    char *end = nullptr;
    errno = 0;
    const T value = StrToT<T>(str_str.c_str(), &end);
    if (end == str_str.c_str() + str_str.size() && errno == 0)
    {
        return value;
    }
    return utils::nullopt;
}
template Optional<float> ParseFloatingPoint(StringView str);
template Optional<double> ParseFloatingPoint(StringView str);
template Optional<long double> ParseFloatingPoint(StringView str);
} // namespace detail

template <typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, Optional<T>>::type stringToNumber(
    StringView str,
    int base = 10)
{
    using detail::signed_type;
    static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<signed_type>::max() &&
                      std::numeric_limits<T>::lowest() >= std::numeric_limits<signed_type>::lowest(),
                  "stringToNumber only supports signed integers as large as long long int");
    Optional<signed_type> value = detail::ParseSigned(str, base);
    if (value && *value >= std::numeric_limits<T>::lowest() && *value <= std::numeric_limits<T>::max())
    {
        return static_cast<T>(*value);
    }
    return utils::nullopt;
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, Optional<T>>::type stringToNumber(
    StringView str,
    int base = 10)
{
    using detail::unsigned_type;
    static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<unsigned_type>::max(),
                  "stringToNumber only supports unsigned integers as large as "
                  "unsigned long long int");
    Optional<unsigned_type> value = detail::ParseUnsigned(str, base);
    if (value && *value <= std::numeric_limits<T>::max())
    {
        return static_cast<T>(*value);
    }
    return utils::nullopt;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Optional<T>>::type stringToNumber(StringView str,
                                                                                            int /* base */ = 10)
{
    static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<long double>::max(),
                  "stringToNumber only supports floating-point numbers as large "
                  "as long double");
    return detail::ParseFloatingPoint<T>(str);
}
} // namespace utils

CXXKIT_END_NAMESPACE

