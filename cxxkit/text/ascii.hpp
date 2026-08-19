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

#include <cxxkit/base/types.hpp>
#include <cxxkit/text/string_view.hpp>

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief 29 bytes should enough for all possible values that ascii_dtostr can produce.
 * Then add 10 for good measure
 */
#define CXXKIT_ASCII_DTOSTR_BUF_SIZE (29 + 10)

/**
 * @brief Functions like the ones in <ctype.h> that are not affected by locale.
 */
typedef enum
{
    CXXKIT_ASCII_ALNUM = 1 << 0,
    CXXKIT_ASCII_ALPHA = 1 << 1,
    CXXKIT_ASCII_CNTRL = 1 << 2,
    CXXKIT_ASCII_DIGIT = 1 << 3,
    CXXKIT_ASCII_GRAPH = 1 << 4,
    CXXKIT_ASCII_LOWER = 1 << 5,
    CXXKIT_ASCII_PRINT = 1 << 6,
    CXXKIT_ASCII_PUNCT = 1 << 7,
    CXXKIT_ASCII_SPACE = 1 << 8,
    CXXKIT_ASCII_UPPER = 1 << 9,
    CXXKIT_ASCII_XDIGIT = 1 << 10
} ascii_t;

/**
 * @brief Declaration for an array of bitfields holding character information.
 */
CXXKIT_CORE_API extern const uint16_t *const ascii_table;

/**
 * @brief Declaration for the array of characters to lower-case characters.
 */
CXXKIT_CORE_API extern const char *const ascii_lower_table;

/**
 * @brief Declaration for the array of characters to upper-case characters.
 */
CXXKIT_CORE_API extern const char *const ascii_upper_table;

/**
 * @brief Determines whether the given character is an alphanumeric character.
 */
static CXXKIT_FORCE_INLINE bool ascii_isalnum(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_ALNUM) != 0);
}

/**
 * @brief Determines whether the given character is an alphabetic character.
 */
static CXXKIT_FORCE_INLINE bool ascii_isalpha(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_ALPHA) != 0);
}

/**
 * @brief Determines whether the given character is a control character.
 */
static CXXKIT_FORCE_INLINE bool ascii_iscntrl(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_CNTRL) != 0);
}

/**
 * @brief Determines whether the given character can be represented as a decimal digit character (i.e. {0-9}).
 */
static CXXKIT_FORCE_INLINE bool ascii_isdigit(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_DIGIT) != 0);
}

/**
 * @brief Determines whether the given character has a graphical representation.
 */
static CXXKIT_FORCE_INLINE bool ascii_isgraph(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_GRAPH) != 0);
}

/**
 * @brief Determines whether the given character is lowercase.
 */
static CXXKIT_FORCE_INLINE bool ascii_islower(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_LOWER) != 0);
}

/**
 * @brief Determines whether the given character is printable, including spaces.
 */
static CXXKIT_FORCE_INLINE bool ascii_isprint(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_PRINT) != 0);
}

/**
 * @brief Determines whether the given character is a punctuation character.
 */
static CXXKIT_FORCE_INLINE bool ascii_ispunct(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_PUNCT) != 0);
}

/**
 * @brief Determines whether the given character is a blank character (space or tab).
 */
static CXXKIT_FORCE_INLINE bool ascii_isblank(unsigned char c)
{
    return (' ' == c || '\t' == c);
}

/**
 * @brief Determines whether the given character is a whitespace character
 * (space, tab, vertical tab, formfeed, linefeed, or carriage return).
 * ((c) == ' ' || (c) == '\\f' || (c) == '\\n' || (c) == '\\r' || (c) == '\\t' || (c) == '\\v')
 */
static CXXKIT_FORCE_INLINE bool ascii_isspace(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_SPACE) != 0);
}

/**
 * @brief Determines whether the given character is uppercase.
 */
static CXXKIT_FORCE_INLINE bool ascii_isupper(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_UPPER) != 0);
}

/**
 * @brief Determines whether the given character can be represented as a hexadecimal digit character
 * (i.e. {0-9} or {A-F}).
 */
static CXXKIT_FORCE_INLINE bool ascii_isxdigit(unsigned char c)
{
    return ((ascii_table[c] & CXXKIT_ASCII_XDIGIT) != 0);
}

/**
 * @brief Determines whether the given character is ASCII.
 */
