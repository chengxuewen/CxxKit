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

#include <cxxkit/memory/zero_memory.hpp>
#include <cxxkit/tools/type_traits.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/containers/array_view.hpp>
#include <cxxkit/tools/checks.hpp>

#include <type_traits>
#include <algorithm>
#include <cstring>
#include <utility>
#include <memory>

#include <stdint.h>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
// (Internal; please don't use outside this file.) Determines if elements of
// type U are compatible with a BufferT<T>. For most types, we just ignore
// top-level const and forbid top-level volatile and require T and U to be
// otherwise equal, but all byte-sized integers (notably char, int8_t, and
// uint8_t) are compatible with each other. (Note: We aim to get rid of this
// behavior, and treat all types the same.)
template <typename T, typename U>
struct BufferCompat
{
    static constexpr bool value = !std::is_volatile<U>::value &&
                                  ((std::is_integral<T>::value && sizeof(T) == 1)
                                       ? (std::is_integral<U>::value && sizeof(U) == 1)
                                       : (std::is_same<T, typename std::remove_const<U>::type>::value));
};
} // namespace detail

// Basic buffer class, can be grown and shrunk dynamically.
// Unlike std::string/vector, does not initialize data when increasing size.
// If "ZeroOnFree" is true, any memory is explicitly cleared before releasing.
// The type alias "ZeroOnFreeBuffer" below should be used instead of setting
// "ZeroOnFree" in the template manually to "true".
template <typename T, bool ZeroOnFree = false>
class BufferT
{
    // We want T's destructor and default constructor to be trivial, i.e. perform
    // no action, so that we don't have to touch the memory we allocate and
    // deallocate. And we want T to be trivially copyable, so that we can copy T
    // instances with std::memcpy. This is precisely the definition of a trivial
    // type.
    static_assert(std::is_trivial<T>::value, "T must be a trivial type.");

    // This class relies heavily on being able to mutate its data.
    static_assert(!std::is_const<T>::value, "T may not be const");

public:
    using value_type = T;
    using const_iterator = const T *;

    // An empty BufferT.
    BufferT()
        : mSize(0)
        , mCapacity(0)
        , mData(nullptr)
    {
        CXXKIT_DCHECK(IsConsistent());
    }

    // Disable copy construction and copy assignment, since copying a buffer is
    // expensive enough that we want to force the user to be explicit about it.
    BufferT(const BufferT &) = delete;
    BufferT &operator=(const BufferT &) = delete;

    BufferT(BufferT &&buf)
        : mSize(buf.size())
        , mCapacity(buf.capacity())
        , mData(std::move(buf.mData))
    {
        CXXKIT_DCHECK(IsConsistent());
        buf.OnMovedFrom();
    }

    // Construct a buffer with the specified number of uninitialized elements.
    explicit BufferT(size_t size)
        : BufferT(size, size)
    {
    }

    BufferT(size_t size, size_t capacity)
        : mSize(size)
        , mCapacity(std::max(size, capacity))
        , mData(mCapacity > 0 ? new T[mCapacity] : nullptr)
    {
        CXXKIT_DCHECK(IsConsistent());
    }

    // Construct a buffer and copy the specified number of elements into it.
    template <typename U, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    BufferT(const U *data, size_t size)
        : BufferT(data, size, size)
    {
    }

    template <typename U, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    BufferT(U *data, size_t size, size_t capacity)
        : BufferT(size, capacity)
    {
        static_assert(sizeof(T) == sizeof(U), "");
        if (size > 0)
        {
            CXXKIT_DCHECK(data);
            std::memcpy(mData.get(), data, size * sizeof(U));
        }
    }

    // Construct a buffer from the contents of an array.
    template <typename U, size_t N, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    BufferT(U (&array)[N])
        : BufferT(array, N)
    {
    }

    ~BufferT() { MaybeZeroCompleteBuffer(); }

    // Implicit conversion to StringView if T is compatible with char.
    template <typename U = T>
    operator typename std::enable_if<detail::BufferCompat<U, char>::value, StringView>::type() const
    {
        return StringView(data<char>(), size());
    }

    // Get a pointer to the data. Just .data() will give you a (const) T*, but if
    // T is a byte-sized integer, you may also use .data<U>() for any other
    // byte-sized integer U.
    template <typename U = T, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    const U *data() const
    {
        CXXKIT_DCHECK(IsConsistent());
        return reinterpret_cast<U *>(mData.get());
    }

    template <typename U = T, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    U *data()
    {
        CXXKIT_DCHECK(IsConsistent());
        return reinterpret_cast<U *>(mData.get());
    }

    bool empty() const
    {
        CXXKIT_DCHECK(IsConsistent());
        return mSize == 0;
    }

    size_t size() const
    {
        CXXKIT_DCHECK(IsConsistent());
        return mSize;
    }

