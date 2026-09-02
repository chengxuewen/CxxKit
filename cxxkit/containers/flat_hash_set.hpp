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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
***********************************************************************************************************************/

#pragma once
// Swiss Table hash set — C++11, no SIMD, no custom allocator
// Port from abseil-cpp/flat_hash_set.h (20220623.2)

#include <cxxkit/base/global.hpp>
#include <cxxkit/containers/detail/raw_hash_set.hpp>
#include <functional>

/**
 * @file flat_hash_set.hpp
 * @brief Swiss Table hash set — high-performance open-addressing set container.
 *
 * Port from abseil-cpp/flat_hash_set.h (20220623.2). C++11 compatible, no SIMD,
 * no custom allocator. Uses Swiss Table algorithm with H1/H2 hash separation and
 * quadratic probing for ~5-10x faster than std::unordered_set.
 *
 * @see flat_hash_map
 * @see detail::raw_hash_set
 */

CXXKIT_BEGIN_NAMESPACE

// Policy for flat_hash_set: value_type = K, key = identity
template <class K>
struct FlatHashSetPolicy
{
    using key_type = K;
    using value_type = K;

    static const key_type &key(const value_type &v) { return v; }

    template <class... Args>
    static value_type construct(Args &&...args)
    {
        return value_type(std::forward<Args>(args)...);
    }
};

template <class K, class Hash = std::hash<K>, class Eq = std::equal_to<K>>
class flat_hash_set : public detail::raw_hash_set<FlatHashSetPolicy<K>, Hash, Eq>
{
    using Base = detail::raw_hash_set<FlatHashSetPolicy<K>, Hash, Eq>;

public:
    using key_type = K;
    using value_type = K;
    using Base::Base;
};

CXXKIT_END_NAMESPACE