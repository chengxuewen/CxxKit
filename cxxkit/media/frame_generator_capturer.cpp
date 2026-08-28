/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License (derived from OpenCTK frame_generator_capturer.cpp, trimmed to a self-contained
** capturer — no CustomVideoCapturer/VideoTrackSource/broadcaster/adapter inheritance)
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

#include <cxxkit/media/frame_generator_capturer.hpp>

#include <cxxkit/tools/checks.hpp>

namespace cxxkit {

FrameGeneratorCapturer::FrameGeneratorCapturer(std::unique_ptr<FrameGenerator> generator, double framerate)
    : mGenerator(std::move(generator))
    , mFramerateController(framerate)
{
    CXXKIT_DCHECK(mGenerator);
}

void FrameGeneratorCapturer::SetFrameCallback(FrameCallback callback)
{
    mCallback = std::move(callback);
}

void FrameGeneratorCapturer::SetFrameRate(double fps)
{
    mFramerateController.SetFrameRate(fps);
}

double FrameGeneratorCapturer::GetFrameRate() const
{
    return mFramerateController.GetFrameRate();
}

void FrameGeneratorCapturer::GenerateOneFrame(int64_t timestamp_ns)
{
    if (!mGenerator || mFramerateController.ShouldDropFrame(timestamp_ns))
    {
        return;
    }

    VideoFrame frame = VideoFrame::Builder()
                           .set_video_frame_buffer(mGenerator->GetNextFrame())
                           .set_timestamp_us(timestamp_ns / 1000)
                           .build();
    if (mCallback)
    {
        mCallback(frame);
    }
}

int FrameGeneratorCapturer::width() const
{
    return mGenerator ? mGenerator->width() : 0;
}

int FrameGeneratorCapturer::height() const
{
    return mGenerator ? mGenerator->height() : 0;
}

std::unique_ptr<FrameGeneratorCapturer> CreateFrameGeneratorCapturer(double framerate,
                                                                     int width,
                                                                     int height,
                                                                     FrameGeneratorCapturer::FrameCallback callback)
{
    std::unique_ptr<FrameGenerator> generator = FrameGenerator::CreateSlideShow(
        std::vector<std::string>(), FrameGenerator::OutputType::kI420, width, height, 1);
    if (!generator)
    {
        return std::unique_ptr<FrameGeneratorCapturer>();
    }
    std::unique_ptr<FrameGeneratorCapturer> capturer(
        new FrameGeneratorCapturer(std::move(generator), framerate));
    capturer->SetFrameCallback(std::move(callback));
    return capturer;
}

}  // namespace cxxkit