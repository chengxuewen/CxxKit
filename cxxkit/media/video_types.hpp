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

// CalcBufferSize: 单一归属（inline 定义，M6），webrtc_libyuv 复用不重定义
inline size_t CalcBufferSize(VideoType type, int width, int height) {
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
