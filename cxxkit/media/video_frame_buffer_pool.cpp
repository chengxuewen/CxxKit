/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
**
** License: MIT License + BSD-3 (ported from libwebrtc common_video/video_frame_buffer_pool.cc,
** trimmed to I420 only)
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

#include <cxxkit/media/video_frame_buffer_pool.hpp>

#include <cxxkit/tools/checks.hpp>

#include <limits>

CXXKIT_BEGIN_NAMESPACE

namespace
{

bool has_one_ref(const SharedRefPtr<VideoFrameBuffer> &buffer)
{
    // Cast to RefCountedObject is safe because this function is only called
    // on locally created VideoFrameBuffers, which are always
    // `RefCountedObject<I420Buffer>` (the pool is I420-only).
    switch (buffer->type())
    {
        case VideoType::kI420: return static_cast<RefCountedObject<I420Buffer> *>(buffer.get())->has_one_ref();
        default: CXXKIT_DCHECK_NOTREACHED();
    }
    return false;
}

} // namespace

VideoFrameBufferPool::VideoFrameBufferPool()
    : VideoFrameBufferPool(false)
{
}

VideoFrameBufferPool::VideoFrameBufferPool(bool zero_initialize)
    : VideoFrameBufferPool(zero_initialize, std::numeric_limits<size_t>::max())
{
}

VideoFrameBufferPool::VideoFrameBufferPool(bool zero_initialize, size_t max_number_of_buffers)
    : mZeroInitialize(zero_initialize)
    , mMaxNumberOfBuffers(max_number_of_buffers)
{
}

VideoFrameBufferPool::~VideoFrameBufferPool() = default;

void VideoFrameBufferPool::release()
{
    mBuffers.clear();
}

bool VideoFrameBufferPool::resize(size_t max_number_of_buffers)
{
    CXXKIT_DCHECK_RUNS_SERIALIZED(&mRaceChecker);
    size_t used_buffers_count = 0;
    for (const SharedRefPtr<VideoFrameBuffer> &buffer : mBuffers)
    {
        // If the buffer is in use, the ref count will be >= 2, one from the
        // list we are looping over and one from the application. If the ref
        // count is 1, then the list we are looping over holds the only
        // reference and it's safe to reuse.
        if (!has_one_ref(buffer))
        {
            used_buffers_count++;
        }
    }
    if (used_buffers_count > max_number_of_buffers)
    {
        return false;
    }
    mMaxNumberOfBuffers = max_number_of_buffers;

    size_t buffers_to_purge = mBuffers.size() - mMaxNumberOfBuffers;
    auto iter = mBuffers.begin();
    while (iter != mBuffers.end() && buffers_to_purge > 0)
    {
        if (has_one_ref(*iter))
        {
            iter = mBuffers.erase(iter);
            buffers_to_purge--;
        }
        else
        {
            ++iter;
        }
    }
    return true;
}

SharedRefPtr<I420Buffer> VideoFrameBufferPool::create_i420_buffer(int width, int height)
{
    CXXKIT_DCHECK_RUNS_SERIALIZED(&mRaceChecker);

    SharedRefPtr<VideoFrameBuffer> existing_buffer = get_existing_buffer(width, height, VideoType::kI420);
    if (existing_buffer)
    {
        // Cast is safe because the only way a kI420 buffer is created is in
        // the same function below, where `RefCountedObject<I420Buffer>` is
        // created.
        RefCountedObject<I420Buffer> *raw_buffer = static_cast<RefCountedObject<I420Buffer> *>(existing_buffer.get());
        // Creates a new SharedRefPtr, which is also pointing to the same
        // RefCountedObject as buffer, increasing the ref count.
        return SharedRefPtr<I420Buffer>(raw_buffer);
    }

    if (mBuffers.size() >= mMaxNumberOfBuffers)
    {
        return SharedRefPtr<I420Buffer>();
    }

    // Allocate a new buffer.
    SharedRefPtr<I420Buffer> buffer = I420Buffer::create(width, height);
    if (mZeroInitialize)
    {
        buffer->initialize_data();
    }

    mBuffers.push_back(buffer);
    return buffer;
}

SharedRefPtr<VideoFrameBuffer> VideoFrameBufferPool::get_existing_buffer(int width, int height, VideoType type)
{
    // release buffers with wrong resolution or different type.
    for (auto it = mBuffers.begin(); it != mBuffers.end();)
    {
        const SharedRefPtr<VideoFrameBuffer> &buffer = *it;
        if (buffer->width() != width || buffer->height() != height || buffer->type() != type)
        {
            it = mBuffers.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Look for a free buffer.
    for (const SharedRefPtr<VideoFrameBuffer> &buffer : mBuffers)
    {
        // If the buffer is in use, the ref count will be >= 2, one from the
        // list we are looping over and one from the application. If the ref
        // count is 1, then the list we are looping over holds the only
        // reference and it's safe to reuse.
        if (has_one_ref(buffer))
        {
            CXXKIT_CHECK(buffer->type() == type);
            return buffer;
        }
    }
    return SharedRefPtr<VideoFrameBuffer>();
}

CXXKIT_END_NAMESPACE