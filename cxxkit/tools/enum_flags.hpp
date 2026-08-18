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

#include <cxxkit/tools/type_info.hpp>

/**
 * @addtogroup core
 * @{
 * @addtogroup EnumFlags
 * @brief The \ref EnumFlags class template is used to store OR-combinations of enum values in a type-safe way.
 * @{
 * @details
 * The values are stored internally inside an integer (unsigned or signed, depending on the underlying type of the enum).
 * Using values from other enums or raw integer (except 0) with this class will result in a compile time error.
 *
 * In order to use this class with your own enum, use CXXKIT_DECLARE_ENUM_FLAGS() and CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS().
 *
 * Typical Usage
 * -------------
 *
 * @code{.cpp}
 *
 * enum class my_option
 * {
 *     ALL_DISABLED    = 0,
 *     OPTION_1        = 1,
 *     OPTION_2        = 2,
 *     OPTION_3        = 4,
 *     OPTION_4        = 8
 * };
 *
 * CXXKIT_DECLARE_ENUM_FLAGS(my_options, my_option)
 * CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(my_options)
 *
 * void my_func(my_options flags)
 * {
 *     if (flags.test_flag(my_option::OPTION_1)
 *     {
 *         // ...
 *     }
 *
 *     if (flags.test_flag(my_option::OPTION_2)
 *     {
 *         // ...
 *     }
* }
 *
 * @endcode
 */

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
class EnumFlag
{
public:
    CXXKIT_CONSTEXPR inline EnumFlag(int value) CXXKIT_NOEXCEPT : mValue(value) { }
    CXXKIT_CONSTEXPR inline operator int32_t() const CXXKIT_NOEXCEPT { return mValue; }

private:
    int32_t mValue;
};

class InvalidEnumFlag
{
public:
    CXXKIT_CONSTEXPR inline explicit InvalidEnumFlag(int value) CXXKIT_NOEXCEPT : mValue(value) { }
    CXXKIT_CONSTEXPR inline operator int() const noexcept { return mValue; }

private:
    int32_t mValue;
};
} // namespace detail

template <typename Enum>
class EnumFlags
{
    static_assert(sizeof(Enum) <= sizeof(int32_t),
                  "Cannot store enums value with the given type."
                  "Please use an enum which fits into an 'int32_t'.");

public:
    using type = Enum;
    using Value =
        traits::conditional_t<std::is_signed<typename std::underlying_type<Enum>::type>::value, int32_t, uint32_t>;
    using Zero = Value *;

    /**
     * Creates an EnumFlags object with no flags set.
     */
    CXXKIT_CONSTEXPR inline EnumFlags(Zero = nullptr) CXXKIT_NOEXCEPT : mValue(0) { }

    /**
     * Creates a EnumFlags object with the given flag \p flag.
     */
    CXXKIT_CONSTEXPR inline EnumFlags(Enum flag) CXXKIT_NOEXCEPT : mValue(static_cast<Value>(flag)) { }

    /**
     * Creates an EnumFlags object initialized with the given integer value \p value.
     *
     * \remark EnumFlag is a wrapper class around an integer to avoid creation
     *          of EnumFlags object from enum values.
     */
    CXXKIT_CONSTEXPR inline EnumFlags(detail::EnumFlag v) CXXKIT_NOEXCEPT : mValue(v) { }

    CXXKIT_CONSTEXPR inline EnumFlags(std::initializer_list<Enum> flags) CXXKIT_NOEXCEPT
        : mValue(initializer_list_helper(flags.begin(), flags.end()))
    {
    }

    /**
     * @brief Performs a bitwise `AND` operation with \p mask
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator&=(int mask) CXXKIT_NOEXCEPT
    {
        mValue &= mask;
        return *this;
    }

    /**
     * @brief Performs a bitwise `AND` operation with @p mask
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator&=(uint32_t mask) CXXKIT_NOEXCEPT
    {
        mValue &= mask;
        return *this;
    }

    /**
     * @brief Performs a bitwise `AND` operation with \p mask
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator&=(Enum mask) CXXKIT_NOEXCEPT
    {
        mValue &= static_cast<Value>(mask);
        return *this;
    }

    /**
     * @brief Performs a bitwise `OR` operation with \p f
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator|=(EnumFlags f) CXXKIT_NOEXCEPT
    {
        mValue |= f.mValue;
        return *this;
    }

    /**
     * @brief Performs a bitwise `OR` operation with \p f
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator|=(Enum f) CXXKIT_NOEXCEPT
    {
        mValue |= static_cast<Value>(f);
        return *this;
    }

    /**
     * @brief Performs a bitwise `XOR` operation with \p f
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator^=(EnumFlags f) CXXKIT_NOEXCEPT
    {
        mValue ^= f.mValue;
        return *this;
    }

    /**
     * @brief Performs a bitwise `XOR` operation with \p f
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline EnumFlags &operator^=(Enum f) CXXKIT_NOEXCEPT
    {
        mValue ^= static_cast<Value>(f);
        return *this;
    }

    /**
     * @brief Performs a bitwise `XOR` operation with \p f
     *        and store the result in this object.
     *
     * @return A reference to this object.
     */
    CXXKIT_CXX14_CONSTEXPR inline operator Value() const CXXKIT_NOEXCEPT { return mValue; }

