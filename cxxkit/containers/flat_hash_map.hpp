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
// Swiss Table hash map — C++11, no SIMD, no custom allocator
// Port from abseil-cpp/flat_hash_map.h (20220623.2)

#include <cxxkit/base/global.hpp>
#include <cxxkit/containers/detail/raw_hash_set.hpp>
#include <functional>
#include <stdexcept>
#include <utility>

/**
 * @file flat_hash_map.hpp
 * @brief Swiss Table hash map — high-performance open-addressing associative container.
 *
 * Port from abseil-cpp/flat_hash_map.h (20220623.2). C++11 compatible, no SIMD,
 * no custom allocator. Uses Swiss Table algorithm with H1/H2 hash separation and
 * quadratic probing for ~5-10x faster than std::unordered_map.
 *
 * @see flat_hash_set
 * @see detail::raw_hash_set
 */

CXXKIT_BEGIN_NAMESPACE

// Policy for flat_hash_map: value_type = pair<K,V>, key = first
template <class K, class V>
struct FlatHashMapPolicy
{
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<K, V>;

    static const key_type &key(const value_type &v) { return v.first; }

    template <class... Args>
    static value_type construct(Args &&...args)
    {
        return value_type(std::forward<Args>(args)...);
    }
};

template <class K, class V, class Hash = std::hash<K>, class Eq = std::equal_to<K>>
class flat_hash_map : public detail::raw_hash_set<FlatHashMapPolicy<K, V>, Hash, Eq>
{
    using Base = detail::raw_hash_set<FlatHashMapPolicy<K, V>, Hash, Eq>;

public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<K, V>;
    using Base::Base;

    // operator[] — insert default-constructed mapped_type if key missing
    V &operator[](const K &key)
    {
        auto res = this->emplace(key, V());
        return res.first->second;
    }

    V &operator[](K &&key)
    {
        auto res = this->emplace(std::move(key), V());
        return res.first->second;
    }

    // at() — bounds-checked access
    V &at(const K &key)
    {
        auto it = this->find(key);
        if (it == this->end())
            throw std::out_of_range("flat_hash_map::at");
        return it->second;
    }
    const V &at(const K &key) const
    {
        auto it = this->find(key);
        if (it == this->end())
            throw std::out_of_range("flat_hash_map::at");
        return it->second;
    }
};

CXXKIT_END_NAMESPACE