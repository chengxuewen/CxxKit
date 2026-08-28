/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2015 The WebRTC project authors.
**
** License: MIT License + BSD-3 (ported from libwebrtc api/video/i420_buffer.h)
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

#include <cxxkit/memory/aligned_malloc.hpp>
#include <cxxkit/memory/ref_counted_object.hpp>
#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>
#include <cxxkit/media/video_rotation.hpp>

#include <cstdint>
#include <memory>

namespace cxxkit {

// Plain I420 buffer in standard (64-byte aligned) memory.
class CXXKIT_MEDIA_API I420Buffer : public I420BufferInterface {
public:
    static SharedRefPtr<I420Buffer> Create(int width, int height);
    static SharedRefPtr<I420Buffer> Create(int width, int height, int stride_y, int stride_u, int stride_v);

    // Create a new buffer and copy the pixel data.
    static SharedRefPtr<I420Buffer> Copy(const I420BufferInterface& buffer);
    static SharedRefPtr<I420Buffer> Copy(int width,
                                         int height,
                                         const uint8_t* data_y,
                                         int stride_y,
                                         const uint8_t* data_u,
                                         int stride_u,
                                         const uint8_t* data_v,
                                         int stride_v);

    // Returns a rotated copy of `src`.
    static SharedRefPtr<I420Buffer> Rotate(const I420BufferInterface& src, VideoRotation rotation);

    // Sets the buffer to all black (Y=0, U=128, V=128).
    void SetBlack();

    // Sets all three planes to all zeros. Used to work around for
    // quirks in memory checkers
    // (https://bugs.chromium.org/p/libyuv/issues/detail?id=377) and
    // ffmpeg (http://crbug.com/390941).
    void InitializeData();

    int width() const override;
    int height() const override;
    const uint8_t* GetDataY() const override;
    const uint8_t* GetDataU() const override;
    const uint8_t* GetDataV() const override;
    int StrideY() const override;
    int StrideU() const override;
    int StrideV() const override;

    uint8_t* MutableDataY();
    uint8_t* MutableDataU();
    uint8_t* MutableDataV();

    // Scale the cropped area of `src` to the size of `this` buffer, and
    // write the result into `this`.
    void CropAndScaleFrom(const I420BufferInterface& src,
                          int offset_x,
                          int offset_y,
                          int crop_width,
                          int crop_height);

    // The common case of a center crop, when needed to adjust the
    // aspect ratio without distorting the image.
    void CropAndScaleFrom(const I420BufferInterface& src);

    // Scale all of `src` to the size of `this` buffer, with no cropping.
    void ScaleFrom(const I420BufferInterface& src);

    // Concrete scale: base interface default has no I420 target, see
    // video_frame_buffer.hpp. Crops `this` to the given area and scales.
    SharedRefPtr<VideoFrameBuffer> CropAndScale(int offset_x,
                                                int offset_y,
                                                int crop_width,
                                                int crop_height,
                                                int scaled_width,
                                                int scaled_height) override;

protected:
    I420Buffer(int width, int height);
    I420Buffer(int width, int height, int stride_y, int stride_u, int stride_v);
    ~I420Buffer() override;

    friend class RefCountedObject<I420Buffer>;

private:
    const int mWidth;
    const int mHeight;
    const int mStrideY;
    const int mStrideU;
    const int mStrideV;
    const std::unique_ptr<uint8_t, AlignedFreeDeleter> mData;
};

}  // namespace cxxkit