static CXXKIT_FORCE_INLINE bool ascii_isascii(unsigned char c)
{
    return c < 128;
}

/**
 * @brief Convert a character to ASCII lower case.
 *
 * Unlike the standard C library tolower() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are lower case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #unsigned char
 * before passing a possibly non-ASCII character in.
 *
 * @param c any character
 * @return the result of converting @c to lower case. If @c is not an ASCII upper case letter, @c is returned unchanged.
 */
static CXXKIT_FORCE_INLINE char ascii_tolower(unsigned char c)
{
    return ascii_lower_table[c];
}

/**
 * @brief Convert a character to ASCII upper case.
 *
 * Unlike the standard C library toupper() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are upper case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #unsigned char
 * before passing a possibly non-ASCII character in.
 *
 * @param c any character
 * @return the result of converting @c to upper case. If @c is not
 * an ASCII lower case letter, @c is returned unchanged.
 */
static CXXKIT_FORCE_INLINE char ascii_toupper(unsigned char c)
{
    return ascii_upper_table[c];
}

/**
 * @brief Determines the numeric value of a character as a decimal digit.
 * Differs from unichar_digit_value() because it takes a char, so there's no worry about sign extension
 * if characters are signed.
 *
 * @param c an ASCII character
 * @return If @c is a decimal digit (according to ascii_is_digit()), its numeric value. Otherwise, -1.
 */
CXXKIT_CORE_API int ascii_digit_value(char c);

/**
 * @brief Determines the numeric value of a character as a hexadecimal digit.
 * Differs from unichar_xdigit_value() because it takes a char, so there's no worry about sign extension
 * if characters are signed.
 *
 * @param c an ASCII character.
 * @return If @c is a hex digit (according to CXXKIT_ASCII_ISXDIGIT()), its numeric value. Otherwise, -1.
 */
CXXKIT_CORE_API int ascii_xdigit_value(char c);

/**
 * @brief Converts a string to a double value.
 *
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually changing
 * the current locale, since that would not be thread-safe.
 * A limitation of the implementation is that this function
 * will still accept localized versions of infinities and NANs.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtod() function.
 *
 * To convert from a double to a string in a locale-insensitive
 * way, use ascii_dtostr().
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 *
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * @param nptr      the string to convert to a numeric value.
 * @param endptr    (out) (transfer none) (optional): if non-%NULL, it returns
 * the character after the last character used in the conversion.
 * @return the double value.
 */
CXXKIT_CORE_API double_t ascii_strtod(const char *nptr, char **endptr);

/**
 * @brief Converts a string to a uint64_t value.
 * This function behaves like the standard strtoull() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * Note that input with a leading minus sign (`-`) is accepted, and will return
 * the negation of the parsed number, unless that would overflow a uint64_t.
 * Critically, this means you cannot assume that a short fixed length input will
 * never result in a low return value, as the input could have a leading `-`.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtoull() function.
 *
 * If the correct value would cause overflow, %CXXKIT_UINT64_MAX
 * is returned, and `CXXKIT_ERANGE` is stored in `errno`.
 * If the base is outside the valid range, zero is returned, and
 * `CXXKIT_EINVAL` is stored in `errno`.
 * If the string conversion fails, zero is returned, and @a endptr returns
 * @a nptr (if @a endptr is non-%NULL).
 *
 * @param nptr      the string to convert to a numeric value.
 * @param endptr    (out) (transfer none) (optional): if non-%NULL, it returns the
 * character after the last character used in the conversion.
 * @param base      to be used for the conversion, 2..36 or 0
 * @return the uint64_t value or zero on error.
 */
CXXKIT_CORE_API uint64_t ascii_strtoull(const char *nptr, char **endptr, unsigned int base);

/**
 * @brief Converts a string to a int64_t value.
 * This function behaves like the standard strtoll() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration
 * files or other non-user input that should be locale independent.
 * To handle input from the user you should normally use the
 * locale-sensitive system strtoll() function.
 *
 * If the correct value would cause overflow, %CXXKIT_INT64_MAX or %CXXKIT_INT64_MIN
 * is returned, and `ERANGE` is stored in `errno`.
 * If the base is outside the valid range, zero is returned, and
 * `EINVAL` is stored in `errno`. If the
 * string conversion fails, zero is returned, and @a endptr returns @a nptr
 * (if @a endptr is non-%NULL).
 *
 * @param nptr: the string to convert to a numeric value.
 * @param endptr: (out) (transfer none) (optional): if non-%NULL, it returns the
 * character after the last character used in the conversion.
 * @param base: to be used for the conversion, 2..36 or 0
 * @return the int64_t value or zero on error.
 */
