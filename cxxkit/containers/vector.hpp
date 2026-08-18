/***********************************************************************************************************************
**
** Library: cxxkit
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

#include <cxxkit/base/global.hpp>

#include <vector>

CXXKIT_BEGIN_NAMESPACE

template <typename T>
class Vector
{
protected:
    using raw_type = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

public:
    class MoveReference
    {
        friend class Vector;

    private:
        Vector<T> &mReference;
        MoveReference(Vector<T> &ref)
            : mReference(ref)
        {
        }
    };

    Vector()
        : mArray(0)
        , mSize(0)
    {
    }
    Vector(T *array, size_t s)
        : mArray(array)
        , mSize(s)
    {
    }

    template <typename Iterable>
    Vector(const Iterable &v)
    {
        mSize = v.size();
        if (v.size() == 0)
        {
            mArray = 0;
        }
        else
        {
            mArray = new T[v.size()];
            size_t i = 0;
            for (typename Iterable::const_iterator it = v.begin(); it != v.end(); ++it)
            {
                mArray[i++] = *it;
            }
        }
    }

    template <typename Iterable, typename Converter>
    Vector(const Iterable &v, Converter convert)
    {
        mSize = v.size();
        if (0 == mSize)
        {
            mArray = 0;
        }
        else
        {
            mArray = new T[v.size()];
            size_t i = 0;
            for (typename Iterable::const_iterator it = v.begin(); it != v.end(); ++it)
            {
                mArray[i++] = convert(*it);
            }
        }
    }

    Vector(std::initializer_list<T> initList)
    {
        mSize = initList.size();
        if (0 == mSize)
        {
            mArray = nullptr;
        }
        else
        {
            mArray = new T[mSize];
            size_t i = 0;
            for (const T &value : initList)
            {
                mArray[i++] = value;
            }
        }
    }

    Vector(const Vector<T> &o)
    {
        mSize = o.mSize;
        if (mSize != 0)
        {
            mArray = new T[o.mSize];
            for (size_t i = 0; i < o.mSize; ++i)
            {
                mArray[i] = o.mArray[i];
            }
        }
    }

    virtual ~Vector() { this->destroyAll(); }

    Vector<T> &operator=(const Vector<T> &o)
    {
        if (mSize < o.mSize)
        {
            this->destroyAll();
            mArray = new T[o.mSize];
        }
        else if (o.mSize == 0 && mSize != 0)
        {
            this->destroyAll();
        }
        mSize = o.mSize;
        for (size_t i = 0; i < o.mSize; ++i)
        {
            mArray[i] = o.mArray[i];
        }
        return *this;
    }

    Vector(MoveReference mr)
        : mArray(mr.mReference.mArray)
        , mSize(mr.mReference.mSize)
    {
    }
    Vector<T> &operator=(MoveReference mr)
    {
        if (mSize != 0)
        {
            this->destroyAll();
        }
        mSize = mr.mReference.mSize;
        mArray = mr.mReference.mArray;
        mr.mReference.mSize = 0;
        mr.mReference.mArray = 0;
        return *this;
    }
    /**
   * Not really safe - can't be used as Vector(something).move(),
   * but Vector tmp(something); other = tmp.move();
   */
    MoveReference move() { return MoveReference(*this); }

    std::vector<T> std_vector() const
    {
        std::vector<T> v;
        v.reserve(mSize);
        for (size_t i = 0; i < mSize; ++i)
        {
            v.push_back(mArray[i]);
        }
        return v;
    }

    const T *data() const { return mArray; }

    size_t size() const { return mSize; }

    T &operator[](size_t i) { return mArray[i]; }

    const T &operator[](size_t i) const { return mArray[i]; }

    void clear() { this->destroyAll(); }

protected:
    void destroy(T *rt) { reinterpret_cast<const T *>(rt)->~T(); }

    void destroyAll()
    {
        for (size_t i = 0; i < mSize; ++i)
        {
            this->destroy(&mArray[i]);
        }
        mSize = 0;
    }


private:
    T *mArray;
    size_t mSize;
};

CXXKIT_END_NAMESPACE
