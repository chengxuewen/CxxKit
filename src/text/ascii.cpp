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

#include "cxxkit/text/ascii.hpp"
#include "cxxkit/tools/assert.hpp"
#include "cxxkit/tools/limits.hpp"
#include "cxxkit/text/string.hpp"

#include <locale.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h> /* For tolower() */

CXXKIT_BEGIN_NAMESPACE

/* The standard delimiters, used in strdelimit(). */
static const uint16_t ascii_table_data[256] = {
    0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x104, 0x104, 0x104, 0x104, 0x104, 0x004,
    0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
    0x004, 0x004, 0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
    0x0d0, 0x0d0, 0x0d0, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x0d0, 0x0d0,
    0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253, 0x253, 0x253, 0x253,
    0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
    0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073, 0x073,
    0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
    0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004
    /* the upper 128 are all zeroes */
};
const uint16_t *const ascii_table = ascii_table_data;

// Array of characters for the ascii_tolower() function.
// For values 'A' through 'Z', return the lower-case character; otherwise, return the identity of the passed character.
static const char ascii_lower_table_data[256] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d',
    '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b',
    '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29',
    '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f', '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
    '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f', '\x40', 'a',    'b',    'c',    'd',    'e',
    'f',    'g',    'h',    'i',    'j',    'k',    'l',    'm',    'n',    'o',    'p',    'q',    'r',    's',
    't',    'u',    'v',    'w',    'x',    'y',    'z',    '\x5b', '\x5c', '\x5d', '\x5e', '\x5f', '\x60', '\x61',
    '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',
    '\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7a', '\x7b', '\x7c', '\x7d',
    '\x7e', '\x7f', '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b',
    '\x8c', '\x8d', '\x8e', '\x8f', '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99',
    '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f', '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
    '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf', '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5',
    '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf', '\xc0', '\xc1', '\xc2', '\xc3',
    '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf', '\xd0', '\xd1',
    '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
    '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed',
    '\xee', '\xef', '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb',
    '\xfc', '\xfd', '\xfe', '\xff',
};
const char *const ascii_lower_table = ascii_lower_table_data;

// Array of characters for the ascii_toupper() function.
// For values 'a' through 'z', return the upper-case character; otherwise, return the identity of the passed character.
static const char ascii_upper_table_data[256] = {
    '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d',
    '\x0e', '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1a', '\x1b',
    '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29',
    '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f', '\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37',
    '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x3e', '\x3f', '\x40', '\x41', '\x42', '\x43', '\x44', '\x45',
    '\x46', '\x47', '\x48', '\x49', '\x4a', '\x4b', '\x4c', '\x4d', '\x4e', '\x4f', '\x50', '\x51', '\x52', '\x53',
    '\x54', '\x55', '\x56', '\x57', '\x58', '\x59', '\x5a', '\x5b', '\x5c', '\x5d', '\x5e', '\x5f', '\x60', 'A',
    'B',    'C',    'D',    'E',    'F',    'G',    'H',    'I',    'J',    'K',    'L',    'M',    'N',    'O',
    'P',    'Q',    'R',    'S',    'T',    'U',    'V',    'W',    'X',    'Y',    'Z',    '\x7b', '\x7c', '\x7d',
    '\x7e', '\x7f', '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b',
    '\x8c', '\x8d', '\x8e', '\x8f', '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99',
    '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f', '\xa0', '\xa1', '\xa2', '\xa3', '\xa4', '\xa5', '\xa6', '\xa7',
    '\xa8', '\xa9', '\xaa', '\xab', '\xac', '\xad', '\xae', '\xaf', '\xb0', '\xb1', '\xb2', '\xb3', '\xb4', '\xb5',
    '\xb6', '\xb7', '\xb8', '\xb9', '\xba', '\xbb', '\xbc', '\xbd', '\xbe', '\xbf', '\xc0', '\xc1', '\xc2', '\xc3',
    '\xc4', '\xc5', '\xc6', '\xc7', '\xc8', '\xc9', '\xca', '\xcb', '\xcc', '\xcd', '\xce', '\xcf', '\xd0', '\xd1',
    '\xd2', '\xd3', '\xd4', '\xd5', '\xd6', '\xd7', '\xd8', '\xd9', '\xda', '\xdb', '\xdc', '\xdd', '\xde', '\xdf',
    '\xe0', '\xe1', '\xe2', '\xe3', '\xe4', '\xe5', '\xe6', '\xe7', '\xe8', '\xe9', '\xea', '\xeb', '\xec', '\xed',
    '\xee', '\xef', '\xf0', '\xf1', '\xf2', '\xf3', '\xf4', '\xf5', '\xf6', '\xf7', '\xf8', '\xf9', '\xfa', '\xfb',
    '\xfc', '\xfd', '\xfe', '\xff',
};
const char *const ascii_upper_table = ascii_upper_table_data;

