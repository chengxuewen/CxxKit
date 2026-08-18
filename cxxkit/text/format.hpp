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

#include "cxxkit/base/global.hpp"

#include <cxxkit/3rdparty/fmt/os.h>
#include <cxxkit/3rdparty/fmt/base.h>
#include <cxxkit/3rdparty/fmt/color.h>
#include <cxxkit/3rdparty/fmt/format.h>
#include <cxxkit/3rdparty/fmt/printf.h>
#include <cxxkit/3rdparty/fmt/ranges.h>
#include <cxxkit/3rdparty/fmt/chrono.h>

namespace fmt
{
template <typename Enum>
struct enum_as_int
{
    Enum value;
    explicit enum_as_int(Enum v)
        : value(v)
    {
    }
};
template <typename Enum>
enum_as_int<Enum> as_int(Enum e)
{
    return enum_as_int<Enum>{e};
}
template <typename Enum>
struct formatter<enum_as_int<Enum>> : formatter<int>
{
    template <typename FormatContext>
    typename FormatContext::iterator format(const enum_as_int<Enum> &wrapper, FormatContext &ctx) const
    {
        return formatter<int>::format(static_cast<int>(wrapper.value), ctx);
    }
};
} // namespace fmt

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
namespace fmt = ::fmt;
} // namespace utils

CXXKIT_END_NAMESPACE