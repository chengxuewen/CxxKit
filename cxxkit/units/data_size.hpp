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

#ifndef _CXXKIT_DATA_SIZE_HPP
#define _CXXKIT_DATA_SIZE_HPP

#include <cxxkit/units/unit_base.hpp>

#include <type_traits>
#include <cstdint>
#include <string>

CXXKIT_BEGIN_NAMESPACE

// DataSize is a class represeting a count of bytes.
class DataSize final : public RelativeUnit<DataSize>
{
public:
    template <typename T>
    static CXXKIT_CXX14_CONSTEXPR DataSize Bytes(T value)
    {
        static_assert(std::is_arithmetic<T>::value, "");
        return FromValue(value);
    }
    static constexpr DataSize Infinity() { return PlusInfinity(); }

    constexpr DataSize() = default;

    template <typename Sink>
    friend void AbslStringify(Sink &sink, DataSize value);

    template <typename T = int64_t>
    constexpr T bytes() const
    {
        return ToValue<T>();
    }

    constexpr int64_t bytes_or(int64_t fallback_value) const { return ToValueOr(fallback_value); }

private:
    friend class UnitBase<DataSize>;

    using RelativeUnit::RelativeUnit;
    static constexpr bool kOneSided = true;
};

namespace utils
{
CXXKIT_CORE_API std::string toString(DataSize value);

// template <typename Sink>
// void stringify(Sink &sink, DataSize value)
// {
//     sink.Append(toString(value));
// }
} // namespace utils

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_DATA_SIZE_HPP
