/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#include <cxxkit/containers/vector.hpp>

#include <map>

CXXKIT_BEGIN_NAMESPACE

template <typename K, typename V>
class Pair
{
public:
    K key;
    V value;
};

template <typename T>
class Identity
{
    T operator()(const T &x) { return x; }
};

template <typename K, typename V>
class VectorMap
{
    typedef Pair<K, V> Item;

    Vector<Item> mData;

    template <typename K2, typename KC, typename V2, typename VC>
    static Item *to_array(const std::map<K2, V2> &m, KC convertKey, VC convertValue)
    {
        Item *data = new Item[m.size()];
        Item *dp = data;
        for (typename std::map<K2, V2>::const_iterator it = m.begin(); it != m.end(); ++it)
        {
            dp->key = convertKey(it->first);
            dp->value = convertValue(it->second);
            ++dp;
        }
        return data;
    }

public:
    class MoveReference
    {
        friend class VectorMap;

        VectorMap<K, V> &mReference;
        MoveReference(VectorMap<K, V> &ref)
            : mReference(ref)
        {
        }
    };

    VectorMap() { }

    VectorMap(const std::map<K, V> &m)
        : mData(to_array(m, Identity<K>(), Identity<V>()), m.size())
    {
    }

    template <typename K2, typename KC, typename V2, typename VC>
    VectorMap(const std::map<K2, V2> &m, KC convertKey = Identity<K>(), VC convertValue = Identity<V>())
        : mData(to_array(m, convertKey, convertValue), m.size())
    {
    }

    VectorMap(const VectorMap<K, V> &o) { mData = o.mData; }

    VectorMap<K, V> &operator=(const VectorMap<K, V> &o)
    {
        mData = o.mData;
        return *this;
    }

    VectorMap(MoveReference mr)
        : mData(mr.mReference.mData.move())
    {
    }
    VectorMap<K, V> &operator=(MoveReference mr)
    {
        mData = mr.mReference.mData.move();
        return *this;
    }
    MoveReference move() { return MoveReference(*this); }

    std::map<K, V> std_map() const
    {
        std::map<K, V> map;
        for (size_t i = 0; i < mData.size(); ++i)
        {
            const Item *dp = mData.data() + i;
            map[convertKey(dp->key)] = convertValue(dp->value);
        }
        return map;
    }

    template <typename K2, typename KC, typename V2, typename VC>
    std::map<K2, V2> std_map(KC convertKey, VC convertValue) const
    {
        std::map<K2, V2> m;
        for (size_t i = 0; i < mData.size(); ++i)
        {
            const Item *dp = mData.data() + i;
            m[convertKey(dp->key)] = convertValue(dp->value);
        }
        return m;
    }

    template <typename K2>
    const Item *get(K2 key, int (*cmp)(K2, const K &)) const
    {
        for (size_t i = 0; i < mData.size(); ++i)
        {
            const Item *dp = mData.data() + i;
            if (!cmp(key, dp->key))
            {
                return dp;
            }
        }
        return 0;
    }

    const Item *data() const { return mData.data(); }

    size_t size() const { return mData.size(); }
};


CXXKIT_END_NAMESPACE
