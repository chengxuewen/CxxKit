/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2018 The Abseil Authors.
**
** License: MIT License
**
** This file contains a simplified reimplementation of FixedArray (API-compatible subset of
** abseil-cpp `absl/container/fixed_array.h`, originally governed by the Apache License 2.0).
** Modified for CxxKit.
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
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

/** @file
 * @brief FixedArray: a fixed-size, heap-backed array with runtime length.
 *
 * Simplified clean-room reimplementation of the abseil `absl::FixedArray` API subset
 * (construct, subscript, fill, iterators, copy/move). Semantics reference:
 * https://github.com/abseil/abseil-cpp `absl/container/fixed_array.h`.
 *
 * Differences from abseil (deliberate simplifications):
 *  - single @c ::operator new byte allocation for the element storage (no inline/EBO
 *    small-array optimization, no custom allocators, no non-default construction from
 *    iterators);
 *  - @c n = 0 is legal and yields @c data() == nullptr;
 *  - element alignment is limited to @c alignof(std::max_align_t) (the ::operator new
 *    guarantee); over-aligned types are not supported;
 *  - iterators are raw @c T* pointers.
 */

#pragma once

#include <cxxkit/base/global.hpp>

#include <cstddef>
#include <initializer_list>
#include <new>
#include <utility>

CXXKIT_BEGIN_NAMESPACE


namespace detail
{

/** @brief Placement-new functor: default-constructs element @c i at @c p. */
template <typename T>
struct DefaultConstruct
{
    void operator()(void *p, size_t) const { ::new (p) T(); }
};

/** @brief Placement-new functor: copy-constructs every element from @c value. */
template <typename T>
struct CopyConstruct
{
    explicit CopyConstruct(const T &value)
        : mValue(value)
    {
    }

    void operator()(void *p, size_t) const { ::new (p) T(mValue); }

private:
    const T &mValue;
};

/** @brief Placement-new functor: copy-constructs element @c i from @c mSource[i]. */
template <typename T>
struct IndexedCopyConstruct
{
    explicit IndexedCopyConstruct(const T *source)
        : mSource(source)
    {
    }

    void operator()(void *p, size_t i) const { ::new (p) T(mSource[i]); }

private:
    const T *mSource;
};

} // namespace detail

/**
 * @brief A non-resizable array of @p T whose length is fixed at construction time.
 *
 * The backing storage is one raw-byte allocation (::operator new) sized for @c n
 * elements; elements are constructed with placement-new and destroyed explicitly.
 * A default-constructed or moved-from FixedArray is empty (size 0, data() == nullptr).
 *
 * Exception safety (basic guarantee): if an element constructor throws during
 * construction, all already-constructed elements are destroyed and the allocation
 * is freed before the exception propagates.
 *
 * @tparam T Element type.
 */
template <typename T>
class FixedArray
{
public:
    typedef T value_type;
    typedef T *iterator;
    typedef const T *const_iterator;
    typedef T &reference;
    typedef const T &const_reference;
    typedef size_t size_type;

    /// @brief Constructs an array of @p n default-constructed elements (n == 0 is legal).
    explicit FixedArray(size_type n);

    /// @brief Constructs an array of @p n copies of @p value (n == 0 is legal).
    FixedArray(size_type n, const T &value);

    /// @brief Constructs from a braced list; elements are copied in list order.
    FixedArray(std::initializer_list<T> initList);

    /// @brief Deep-copies @p o (new allocation, element-wise copy construction).
    FixedArray(const FixedArray<T> &o);

    /// @brief Transfers @p o's buffer; @p o is left empty (size 0, data() == nullptr).
    FixedArray(FixedArray<T> &&o) CXXKIT_NOEXCEPT;

    ~FixedArray();

    /// @brief Deep-copy assignment (allocates if sizes differ; element-wise copy).
    FixedArray<T> &operator=(const FixedArray<T> &o);

    /// @brief Move assignment: frees own buffer, transfers @p o's; @p o left empty.
    FixedArray<T> &operator=(FixedArray<T> &&o) CXXKIT_NOEXCEPT;

    /// @brief Returns the number of elements.
    size_type size() const;

    /// @brief Returns true when the array holds no elements.
    bool empty() const;

    /// @brief Returns the element storage pointer (nullptr for an empty array).
    T *data();

    /// @brief Returns the element storage pointer (nullptr for an empty array).
    const T *data() const;

    /// @brief Non-bounds-checked element access.
    T &operator[](size_type i);

    /// @brief Non-bounds-checked element access (const).
    const T &operator[](size_type i) const;

    /// @brief Assigns @p value to every element.
    void fill(const T &value);

    /// @brief Returns an iterator to the first element (nullptr for an empty array).
    iterator begin();

    /// @brief Returns an iterator past the last element.
    iterator end();

    /// @brief Returns a const iterator to the first element.
    const_iterator begin() const;

    /// @brief Returns a const iterator past the last element.
    const_iterator end() const;

    /// @brief Returns a const iterator to the first element.
    const_iterator cbegin() const;

    /// @brief Returns a const iterator past the last element.
    const_iterator cend() const;

private:
    /// @brief Allocates raw storage for @p n elements; @p construct(p, i) placement-news each.
    template <typename Constructor>
    void allocate_and_construct(size_type n, Constructor construct);