CXXKIT_CORE_API int64_t ascii_strtoll(const char *nptr, char **endptr, unsigned int base);

/**
 * @brief Converts a double to a string, using the '.' as decimal point.
 * To format the number you pass in a printf()-style format string.
 * Allowed conversion specifiers are 'e', 'E', 'f', 'F', 'g' and 'G'.
 *
 * The @a format must just be a single format specifier starting with `%`, expecting a double argument.
 *
 * The returned buffer is guaranteed to be nul-terminated.
 *
 * If you just want to want to serialize the value into a string, use CXXKIT_ascii_dtostr().
 *
 * @param buffer    A buffer to place the resulting string in
 * @param buf_len   The length of the buffer.
 * @param format    The printf()-style format to use for the code to use for converting
 * @param d         The double to convert
 * @return          The pointer to the buffer with the converted string.
 */
CXXKIT_CORE_API char *ascii_formatd(char *buffer, int buf_len, const char *format, double_t d);

/**
 * @brief Converts a double to a string, using the '.' as decimal point.
 *
 * This function generates enough precision that converting
 * the string back using ascii_strtod() gives the same machine-number
 * (on machines with IEEE compatible 64bit doubles). It is
 * guaranteed that the size of the resulting string will never
 * be larger than @e CXXKIT_ASCII_DTOSTR_BUF_SIZE bytes, including the terminating
 * nul character, which is always added.
 *
 * @param buffer    A buffer to place the resulting string in
 * @param buf_len   The length of the buffer.
 * @param d         The double to convert
 * @return The pointer to the buffer with the converted string.
 */
static CXXKIT_FORCE_INLINE char *ascii_dtostr(char *buffer, int buf_len, double_t d)
{
    return ascii_formatd(buffer, buf_len, "%.17g", d);
}

/**
 * @brief Compare two strings, ignoring the case of ASCII characters.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * bytes as if they are not letters.
 *
 * This function should be used only on strings that are known to be
 * in encodings where the bytes corresponding to ASCII letters always
 * represent themselves. This includes UTF-8 and the ISO-8859-*
 * charsets, but not for instance double-byte encodings like the
 * Windows Codepage 932, where the trailing bytes of double-byte
 * characters include all ASCII letters. If you compare two CP932
 * strings using this function, you will get false matches.
 *
 * Both @a s1 and @a s2 must be non-%NULL.
 *
 * @param s1    string to compare with @a s2
 * @param s2    string to compare with @a s1
 * @return 0    if the strings match, a negative value if @a s1 < @a s2, or a positive value if @a s1 > @a s2.
 */
CXXKIT_CORE_API int ascii_strcasecmp(const char *s1, const char *s2);

/**
 * @brief Compare @a s1 and @a s2, ignoring the case of ASCII characters and any characters after the first @n in each
 * string.
 * If either string is less than @n bytes long, comparison will stop at the first nul byte encountered.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * characters as if they are not letters.
 *
 * The same warning as in ascii_strcasecmp() applies: Use this
 * function only on strings known to be in encodings where bytes
 * corresponding to ASCII letters always represent themselves.
 *
 * @param s1    string to compare with @a s2
 * @param s2    string to compare with @a s1
 * @param n     number of characters to compare
 * @return 0 if the strings match, a negative value if @a s1 < @a s2, or a positive value if @a s1 > @a s2.
 */
CXXKIT_CORE_API int ascii_strncasecmp(const char *s1, const char *s2, size_t n);

/**
 * @brief Converts all upper case ASCII letters to lower case ASCII letters.
 *
 * @param str   a string
 * @param len   length of @a str in bytes, or -1 if @a str is nul-terminated
 * @return a newly-allocated string, with all the upper case characters in @a str converted to lower case, with
 * semantics that exactly match ascii_tolower().
 * (Note that this is unlike the old strlwr(), which modified the string in place.)
 */
CXXKIT_CORE_API char *ascii_strlwr(const char *str, ssize_t len);

