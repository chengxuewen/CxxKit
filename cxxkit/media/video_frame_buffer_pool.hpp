/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
**
** License: MIT License + BSD-3 (ported from libwebrtc common_video/include/video_frame_buffer_pool.h,
** trimmed to I420 only — NV12/444/422/010/210/410 buffers are not ported yet)
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

#include <cxxkit/media/i420_buffer.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>
#include <cxxkit/thread/race_checker.hpp>

#include <cstddef>

#include <list>

CXXKIT_BEGIN_NAMESPACE

// Simple buffer pool to avoid unnecessary allocations of video frame buffers.
// The pool manages the memory of the I420Buffer returned from
// create_i420_buffer. When the buffer is destructed, the memory is returned to
// the pool for use by subsequent calls to create_i420_buffer. If the resolution
// passed to create_i420_buffer changes, old buffers will be purged from the
// pool.
// Note that create_i420_buffer will return nullptr if more than
// `max_number_of_buffers` are created. This is to prevent memory leaks where
// frames are not returned.
class CXXKIT_MEDIA_API VideoFrameBufferPool
{
public:
    VideoFrameBufferPool();
    explicit VideoFrameBufferPool(bool zero_initialize);
    VideoFrameBufferPool(bool zero_initialize, size_t max_number_of_buffers);
    ~VideoFrameBufferPool();

    // Returns a buffer from the pool. If no suitable buffer exists in the pool
    // and there are fewer than `max_number_of_buffers` pending, a buffer is
    // created. Returns null otherwise.
    SharedRefPtr<I420Buffer> create_i420_buffer(int width, int height);

    // Changes the max amount of buffers in the pool to the new value.
    // Returns true if the change was successful and false if the amount of
    // already allocated buffers is bigger than the new value.
    bool resize(size_t max_number_of_buffers);

    // Clears mBuffers and detaches the thread checker so that it can be reused
    // later from another thread.
    void release();

private:
    SharedRefPtr<VideoFrameBuffer> get_existing_buffer(int width, int height, VideoType type);

    race_checker mRaceChecker;
    std::list<SharedRefPtr<VideoFrameBuffer>> mBuffers;
    // If true, newly allocated buffers are zero-initialized. Note that
    // recycled buffers are not zero'd before reuse.
    const bool mZeroInitialize;
    // Max number of buffers this pool can have pending.
    size_t mMaxNumberOfBuffers;
};

CXXKIT_END_NAMESPACE