    /// @brief Destroys all elements and frees the single allocation.
    void destroy_all() CXXKIT_NOEXCEPT;

    T *mData;
    size_type mSize;
};

/**
 * @brief Private helper: allocates @p n elements and constructs each in place via
 * @p construct(p, i) (placement-new functor; see detail/ construct helpers).
 *
 * Basic exception guarantee: if any construction throws, elements constructed so
 * far are destroyed and the raw allocation is freed before rethrowing.
 */
template <typename T>
template <typename Constructor>
void FixedArray<T>::allocate_and_construct(size_type n, Constructor construct)
{
    mSize = n;
    if (n == 0)
    {
        mData = nullptr;
        return;
    }
    mData = static_cast<T *>(::operator new(n * sizeof(T)));
    size_type i = 0;
    try
    {
        for (; i < n; ++i)
        {
            construct(static_cast<void *>(mData + i), i);
        }
    }
    catch (...)
    {
        while (i > 0)
        {
            --i;
            mData[i].~T();
        }
        ::operator delete(mData);
        mData = nullptr;
        mSize = 0;
        throw;
    }
}

template <typename T>
void FixedArray<T>::destroy_all() CXXKIT_NOEXCEPT
{
    if (mData == nullptr)
    {
        return;
    }
    for (size_type i = mSize; i > 0; --i)
    {
        mData[i - 1].~T();
    }
    ::operator delete(mData);
    mData = nullptr;
    mSize = 0;
}

template <typename T>
FixedArray<T>::FixedArray(size_type n)
{
    allocate_and_construct(n, detail::DefaultConstruct<T>());
}

template <typename T>
FixedArray<T>::FixedArray(size_type n, const T &value)
{
    allocate_and_construct(n, detail::CopyConstruct<T>(value));
}
template <typename T>
FixedArray<T>::FixedArray(std::initializer_list<T> initList)
{
    const T *first = initList.begin();
    allocate_and_construct(initList.size(), detail::IndexedCopyConstruct<T>(first));
}
template <typename T>
FixedArray<T>::FixedArray(const FixedArray<T> &o)
{
    allocate_and_construct(o.mSize, detail::IndexedCopyConstruct<T>(o.mData));
}

template <typename T>
FixedArray<T>::FixedArray(FixedArray<T> &&o) CXXKIT_NOEXCEPT : mData(o.mData), mSize(o.mSize)
{
    o.mData = nullptr;
    o.mSize = 0;
}

template <typename T>
FixedArray<T>::~FixedArray()
{
    destroy_all();
}

template <typename T>
FixedArray<T> &FixedArray<T>::operator=(const FixedArray<T> &o)
{
    if (this == &o)
    {
        return *this;
    }
    if (mSize == o.mSize)
    {
        for (size_type i = 0; i < mSize; ++i)
        {
            mData[i] = o.mData[i];
        }
        return *this;
    }
    // Sizes differ: build a copy first (basic guarantee), then swap in.
    FixedArray<T> tmp(o);
    destroy_all();
    mData = tmp.mData;
    mSize = tmp.mSize;
    tmp.mData = nullptr;
    tmp.mSize = 0;
    return *this;
}

template <typename T>
FixedArray<T> &FixedArray<T>::operator=(FixedArray<T> &&o) CXXKIT_NOEXCEPT
{
    if (this == &o)
    {
        return *this;
    }
    destroy_all();
    mData = o.mData;
    mSize = o.mSize;
    o.mData = nullptr;
    o.mSize = 0;
    return *this;
}

template <typename T>
inline typename FixedArray<T>::size_type FixedArray<T>::size() const
{
    return mSize;
}

template <typename T>
inline bool FixedArray<T>::empty() const
{
    return mSize == 0;
}

template <typename T>
inline T *FixedArray<T>::data()
{
    return mData;
}

template <typename T>
inline const T *FixedArray<T>::data() const
{
    return mData;
}

template <typename T>
inline T &FixedArray<T>::operator[](size_type i)
{
    return mData[i];
}

template <typename T>
inline const T &FixedArray<T>::operator[](size_type i) const
{
    return mData[i];
}

template <typename T>
void FixedArray<T>::fill(const T &value)
{
    for (size_type i = 0; i < mSize; ++i)
    {
        mData[i] = value;
    }
}

template <typename T>
inline typename FixedArray<T>::iterator FixedArray<T>::begin()
{
    return mData;
}

template <typename T>
inline typename FixedArray<T>::iterator FixedArray<T>::end()
{
    return mData + mSize;
}

template <typename T>
inline typename FixedArray<T>::const_iterator FixedArray<T>::begin() const
{
    return mData;
}

template <typename T>
inline typename FixedArray<T>::const_iterator FixedArray<T>::end() const
{
    return mData + mSize;
}

template <typename T>
inline typename FixedArray<T>::const_iterator FixedArray<T>::cbegin() const
{
    return mData;
}

template <typename T>
inline typename FixedArray<T>::const_iterator FixedArray<T>::cend() const
{
    return mData + mSize;
}

CXXKIT_END_NAMESPACE
