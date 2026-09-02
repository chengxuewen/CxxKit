/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2015 The WebRTC project authors.
**
** License: MIT License + BSD-3 (ported from libwebrtc api/video/i420_buffer.cc)
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

#include <cxxkit/media/i420_buffer.hpp>

#include <cxxkit/tools/checks.hpp>

#include <libyuv.h>

#include <algorithm>
#include <cstdint>
#include <utility>

// Aligning pointer to 64 bytes for improved performance, e.g. use SIMD.
static const int kBufferAlignment = 64;

CXXKIT_BEGIN_NAMESPACE

namespace
{

int I420DataSize(int height, int stride_y, int stride_u, int stride_v)
{
    return stride_y * height + (stride_u + stride_v) * ((height + 1) / 2);
}

} // namespace

I420Buffer::I420Buffer(int width, int height)
    : I420Buffer(width, height, width, (width + 1) / 2, (width + 1) / 2)
{
}

I420Buffer::I420Buffer(int width, int height, int stride_y, int stride_u, int stride_v)
    : mWidth(width)
    , mHeight(height)
    , mStrideY(stride_y)
    , mStrideU(stride_u)
    , mStrideV(stride_v)
    , mData(static_cast<uint8_t *>(
          utils::aligned_malloc(I420DataSize(height, stride_y, stride_u, stride_v), kBufferAlignment)))
{
    CXXKIT_DCHECK_GT(width, 0);
    CXXKIT_DCHECK_GT(height, 0);
    CXXKIT_DCHECK_GE(stride_y, width);
    CXXKIT_DCHECK_GE(stride_u, (width + 1) / 2);
    CXXKIT_DCHECK_GE(stride_v, (width + 1) / 2);
}

I420Buffer::~I420Buffer()
{
}

// static
SharedRefPtr<I420Buffer> I420Buffer::create(int width, int height)
{
    return SharedRefPtr<I420Buffer>(new RefCountedObject<I420Buffer>(width, height));
}

// static
SharedRefPtr<I420Buffer> I420Buffer::create(int width, int height, int stride_y, int stride_u, int stride_v)
{
    return SharedRefPtr<I420Buffer>(new RefCountedObject<I420Buffer>(width, height, stride_y, stride_u, stride_v));
}

// static
SharedRefPtr<I420Buffer> I420Buffer::copy(const I420BufferInterface &source)
{
    return copy(source.width(),
                source.height(),
                source.get_data_y(),
                source.stride_y(),
                source.get_data_u(),
                source.stride_u(),
                source.get_data_v(),
                source.stride_v());
}

// static
SharedRefPtr<I420Buffer> I420Buffer::copy(int width,
                                          int height,
                                          const uint8_t *data_y,
                                          int stride_y,
                                          const uint8_t *data_u,
                                          int stride_u,
                                          const uint8_t *data_v,
                                          int stride_v)
{
    // Note: May use different strides than the input data.
    SharedRefPtr<I420Buffer> buffer = create(width, height);
    CXXKIT_CHECK_EQ(0,
                    libyuv::I420Copy(data_y,
                                     stride_y,
                                     data_u,
                                     stride_u,
                                     data_v,
                                     stride_v,
                                     buffer->mutable_data_y(),
                                     buffer->stride_y(),
                                     buffer->mutable_data_u(),
                                     buffer->stride_u(),
                                     buffer->mutable_data_v(),
                                     buffer->stride_v(),
                                     width,
                                     height));
    return buffer;
}

// static
SharedRefPtr<I420Buffer> I420Buffer::rotate(const I420BufferInterface &src, VideoRotation rotation)
{
    CXXKIT_CHECK(src.get_data_y());
    CXXKIT_CHECK(src.get_data_u());
    CXXKIT_CHECK(src.get_data_v());

    int rotated_width = src.width();
    int rotated_height = src.height();
    if (rotation == VideoRotation::kVideoRotation_90 || rotation == VideoRotation::kVideoRotation_270)
    {
        std::swap(rotated_width, rotated_height);
    }

    SharedRefPtr<I420Buffer> buffer = I420Buffer::create(rotated_width, rotated_height);

    CXXKIT_CHECK_EQ(0,
                    libyuv::I420Rotate(src.get_data_y(),
                                       src.stride_y(),
                                       src.get_data_u(),
                                       src.stride_u(),
                                       src.get_data_v(),
                                       src.stride_v(),
                                       buffer->mutable_data_y(),
                                       buffer->stride_y(),
                                       buffer->mutable_data_u(),
                                       buffer->stride_u(),
                                       buffer->mutable_data_v(),
                                       buffer->stride_v(),
                                       src.width(),
                                       src.height(),
                                       static_cast<libyuv::RotationMode>(rotation)));

    return buffer;
}

