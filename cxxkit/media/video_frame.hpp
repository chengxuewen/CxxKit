/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2013 The WebRTC project authors.
**
** License: MIT License + BSD-3 (ported from libwebrtc api/video/video_frame.h,
** trimmed: no RTP packet infos / render parameters / processing time)
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

#include <cxxkit/memory/ref_counted_object.hpp>
#include <cxxkit/memory/shared_ref_ptr.hpp>
#include <cxxkit/media/color_space.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>
#include <cxxkit/media/video_rotation.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/tools/optional.hpp>

#include <cstdint>

namespace cxxkit {

// VideoFrame stores the underlying pixel data (a VideoFrameBuffer) plus frame
// metadata: id, timestamps, rotation, color space and the updated region.
// RTP-specific fields (packet infos, render parameters) are intentionally
// trimmed — CxxKit has no RTP requirement.
class CXXKIT_MEDIA_API VideoFrame {
public:
    static constexpr uint16_t kNotSetId = 0;

    struct CXXKIT_MEDIA_API UpdateRect {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;

        UpdateRect() = default;
        UpdateRect(int x, int y, int width, int height);

        // Returns the bounding box of this and other rect.
        UpdateRect Union(const UpdateRect& other) const;

        // Returns the intersection of this and other rect (empty if disjoint).
        UpdateRect Intersect(const UpdateRect& other) const;

        // Sets everything to 0, making this UpdateRect a zero-size (empty) update.
        void MakeEmptyUpdate();

        bool IsEmpty() const;

        // Per-member equality check. Empty rectangles with different offsets would
        // be considered different.
        bool operator==(const UpdateRect& other) const;
        bool operator!=(const UpdateRect& other) const { return !(*this == other); }

        // Scales updateRect given original frame dimensions.
        // Cropping is applied first, then rect is scaled down.
        // Update rect is snapped to 2x2 grid due to possible UV subsampling and
        // then expanded by additional 2 pixels in each direction to accommodate any
        // possible scaling artifacts.
        // Note, close but not equal update_rects on original frame may result in
        // the same scaled update rects.
        UpdateRect ScaleWithFrame(int frame_width,
                                  int frame_height,
                                  int crop_x,
                                  int crop_y,
                                  int crop_width,
                                  int crop_height,
                                  int scaled_width,
                                  int scaled_height) const;
    };

    // Preferred way of building VideoFrame objects.
    class CXXKIT_MEDIA_API Builder {
    public:
        Builder() = default;
        ~Builder() = default;

        VideoFrame build();
        Builder& set_video_frame_buffer(const SharedRefPtr<VideoFrameBuffer>& buffer);

        Builder& set_timestamp_rtp(uint32_t rtp_timestamp)
        {
            timestamp_rtp_ = rtp_timestamp;
            return *this;
        }

        Builder& set_timestamp_us(int64_t timestamp_us)
        {
            timestamp_us_ = timestamp_us;
            return *this;
        }

        Builder& set_rotation(VideoRotation rotation)
        {
            rotation_ = rotation;
            return *this;
        }

        Builder& set_color_space(const Optional<ColorSpace>& color_space)
        {
            color_space_ = color_space;
            return *this;
        }

        Builder& set_color_space(const ColorSpace* color_space);

        Builder& set_id(uint16_t id)
        {
            id_ = id;
            return *this;
        }

        Builder& set_update_rect(const Optional<UpdateRect>& update_rect)
        {
            update_rect_ = update_rect;
            return *this;
        }

    private:
        uint16_t id_ = kNotSetId;
        SharedRefPtr<VideoFrameBuffer> video_frame_buffer_;
        int64_t timestamp_us_ = 0;
        uint32_t timestamp_rtp_ = 0;
        VideoRotation rotation_ = VideoRotation::kVideoRotation_0;
        Optional<ColorSpace> color_space_;
        Optional<UpdateRect> update_rect_;
    };

    VideoFrame(uint16_t id,
               const SharedRefPtr<VideoFrameBuffer>& video_frame_buffer,
               int64_t timestamp_us,
               uint32_t timestamp_rtp,
               VideoRotation rotation,
               const Optional<ColorSpace>& color_space,
               const Optional<UpdateRect>& update_rect);

    // System monotonic clock, same timebase as rtc::TimeMicros().
    int64_t timestamp_us() const { return timestamp_us_; }
    void set_timestamp_us(int64_t timestamp_us) { timestamp_us_ = timestamp_us; }

    // Set frame timestamp (90kHz).
    void set_timestamp_rtp(uint32_t rtp_timestamp) { timestamp_rtp_ = rtp_timestamp; }
    // Get frame timestamp (90kHz).
    uint32_t timestamp_rtp() const { return timestamp_rtp_; }

    // Get frame ID. Returns `kNotSetId` if ID is not set.
    uint16_t id() const { return id_; }
    void set_id(uint16_t id) { id_ = id; }

    VideoRotation rotation() const { return rotation_; }
    void set_rotation(VideoRotation rotation) { rotation_ = rotation; }

    // Get color space when available.
    const Optional<ColorSpace>& color_space() const { return color_space_; }
    void set_color_space(const Optional<ColorSpace>& color_space) { color_space_ = color_space; }

    // Return the underlying buffer. Never nullptr for a properly initialized VideoFrame.
    SharedRefPtr<VideoFrameBuffer> video_frame_buffer() const { return video_frame_buffer_; }
    void set_video_frame_buffer(const SharedRefPtr<VideoFrameBuffer>& buffer);

    int width() const;
    int height() const;
    uint32_t size() const;  // Get frame size in pixels.

    bool has_update_rect() const { return update_rect_.has_value(); }

    // Returns updateRect set by the builder or set_update_rect() or whole frame rect if no update rect is available.
    UpdateRect update_rect() const { return update_rect_.value_or(UpdateRect{0, 0, width(), height()}); }
    // Rectangle must be within the frame dimensions.
    void set_update_rect(const VideoFrame::UpdateRect& update_rect);
    void clear_update_rect() { update_rect_ = utils::nullopt; }

private:
    uint16_t id_;
    // An opaque reference counted handle that stores the pixel data.
    SharedRefPtr<VideoFrameBuffer> video_frame_buffer_;
    uint32_t timestamp_rtp_;
    int64_t timestamp_us_;
    VideoRotation rotation_;
    Optional<ColorSpace> color_space_;
    // Updated since the last frame area. If present it means that the bounding
    // box of all the changes is within the rectangular area and is close to it.
    // If absent, it means that there's no information about the change at all and
    // updateRect() will return a rectangle corresponding to the entire frame.
    Optional<UpdateRect> update_rect_;
};

}  // namespace cxxkit