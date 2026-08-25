#pragma once
// VideoRotation enum — port from libwebrtc api/video/video_rotation.h

#include <cxxkit/media/media_global.hpp>
#include <cstdint>

namespace cxxkit {

enum class CXXKIT_MEDIA_API VideoRotation : int {
    kVideoRotation_0 = 0,
    kVideoRotation_90 = 90,
    kVideoRotation_180 = 180,
    kVideoRotation_270 = 270,
};

}  // namespace cxxkit