#define ISSPACE(c) ((c) == ' ' || (c) == '\f' || (c) == '\n' || (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define ISALPHA(c) (ISUPPER(c) || ISLOWER(c))
#define TOUPPER(c) (ISLOWER(c) ? (c) - 'a' + 'A' : (c))
#define TOLOWER(c) (ISUPPER(c) ? (c) - 'A' + 'a' : (c))

CXXKIT_WARNING_PUSH
CXXKIT_WARNING_DISABLE_DEPRECATED
static uint64_t _parse_long_long(const char *nptr, const char **endptr, uint_t base, bool *negative)
{
    /*
     * This code is based on on the strtol(3) code from GNU libc released under the GNU Lesser General Public License.
     * Copyright (C) 1991,92,94,95,96,97,98,99,2000,01,02 Free Software Foundation, Inc.
     */
    bool overflow;
    uint64_t cutoff;
    uint64_t cutlim;
    uint64_t ui64;
    const char *s, *save;
    uchar_t c;

    CXXKIT_ASSERT(nptr != NULL);

    *negative = false;
    if (base == 1 || base > 36)
    {
        errno = EINVAL;
        if (endptr)
        {
            *endptr = nptr;
        }
        return 0;
    }

    save = s = nptr;

    /* Skip white space.  */
    while (ISSPACE(*s))
    {
        ++s;
    }

    if (CXXKIT_UNLIKELY(!*s))
    {
        goto noconv;
    }

    /* Check for a sign.  */
    if (*s == '-')
    {
        *negative = true;
        ++s;
    }
    else if (*s == '+')
    {
        ++s;
    }

    /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
    if (*s == '0')
    {
        if ((base == 0 || base == 16) && TOUPPER(s[1]) == 'X')
        {
            s += 2;
            base = 16;
        }
        else if (base == 0)
        {
            base = 8;
        }
    }
    else if (base == 0)
    {
        base = 10;
    }

    /* Save the pointer so we can check later if anything happened.  */
    save = s;
    cutoff = kUInt64Max / base;
    cutlim = kUInt64Max % base;

    overflow = false;
    ui64 = 0;
    c = *s;
    for (; c; c = *++s)
    {
        if (c >= '0' && c <= '9')
        {
            c -= '0';
        }
        else if (ISALPHA(c))
        {
            c = TOUPPER(c) - 'A' + 10;
        }
        else
        {
            break;
        }
        if (c >= base)
        {
            break;
        }
        /* Check for overflow.  */
        if (ui64 > cutoff || (ui64 == cutoff && c > cutlim))
        {
            overflow = true;
        }
        else
        {
            ui64 *= base;
            ui64 += c;
        }
    }

    /* Check if anything actually happened.  */
    if (s == save)
    {
        goto noconv;
    }

    /* Store in ENDPTR the address of one character
     past the last character we converted.  */
    if (endptr)
    {
        *endptr = s;
    }

    if (CXXKIT_UNLIKELY(overflow))
    {
        strerror(ERANGE);
        return kUInt64Max;
    }

    return ui64;

noconv:
    /* We must handle a special case here: the base is 0 or 16 and the
     first two characters are '0' and 'x', but the rest are no
     hexadecimal digits.  This is no error case.  We return 0 and
     ENDPTR points to the `x`.  */
    if (endptr)
    {
        if (save - nptr >= 2 && TOUPPER(save[-1]) == 'X' && save[-2] == '0')
        {
            *endptr = &save[-1];
        }
        else
        {
            /*  There was no number to convert.  */
            *endptr = nptr;
        }
    }
    return 0;
}
CXXKIT_WARNING_POP

int ascii_digit_value(char c)
{
    if (ascii_isdigit(c))
    {
        return c - '0';
    }
    return -1;
}

int ascii_xdigit_value(char c)
{
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return ascii_digit_value(c);
}

