/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License (ported from OpenCTK framerate_controller)
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

#include "framerate_controller.hpp"

#include <cstdlib>
#include <limits>

namespace cxxkit {

namespace {

// Below this rate every frame is dropped (throttling is meaningless).
const double kMinFramerate = 0.5;
const int64_t kNSecsPerSec = 1000000000LL;

}  // namespace

FramerateController::FramerateController()
    : FramerateController(std::numeric_limits<double>::max())
{
}

FramerateController::FramerateController(double max_framerate)
    : max_framerate_(max_framerate)
{
}

void FramerateController::SetFrameRate(double max_framerate)
{
    max_framerate_ = max_framerate;
}

double FramerateController::GetFrameRate() const
{
    return max_framerate_;
}

bool FramerateController::ShouldDropFrame(int64_t in_timestamp_nsecs)
{
    if (max_framerate_ < kMinFramerate)
    {
        return true;
    }

    // If `max_framerate_` is not set (i.e. maxdouble), `frame_interval_ns` is
    // rounded to 0.
    int64_t frame_interval_ns = kNSecsPerSec / max_framerate_;
    if (frame_interval_ns <= 0)
    {
        // Frame rate throttling not enabled.
        return false;
    }

    if (next_frame_timestamp_ns_)
    {
        // Time until next frame should be outputted.
        const int64_t time_until_next_frame_ns = (*next_frame_timestamp_ns_ - in_timestamp_nsecs);
        // Continue if timestamp is within expected range.
        if (std::abs(time_until_next_frame_ns) < 2 * frame_interval_ns)
        {
            // Drop if a frame shouldn't be outputted yet.
            if (time_until_next_frame_ns > 0)
            {
                return true;
            }
            // Time to output new frame.
            *next_frame_timestamp_ns_ += frame_interval_ns;
            return false;
        }
    }

    // First timestamp received or timestamp is way outside expected range, so
    // reset. Set first timestamp target to just half the interval to prefer
    // keeping frames in case of jitter.
    next_frame_timestamp_ns_ = in_timestamp_nsecs + frame_interval_ns / 2;
    return false;
}

void FramerateController::Reset()
{
    max_framerate_ = std::numeric_limits<double>::max();
    next_frame_timestamp_ns_ = utils::nullopt;
}

void FramerateController::KeepFrame(int64_t in_timestamp_nsecs)
{
    if (ShouldDropFrame(in_timestamp_nsecs))
    {
        if (max_framerate_ < kMinFramerate)
        {
            return;
        }

        int64_t frame_interval_ns = kNSecsPerSec / max_framerate_;
        if (next_frame_timestamp_ns_)
        {
            *next_frame_timestamp_ns_ += frame_interval_ns;
        }
    }
}

}  // namespace cxxkit