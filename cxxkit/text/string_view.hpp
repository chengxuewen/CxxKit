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

#include <cxxkit/base/global.hpp>
#include <cxxkit/base/core_config.hpp>

#if CXXKIT_BUILD_CXX_STANDARD_17
#    define nssv_CONFIG_SELECT_STRING_VIEW 0
#else
#    define nssv_CONFIG_SELECT_STRING_VIEW 1
#endif
#include <cxxkit/3rdparty/nonstd/string_view.hpp>

CXXKIT_BEGIN_NAMESPACE

using StringView = nonstd::string_view;
using WStringView = nonstd::wstring_view;
using U16StringView = nonstd::u16string_view;
using U32StringView = nonstd::u32string_view;
template <typename CharT, typename Traits>
using BaseStringView = nonstd::basic_string_view<CharT, Traits>;

using nonstd::to_string;
using nonstd::to_string_view;

// CXX14_CONSTEXPR degrades comparison to plain inline under C++11 (nonstd::string_view::compare(basic_string_view)
// is CXX14-only; keeps these operators C++11-usable). text must stay C++11 per AGENTS.md.
inline CXXKIT_CXX14_CONSTEXPR bool operator<(StringView lhs, StringView rhs) noexcept
{
    return lhs.compare(rhs) < 0;
}
inline CXXKIT_CXX14_CONSTEXPR bool operator>(StringView lhs, StringView rhs) noexcept
{
    return rhs < lhs;
}
inline CXXKIT_CXX14_CONSTEXPR bool operator<=(StringView lhs, StringView rhs) noexcept
{
    return !(rhs < lhs);
}
inline CXXKIT_CXX14_CONSTEXPR bool operator>=(StringView lhs, StringView rhs) noexcept
{
    return !(lhs < rhs);
}

struct StringViewCmp
{
    using is_transparent = void;
    bool operator()(StringView lhs, StringView rhs) const { return lhs < rhs; }
};

CXXKIT_END_NAMESPACE