double_t ascii_strtod(const char *nptr, char **endptr)
{
    char *fail_pos = NULL;
    double val;
#ifndef __BIONIC__
    struct lconv *locale_data;
#endif
    const char *decimal_point;
    size_t decimal_point_len;
    const char *p, *decimal_point_pos;
    const char *end = NULL; /* Silence gcc */
    int strtod_errno;

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(nptr != NULL, CXXKIT_NAN, CXXKIT_EINVAL);

#ifndef __BIONIC__
    locale_data = localeconv();
    decimal_point = locale_data->decimal_point;
    decimal_point_len = strlen(decimal_point);
#else
    decimal_point = ".";
    decimal_point_len = 1;
#endif

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(decimal_point_len != 0, CXXKIT_NAN, CXXKIT_EINVAL);

    decimal_point_pos = NULL;
    end = NULL;

    if (decimal_point[0] != '.' || decimal_point[1] != 0)
    {
        p = nptr;
        /* Skip leading space */
        while (ascii_isspace(*p))
        {
            p++;
        }

        /* Skip leading optional sign */
        if (*p == '+' || *p == '-')
        {
            p++;
        }

        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
        {
            p += 2;
            /* HEX - find the (optional) decimal point */

            while (ascii_isxdigit(*p))
            {
                p++;
            }

            if (*p == '.')
            {
                decimal_point_pos = p++;
            }

            while (ascii_isxdigit(*p))
            {
                p++;
            }

            if (*p == 'p' || *p == 'P')
            {
                p++;
            }
            if (*p == '+' || *p == '-')
            {
                p++;
            }
            while (ascii_isdigit(*p))
            {
                p++;
            }

            end = p;
        }
        else if (ascii_isdigit(*p) || *p == '.')
        {
            while (ascii_isdigit(*p))
            {
                p++;
            }

            if (*p == '.')
            {
                decimal_point_pos = p++;
            }

            while (ascii_isdigit(*p))
            {
                p++;
            }

            if (*p == 'e' || *p == 'E')
            {
                p++;
            }
            if (*p == '+' || *p == '-')
            {
                p++;
            }
            while (ascii_isdigit(*p))
            {
                p++;
            }

            end = p;
        }
        /* For the other cases, we need not convert the decimal point */
    }

    if (decimal_point_pos)
    {
        /* We need to convert the '.' to the locale specific decimal point */
        char *copy = (char *)std::malloc(end - nptr + 1 + decimal_point_len);

        char *c = copy;
        std::memcpy(c, nptr, decimal_point_pos - nptr);
        c += decimal_point_pos - nptr;
        std::memcpy(c, decimal_point, decimal_point_len);
        c += decimal_point_len;
        std::memcpy(c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
        c += end - (decimal_point_pos + 1);
        *c = 0;

        errno = 0;
        val = strtod(copy, &fail_pos);
        strtod_errno = errno;

        if (fail_pos)
        {
            if (fail_pos - copy > decimal_point_pos - nptr)
            {
                fail_pos = (char *)nptr + (fail_pos - copy) - (decimal_point_len - 1);
            }
            else
            {
                fail_pos = (char *)nptr + (fail_pos - copy);
            }
        }

        std::free(copy);
    }
    else if (end)
    {
        char *copy = (char *)std::malloc(end - (char *)nptr + 1);
        std::memcpy(copy, nptr, end - nptr);
        *(copy + (end - (char *)nptr)) = 0;

        errno = 0;
        val = strtod(copy, &fail_pos);
        strtod_errno = errno;

        if (fail_pos)
        {
            fail_pos = (char *)nptr + (fail_pos - copy);
        }

        std::free(copy);
    }
    else
    {
        errno = 0;
        val = strtod(nptr, &fail_pos);
        strtod_errno = errno;
    }

    if (endptr)
    {
        *endptr = fail_pos;
    }

    errno = strtod_errno;

    return val;
}

uint64_t ascii_strtoull(const char *nptr, char **endptr, uint_t base)
{
    bool negative;
    uint64_t result;

    result = _parse_long_long(nptr, (const char **)endptr, base, &negative);

    /* Return the result of the appropriate sign.  */
    return negative ? -result : result;
}

int64_t ascii_strtoll(const char *nptr, char **endptr, uint_t base)
{
    bool negative = false;
    uint64_t result = _parse_long_long(nptr, (const char **)endptr, base, &negative);
    if (negative && result > (uint64_t)kInt64Min)
    {
        errno = ERANGE;
        return kInt64Min;
    }
    else if (!negative && result > (uint64_t)kInt64Max)
    {
        errno = ERANGE;
        return kInt64Max;
    }
    else if (negative)
    {
        return -(int64_t)result;
    }
    else
    {
        return (int64_t)result;
    }
}

char *ascii_formatd(char *buffer, int buf_len, const char *format, double_t d)
{
#ifndef __BIONIC__
    struct lconv *locale_data;
#endif
    const char *decimal_point;
    size_t decimal_point_len;
    char *p;
    size_t rest_len;

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(buffer != NULL, NULL, CXXKIT_EINVAL);
    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(format[0] == '%', NULL, CXXKIT_EINVAL);
    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(strpbrk(format + 1, "'l%") == NULL, NULL, CXXKIT_EINVAL);

    char format_char = format[strlen(format) - 1];

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(format_char == 'e' || format_char == 'E' || format_char == 'f' ||
    //                                  format_char == 'F' || format_char == 'g' || format_char == 'G',
    //                                  NULL,
    //                                  CXXKIT_EINVAL);

    if (format[0] != '%')
    {
        return nullptr;
    }

    if (strpbrk(format + 1, "'l%"))
    {
        return nullptr;
    }

    if (!(format_char == 'e' || format_char == 'E' || format_char == 'f' || format_char == 'F' || format_char == 'g' ||
          format_char == 'G'))
    {
        return nullptr;
    }

    snprintf(buffer, buf_len, format, d);

#ifndef __BIONIC__
    locale_data = localeconv();
    decimal_point = locale_data->decimal_point;
    decimal_point_len = strlen(decimal_point);
#else
    decimal_point = ".";
    decimal_point_len = 1;
#endif

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(decimal_point_len != 0, NULL, CXXKIT_EINVAL);

    if (decimal_point[0] != '.' || decimal_point[1] != 0)
    {
        p = buffer;

        while (ascii_isspace(*p))
        {
            p++;
        }

        if (*p == '+' || *p == '-')
        {
            p++;
        }

        while (isdigit((uchar_t)*p))
        {
            p++;
        }

        if (strncmp(p, decimal_point, decimal_point_len) == 0)
        {
            *p = '.';
            p++;
            if (decimal_point_len > 1)
            {
                rest_len = strlen(p + (decimal_point_len - 1));
                memmove(p, p + (decimal_point_len - 1), rest_len);
                p[rest_len] = 0;
            }
        }
    }

    return buffer;
}

int ascii_strcasecmp(const char *s1, const char *s2)
{
    int c1, c2;

    CXXKIT_ASSERT(s1 != NULL);
    CXXKIT_ASSERT(s2 != NULL);

    while (*s1 && *s2)
    {
        c1 = (int)(uchar_t)TOLOWER(*s1);
        c2 = (int)(uchar_t)TOLOWER(*s2);
        if (c1 != c2)
        {
            return (c1 - c2);
        }
        s1++;
        s2++;
    }

    return (((int)(uchar_t)*s1) - ((int)(uchar_t)*s2));
}

int ascii_strncasecmp(const char *s1, const char *s2, size_t n)
{
    int c1, c2;

    CXXKIT_ASSERT(s2 != NULL);
    CXXKIT_ASSERT(s2 != NULL);

    while (n && *s1 && *s2)
    {
        n -= 1;
        c1 = (int)(uchar_t)TOLOWER(*s1);
        c2 = (int)(uchar_t)TOLOWER(*s2);
        if (c1 != c2)
        {
            return (c1 - c2);
        }
        s1++;
        s2++;
    }

    if (n)
    {
        return (((int)(uchar_t)*s1) - ((int)(uchar_t)*s2));
    }
    else
    {
        return 0;
    }
}

char *ascii_strlwr(const char *str, ssize_t len)
{
    CXXKIT_ASSERT(str != NULL);

    if (len < 0)
    {
        len = (ssize_t)strlen(str);
    }

    char *result = String::strndup(str, (size_t)len);
    for (char *s = result; *s; s++)
    {
        *s = ascii_tolower(*s);
    }

    return result;
}

char *ascii_strupr(const char *str, ssize_t len)
{
    CXXKIT_ASSERT(str != NULL);

    if (len < 0)
    {
        len = (ssize_t)strlen(str);
    }

    char *result = String::strndup(str, (size_t)len);
    for (char *s = result; *s; s++)
    {
        *s = ascii_toupper(*s);
    }

    return result;
}

static bool _str_has_sign(const char *str)
{
    return str[0] == '-' || str[0] == '+';
}

static bool _str_has_hex_prefix(const char *str)
{
    return str[0] == '0' && ascii_tolower(str[1]) == 'x';
}

bool ascii_string_to_signed(const char *str, uint_t base, int64_t min, int64_t max, int64_t *out_num)
{
    int64_t number;
    const char *end_ptr = NULL;
    int saved_errno = 0;

    /*
     * We do not allow leading whitespace, but ascii_strtoll accepts it and just skips it, so we need to
     * check for it ourselves.
     */
    const bool isspace = ascii_isspace(str[0]);
    /* We don't support hexadecimal numbers prefixed with 0x or 0X. */
    const bool support = (base == 16 &&
                          (_str_has_sign(str) ? _str_has_hex_prefix(str + 1) : _str_has_hex_prefix(str))) ||
                         (saved_errno != 0 && saved_errno != ERANGE) || end_ptr == NULL || *end_ptr != '\0';

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(min <= max, false, CXXKIT_EINVAL);
    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(str != NULL, false, CXXKIT_EINVAL);
    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(base >= 2 && base <= 36, false, CXXKIT_EINVAL);

    if (str[0] == '\0')
    {
        //TODO
        //        error (error, _NUMBER_PARSER_ERROR, _NUMBER_PARSER_ERROR_INVALID, _("Empty string is not a number"));
        return false;
    }

    errno = 0;
    number = ascii_strtoll(str, (char **)&end_ptr, base);
    saved_errno = errno;

    if (isspace || support)
    {
        //        error (error, _NUMBER_PARSER_ERROR, _NUMBER_PARSER_ERROR_INVALID, _(""%s" is not a signed number"), str);
        return false;
    }
    if (saved_errno == ERANGE || number < min || number > max)
    {
        char min_str[128] = {0};
        char max_str[128] = {0};
        std::snprintf(min_str, sizeof(min_str), "%" CXXKIT_INT64_FORMAT, min);
        std::snprintf(max_str, sizeof(max_str), "%" CXXKIT_INT64_FORMAT, max);
        // CXXKIT_ERROR(error, _NUMBER_PARSER_ERROR, _NUMBER_PARSER_ERROR_OUT_OF_BOUNDS,
        //           _("Number "%s" is out of bounds [%s, %s]"), str, min_str, max_str);
        return false;
    }
    if (out_num != NULL)
    {
        *out_num = number;
    }
    return true;
}

bool ascii_string_to_unsigned(const char *str, uint_t base, uint64_t min, uint64_t max, uint64_t *out_num)
{
    uint64_t number;
    const char *end_ptr = NULL;
    int saved_errno = 0;

    /*
     * We do not allow leading whitespace, but ascii_strtoull accepts it and just skips it, so we need to
     * check for it ourselves.
     */
    const bool isspace = ascii_isspace(str[0]);
    /* Unsigned number should have no sign. */
    const bool has_sign = _str_has_sign(str);
    /* We don't support hexadecimal numbers prefixed with 0x or 0X. */
    const bool support = (base == 16 && _str_has_hex_prefix(str)) || (saved_errno != 0 && saved_errno != ERANGE) ||
                         end_ptr == NULL || *end_ptr != '\0';
    if (str[0] == '\0')
    {
        //        error (error, _NUMBER_PARSER_ERROR, _NUMBER_PARSER_ERROR_INVALID,  _("Empty string is not a number"));
        return false;
    }

    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(min <= max, false, CXXKIT_EINVAL);
    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(str != NULL, false, CXXKIT_EINVAL);
    // CXXKIT_CHECK_SET_STDERR_RETURN_VAL(base >= 2 && base <= 36, false, CXXKIT_EINVAL);

    errno = 0;
    number = ascii_strtoull(str, (char **)&end_ptr, base);
    saved_errno = errno;

    if (isspace || has_sign || support)
    {
        //        error (error, _NUMBER_PARSER_ERROR, _NUMBER_PARSER_ERROR_INVALID,
        //                     _(""%s" is not an unsigned number"), str);
        return false;
    }
    if (saved_errno == ERANGE || number < min || number > max)
    {
        char min_str[128] = {0};
        char max_str[128] = {0};
        std::snprintf(min_str, sizeof(min_str), "%" CXXKIT_INT64_FORMAT, min);
        std::snprintf(max_str, sizeof(max_str), "%" CXXKIT_INT64_FORMAT, max);
        //        error (error, _NUMBER_PARSER_ERROR, _NUMBER_PARSER_ERROR_OUT_OF_BOUNDS,
        //                     _("Number "%s" is out of bounds [%s, %s]"), str, min_str, max_str);
        return false;
    }
    if (out_num != NULL)
    {
        *out_num = number;
    }
    return true;
}

CXXKIT_END_NAMESPACE
