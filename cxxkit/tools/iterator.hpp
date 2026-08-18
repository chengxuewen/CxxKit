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

#ifndef _CXXKIT_ITERATOR_HPP
#define _CXXKIT_ITERATOR_HPP

#include "cxxkit/base/global.hpp"

#include <iterator>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
/***********************************************************************************************************************
  * like cxx14/17 std::make_reverse_iterator
***********************************************************************************************************************/
template <typename Iterator>
std::reverse_iterator<Iterator> makeReverseIterator(Iterator it)
{
    return std::reverse_iterator<Iterator>(it);
}

/***********************************************************************************************************************
  * like cxx17 std::size
***********************************************************************************************************************/
template <typename C> // SFINAE
constexpr auto size(const C &c) -> decltype(c.size()) { return c.size(); }
template <typename T, std::size_t N>
constexpr std::size_t size(const T (&)[N]) noexcept { return N; }
} // namespace utils

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_ITERATOR_HPP
