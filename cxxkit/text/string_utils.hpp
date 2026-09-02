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
 *  assert(absl::string_contains(s, sv));
 *
 *  Note: The order of parameters in these functions is designed to mimic the
 *  order an equivalent member function would exhibit;
 *  e.g. `s.Contains(x)` ==> `string_contains(s, x)`.
 */

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
template <typename T>
std::string pointer_to_string(T *ptr)
{
    std::stringstream ss;
    ss << ptr;
    return ss.str();
}

/**
 * @brief
 * @param file_path
 * @return
 */
CXXKIT_TEXT_API const char *extract_file_name(const char *file_path);

/**
 * @brief Extracts the function name from a function signature string.
 * @param function the signature/name string.
 * @param suffix optional suffix to strip (e.g. parameter list).
 * @return the extracted function name.
 */
CXXKIT_TEXT_API std::string extract_function_name(const char *function, const char *suffix = "");

/**
 * @brief
 * @param s1
 * @param s2
 * @param len
 * @param ignoreCase
 * @return
 */
CXXKIT_TEXT_API bool string_compare(const char *s1, const char *s2, size_t len, bool ignoreCase);

/**
 * @brief Performs a byte-by-byte comparison of `len` bytes of the strings `s1` and `s2`,
 * ignoring the case of the characters.
 * It returns an integer less than, equal to, or greater than zero if `s1` is found, respectively, to be less than,
 * to match, or be greater than `s2`.
 */
CXXKIT_TEXT_API int string_case_cmp(const char *s1, const char *s2, size_t len);

/**
 * @brief Returns whether a given ASCII string `haystack` contains the ASCII substring `needle`,
 * ignoring case in the comparison.
 */
CXXKIT_TEXT_API bool string_contains_ignore_case(StringView haystack, StringView needle) noexcept;

/**
 * @brief
 * @param haystack
 * @param needle
 * @return
 */
CXXKIT_TEXT_API bool string_contains_ignore_case(StringView haystack, char needle) noexcept;

/**
 * @brief Returns whether a given string `haystack` contains the substring `needle`.
 */
static CXXKIT_FORCE_INLINE bool string_contains(StringView haystack, StringView needle) noexcept
{
    return haystack.empty() && needle.empty() ? true : haystack.find(needle, 0) != haystack.npos;
}

static CXXKIT_FORCE_INLINE bool string_contains(StringView haystack, char needle) noexcept
{
    return haystack.find(needle) != haystack.npos;
}

/**
 * @brief Returns whether a given string `text` begins with `prefix`.
 */
static CXXKIT_FORCE_INLINE bool string_starts_with(StringView text, StringView prefix) noexcept
{
    return prefix.empty() || (text.size() >= prefix.size() && memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

/**
 * @brief Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring case in the comparison.
 */
static CXXKIT_FORCE_INLINE bool string_equals_ignore_case(StringView piece1, StringView piece2) noexcept
{
    return (piece1.size() == piece2.size() && 0 == string_case_cmp(piece1.data(), piece2.data(), piece1.size()));
}

/**
 * @brief Returns whether a given ASCII string `text` starts with `prefix`, ignoring case in the comparison.s
 */
static CXXKIT_FORCE_INLINE bool string_starts_with_ignore_case(StringView text, StringView prefix) noexcept
{
    return (text.size() >= prefix.size()) && string_equals_ignore_case(text.substr(0, prefix.size()), prefix);
}

/**
 * @brief Returns whether a given ASCII string `text` ends with `suffix`, ignoring case in the comparison.
 */
static CXXKIT_FORCE_INLINE bool string_ends_with_ignore_case(StringView text, StringView suffix) noexcept
{
    return (text.size() >= suffix.size()) &&
           string_equals_ignore_case(text.substr(text.size() - suffix.size()), suffix);
}

/**
 * @brief Returns whether a given string `text` ends with `suffix`.
 */
static CXXKIT_FORCE_INLINE bool string_ends_with(StringView text, StringView suffix) noexcept
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
CXXKIT_TEXT_API std::string string_format(const char *format, ...) CXXKIT_ATTRIBUTE_FORMAT_PRINTF(1, 2);


// Splits the source string into multiple fields separated by delimiter,
// with duplicates of delimiter creating empty fields. Empty input produces a
// single, empty, field.
CXXKIT_TEXT_API std::vector<StringView> string_split(StringView source, char delimiter);

///////////////////////////////////////////////////////////////////////////////
// UTF helpers (Windows only)
///////////////////////////////////////////////////////////////////////////////

#if defined(CXXKIT_OS_WIN)

inline std::wstring to_utf16(const char *utf8, size_t len)
{
    if (len == 0)
    {
        return std::wstring();
    }
    int len16 = ::multi_byte_to_wide_char(CP_UTF8, 0, utf8, static_cast<int>(len), nullptr, 0);
    std::wstring ws(len16, 0);
    ::multi_byte_to_wide_char(CP_UTF8, 0, utf8, static_cast<int>(len), &*ws.begin(), len16);
    return ws;
}

inline std::wstring to_utf16(StringView str)
{
    return to_utf16(str.data(), str.length());
}

inline std::string to_utf8(const wchar_t *wide, size_t len)
{
    if (len == 0)
    {
        return std::string();
    }
    int len8 = ::wide_char_to_multi_byte(CP_UTF8, 0, wide, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    std::string ns(len8, 0);
    ::wide_char_to_multi_byte(CP_UTF8, 0, wide, static_cast<int>(len), &*ns.begin(), len8, nullptr, nullptr);
    return ns;
}

inline std::string to_utf8(const wchar_t *wide)
{
    return to_utf8(wide, wcslen(wide));
}

inline std::string to_utf8(const std::wstring &wstr)
{
    return to_utf8(wstr.data(), wstr.length());
}

#endif // defined(CXXKIT_OS_WIN)
} // namespace utils

CXXKIT_END_NAMESPACE