/**
 * @brief Converts all lower case ASCII letters to upper case ASCII letters.
 *
 * @param str   a string
 * @param len   length of @a str in bytes, or -1 if @a str is nul-terminated
 * @return a newly allocated string, with all the lower case characters in @a str converted to upper case, with
 * semantics that exactly match ascii_toupper().
 * (Note that this is unlike the old strupr(), which modified the string in place.)
 */
CXXKIT_CORE_API char *ascii_strupr(const char *str, ssize_t len);

/**
 * @brief A convenience function for converting a string to a signed number.
 *
 * This function assumes that @a str contains only a number of the given
 * @a base that is within inclusive bounds limited by @a min and @a max. If
 * this is true, then the converted number is stored in @a out_num. An
 * empty string is not a valid input. A string with leading or
 * trailing whitespace is also an invalid input.
 *
 * @a base can be between 2 and 36 inclusive. Hexadecimal numbers must
 * not be prefixed with "0x" or "0X". Such a problem does not exist
 * for octal numbers, since they were usually prefixed with a zero
 * which does not change the value of the parsed number.
 *
 * Parsing failures result in an error with the %CXXKIT_NUMBER_PARSER_ERROR
 * domain. If the input is invalid, the error code will be
 * %CXXKIT_NUMBER_PARSER_ERROR_INVALID. If the parsed number is out of
 * bounds - %CXXKIT_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS.
 *
 * See ascii_strtoll() if you have more complex needs such as
 * parsing a string which starts with a number, but then has other
 * characters.
 *
 * @param str: a string
 * @param base: base of a parsed number
 * @param min: a lower bound (inclusive)
 * @param max: an upper bound (inclusive)
 * @param out_num: (out) (optional): a return location for a number
 * @return Return true if @a str was a number, otherwise false.
 */
CXXKIT_CORE_API bool ascii_string_to_signed(const char *str,
                                            unsigned int base,
                                            int64_t min,
                                            int64_t max,
                                            int64_t *out_num);

/**
 * @brief A convenience function for converting a string to an unsigned number.
 *
 * This function assumes that @a str contains only a number of the given @a base that is within inclusive bounds
 * limited by @a min and @a max. If this is true, then the converted number is stored in @a out_num.
 * An empty string is not a valid input. A string with leading or trailing whitespace is also an invalid input.
 * A string with a leading sign (`-` or `+`) is not a valid input for the unsigned parser.
 *
 * @a base can be between 2 and 36 inclusive. Hexadecimal numbers must not be prefixed with "0x" or "0X".
 * Such a problem does not exist for octal numbers, since they were usually prefixed with a zero which does not
 * change the value of the parsed number.
 *
 * Parsing failures result in an error with the %CXXKIT_NUMBER_PARSER_ERROR domain.
 * If the input is invalid, the error code will be %CXXKIT_NUMBER_PARSER_ERROR_INVALID.
 * If the parsed number is out of bounds - %CXXKIT_NUMBER_PARSER_ERROR_OUT_OF_BOUNDS.
 *
 * See ascii_strtoull() if you have more complex needs such as parsing a string which starts with a number,
 * but then has other characters.
 *
 * @param str: a string
 * @param base: base of a parsed number
 * @param min: a lower bound (inclusive)
 * @param max: an upper bound (inclusive)
 * @param out_num: (out) (optional): a return location for a number
 * @return: Return true if @a str was a number, otherwise false.
 */
CXXKIT_CORE_API bool ascii_string_to_unsigned(const char *str,
                                              unsigned int base,
                                              uint64_t min,
                                              uint64_t max,
                                              uint64_t *out_num);

/**
 * @brief
 * @param s
 */
static CXXKIT_FORCE_INLINE void ascii_string_tolower(std::string &s)
{
    s = ascii_strlwr(s.data(), s.length());
}

/**
 * @brief
 * @param s
 * @return
 */
static CXXKIT_FORCE_INLINE std::string ascii_string_tolower(StringView s)
{
    return ascii_strlwr(s.data(), s.length());
}

/**
 * @brief
 * @param s
 */
static CXXKIT_FORCE_INLINE void ascii_string_toupper(std::string &s)
{
    s = ascii_strupr(s.data(), s.length());
}

/**
 * @brief
 * @param s
 * @return
 */
static CXXKIT_FORCE_INLINE std::string ascii_string_toupper(StringView s)
{
    return ascii_strupr(s.data(), s.length());
}

CXXKIT_END_NAMESPACE