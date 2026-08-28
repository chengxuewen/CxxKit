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
    : mOffset(0)
    , mSize(0)
{
    CXXKIT_DCHECK(is_consistent());
}

SharedBuffer::SharedBuffer(const SharedBuffer &buf)
    : mBuffer(buf.mBuffer)
    , mOffset(buf.mOffset)
    , mSize(buf.mSize)
{
}

SharedBuffer::SharedBuffer(SharedBuffer &&buf) noexcept
    : mBuffer(std::move(buf.mBuffer))
    , mOffset(buf.mOffset)
    , mSize(buf.mSize)
{
    buf.mOffset = 0;
    buf.mSize = 0;
    CXXKIT_DCHECK(is_consistent());
}

SharedBuffer::SharedBuffer(StringView s)
    : SharedBuffer(s.data(), s.length())
{
}

SharedBuffer::SharedBuffer(size_t size)
    : mBuffer(size > 0 ? new RefCountedBuffer(size) : nullptr)
    , mOffset(0)
    , mSize(size)
{
    CXXKIT_DCHECK(is_consistent());
}

SharedBuffer::SharedBuffer(size_t size, size_t capacity)
    : mBuffer(size > 0 || capacity > 0 ? new RefCountedBuffer(size, capacity) : nullptr)
    , mOffset(0)
    , mSize(size)
{
    CXXKIT_DCHECK(is_consistent());
}

SharedBuffer::~SharedBuffer() = default;

bool SharedBuffer::operator==(const SharedBuffer &buf) const
{
    // Must either be the same view of the same buffer or have the same contents.
    CXXKIT_DCHECK(is_consistent());
    CXXKIT_DCHECK(buf.is_consistent());
    return mSize == buf.mSize && (cdata() == buf.cdata() || memcmp(cdata(), buf.cdata(), mSize) == 0);
}

void SharedBuffer::set_size(size_t size)
{
    CXXKIT_DCHECK(is_consistent());
    if (!mBuffer)
    {
        if (size > 0)
        {
            mBuffer = new RefCountedBuffer(size);
            mOffset = 0;
            mSize = size;
        }
        CXXKIT_DCHECK(is_consistent());
        return;
    }

    if (size <= mSize)
    {
        mSize = size;
        return;
    }

    unshare_and_ensure_capacity(std::max(capacity(), size));
    mBuffer->set_size(size + mOffset);
    mSize = size;
    CXXKIT_DCHECK(is_consistent());
}

void SharedBuffer::ensure_capacity(size_t new_capacity)
{
    CXXKIT_DCHECK(is_consistent());
    if (!mBuffer)
    {
        if (new_capacity > 0)
        {
            mBuffer = new RefCountedBuffer(0, new_capacity);
            mOffset = 0;
            mSize = 0;
        }
        CXXKIT_DCHECK(is_consistent());
        return;
    }
    else if (new_capacity <= capacity())
    {
        return;
    }

    unshare_and_ensure_capacity(new_capacity);
    CXXKIT_DCHECK(is_consistent());
}

void SharedBuffer::clear()
{
    if (!mBuffer)
    {
        return;
    }

    if (mBuffer->has_one_ref())
    {
        mBuffer->clear();
    }
    else
    {
        mBuffer = new RefCountedBuffer(0, capacity());
    }
    mOffset = 0;
    mSize = 0;
    CXXKIT_DCHECK(is_consistent());
}

void SharedBuffer::unshare_and_ensure_capacity(size_t new_capacity)
{
    if (mBuffer->has_one_ref() && new_capacity <= capacity())
    {
        return;
    }

    mBuffer = new RefCountedBuffer(mBuffer->data() + mOffset, mSize, new_capacity);
    mOffset = 0;
    CXXKIT_DCHECK(is_consistent());
}
CXXKIT_END_NAMESPACE
