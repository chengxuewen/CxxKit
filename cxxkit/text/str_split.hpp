/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2017 The Abseil Authors.
**
** License: MIT License
**
** This file contains a simplified reimplementation of string splitting (API-compatible subset of
** abseil-cpp `absl/strings/str_split.h`, originally governed by the Apache License 2.0).
** Modified for CxxKit.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

/** @file
 * @brief String splitting, a trimmed port of abseil `absl::StrSplit`.
 *
 * Splits a text buffer into a vector of non-owning tokens separated by a delimiter.
 * Supported delimiters: a single `char`, a `by_any_of` character-set delimiter
 * (abseil `ByAnyChar`, 256-entry bool-table driven), and a multi-char `by_string`
 * delimiter (abseil `ByString`). Optional tag predicates `skip_empty` / `allow_empty`
 * filter the resulting tokens at compile time (tag dispatch, zero runtime overhead).
 *
 * @warning The returned tokens are non-owning `cxxkit::StringView` views that reference
 *          the source buffer. If the source buffer dies, the views dangle. Copy to owned
 *          storage (e.g. `std::string`) if the results must outlive the source.
 */

#pragma once

#include <cxxkit/text/string_view.hpp>

#include <string>
#include <vector>

CXXKIT_BEGIN_NAMESPACE

/** @brief Predicate tag: drop empty tokens from the split result. */
struct skip_empty
{
};
/** @brief Predicate tag: keep empty tokens (default behavior, explicit form). */
struct allow_empty
{
};

/** @brief Character-set delimiter: splits at any single char in the set (abseil `ByAnyChar`).
 *
 * Table-driven: a 256-entry bool table marks the delimiter chars, so find() is a
 * linear scan with one table lookup per byte. An empty delimiter set matches every
 * character position (mirrors abseil: each byte becomes its own token boundary).
 */
class by_any_of
{
public:
    /** @brief Constructs the delimiter set from a NUL-terminated char list. */
    explicit by_any_of(const char *delims)
        : mTable()
    {
        for (; *delims != '\0'; ++delims)
        {
            mTable[static_cast<unsigned char>(*delims)] = true;
        }
    }

    /** @brief Returns the found delimiter as a view, or an empty view at text end when none remains. */
    StringView find(StringView text, size_t pos) const
    {
        if (pos < text.size())
        {
            const char *begin = text.data();
            for (size_t i = pos; i < text.size(); ++i)
            {
                if (mTable[static_cast<unsigned char>(begin[i])])
                {
                    return StringView(begin + i, 1);
                }
            }
        }
        return StringView(text.data() + text.size(), 0);
    }

private:
    bool mTable[256];
};

/** @brief Multi-character literal delimiter (abseil `ByString`). */
class by_string
{
public:
    /** @brief Constructs the delimiter from a string view (empty delimiter = per-byte split). */
    explicit by_string(StringView delims)
        : mDelims(delims)
    {
    }

    /** @brief Returns the found delimiter as a view, or an empty view at text end when none remains. */
    StringView find(StringView text, size_t pos) const
    {
        if (mDelims.empty() && !text.empty())
        {
            return StringView(text.data() + pos + 1, 0);
        }
        const size_t found = text.find(mDelims, pos);
        if (found == StringView::npos)
        {
            return StringView(text.data() + text.size(), 0);
        }
        return text.substr(found, mDelims.size());
    }

private:
    StringView mDelims;
};

/** @brief Splits `text` on every occurrence of the single char `delim`. */
inline std::vector<StringView> str_split(StringView text, char delim)
{
    std::vector<StringView> tokens;
    const char *data = text.data();
    const size_t size = text.size();
    size_t start = 0;
    while (start <= size)
    {
        size_t i = start;
        while (i < size && data[i] != delim)
        {
            ++i;
        }
        tokens.push_back(StringView(data + start, i - start));
        start = i + 1;
    }
    return tokens;
}

/** @brief Splits `text` using a `by_any_of` delimiter. */
inline std::vector<StringView> str_split(StringView text, const by_any_of &delim)
{
    std::vector<StringView> tokens;
    const char *data = text.data();
    const size_t size = text.size();
    size_t start = 0;
    while (start <= size)
    {
        const StringView found = delim.find(text, start);
        const size_t end = static_cast<size_t>(found.data() - data);
        const size_t len = (found.size() == 0) ? (size - start) : (end - start);
        tokens.push_back(StringView(data + start, len));
        if (found.size() == 0)
        {
            break;
        }
        start = end + found.size();
    }
    return tokens;
}

/** @brief Splits `text` using a `by_string` delimiter. */
inline std::vector<StringView> str_split(StringView text, const by_string &delim)
{
    std::vector<StringView> tokens;
    const char *data = text.data();
    const size_t size = text.size();
    size_t start = 0;
    while (start <= size)
    {
        const StringView found = delim.find(text, start);
        const size_t end = static_cast<size_t>(found.data() - data);
        const size_t len = (found.size() == 0) ? (size - start) : (end - start);
        tokens.push_back(StringView(data + start, len));
        if (found.size() == 0)
        {
            break;
        }
        start = end + found.size();
    }
    return tokens;
}


/** @brief Dispatch helper: single-char delimiter find. */
inline StringView find_delim(char delim, StringView text, size_t pos)
{
    const size_t found = text.find(delim, pos);
    if (found == StringView::npos)
    {
        return StringView(text.data() + text.size(), 0);
    }
    return StringView(text.data() + found, 1);
}

/** @brief Dispatch helper: forwards to `by_any_of::find` / `by_string::find` (member lookup wins). */
template <typename Delimiter>
inline StringView find_delim(const Delimiter &delim, StringView text, size_t pos)
{
    return delim.find(text, pos);
}


/** @brief Splits `text` on `delim`, dropping empty tokens (works for char, by_any_of, by_string). */
template <typename Delimiter>
inline std::vector<StringView> str_split(StringView text, Delimiter delim, skip_empty)
{
    std::vector<StringView> tokens;
    const char *data = text.data();
    const size_t size = text.size();
    size_t start = 0;
    while (start <= size)
    {
        const StringView found = find_delim(delim, text, start);
        const size_t end = static_cast<size_t>(found.data() - data);
        const size_t len = (found.size() == 0) ? (size - start) : (end - start);
        if (len > 0)
        {
            tokens.push_back(StringView(data + start, len));
        }
        if (found.size() == 0)
        {
            break;
        }
        start = end + found.size();
    }
    return tokens;
}


/** @brief Splits `text` on `delim`, keeping empty tokens. */
template <typename Delimiter>
inline std::vector<StringView> str_split(StringView text, Delimiter delim, allow_empty)
{
    return str_split(text, delim);
}

CXXKIT_END_NAMESPACE
