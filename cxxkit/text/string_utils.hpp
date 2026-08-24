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

#include <cxxkit/text/text_global.hpp>

#include <cxxkit/text/string_view.hpp>
#include <cxxkit/base/global.hpp>

#include <cstring>
#include <sstream>
#include <string.h>

/**
 * @brief
 * This file contains simple utilities for performing string matching checks.
 * All of these function parameters are specified as `StringView`,
 * meaning that these functions can accept `std::string`, `StringView` or NUL-terminated C-style strings.
 *
 * Examples:
 *  std::string s = "foo";
 *  StringView sv = "f";
 *  assert(absl::stringContains(s, sv));
 *
 *  Note: The order of parameters in these functions is designed to mimic the
 *  order an equivalent member function would exhibit;
 *  e.g. `s.Contains(x)` ==> `stringContains(s, x)`.
 */

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
template <typename T>
std::string pointerToString(T *ptr)
{
    std::stringstream ss;
    ss << ptr;
    return ss.str();
}

/**
 * @brief
 * @param filePath
 * @return
 */
CXXKIT_TEXT_API const char *extractFileName(const char *filePath);

/**
 * @brief Extracts the function name from a function signature string.
 * @param function the signature/name string.
 * @param suffix optional suffix to strip (e.g. parameter list).
 * @return the extracted function name.
 */
CXXKIT_TEXT_API std::string extractFunctionName(const char *function, const char *suffix = "");

/**
 * @brief
 * @param s1
 * @param s2
 * @param len
 * @param ignoreCase
 * @return
 */
CXXKIT_TEXT_API bool stringCompare(const char *s1, const char *s2, size_t len, bool ignoreCase);

/**
 * @brief Performs a byte-by-byte comparison of `len` bytes of the strings `s1` and `s2`,
 * ignoring the case of the characters.
 * It returns an integer less than, equal to, or greater than zero if `s1` is found, respectively, to be less than,
 * to match, or be greater than `s2`.
 */
CXXKIT_TEXT_API int stringCaseCmp(const char *s1, const char *s2, size_t len);

/**
 * @brief Returns whether a given ASCII string `haystack` contains the ASCII substring `needle`,
 * ignoring case in the comparison.
 */
CXXKIT_TEXT_API bool stringContainsIgnoreCase(StringView haystack, StringView needle) noexcept;

/**
 * @brief
 * @param haystack
 * @param needle
 * @return
 */
CXXKIT_TEXT_API bool stringContainsIgnoreCase(StringView haystack, char needle) noexcept;

/**
 * @brief Returns whether a given string `haystack` contains the substring `needle`.
 */
static CXXKIT_FORCE_INLINE bool stringContains(StringView haystack, StringView needle) noexcept
{
    return haystack.empty() && needle.empty() ? true : haystack.find(needle, 0) != haystack.npos;
}

static CXXKIT_FORCE_INLINE bool stringContains(StringView haystack, char needle) noexcept
{
    return haystack.find(needle) != haystack.npos;
}

/**
 * @brief Returns whether a given string `text` begins with `prefix`.
 */
static CXXKIT_FORCE_INLINE bool stringStartsWith(StringView text, StringView prefix) noexcept
{
    return prefix.empty() || (text.size() >= prefix.size() && memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

/**
 * @brief Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring case in the comparison.
 */
static CXXKIT_FORCE_INLINE bool stringEqualsIgnoreCase(StringView piece1, StringView piece2) noexcept
{
    return (piece1.size() == piece2.size() && 0 == stringCaseCmp(piece1.data(), piece2.data(), piece1.size()));
}

/**
 * @brief Returns whether a given ASCII string `text` starts with `prefix`, ignoring case in the comparison.s
 */
static CXXKIT_FORCE_INLINE bool stringStartsWithIgnoreCase(StringView text, StringView prefix) noexcept
{
    return (text.size() >= prefix.size()) && stringEqualsIgnoreCase(text.substr(0, prefix.size()), prefix);
}

/**
 * @brief Returns whether a given ASCII string `text` ends with `suffix`, ignoring case in the comparison.
 */
static CXXKIT_FORCE_INLINE bool stringEndsWithIgnoreCase(StringView text, StringView suffix) noexcept
{
    return (text.size() >= suffix.size()) && stringEqualsIgnoreCase(text.substr(text.size() - suffix.size()), suffix);
}

/**
 * @brief Returns whether a given string `text` ends with `suffix`.
 */
static CXXKIT_FORCE_INLINE bool stringEndsWith(StringView text, StringView suffix) noexcept
{
    return suffix.empty() || (text.size() >= suffix.size() &&
                              memcmp(text.data() + (text.size() - suffix.size()), suffix.data(), suffix.size()) == 0);
}

/**
 * @brief Return a C++ string given printf-like input.
 * Based on base::StringPrintf() in Chrome but without its fancy dynamic memory allocation for any size of the input buffer.
 * @param format
 * @param ...
 * @return
 */
CXXKIT_TEXT_API std::string stringFormat(const char *format, ...) CXXKIT_ATTRIBUTE_FORMAT_PRINTF(1, 2);


// Splits the source string into multiple fields separated by delimiter,
// with duplicates of delimiter creating empty fields. Empty input produces a
// single, empty, field.
CXXKIT_TEXT_API std::vector<StringView> stringSplit(StringView source, char delimiter);

///////////////////////////////////////////////////////////////////////////////
// UTF helpers (Windows only)
///////////////////////////////////////////////////////////////////////////////

#if defined(CXXKIT_OS_WIN)

inline std::wstring toUtf16(const char *utf8, size_t len)
{
    if (len == 0)
    {
        return std::wstring();
    }
    int len16 = ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(len), nullptr, 0);
    std::wstring ws(len16, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(len), &*ws.begin(), len16);
    return ws;
}

inline std::wstring toUtf16(StringView str)
{
    return toUtf16(str.data(), str.length());
}

inline std::string toUtf8(const wchar_t *wide, size_t len)
{
    if (len == 0)
    {
        return std::string();
    }
    int len8 = ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    std::string ns(len8, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len), &*ns.begin(), len8, nullptr, nullptr);
    return ns;
}

inline std::string toUtf8(const wchar_t *wide)
{
    return toUtf8(wide, wcslen(wide));
}

inline std::string toUtf8(const std::wstring &wstr)
{
    return toUtf8(wstr.data(), wstr.length());
}

#endif // defined(CXXKIT_OS_WIN)
} // namespace utils

CXXKIT_END_NAMESPACE