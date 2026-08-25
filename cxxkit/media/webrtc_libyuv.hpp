/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
**
** License: MIT License + BSD-3 (ported from libwebrtc common_video/libyuv/webrtc_libyuv.h,
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

#pragma once

#include <cxxkit/media/media_global.hpp>
#include <cxxkit/media/video_frame.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>
#include <cxxkit/media/video_types.hpp>

#include <cstddef>
#include <cstdint>

namespace cxxkit {

// This is the max PSNR value our algorithms can return.
const double kPerfectPSNR = 48.0;

// Extract buffer from an I420BufferInterface (consecutive planes, no stride).
// Input:
//   - input_frame : Reference to an I420 buffer.
//   - size        : Size of the allocated buffer. If insufficient, -1 is
//                   returned.
//   - buffer      : Pointer to destination buffer.
// Return value: length of buffer if OK, < 0 otherwise.
int ExtractBuffer(const I420BufferInterface& input_frame, size_t size, uint8_t* buffer);

// Convert From I420.
// Input:
//   - src_frame        : Reference to a source frame.
//   - dst_video_type   : Type of output video (NV12/ARGB/RGB24/RGB565/UYVY).
//   - dst_width        : Width of the destination frame (row pitch is derived
//                        from the format).
//   - dst_height       : Height of the destination frame.
//   - dst_frame        : Pointer to a destination frame.
// Return value: 0 if OK, < 0 otherwise.
int ConvertFromI420(const VideoFrame& src_frame,
                    VideoType dst_video_type,
                    int dst_width,
                    int dst_height,
                    uint8_t* dst_frame);

// Scales an I420 frame to a new resolution. Uses libyuv::I420Scale with box
// filtering.
SharedRefPtr<I420BufferInterface> ScaleVideoFrameBuffer(const I420BufferInterface& source,
                                                        int dst_width,
                                                        int dst_height);

// Compute PSNR for an I420 frame (all planes). Returns the PSNR in decibel,
// to a maximum of kPerfectPSNR. If the buffers differ in size, the test frame
// is first scaled up to the reference resolution.
double I420Psnr(const I420BufferInterface& ref_buffer, const I420BufferInterface& test_buffer);

// Compute SSIM for an I420 frame (all planes). If the buffers differ in size,
// the test frame is first scaled up to the reference resolution.
double I420Ssim(const I420BufferInterface& ref_buffer, const I420BufferInterface& test_buffer);

}  // namespace cxxkit