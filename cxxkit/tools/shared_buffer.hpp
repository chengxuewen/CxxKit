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

#include <cxxkit/tools/tools_global.hpp>

#include <cxxkit/memory/ref_counted_object.hpp>
#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/tools/type_traits.hpp>
#include <cxxkit/tools/buffer.hpp>

#include <algorithm>
#include <cstring>
#include <utility>
#include <cstdint>
#include <string>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_TOOLS_API SharedBuffer
{
public:
    // An empty buffer.
    SharedBuffer();
    // Share the data with an existing buffer.
    SharedBuffer(const SharedBuffer &buf);
    // Move contents from an existing buffer.
    SharedBuffer(SharedBuffer &&buf) noexcept;

    // Construct a buffer from a string, convenient for unittests.
    explicit SharedBuffer(StringView s);

    // Construct a buffer with the specified number of uninitialized bytes.
    explicit SharedBuffer(size_t size);
    SharedBuffer(size_t size, size_t capacity);

    // Construct a buffer and copy the specified number of bytes into it. The
    // source array may be (const) uint8_t*, int8_t*, or char*.
    template <typename T, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    SharedBuffer(const T *data, size_t size)
        : SharedBuffer(data, size, size)
    {
    }
    template <typename T, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    SharedBuffer(const T *data, size_t size, size_t capacity)
        : SharedBuffer(size, capacity)
    {
        if (mBuffer)
        {
            std::memcpy(mBuffer->data(), data, size);
            mOffset = 0;
            mSize = size;
        }
    }

    // Construct a buffer from the contents of an array.
    template <typename T, size_t N, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    SharedBuffer(const T (&array)[N]) // NOLINT: runtime/explicit
        : SharedBuffer(array, N)
    {
    }

    // Construct a buffer from a vector like type.
    template <typename VecT,
              typename ElemT = typename std::remove_pointer<decltype(std::declval<VecT>().data())>::type,
              typename std::enable_if<!std::is_same<VecT, SharedBuffer>::value && HasDataAndSize<VecT, ElemT>::value &&
                                      detail::BufferCompat<uint8_t, ElemT>::value>::type * = nullptr>
    explicit SharedBuffer(const VecT &v)
        : SharedBuffer(v.data(), v.size())
    {
    }

    // Construct a buffer from a vector like type and a capacity argument
    template <typename VecT,
              typename ElemT = typename std::remove_pointer<decltype(std::declval<VecT>().data())>::type,
              typename std::enable_if<!std::is_same<VecT, SharedBuffer>::value && HasDataAndSize<VecT, ElemT>::value &&
                                      detail::BufferCompat<uint8_t, ElemT>::value>::type * = nullptr>
    explicit SharedBuffer(const VecT &v, size_t capacity)
        : SharedBuffer(v.data(), v.size(), capacity)
    {
    }

    ~SharedBuffer();

    // get a pointer to the data. Just .data() will give you a (const) uint8_t*,
    // but you may also use .data<int8_t>() and .data<char>().
    template <typename T = uint8_t, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    const T *data() const
    {
        return cdata<T>();
    }

    // get writable pointer to the data. This will create a copy of the underlying
    // data if it is shared with other buffers.
    template <typename T = uint8_t, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    T *mutable_data()
    {
        CXXKIT_DCHECK(is_consistent());
        if (!mBuffer)
        {
            return nullptr;
        }
        unshare_and_ensure_capacity(capacity());
        return mBuffer->data<T>() + mOffset;
    }

    // get const pointer to the data. This will not create a copy of the
    // underlying data if it is shared with other buffers.
    template <typename T = uint8_t, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    const T *cdata() const
    {
        CXXKIT_DCHECK(is_consistent());
        if (!mBuffer)
        {
            return nullptr;
        }
        return mBuffer->data<T>() + mOffset;
    }

    bool empty() const { return mSize == 0; }

    size_t size() const
    {
        CXXKIT_DCHECK(is_consistent());
        return mSize;
    }

    size_t capacity() const
    {
        CXXKIT_DCHECK(is_consistent());
        return mBuffer ? mBuffer->capacity() - mOffset : 0;
    }

    const uint8_t *begin() const { return data(); }
    const uint8_t *end() const { return data() + mSize; }

    SharedBuffer &operator=(const SharedBuffer &buf)
    {
        CXXKIT_DCHECK(is_consistent());
        CXXKIT_DCHECK(buf.is_consistent());
        if (&buf != this)
        {
            mBuffer = buf.mBuffer;
            mOffset = buf.mOffset;
            mSize = buf.mSize;
        }
        return *this;
    }

    SharedBuffer &operator=(SharedBuffer &&buf)
    {
        CXXKIT_DCHECK(is_consistent());
        CXXKIT_DCHECK(buf.is_consistent());
        mBuffer = std::move(buf.mBuffer);
        mOffset = buf.mOffset;
        mSize = buf.mSize;
        buf.mOffset = 0;
        buf.mSize = 0;
        return *this;
    }

    bool operator==(const SharedBuffer &buf) const;

    bool operator!=(const SharedBuffer &buf) const { return !(*this == buf); }

    uint8_t operator[](size_t index) const
    {
        CXXKIT_DCHECK_LT(index, size());
        return cdata()[index];
    }

    // Replace the contents of the buffer. Accepts the same types as the
    // constructors.
    template <typename T, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void set_data(const T *data, size_t size)
    {
        CXXKIT_DCHECK(is_consistent());
        if (!mBuffer)
        {
            mBuffer = size > 0 ? new RefCountedBuffer(data, size) : nullptr;
        }
        else if (!mBuffer->has_one_ref())
        {
            mBuffer = new RefCountedBuffer(data, size, capacity());
        }
        else
        {
            mBuffer->set_data(data, size);
        }
        mOffset = 0;
        mSize = size;

        CXXKIT_DCHECK(is_consistent());
    }

    template <typename T, size_t N, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void set_data(const T (&array)[N])
    {
        set_data(array, N);
    }

    void set_data(const SharedBuffer &buf)
    {
        CXXKIT_DCHECK(is_consistent());
        CXXKIT_DCHECK(buf.is_consistent());
        if (&buf != this)
        {
            mBuffer = buf.mBuffer;
            mOffset = buf.mOffset;
            mSize = buf.mSize;
        }
    }

    // append data to the buffer. Accepts the same types as the constructors.
    template <typename T, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void append_data(const T *data, size_t size)
    {
        CXXKIT_DCHECK(is_consistent());
        if (!mBuffer)
        {
            mBuffer = new RefCountedBuffer(data, size);
            mOffset = 0;
            mSize = size;
            CXXKIT_DCHECK(is_consistent());
            return;
        }

        unshare_and_ensure_capacity(std::max(capacity(), mSize + size));

        mBuffer->set_size(mOffset + mSize); // Remove data to the right of the slice.
        mBuffer->append_data(data, size);
        mSize += size;

        CXXKIT_DCHECK(is_consistent());
    }

    template <typename T, size_t N, typename std::enable_if<detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void append_data(const T (&array)[N])
    {
        append_data(array, N);
    }

    template <typename VecT,
              typename ElemT = typename std::remove_pointer<decltype(std::declval<VecT>().data())>::type,
              typename std::enable_if<HasDataAndSize<VecT, ElemT>::value &&
                                      detail::BufferCompat<uint8_t, ElemT>::value>::type * = nullptr>
    void append_data(const VecT &v)
    {
        append_data(v.data(), v.size());
    }

    // Sets the size of the buffer. If the new size is smaller than the old, the
    // buffer contents will be kept but truncated; if the new size is greater,
    // the existing contents will be kept and the new space will be
    // uninitialized.
    void set_size(size_t size);

    // Ensure that the buffer size can be increased to at least capacity without
    // further reallocation. (Of course, this operation might need to reallocate
    // the buffer.)
    void ensure_capacity(size_t capacity);

    // Resets the buffer to zero size without altering capacity. Works even if the
    // buffer has been moved from.
    void clear();

    // Swaps two buffers.
    friend void swap(SharedBuffer &a, SharedBuffer &b)
    {
        a.mBuffer.swap(b.mBuffer);
        std::swap(a.mOffset, b.mOffset);
        std::swap(a.mSize, b.mSize);
    }

    SharedBuffer slice(size_t offset, size_t length) const
    {
        SharedBuffer slice(*this);
        CXXKIT_DCHECK_LE(offset, mSize);
        CXXKIT_DCHECK_LE(length + offset, mSize);
        slice.mOffset += offset;
        slice.mSize = length;
        return slice;
    }

private:
    using RefCountedBuffer = FinalRefCountedObject<Buffer>;
    // create a copy of the underlying data if it is referenced from other Buffer
    // objects or there is not enough capacity.
    void unshare_and_ensure_capacity(size_t new_capacity);

    // Pre- and postcondition of all methods.
    bool is_consistent() const
    {
        if (mBuffer)
        {
            return mBuffer->capacity() > 0 && mOffset <= mBuffer->size() && mOffset + mSize <= mBuffer->size();
        }
        else
        {
            return mSize == 0 && mOffset == 0;
        }
    }

    // mBuffer is either null, or points to an rtc::Buffer with capacity > 0.
    SharedRefPtr<RefCountedBuffer> mBuffer;
    // This buffer may represent a slice of a original data.
    size_t mOffset; // Offset of a current slice in the original data in mBuffer.
    // Should be 0 if the mBuffer is empty.
    size_t mSize; // Size of a current slice in the original data in mBuffer.
    // Should be 0 if the mBuffer is empty.
};
CXXKIT_END_NAMESPACE
