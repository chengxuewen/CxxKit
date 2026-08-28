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
// VideoType pixel-format enum — port from libwebrtc api/video/video_frame_buffer.h (VideoType)

#include <cxxkit/media/media_global.hpp>
#include <cstddef>

namespace cxxkit {

enum class CXXKIT_MEDIA_API VideoType : uint8_t {
    kUnknown,
    kI420,
    kIYUV,
    kRGB24,
    kABGR,
    kARGB,
    kARGB4444,
    kRGB565,
    kARGB1555,
    kYUY2,
    kYV12,
    kUYVY,
    kMJPEG,
    kNV12,
    kNV21,
    kBGRA,
    kI010,
    kI210,
    kI410,
    kI422,
    kI444,
};

// calc_buffer_size: 单一归属（inline 定义，M6），webrtc_libyuv 复用不重定义
inline size_t calc_buffer_size(VideoType type, int width, int height) {
    if (width <= 0 || height <= 0) return 0;
    switch (type) {
        case VideoType::kI420: case VideoType::kIYUV: case VideoType::kYV12:
        case VideoType::kNV12: case VideoType::kNV21:
            return static_cast<size_t>(width) * height * 3 / 2;
        case VideoType::kRGB24:
            return static_cast<size_t>(width) * height * 3;
        case VideoType::kABGR: case VideoType::kARGB: case VideoType::kBGRA:
            return static_cast<size_t>(width) * height * 4;
        case VideoType::kI422: case VideoType::kUYVY: case VideoType::kYUY2:
            return static_cast<size_t>(width) * height * 2;
        case VideoType::kI444: case VideoType::kI010:
            return static_cast<size_t>(width) * height * 3;
        case VideoType::kMJPEG: case VideoType::kUnknown:
        case VideoType::kARGB4444: case VideoType::kARGB1555:
        case VideoType::kRGB565: case VideoType::kI210: case VideoType::kI410:
            return 0;  // 未支持格式
    }
    return 0;
}

}  // namespace cxxkit
