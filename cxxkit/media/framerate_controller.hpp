/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License (ported from OpenCTK framerate_controller, itself a
** port of libwebrtc common_video/framerate_controller)
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
#include <cxxkit/tools/optional.hpp>

#include <cstdint>

CXXKIT_BEGIN_NAMESPACE

// Determines which frames should be dropped based on input framerate and
// requested (target) framerate. API follows OpenCTK: should_drop_frame() takes
// the incoming frame timestamp in nanoseconds and reports whether it must be
// dropped to keep the configured frame rate.
class CXXKIT_MEDIA_API FramerateController
{
public:
    FramerateController();
    explicit FramerateController(double max_framerate);

    // Sets max framerate (default is maxdouble = no throttling).
    void set_frame_rate(double max_framerate);
    double get_frame_rate() const;

    // Returns true if the frame with the given timestamp (ns) should be dropped,
    // false otherwise. Timestamps are expected to be monotonically increasing.
    bool should_drop_frame(int64_t in_timestamp_nsecs);

    // Resets to the default state: max framerate and no pending frame.
    void reset();

    // Registers the frame as kept even if should_drop_frame would have dropped it.
    void keep_frame(int64_t in_timestamp_nsecs);

private:
    double mMaxFramerate;
    Optional<int64_t> mNextFrameTimestampNs;
};

CXXKIT_END_NAMESPACE