    /**
     * @brief Performs a bitwise `OR` operation on this object and \p f
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `OR` operation on this object and \p f.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator|(Enum f) const CXXKIT_NOEXCEPT
    {
        return EnumFlags(mValue | static_cast<Value>(f));
    }

    /**
     * @brief Performs a bitwise `OR` operation on this object and \p f
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `OR` operation on this object and \p f.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator|(EnumFlags f) const CXXKIT_NOEXCEPT { return EnumFlags(mValue | f.mValue); }

    /**
     * @brief Performs a bitwise `XOR` operation on this object and \p f
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `XOR` operation on this object and \p f.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator^(Enum f) const CXXKIT_NOEXCEPT
    {
        return EnumFlags(mValue ^ static_cast<Value>(f));
    }

    /**
     * @brief Performs a bitwise `XOR` operation on this object and \p f
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `XOR` operation on this object and \p f.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator^(EnumFlags f) const CXXKIT_NOEXCEPT { return EnumFlags(mValue ^ f.mValue); }

    /**
     * @brief Performs a bitwise `AND` operation on this object and \p f
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `AND` operation on this object and \p f.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator&(Enum f) const CXXKIT_NOEXCEPT
    {
        return EnumFlags(mValue & static_cast<Value>(f));
    }

    /**
     * @brief Performs a bitwise `AND` operation on this object and \p mask
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `AND` operation on this object and \p mask.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator&(int mask) const CXXKIT_NOEXCEPT { return EnumFlags(mValue & mask); }

    /**
     * @brief Performs a bitwise `AND` operation on this object and \p mask
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object containing the result of the bitwise `AND` operation on this object and \p mask.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator&(uint32_t mask) const CXXKIT_NOEXCEPT { return EnumFlags(mValue & mask); }

    /**
     * @brief Performs a bitwise negation of the current object
     *        and return the result as new EnumFlags object.
     *
     * @return A EnumFlags object that contains the bitwise negation of this object.
     */
    CXXKIT_CONSTEXPR inline EnumFlags operator~() const CXXKIT_NOEXCEPT { return EnumFlags(~mValue); }

    /**
     * @brief This will test whether a flag was set or not.
     *
     * @return `true`, when no flag is set, otherwise `false`.
     */
    CXXKIT_CONSTEXPR inline bool operator!() const CXXKIT_NOEXCEPT { return (!mValue); }

    /**
     * @brief This will test whether the given flag \p flag was set.
     *
     * @return `true`, when the flag is set, otherwise `false`.
     */
    CXXKIT_CONSTEXPR inline bool testFlag(Enum flag) const CXXKIT_NOEXCEPT
    {
        return ((mValue & static_cast<Value>(flag)) == static_cast<Value>(flag) &&
                (static_cast<Value>(flag) != 0 || mValue == static_cast<Value>(flag)));
    }

    CXXKIT_CONSTEXPR inline EnumFlags &setFlag(Enum flag, bool on = true) CXXKIT_NOEXCEPT
    {
        return on ? (*this |= flag) : (*this &= ~Int(flag));
    }

private:
    CXXKIT_CONSTEXPR static inline Value initializer_list_helper(typename std::initializer_list<Enum>::const_iterator it,
                                                               typename std::initializer_list<Enum>::const_iterator end)
        CXXKIT_NOEXCEPT
    {
        return (it == end ? Value(0) : (Value(*it) | initializer_list_helper(it + 1, end)));
    }

    Value mValue;
};

CXXKIT_END_NAMESPACE

#define CXXKIT_DECLARE_ENUM_FLAGS(Flags, Enum) using Flags = cxxkit::EnumFlags<Enum>;

#define CXXKIT_DECLARE_ENUM_FLAGS_OPERATORS(Flags)                                                                       \
    CXXKIT_CONSTEXPR inline cxxkit::EnumFlags<Flags::type> operator|(Flags::type lhs, Flags::type rhs) CXXKIT_NOEXCEPT       \
    {                                                                                                                  \
        return (cxxkit::EnumFlags<Flags::type>(lhs) | rhs);                                                              \
    }                                                                                                                  \
    CXXKIT_CONSTEXPR inline cxxkit::EnumFlags<Flags::type> operator|(Flags::type lhs, cxxkit::EnumFlags<Flags::type> rhs)    \
        CXXKIT_NOEXCEPT                                                                                                  \
    {                                                                                                                  \
        return (rhs | lhs);                                                                                            \
    }                                                                                                                  \
    CXXKIT_CONSTEXPR inline cxxkit::detail::InvalidEnumFlag operator|(Flags::type lhs, int rhs)                            \
    {                                                                                                                  \
        return cxxkit::detail::InvalidEnumFlag(int(lhs) | rhs);                                                          \
    }

template <typename T>
CXXKIT_DECLARE_TYPEINFO_BODY(cxxkit::EnumFlags<T>, CXXKIT_PRIMITIVE_TYPE);

/**
 * @}
 * @}
 */
