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

// exp_media: I420 frame buffers -- generate, rotate, scale, PSNR.

#include <iostream>

#include <cxxkit/media/frame_generator.hpp>
#include <cxxkit/media/i420_buffer.hpp>
#include <cxxkit/media/video_rotation.hpp>
#include <cxxkit/media/webrtc_libyuv.hpp>

using namespace cxxkit;

namespace
{
// Fixed checkerboard-ish pattern (no rand): horizontal gradient + vertical stripes for some detail.
void fill_pattern(I420Buffer &buffer)
{
    const int width = buffer.width();
    const int height = buffer.height();
    uint8_t *data_y = buffer.mutable_data_y();
    uint8_t *data_u = buffer.mutable_data_u();
    uint8_t *data_v = buffer.mutable_data_v();
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            data_y[y * buffer.stride_y() + x] = static_cast<uint8_t>(((x * 255) / (width - 1)) ^ ((y / 8) % 2) * 40);
        }
    }
    for (int y = 0; y < height / 2; ++y)
    {
        for (int x = 0; x < width / 2; ++x)
        {
            data_u[y * buffer.stride_u() + x] = 128;
            data_v[y * buffer.stride_v() + x] = 128;
        }
    }
}
} // namespace

int main()
{
    // 1) FrameGenerator: synthesized test frames (moving squares), fixed by repeat=1.
    std::unique_ptr<FrameGenerator> generator = FrameGenerator::create_slide_show({},
                                                                                  FrameGenerator::OutputType::kI420,
                                                                                  320,
                                                                                  180,
                                                                                  1);
    SharedRefPtr<VideoFrameBuffer> generated = generator->get_next_frame();
    std::cout << "[generator] " << generator->width() << "x" << generator->height()
              << ", buffer type=" << static_cast<int>(generated->type()) << std::endl;

    // 2) I420Buffer: create + fill a fixed pattern, rotate 90, then scale 320x180 -> 160x90.
    SharedRefPtr<I420Buffer> source = I420Buffer::create(320, 180);
    fill_pattern(*source);
    SharedRefPtr<I420Buffer> rotated = I420Buffer::rotate(*source, VideoRotation::kVideoRotation_90);
    SharedRefPtr<I420Buffer> scaled = I420Buffer::create(160, 90);
    scaled->scale_from(*source);
    std::cout << "[transform] source " << source->width() << "x" << source->height() << " -> rotated "
              << rotated->width() << "x" << rotated->height() << " -> scaled " << scaled->width() << "x"
              << scaled->height() << std::endl;

    // 3) PSNR between original and downscaled frame (test frame upscaled internally by I420Psnr).
    double psnr = I420Psnr(*source, *scaled);
    std::cout << "[psnr] source vs 160x90 downscale: " << psnr << " dB (perfect = " << kPerfectPSNR << " dB)"
              << std::endl;
    return 0;
}
