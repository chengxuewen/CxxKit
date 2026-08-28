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

#include <cxxkit/text/string_utils.hpp>
#include <cassert>
#include <cxxkit/base/macros.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/text/ascii.hpp>

#include <algorithm>
#include <cstdint>

CXXKIT_BEGIN_NAMESPACE

namespace utils
{

const char *extract_file_name(const char *file_path)
{
    assert(nullptr != file_path);

    char path[CXXKIT_PATH_MAX] = {0};
    size_t length = std::min<size_t>(CXXKIT_PATH_MAX, strlen(file_path));
    std::memcpy(path, file_path, length);
    while (length > 0 && ('/' == path[length - 1] || '\\' == path[length - 1]))
    {
        path[--length] = '\0';
    }
    const char *last_slash = strrchr(path, '/');
    const char *last_slash_win = strrchr(path, '\\'); // windows path
    const char *file_name = path;
    if (NULL == last_slash)
    {
        file_name = (NULL == last_slash_win) ? path : last_slash_win + 1;
    }
    else if (NULL != last_slash_win)
    {
        file_name = (last_slash_win - last_slash > 0) ? last_slash_win + 1 : last_slash + 1;
    }
    else
    {
        file_name = last_slash + 1;
    }
    return file_path + (file_name - path);
}

std::string extract_function_name(const char *function, const char *suffix)
{
    const StringView func_string_view(function);
    const auto end = func_string_view.find_last_of('(');
    if (std::string::npos != end)
    {
        const auto start = func_string_view.find_last_of(' ', end);
        if (std::string::npos != start)
        {
            return std::string(function + start, function + end) + suffix;
        }
    }
    return function;
}

bool string_compare(const char *s1, const char *s2, size_t len, bool ignoreCase)
{
    const unsigned char *us1 = reinterpret_cast<const unsigned char *>(s1);
    const unsigned char *us2 = reinterpret_cast<const unsigned char *>(s2);

    for (size_t i = 0; i < len; i++)
    {
        unsigned char c1 = us1[i];
        unsigned char c2 = us2[i];
        // If bytes are the same, they will be the same when converted to lower.
        // So we only need to convert if bytes are not equal.
        if (c1 != c2)
        {
            if (ignoreCase)
            {
                c1 = c1 >= 'A' && c1 <= 'Z' ? c1 - 'A' + 'a' : c1;
                c2 = c2 >= 'A' && c2 <= 'Z' ? c2 - 'A' + 'a' : c2;
            }
            if (c1 != c2)
            {
                return false;
            }
        }
    }
    return true;
}

int string_case_cmp(const char *s1, const char *s2, size_t len)
{
    const unsigned char *us1 = reinterpret_cast<const unsigned char *>(s1);
    const unsigned char *us2 = reinterpret_cast<const unsigned char *>(s2);

    for (size_t i = 0; i < len; i++)
    {
        unsigned char c1 = us1[i];
        unsigned char c2 = us2[i];
        // If bytes are the same, they will be the same when converted to lower.
        // So we only need to convert if bytes are not equal.
        if (c1 != c2)
        {
            c1 = c1 >= 'A' && c1 <= 'Z' ? c1 - 'A' + 'a' : c1;
            c2 = c2 >= 'A' && c2 <= 'Z' ? c2 - 'A' + 'a' : c2;
            const int diff = int{c1} - int{c2};
            if (diff != 0)
            {
                return diff;
            }
        }
    }
    return 0;
}

bool string_contains_ignore_case(StringView haystack, StringView needle) noexcept
{
    while (haystack.size() >= needle.size())
    {
        if (utils::string_starts_with_ignore_case(haystack, needle))
        {
            return true;
        }
        haystack.remove_prefix(1);
    }
    return false;
}

bool string_contains_ignore_case(StringView haystack, char needle) noexcept
{
    char upper_needle = ascii_toupper(static_cast<unsigned char>(needle));
    char lower_needle = ascii_tolower(static_cast<unsigned char>(needle));
    if (upper_needle == lower_needle)
    {
        return string_contains(haystack, needle);
    }
    else
    {
        const char both_cstr[3] = {lower_needle, upper_needle, '\0'};
        return haystack.find_first_of(both_cstr) != StringView::npos;
    }
}

std::vector<StringView> string_split(StringView source, char delimiter)
{
    std::vector<StringView> fields;
    size_t last = 0;
    for (size_t i = 0; i < source.length(); ++i)
    {
        if (source[i] == delimiter)
        {
            fields.push_back(source.substr(last, i - last));
            last = i + 1;
        }
    }
    fields.push_back(source.substr(last));
    return fields;
}

namespace
{
// This is an arbitrary limitation that can be changed if necessary, or removed if someone has the time and
// inclination to replicate the fancy logic from Chromium's base::StringPrinf().
constexpr int kMaxSize = 512;
} // namespace

std::string string_format(const char *format, ...)
{
    char buffer[kMaxSize];
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, kMaxSize, format, args);
    va_end(args);
    CXXKIT_DCHECK_GE(result, 0) << "ERROR: vsnprintf() failed with error " << result;
    CXXKIT_DCHECK_LT(result, kMaxSize) << "WARNING: string was truncated from " << result << " to " << (kMaxSize - 1)
                                       << " characters";
    return std::string(buffer);
}
} // namespace utils

CXXKIT_END_NAMESPACE
