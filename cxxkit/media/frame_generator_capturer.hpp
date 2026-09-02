/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License (derived from OpenCTK frame_generator_capturer.hpp, trimmed to a self-contained
** capturer — no CustomVideoCapturer/VideoTrackSource/broadcaster/adapter inheritance, none of which are
** ported to CxxKit)
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

#include <cxxkit/media/frame_generator.hpp>
#include <cxxkit/media/framerate_controller.hpp>
#include <cxxkit/media/media_global.hpp>
#include <cxxkit/media/video_frame.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>

#include <cstdint>
#include <functional>
#include <memory>

CXXKIT_BEGIN_NAMESPACE

// Self-contained capturer that emits VideoFrames from a FrameGenerator,
// throttled by a FramerateController. Unlike the OpenCTK original it does not
// inherit CustomVideoCapturer/VideoTrackSource (not ported); consumers receive
// frames through a callback instead of a video sink graph.
class CXXKIT_MEDIA_API FrameGeneratorCapturer
{
public:
    using FrameCallback = std::function<void(const VideoFrame &)>;

    FrameGeneratorCapturer(std::unique_ptr<FrameGenerator> generator, double framerate);
    ~FrameGeneratorCapturer() = default;

    void set_frame_callback(FrameCallback callback);
    void set_frame_rate(double fps);
    double get_frame_rate() const;

    // Synchronously generate one frame at the given monotonic timestamp (ns)
    // and, if the framerate throttle keeps it, build a VideoFrame and invoke
    // the callback. The frame's timestamp_us is `timestamp_ns / 1000`.
    // ponytail: no capture thread — tests/drivers call this directly. add a
    // threaded Start()/Stop() emission loop when a real-time capturer is
    // needed.
    void generate_one_frame(int64_t timestamp_ns);

    int width() const;
    int height() const;

private:
    std::unique_ptr<FrameGenerator> mGenerator;
    FramerateController mFramerateController;
    FrameCallback mCallback;
};

// Factory: creates a FrameGeneratorCapturer backed by a synthetic slideshow
// FrameGenerator of the given resolution.
CXXKIT_MEDIA_API std::unique_ptr<FrameGeneratorCapturer> create_frame_generator_capturer(
    double framerate,
    int width,
    int height,
    FrameGeneratorCapturer::FrameCallback callback);

CXXKIT_END_NAMESPACE