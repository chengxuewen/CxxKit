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

#include <cxxkit/tools/shared_buffer.hpp>

CXXKIT_BEGIN_NAMESPACE

SharedBuffer::SharedBuffer()
    : offset_(0)
    , size_(0)
{
    CXXKIT_DCHECK(IsConsistent());
}

SharedBuffer::SharedBuffer(const SharedBuffer &buf)
    : buffer_(buf.buffer_)
    , offset_(buf.offset_)
    , size_(buf.size_)
{
}

SharedBuffer::SharedBuffer(SharedBuffer &&buf) noexcept
    : buffer_(std::move(buf.buffer_))
    , offset_(buf.offset_)
    , size_(buf.size_)
{
    buf.offset_ = 0;
    buf.size_ = 0;
    CXXKIT_DCHECK(IsConsistent());
}

SharedBuffer::SharedBuffer(StringView s)
    : SharedBuffer(s.data(), s.length())
{
}

SharedBuffer::SharedBuffer(size_t size)
    : buffer_(size > 0 ? new RefCountedBuffer(size) : nullptr)
    , offset_(0)
    , size_(size)
{
    CXXKIT_DCHECK(IsConsistent());
}

SharedBuffer::SharedBuffer(size_t size, size_t capacity)
    : buffer_(size > 0 || capacity > 0 ? new RefCountedBuffer(size, capacity) : nullptr)
    , offset_(0)
    , size_(size)
{
    CXXKIT_DCHECK(IsConsistent());
}

SharedBuffer::~SharedBuffer() = default;

bool SharedBuffer::operator==(const SharedBuffer &buf) const
{
    // Must either be the same view of the same buffer or have the same contents.
    CXXKIT_DCHECK(IsConsistent());
    CXXKIT_DCHECK(buf.IsConsistent());
    return size_ == buf.size_ && (cdata() == buf.cdata() || memcmp(cdata(), buf.cdata(), size_) == 0);
}

void SharedBuffer::SetSize(size_t size)
{
    CXXKIT_DCHECK(IsConsistent());
    if (!buffer_)
    {
        if (size > 0)
        {
            buffer_ = new RefCountedBuffer(size);
            offset_ = 0;
            size_ = size;
        }
        CXXKIT_DCHECK(IsConsistent());
        return;
    }

    if (size <= size_)
    {
        size_ = size;
        return;
    }

    UnshareAndEnsureCapacity(std::max(capacity(), size));
    buffer_->SetSize(size + offset_);
    size_ = size;
    CXXKIT_DCHECK(IsConsistent());
}

void SharedBuffer::EnsureCapacity(size_t new_capacity)
{
    CXXKIT_DCHECK(IsConsistent());
    if (!buffer_)
    {
        if (new_capacity > 0)
        {
            buffer_ = new RefCountedBuffer(0, new_capacity);
            offset_ = 0;
            size_ = 0;
        }
        CXXKIT_DCHECK(IsConsistent());
        return;
    }
    else if (new_capacity <= capacity())
    {
        return;
    }

    UnshareAndEnsureCapacity(new_capacity);
    CXXKIT_DCHECK(IsConsistent());
}

void SharedBuffer::Clear()
{
    if (!buffer_)
    {
        return;
    }

    if (buffer_->HasOneRef())
    {
        buffer_->Clear();
    }
    else
    {
        buffer_ = new RefCountedBuffer(0, capacity());
    }
    offset_ = 0;
    size_ = 0;
    CXXKIT_DCHECK(IsConsistent());
}

void SharedBuffer::UnshareAndEnsureCapacity(size_t new_capacity)
{
    if (buffer_->HasOneRef() && new_capacity <= capacity())
    {
        return;
    }

    buffer_ = new RefCountedBuffer(buffer_->data() + offset_, size_, new_capacity);
    offset_ = 0;
    CXXKIT_DCHECK(IsConsistent());
}
CXXKIT_END_NAMESPACE