void I420Buffer::initialize_data()
{
    memset(mData.get(), 0, I420DataSize(mHeight, mStrideY, mStrideU, mStrideV));
}

int I420Buffer::width() const
{
    return mWidth;
}

int I420Buffer::height() const
{
    return mHeight;
}

const uint8_t *I420Buffer::get_data_y() const
{
    return mData.get();
}

const uint8_t *I420Buffer::get_data_u() const
{
    return mData.get() + mStrideY * mHeight;
}

const uint8_t *I420Buffer::get_data_v() const
{
    return mData.get() + mStrideY * mHeight + mStrideU * ((mHeight + 1) / 2);
}

int I420Buffer::stride_y() const
{
    return mStrideY;
}

int I420Buffer::stride_u() const
{
    return mStrideU;
}

int I420Buffer::stride_v() const
{
    return mStrideV;
}

uint8_t *I420Buffer::mutable_data_y()
{
    return const_cast<uint8_t *>(get_data_y());
}

uint8_t *I420Buffer::mutable_data_u()
{
    return const_cast<uint8_t *>(get_data_u());
}

uint8_t *I420Buffer::mutable_data_v()
{
    return const_cast<uint8_t *>(get_data_v());
}

void I420Buffer::set_black()
{
    CXXKIT_CHECK(libyuv::I420Rect(mutable_data_y(),
                                  stride_y(),
                                  mutable_data_u(),
                                  stride_u(),
                                  mutable_data_v(),
                                  stride_v(),
                                  0,
                                  0,
                                  mWidth,
                                  mHeight,
                                  0,
                                  128,
                                  128) == 0);
}

void I420Buffer::crop_and_scale_from(const I420BufferInterface &src,
                                     int offset_x,
                                     int offset_y,
                                     int crop_width,
                                     int crop_height)
{
    CXXKIT_CHECK_LE(crop_width, src.width());
    CXXKIT_CHECK_LE(crop_height, src.height());
    CXXKIT_CHECK_LE(crop_width + offset_x, src.width());
    CXXKIT_CHECK_LE(crop_height + offset_y, src.height());
    CXXKIT_CHECK_GE(offset_x, 0);
    CXXKIT_CHECK_GE(offset_y, 0);

    // Make sure offset is even so that u/v plane becomes aligned.
    const int uv_offset_x = offset_x / 2;
    const int uv_offset_y = offset_y / 2;
    offset_x = uv_offset_x * 2;
    offset_y = uv_offset_y * 2;

    const uint8_t *y_plane = src.get_data_y() + src.stride_y() * offset_y + offset_x;
    const uint8_t *u_plane = src.get_data_u() + src.stride_u() * uv_offset_y + uv_offset_x;
    const uint8_t *v_plane = src.get_data_v() + src.stride_v() * uv_offset_y + uv_offset_x;
    int res = libyuv::I420Scale(y_plane,
                                src.stride_y(),
                                u_plane,
                                src.stride_u(),
                                v_plane,
                                src.stride_v(),
                                crop_width,
                                crop_height,
                                mutable_data_y(),
                                stride_y(),
                                mutable_data_u(),
                                stride_u(),
                                mutable_data_v(),
                                stride_v(),
                                mWidth,
                                mHeight,
                                libyuv::kFilterBox);

    CXXKIT_DCHECK_EQ(res, 0);
}

void I420Buffer::crop_and_scale_from(const I420BufferInterface &src)
{
    const int width = this->width();
    const int height = this->height();
    const int crop_width = height > 0 ? std::min(src.width(), width * src.height() / height) : src.width();
    const int crop_height = width > 0 ? std::min(src.height(), height * src.width() / width) : src.height();
    this->crop_and_scale_from(src,
                              (src.width() - crop_width) / 2,
                              (src.height() - crop_height) / 2,
                              crop_width,
                              crop_height);
}

void I420Buffer::scale_from(const I420BufferInterface &src)
{
    crop_and_scale_from(src, 0, 0, src.width(), src.height());
}

SharedRefPtr<VideoFrameBuffer> I420Buffer::crop_and_scale(int offset_x,
                                                          int offset_y,
                                                          int crop_width,
                                                          int crop_height,
                                                          int scaled_width,
                                                          int scaled_height)
{
    SharedRefPtr<I420Buffer> result = I420Buffer::create(scaled_width, scaled_height);
    result->crop_and_scale_from(*this, offset_x, offset_y, crop_width, crop_height);
    return result;
}

CXXKIT_END_NAMESPACE