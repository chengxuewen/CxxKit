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

#ifndef _CXXKIT_SHARED_BUFFER_HPP
#define _CXXKIT_SHARED_BUFFER_HPP

#include "cxxkit/memory/ref_counted_object.hpp"
#include "cxxkit/memory/shared_ref_ptr.hpp"
#include "cxxkit/text/string_view.hpp"
#include "cxxkit/tools/type_traits.hpp"
#include "cxxkit/tools/buffer.hpp"

#include <algorithm>
#include <cstring>
#include <utility>
#include <cstdint>
#include <string>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_CORE_API SharedBuffer
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
    template <typename T,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    SharedBuffer(const T *data, size_t size)
        : SharedBuffer(data, size, size) {}
    template <typename T,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    SharedBuffer(const T *data, size_t size, size_t capacity)
        : SharedBuffer(size, capacity)
    {
        if (buffer_)
        {
            std::memcpy(buffer_->data(), data, size);
            offset_ = 0;
            size_ = size;
        }
    }

    // Construct a buffer from the contents of an array.
    template <typename T,
        size_t N,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    SharedBuffer(const T (&array)[N])  // NOLINT: runtime/explicit
        : SharedBuffer(array, N) {}

    // Construct a buffer from a vector like type.
    template <typename VecT,
              typename ElemT = typename std::remove_pointer<decltype(std::declval<VecT>().data())>::type,
        typename std::enable_if<
            !std::is_same<VecT, SharedBuffer>::value &&
            HasDataAndSize<VecT, ElemT>::value &&
            detail::BufferCompat<uint8_t, ElemT>::value>::type * = nullptr>
    explicit SharedBuffer(const VecT &v)
        : SharedBuffer(v.data(), v.size()) {}

    // Construct a buffer from a vector like type and a capacity argument
    template <typename VecT,
              typename ElemT = typename std::remove_pointer<
                  decltype(std::declval<VecT>().data())>::type,
        typename std::enable_if<
            !std::is_same<VecT, SharedBuffer>::value &&
            HasDataAndSize<VecT, ElemT>::value &&
            detail::BufferCompat<uint8_t, ElemT>::value>::type * = nullptr>
    explicit SharedBuffer(const VecT &v, size_t capacity)
        : SharedBuffer(v.data(), v.size(), capacity) {}

    ~SharedBuffer();

    // Get a pointer to the data. Just .data() will give you a (const) uint8_t*,
    // but you may also use .data<int8_t>() and .data<char>().
    template <typename T = uint8_t,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    const T *data() const
    {
        return cdata<T>();
    }

    // Get writable pointer to the data. This will create a copy of the underlying
    // data if it is shared with other buffers.
    template <typename T = uint8_t,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    T *MutableData()
    {
        CXXKIT_DCHECK(IsConsistent());
        if (!buffer_)
        {
            return nullptr;
        }
        UnshareAndEnsureCapacity(capacity());
        return buffer_->data<T>() + offset_;
    }

    // Get const pointer to the data. This will not create a copy of the
    // underlying data if it is shared with other buffers.
    template <typename T = uint8_t,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    const T *cdata() const
    {
        CXXKIT_DCHECK(IsConsistent());
        if (!buffer_)
        {
            return nullptr;
        }
        return buffer_->data<T>() + offset_;
    }

    bool empty() const { return size_ == 0; }

    size_t size() const
    {
        CXXKIT_DCHECK(IsConsistent());
        return size_;
    }

    size_t capacity() const
    {
        CXXKIT_DCHECK(IsConsistent());
        return buffer_ ? buffer_->capacity() - offset_ : 0;
    }

    const uint8_t *begin() const { return data(); }
    const uint8_t *end() const { return data() + size_; }

    SharedBuffer &operator=(const SharedBuffer &buf)
    {
        CXXKIT_DCHECK(IsConsistent());
        CXXKIT_DCHECK(buf.IsConsistent());
        if (&buf != this)
        {
            buffer_ = buf.buffer_;
            offset_ = buf.offset_;
            size_ = buf.size_;
        }
        return *this;
    }

    SharedBuffer &operator=(SharedBuffer &&buf)
    {
        CXXKIT_DCHECK(IsConsistent());
        CXXKIT_DCHECK(buf.IsConsistent());
        buffer_ = std::move(buf.buffer_);
        offset_ = buf.offset_;
        size_ = buf.size_;
        buf.offset_ = 0;
        buf.size_ = 0;
        return *this;
    }

    bool operator==(const SharedBuffer &buf) const;

    bool operator!=(const SharedBuffer &buf) const
    {
        return !(*this == buf);
    }

    uint8_t operator[](size_t index) const
    {
        CXXKIT_DCHECK_LT(index, size());
        return cdata()[index];
    }

    // Replace the contents of the buffer. Accepts the same types as the
    // constructors.
    template <typename T,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void SetData(const T *data, size_t size)
    {
        CXXKIT_DCHECK(IsConsistent());
        if (!buffer_)
        {
            buffer_ = size > 0 ? new RefCountedBuffer(data, size) : nullptr;
        }
        else if (!buffer_->HasOneRef())
        {
            buffer_ = new RefCountedBuffer(data, size, capacity());
        }
        else
        {
            buffer_->SetData(data, size);
        }
        offset_ = 0;
        size_ = size;

        CXXKIT_DCHECK(IsConsistent());
    }

    template <typename T,
        size_t N,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void SetData(const T (&array)[N])
    {
        SetData(array, N);
    }

    void SetData(const SharedBuffer &buf)
    {
        CXXKIT_DCHECK(IsConsistent());
        CXXKIT_DCHECK(buf.IsConsistent());
        if (&buf != this)
        {
            buffer_ = buf.buffer_;
            offset_ = buf.offset_;
            size_ = buf.size_;
        }
    }

    // Append data to the buffer. Accepts the same types as the constructors.
    template <typename T,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void AppendData(const T *data, size_t size)
    {
        CXXKIT_DCHECK(IsConsistent());
        if (!buffer_)
        {
            buffer_ = new RefCountedBuffer(data, size);
            offset_ = 0;
            size_ = size;
            CXXKIT_DCHECK(IsConsistent());
            return;
        }

        UnshareAndEnsureCapacity(std::max(capacity(), size_ + size));

        buffer_->SetSize(offset_ +
                         size_);  // Remove data to the right of the slice.
        buffer_->AppendData(data, size);
        size_ += size;

        CXXKIT_DCHECK(IsConsistent());
    }

    template <typename T,
        size_t N,
        typename std::enable_if<
            detail::BufferCompat<uint8_t, T>::value>::type * = nullptr>
    void AppendData(const T (&array)[N])
    {
        AppendData(array, N);
    }

    template <typename VecT,
              typename ElemT = typename std::remove_pointer<
                  decltype(std::declval<VecT>().data())>::type,
        typename std::enable_if<
            HasDataAndSize<VecT, ElemT>::value &&
            detail::BufferCompat<uint8_t, ElemT>::value>::type * = nullptr>
    void AppendData(const VecT &v)
    {
        AppendData(v.data(), v.size());
    }

    // Sets the size of the buffer. If the new size is smaller than the old, the
    // buffer contents will be kept but truncated; if the new size is greater,
    // the existing contents will be kept and the new space will be
    // uninitialized.
    void SetSize(size_t size);

    // Ensure that the buffer size can be increased to at least capacity without
    // further reallocation. (Of course, this operation might need to reallocate
    // the buffer.)
    void EnsureCapacity(size_t capacity);

    // Resets the buffer to zero size without altering capacity. Works even if the
    // buffer has been moved from.
    void Clear();

    // Swaps two buffers.
    friend void swap(SharedBuffer &a, SharedBuffer &b)
    {
        a.buffer_.swap(b.buffer_);
        std::swap(a.offset_, b.offset_);
        std::swap(a.size_, b.size_);
    }

    SharedBuffer Slice(size_t offset, size_t length) const
    {
        SharedBuffer slice(*this);
        CXXKIT_DCHECK_LE(offset, size_);
        CXXKIT_DCHECK_LE(length + offset, size_);
        slice.offset_ += offset;
        slice.size_ = length;
        return slice;
    }

private:
    using RefCountedBuffer = FinalRefCountedObject<Buffer>;
    // Create a copy of the underlying data if it is referenced from other Buffer
    // objects or there is not enough capacity.
    void UnshareAndEnsureCapacity(size_t new_capacity);

    // Pre- and postcondition of all methods.
    bool IsConsistent() const
    {
        if (buffer_)
        {
            return buffer_->capacity() > 0 && offset_ <= buffer_->size() &&
                   offset_ + size_ <= buffer_->size();
        }
        else
        {
            return size_ == 0 && offset_ == 0;
        }
    }

    // buffer_ is either null, or points to an rtc::Buffer with capacity > 0.
    SharedRefPtr<RefCountedBuffer> buffer_;
    // This buffer may represent a slice of a original data.
    size_t offset_;  // Offset of a current slice in the original data in buffer_.
    // Should be 0 if the buffer_ is empty.
    size_t size_;    // Size of a current slice in the original data in buffer_.
    // Should be 0 if the buffer_ is empty.
};
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_SHARED_BUFFER_HPP
