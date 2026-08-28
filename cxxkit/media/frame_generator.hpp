/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License (derived from OpenCTK frame_generator.hpp / libwebrtc test/frame_generator.h,
** trimmed to a self-contained I420 slideshow generator — no YUV-file/NV12/Scrolling generators)
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
#include <cxxkit/media/video_frame_buffer.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cxxkit {

// Abstract frame source producing I420 VideoFrameBuffers on demand. Used by
// FrameGeneratorCapturer for testing/placeholder/demo pipelines.
class CXXKIT_MEDIA_API FrameGenerator
{
public:
    enum class OutputType
    {
        kI420,
    };

    virtual ~FrameGenerator() = default;

    // Returns the next generated frame. Never null.
    virtual SharedRefPtr<VideoFrameBuffer> get_next_frame() = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;

    // Creates a slideshow generator. `filenames` is reserved for YUV-file
    // input (not implemented yet); pass an empty list to get a synth generator
    // that fills frames with randomly sized/colored moving squares.
    // `frame_repeat_count` determines how many times each slide is shown
    // before a new one is generated (1 = new slide per frame).
    static std::unique_ptr<FrameGenerator> create_slide_show(std::vector<std::string> filenames,
                                                           OutputType type,
                                                           int width,
                                                           int height,
                                                           int frame_repeat_count);
};

}  // namespace cxxkit