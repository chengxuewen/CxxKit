/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
**
** License: MIT License + BSD-3 (ported from libwebrtc common_video/libyuv/webrtc_libyuv.cc,
** trimmed: no I420A/I010/NV12-file helpers, no NV12ToI420Scaler)
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

#include <cxxkit/media/webrtc_libyuv.hpp>

#include <cxxkit/media/i420_buffer.hpp>
#include <cxxkit/tools/checks.hpp>

#include <libyuv.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{

// Maps a VideoType to the libyuv FourCC used by ConvertFromI420.
int video_type_to_four_cc(VideoType video_type)
{
    switch (video_type)
    {
        case VideoType::kI420: return libyuv::FOURCC_I420;
        case VideoType::kRGB24: return libyuv::FOURCC_24BG;
        case VideoType::kARGB: return libyuv::FOURCC_ARGB;
        case VideoType::kRGB565: return libyuv::FOURCC_RGBP;
        case VideoType::kUYVY: return libyuv::FOURCC_UYVY;
        case VideoType::kNV12: return libyuv::FOURCC_NV12;
        default: break;
    }
    CXXKIT_CHECK_NOTREACHED() << "Unsupported destination format " << static_cast<int>(video_type);
    return libyuv::FOURCC_ANY;
}

// Row pitch in bytes for a packed destination buffer of the given format.
int sample_size(VideoType video_type, int dst_width)
{
    switch (video_type)
    {
        case VideoType::kI420:
        case VideoType::kNV12:
        case VideoType::kUYVY: return dst_width;
        case VideoType::kRGB24: return dst_width * 3;
        case VideoType::kARGB: return dst_width * 4;
        case VideoType::kRGB565: return dst_width * 2;
        default: break;
    }
    CXXKIT_CHECK_NOTREACHED() << "Unsupported destination format " << static_cast<int>(video_type);
    return 0;
}

} // namespace

int extract_buffer(const I420BufferInterface &input_frame, size_t size, uint8_t *buffer)
{
    CXXKIT_DCHECK(buffer);
    if (!buffer)
    {
        return -1;
    }
    const int width = input_frame.width();
    const int height = input_frame.height();
    const size_t length = calc_buffer_size(VideoType::kI420, width, height);
    if (size < length)
    {
        return -1;
    }

    const int chroma_width = input_frame.chroma_width();
    const int chroma_height = input_frame.chroma_height();

    libyuv::I420Copy(input_frame.get_data_y(),
                     input_frame.stride_y(),
                     input_frame.get_data_u(),
                     input_frame.stride_u(),
                     input_frame.get_data_v(),
                     input_frame.stride_v(),
                     buffer,
                     width,
                     buffer + width * height,
                     chroma_width,
                     buffer + width * height + chroma_width * chroma_height,
                     chroma_width,
                     width,
                     height);

    return static_cast<int>(length);
}

int convert_from_i420(const VideoFrame &src_frame,
                      VideoType dst_video_type,
                      int dst_width,
                      int dst_height,
                      uint8_t *dst_frame)
{
    const SharedRefPtr<I420BufferInterface> i420_buffer = src_frame.video_frame_buffer()->to_i420();
    if (!i420_buffer)
    {
        return -1;
    }
    return libyuv::ConvertFromI420(i420_buffer->get_data_y(),
                                   i420_buffer->stride_y(),
                                   i420_buffer->get_data_u(),
                                   i420_buffer->stride_u(),
                                   i420_buffer->get_data_v(),
                                   i420_buffer->stride_v(),
                                   dst_frame,
                                   sample_size(dst_video_type, dst_width),
                                   dst_width,
                                   dst_height,
                                   video_type_to_four_cc(dst_video_type));
}

SharedRefPtr<I420BufferInterface> scale_video_frame_buffer(const I420BufferInterface &source,
                                                           int dst_width,
                                                           int dst_height)
{
    const SharedRefPtr<I420Buffer> scaled_buffer = I420Buffer::create(dst_width, dst_height);
    scaled_buffer->scale_from(source);
    return scaled_buffer;
}

double I420Psnr(const I420BufferInterface &ref_buffer, const I420BufferInterface &test_buffer)
{
    CXXKIT_DCHECK_GE(ref_buffer.width(), test_buffer.width());
    CXXKIT_DCHECK_GE(ref_buffer.height(), test_buffer.height());
    if ((ref_buffer.width() != test_buffer.width()) || (ref_buffer.height() != test_buffer.height()))
    {
        const SharedRefPtr<I420Buffer> scaled_buffer = I420Buffer::create(ref_buffer.width(), ref_buffer.height());
        scaled_buffer->scale_from(test_buffer);
        return I420Psnr(ref_buffer, *scaled_buffer);
    }

    double psnr = libyuv::I420Psnr(ref_buffer.get_data_y(),
                                   ref_buffer.stride_y(),
                                   ref_buffer.get_data_u(),
                                   ref_buffer.stride_u(),
                                   ref_buffer.get_data_v(),
                                   ref_buffer.stride_v(),
                                   test_buffer.get_data_y(),
                                   test_buffer.stride_y(),
                                   test_buffer.get_data_u(),
                                   test_buffer.stride_u(),
                                   test_buffer.get_data_v(),
                                   test_buffer.stride_v(),
                                   test_buffer.width(),
                                   test_buffer.height());

    // LibYuv sets the max psnr value to 128; we restrict it here. In case of
    // 0 mse in one frame, 128 can skew the results significantly.
    return (psnr > kPerfectPSNR) ? kPerfectPSNR : psnr;
}

double I420Ssim(const I420BufferInterface &ref_buffer, const I420BufferInterface &test_buffer)
{
    CXXKIT_DCHECK_GE(ref_buffer.width(), test_buffer.width());
    CXXKIT_DCHECK_GE(ref_buffer.height(), test_buffer.height());
    if ((ref_buffer.width() != test_buffer.width()) || (ref_buffer.height() != test_buffer.height()))
    {
        const SharedRefPtr<I420Buffer> scaled_buffer = I420Buffer::create(ref_buffer.width(), ref_buffer.height());
        scaled_buffer->scale_from(test_buffer);
        return I420Ssim(ref_buffer, *scaled_buffer);
    }

    return libyuv::I420Ssim(ref_buffer.get_data_y(),
                            ref_buffer.stride_y(),
                            ref_buffer.get_data_u(),
                            ref_buffer.stride_u(),
                            ref_buffer.get_data_v(),
                            ref_buffer.stride_v(),
                            test_buffer.get_data_y(),
                            test_buffer.stride_y(),
                            test_buffer.get_data_u(),
                            test_buffer.stride_u(),
                            test_buffer.get_data_v(),
                            test_buffer.stride_v(),
                            test_buffer.width(),
                            test_buffer.height());
}

CXXKIT_END_NAMESPACE