    size_t capacity() const
    {
        CXXKIT_DCHECK(IsConsistent());
        return mCapacity;
    }

    BufferT &operator=(BufferT &&buf)
    {
        CXXKIT_DCHECK(buf.IsConsistent());
        MaybeZeroCompleteBuffer();
        mSize = buf.mSize;
        mCapacity = buf.mCapacity;
        using std::swap;
        swap(mData, buf.mData);
        buf.mData.reset();
        buf.OnMovedFrom();
        return *this;
    }

    bool operator==(const BufferT &buf) const
    {
        CXXKIT_DCHECK(IsConsistent());
        if (mSize != buf.mSize)
        {
            return false;
        }
        if (std::is_integral<T>::value)
        {
            // Optimization.
            return std::memcmp(mData.get(), buf.mData.get(), mSize * sizeof(T)) == 0;
        }
        for (size_t i = 0; i < mSize; ++i)
        {
            if (mData[i] != buf.mData[i])
            {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const BufferT &buf) const { return !(*this == buf); }

    T &operator[](size_t index)
    {
        CXXKIT_DCHECK_LT(index, mSize);
        return data()[index];
    }

    T operator[](size_t index) const
    {
        CXXKIT_DCHECK_LT(index, mSize);
        return data()[index];
    }

    T *begin() { return data(); }
    T *end() { return data() + size(); }
    const T *begin() const { return data(); }
    const T *end() const { return data() + size(); }
    const T *cbegin() const { return data(); }
    const T *cend() const { return data() + size(); }

    // The SetData functions replace the contents of the buffer. They accept the
    // same input types as the constructors.
    template <typename U, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    void SetData(const U *data, size_t size)
    {
        CXXKIT_DCHECK(IsConsistent());
        const size_t old_size = mSize;
        mSize = 0;
        AppendData(data, size);
        if (ZeroOnFree && mSize < old_size)
        {
            ZeroTrailingData(old_size - mSize);
        }
    }

    template <typename U, size_t N, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    void SetData(const U (&array)[N])
    {
        SetData(array, N);
    }

    template <typename W, typename std::enable_if<HasDataAndSize<const W, const T>::value>::type * = nullptr>
    void SetData(const W &w)
    {
        SetData(w.data(), w.size());
    }

    // Replaces the data in the buffer with at most `max_elements` of data, using
    // the function `setter`, which should have the following signature:
    //
    //   size_t setter(ArrayView<U> view)
    //
    // `setter` is given an appropriately typed ArrayView of length exactly
    // `max_elements` that describes the area where it should write the data; it
    // should return the number of elements actually written. (If it doesn't fill
    // the whole ArrayView, it should leave the unused space at the end.)
    template <typename U = T, typename F, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    size_t SetData(size_t max_elements, F &&setter)
    {
        CXXKIT_DCHECK(IsConsistent());
        const size_t old_size = mSize;
        mSize = 0;
        const size_t written = AppendData<U>(max_elements, std::forward<F>(setter));
        if (ZeroOnFree && mSize < old_size)
        {
            ZeroTrailingData(old_size - mSize);
        }
        return written;
    }

    // The AppendData functions add data to the end of the buffer. They accept
    // the same input types as the constructors.
    template <typename U, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    void AppendData(const U *data, size_t size)
    {
        if (size == 0)
        {
            return;
        }
        CXXKIT_DCHECK(data);
        CXXKIT_DCHECK(IsConsistent());
        const size_t new_size = mSize + size;
        EnsureCapacityWithHeadroom(new_size, true);
        static_assert(sizeof(T) == sizeof(U), "");
        std::memcpy(mData.get() + mSize, data, size * sizeof(U));
        mSize = new_size;
        CXXKIT_DCHECK(IsConsistent());
    }

    template <typename U, size_t N, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    void AppendData(const U (&array)[N])
    {
        AppendData(array, N);
    }

    template <typename W, typename std::enable_if<HasDataAndSize<const W, const T>::value>::type * = nullptr>
    void AppendData(const W &w)
    {
        AppendData(w.data(), w.size());
    }

    template <typename U, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    void AppendData(const U &item)
    {
        AppendData(&item, 1);
    }

    // Appends at most `max_elements` to the end of the buffer, using the function
    // `setter`, which should have the following signature:
    //
    //   size_t setter(ArrayView<U> view)
    //
    // `setter` is given an appropriately typed ArrayView of length exactly
    // `max_elements` that describes the area where it should write the data; it
    // should return the number of elements actually written. (If it doesn't fill
    // the whole ArrayView, it should leave the unused space at the end.)
    template <typename U = T, typename F, typename std::enable_if<detail::BufferCompat<T, U>::value>::type * = nullptr>
    size_t AppendData(size_t max_elements, F &&setter)
    {
        CXXKIT_DCHECK(IsConsistent());
        const size_t old_size = mSize;
        SetSize(old_size + max_elements);
        U *base_ptr = data<U>() + old_size;
        size_t written_elements = setter(ArrayView<U>(base_ptr, max_elements));

        CXXKIT_CHECK_LE(written_elements, max_elements);
        mSize = old_size + written_elements;
        CXXKIT_DCHECK(IsConsistent());
        return written_elements;
    }

    // Sets the size of the buffer. If the new size is smaller than the old, the
    // buffer contents will be kept but truncated; if the new size is greater,
    // the existing contents will be kept and the new space will be
    // uninitialized.
    void SetSize(size_t size)
    {
        const size_t old_size = mSize;
        EnsureCapacityWithHeadroom(size, true);
        mSize = size;
        if (ZeroOnFree && mSize < old_size)
        {
            ZeroTrailingData(old_size - mSize);
        }
    }

    // Ensure that the buffer size can be increased to at least capacity without
    // further reallocation. (Of course, this operation might need to reallocate
    // the buffer.)
    void EnsureCapacity(size_t capacity)
    {
        // Don't allocate extra headroom, since the user is asking for a specific
        // capacity.
        EnsureCapacityWithHeadroom(capacity, false);
    }

    // Resets the buffer to zero size without altering capacity. Works even if the
    // buffer has been moved from.
    void Clear()
    {
        MaybeZeroCompleteBuffer();
        mSize = 0;
        CXXKIT_DCHECK(IsConsistent());
    }

    // Swaps two buffers. Also works for buffers that have been moved from.
    friend void swap(BufferT &a, BufferT &b)
    {
        using std::swap;
        swap(a.mSize, b.mSize);
        swap(a.mCapacity, b.mCapacity);
        swap(a.mData, b.mData);
    }

private:
    void EnsureCapacityWithHeadroom(size_t capacity, bool extra_headroom)
    {
        CXXKIT_DCHECK(IsConsistent());
        if (capacity <= mCapacity)
        {
            return;
        }

        // If the caller asks for extra headroom, ensure that the new capacity is
        // >= 1.5 times the old capacity. Any constant > 1 is sufficient to prevent
        // quadratic behavior; as to why we pick 1.5 in particular, see
        // https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md and
        // http://www.gahcep.com/cpp-internals-stl-vector-part-1/.
        const size_t new_capacity = extra_headroom ? std::max(capacity, mCapacity + mCapacity / 2) : capacity;

        std::unique_ptr<T[]> new_data(new T[new_capacity]);
        if (mData != nullptr)
        {
            std::memcpy(new_data.get(), mData.get(), mSize * sizeof(T));
        }
        MaybeZeroCompleteBuffer();
        mData = std::move(new_data);
        mCapacity = new_capacity;
        CXXKIT_DCHECK(IsConsistent());
    }

    // Zero the complete buffer if template argument "ZeroOnFree" is true.
    void MaybeZeroCompleteBuffer()
    {
        if (ZeroOnFree && mCapacity > 0)
        {
            // It would be sufficient to only zero "mSize" elements, as all other
            // methods already ensure that the unused capacity contains no sensitive
            // data---but better safe than sorry.
            ExplicitZeroMemory(mData.get(), mCapacity * sizeof(T));
        }
    }

    // Zero the first "count" elements of unused capacity.
    void ZeroTrailingData(size_t count)
    {
        CXXKIT_DCHECK(IsConsistent());
        CXXKIT_DCHECK_LE(count, mCapacity - mSize);
        ExplicitZeroMemory(mData.get() + mSize, count * sizeof(T));
    }

    // Precondition for all methods except Clear, operator= and the destructor.
    // Postcondition for all methods except move construction and move
    // assignment, which leave the moved-from object in a possibly inconsistent
    // state.
    bool IsConsistent() const { return (mData || mCapacity == 0) && mCapacity >= mSize; }

    // Called when *this has been moved from. Conceptually it's a no-op, but we
    // can mutate the state slightly to help subsequent sanity checks catch bugs.
    void OnMovedFrom()
    {
        CXXKIT_DCHECK(!mData); // Our heap block should have been stolen.
#if CXXKIT_DCHECK_IS_ON
        // Ensure that *this is always inconsistent, to provoke bugs.
        mSize = 1;
        mCapacity = 0;
#else
        // Make *this consistent and empty. Shouldn't be necessary, but better safe
        // than sorry.
        mSize = 0;
        mCapacity = 0;
#endif
    }

    size_t mSize{0};
    size_t mCapacity{0};
    std::unique_ptr<T[]> mData;
};

// By far the most common sort of buffer.
using Buffer = BufferT<uint8_t>;

// A buffer that zeros memory before releasing it.
template <typename T>
using ZeroOnFreeBuffer = BufferT<T, true>;

CXXKIT_END_NAMESPACE