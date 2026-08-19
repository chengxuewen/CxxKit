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

#include <cxxkit/text/string_encode.hpp>

CXXKIT_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////////////////
// String Encoding Utilities
/////////////////////////////////////////////////////////////////////////////

namespace utils
{

namespace
{
const char HEX[] = "0123456789abcdef";

// Convert an unsigned value from 0 to 15 to the hex character equivalent...
char hex_encode(unsigned char val)
{
    CXXKIT_DCHECK_LT(val, 16);
    return (val < 16) ? HEX[val] : '!';
}

// ...and vice-versa.
bool hex_decode(char ch, unsigned char *val)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        *val = ch - '0';
    }
    else if ((ch >= 'A') && (ch <= 'F'))
    {
        *val = (ch - 'A') + 10;
    }
    else if ((ch >= 'a') && (ch <= 'f'))
    {
        *val = (ch - 'a') + 10;
    }
    else
    {
        return false;
    }
    return true;
}

size_t hex_encode_output_length(size_t srclen, char delimiter)
{
    return delimiter && srclen > 0 ? (srclen * 3 - 1) : (srclen * 2);
}

// hex_encode shows the hex representation of binary data in ascii, with
// `delimiter` between bytes, or none if `delimiter` == 0.
void hex_encode_with_delimiter(char *buffer, StringView source, char delimiter)
{
    CXXKIT_DCHECK(buffer);

    // Init and check bounds.
    const unsigned char *bsource = reinterpret_cast<const unsigned char *>(source.data());
    size_t srcpos = 0, bufpos = 0;

    size_t srclen = source.length();
    while (srcpos < srclen)
    {
        unsigned char ch = bsource[srcpos++];
        buffer[bufpos] = hex_encode((ch >> 4) & 0xF);
        buffer[bufpos + 1] = hex_encode((ch) & 0xF);
        bufpos += 2;

        // Don't write a delimiter after the last byte.
        if (delimiter && (srcpos < srclen))
        {
            buffer[bufpos] = delimiter;
            ++bufpos;
        }
    }
}
} // namespace

std::string hex_encode(StringView str)
{
    return hex_encode_with_delimiter(str, 0);
}

std::string hex_encode_with_delimiter(StringView source, char delimiter)
{
    std::string s(hex_encode_output_length(source.length(), delimiter), 0);
    hex_encode_with_delimiter(&s[0], source, delimiter);
    return s;
}

size_t hex_decode_with_delimiter(ArrayView<char> cbuffer, StringView source, char delimiter)
{
    if (cbuffer.empty())
    {
        return 0;
    }

    // Init and bounds check.
    unsigned char *bbuffer = reinterpret_cast<unsigned char *>(cbuffer.data());
    size_t srcpos = 0, bufpos = 0;
    size_t srclen = source.length();

    size_t needed = (delimiter) ? (srclen + 1) / 3 : srclen / 2;
    if (cbuffer.size() < needed)
    {
        return 0;
    }

    while (srcpos < srclen)
    {
        if ((srclen - srcpos) < 2)
        {
            // This means we have an odd number of bytes.
            return 0;
        }

        unsigned char h1, h2;
        if (!hex_decode(source[srcpos], &h1) || !hex_decode(source[srcpos + 1], &h2))
        {
            return 0;
        }

        bbuffer[bufpos++] = (h1 << 4) | h2;
        srcpos += 2;

        // Remove the delimiter if needed.
        if (delimiter && (srclen - srcpos) > 1)
        {
            if (source[srcpos] != delimiter)
            {
                return 0;
            }
            ++srcpos;
        }
    }

    return bufpos;
}

size_t hex_decode(ArrayView<char> buffer, StringView source)
{
    return hex_decode_with_delimiter(buffer, source, 0);
}

size_t tokenize(StringView source, char delimiter, std::vector<std::string> *fields)
{
    fields->clear();
    size_t last = 0;
    for (size_t i = 0; i < source.length(); ++i)
    {
        if (source[i] == delimiter)
        {
            if (i != last)
            {
                fields->emplace_back(source.substr(last, i - last));
            }
            last = i + 1;
        }
    }
    if (last != source.length())
    {
        fields->emplace_back(source.substr(last, source.length() - last));
    }
    return fields->size();
}

bool tokenize_first(StringView source, const char delimiter, std::string *token, std::string *rest)
{
    // Find the first delimiter
    size_t left_pos = source.find(delimiter);
    if (left_pos == StringView::npos)
    {
        return false;
    }

    // Look for additional occurrances of delimiter.
    size_t right_pos = left_pos + 1;
    while (right_pos < source.size() && source[right_pos] == delimiter)
    {
        right_pos++;
    }

    *token = std::string(source.substr(0, left_pos));
    *rest = std::string(source.substr(right_pos));
    return true;
}

bool FromString(StringView s, bool *b)
{
    if (s == "false")
    {
        *b = false;
        return true;
    }
    if (s == "true")
    {
        *b = true;
        return true;
    }
    return false;
}
} // namespace utils

CXXKIT_END_NAMESPACE
