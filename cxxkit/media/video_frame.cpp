/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2013 The WebRTC project authors.
**
** License: MIT License + BSD-3 (ported from libwebrtc api/video/video_frame.cc)
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

#include <cxxkit/media/video_frame.hpp>

#include <algorithm>

namespace cxxkit {

// Out-of-line definition for the C++11 ODR-used static constexpr member.
constexpr uint16_t VideoFrame::kNotSetId;

void VideoFrame::UpdateRect::MakeEmptyUpdate()
{
    width = height = x = y = 0;
}

bool VideoFrame::UpdateRect::IsEmpty() const
{
    return width == 0 && height == 0;
}

VideoFrame::UpdateRect::UpdateRect(int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height)
{
}

VideoFrame::UpdateRect VideoFrame::UpdateRect::Union(const UpdateRect& other) const
{
    if (other.IsEmpty())
    {
        return *this;
    }
    if (IsEmpty())
    {
        return other;
    }
    const int right = std::max(x + width, other.x + other.width);
    const int bottom = std::max(y + height, other.y + other.height);
    const int left = std::min(x, other.x);
    const int top = std::min(y, other.y);
    return UpdateRect(left, top, right - left, bottom - top);
}

VideoFrame::UpdateRect VideoFrame::UpdateRect::Intersect(const UpdateRect& other) const
{
    if (other.IsEmpty() || IsEmpty())
    {
        return UpdateRect();
    }

    const int right = std::min(x + width, other.x + other.width);
    const int bottom = std::min(y + height, other.y + other.height);
    const int left = std::max(x, other.x);
    const int top = std::max(y, other.y);
    const int width = right - left;
    const int height = bottom - top;
    if (width <= 0 || height <= 0)
    {
        return UpdateRect();
    }
    return UpdateRect(left, top, width, height);
}

bool VideoFrame::UpdateRect::operator==(const UpdateRect& other) const
{
    return other.x == x && other.y == y && other.width == width && other.height == height;
}

VideoFrame::UpdateRect VideoFrame::UpdateRect::ScaleWithFrame(int frame_width,
                                                              int frame_height,
                                                              int crop_x,
                                                              int crop_y,
                                                              int crop_width,
                                                              int crop_height,
                                                              int scaled_width,
                                                              int scaled_height) const
{
    CXXKIT_DCHECK_GT(frame_width, 0);
    CXXKIT_DCHECK_GT(frame_height, 0);
    CXXKIT_DCHECK_GT(crop_width, 0);
    CXXKIT_DCHECK_GT(crop_height, 0);
    CXXKIT_DCHECK_LE(crop_width + crop_x, frame_width);
    CXXKIT_DCHECK_LE(crop_height + crop_y, frame_height);
    CXXKIT_DCHECK_GT(scaled_width, 0);
    CXXKIT_DCHECK_GT(scaled_height, 0);

    // Check if update rect is out of the cropped area.
    if (x + width < crop_x || x > crop_x + crop_width || y + height < crop_y || y > crop_y + crop_height)
    {
        return UpdateRect();
    }

    int new_x = x - crop_x;
    int new_w = width;
    if (new_x < 0)
    {
        new_w += new_x;
        new_x = 0;
    }
    int new_y = y - crop_y;
    int new_h = height;
    if (new_y < 0)
    {
        new_h += new_y;
        new_y = 0;
    }

    // Lower corner is rounded down.
    new_x = new_x * scaled_width / crop_width;
    new_y = new_y * scaled_height / crop_height;
    // Upper corner is rounded up.
    new_w = (new_w * scaled_width + crop_width - 1) / crop_width;
    new_h = (new_h * scaled_height + crop_height - 1) / crop_height;

    // Round to full 2x2 blocks due to possible subsampling in the pixel data.
    if (new_x % 2)
    {
        --new_x;
        ++new_w;
    }
    if (new_y % 2)
    {
        --new_y;
        ++new_h;
    }
    if (new_w % 2)
    {
        ++new_w;
    }
    if (new_h % 2)
    {
        ++new_h;
    }

    // Expand the update rect by 2 pixels in each direction to include any
    // possible scaling artifacts.
    if (scaled_width != crop_width || scaled_height != crop_height)
    {
        if (new_x > 0)
        {
            new_x -= 2;
            new_w += 2;
        }
        if (new_y > 0)
        {
            new_y -= 2;
            new_h += 2;
        }
        new_w += 2;
        new_h += 2;
    }

    // Ensure update rect is inside frame dimensions.
    if (new_x + new_w > scaled_width)
    {
        new_w = scaled_width - new_x;
    }
    if (new_y + new_h > scaled_height)
    {
        new_h = scaled_height - new_y;
    }
    CXXKIT_DCHECK_GE(new_w, 0);
    CXXKIT_DCHECK_GE(new_h, 0);
    if (new_w == 0 || new_h == 0)
    {
        return UpdateRect();
    }

    return UpdateRect(new_x, new_y, new_w, new_h);
}

VideoFrame VideoFrame::Builder::build()
{
    CXXKIT_CHECK(mVideoFrameBuffer != nullptr);
    return VideoFrame(mId,
                      mVideoFrameBuffer,
                      mTimestampUs,
                      mTimestampRtp,
                      mRotation,
                      mColorSpace,
                      mUpdateRect);
}

VideoFrame::Builder& VideoFrame::Builder::set_video_frame_buffer(const SharedRefPtr<VideoFrameBuffer>& buffer)
{
    mVideoFrameBuffer = buffer;
    return *this;
}

VideoFrame::Builder& VideoFrame::Builder::set_color_space(const ColorSpace* color_space)
{
    mColorSpace = color_space ? utils::make_optional(*color_space) : utils::nullopt;
    return *this;
}

VideoFrame::VideoFrame(uint16_t id,
                       const SharedRefPtr<VideoFrameBuffer>& video_frame_buffer,
                       int64_t timestamp_us,
                       uint32_t timestamp_rtp,
                       VideoRotation rotation,
                       const Optional<ColorSpace>& color_space,
                       const Optional<UpdateRect>& update_rect)
    : mId(id)
    , mVideoFrameBuffer(video_frame_buffer)
    , mTimestampRtp(timestamp_rtp)
    , mTimestampUs(timestamp_us)
    , mRotation(rotation)
    , mColorSpace(color_space)
    , mUpdateRect(update_rect)
{
    if (mUpdateRect)
    {
        CXXKIT_DCHECK_GE(mUpdateRect->x, 0);
        CXXKIT_DCHECK_GE(mUpdateRect->y, 0);
        CXXKIT_DCHECK_LE(mUpdateRect->x + mUpdateRect->width, width());
        CXXKIT_DCHECK_LE(mUpdateRect->y + mUpdateRect->height, height());
    }
}

int VideoFrame::width() const
{
    return mVideoFrameBuffer ? mVideoFrameBuffer->width() : 0;
}

int VideoFrame::height() const
{
    return mVideoFrameBuffer ? mVideoFrameBuffer->height() : 0;
}

uint32_t VideoFrame::size() const
{
    return width() * height();
}

void VideoFrame::set_video_frame_buffer(const SharedRefPtr<VideoFrameBuffer>& buffer)
{
    CXXKIT_CHECK(buffer != nullptr);
    mVideoFrameBuffer = buffer;
}

void VideoFrame::set_update_rect(const VideoFrame::UpdateRect& update_rect)
{
    CXXKIT_DCHECK_GE(update_rect.x, 0);
    CXXKIT_DCHECK_GE(update_rect.y, 0);
    CXXKIT_DCHECK_LE(update_rect.x + update_rect.width, width());
    CXXKIT_DCHECK_LE(update_rect.y + update_rect.height, height());
    mUpdateRect = update_rect;
}

}  // namespace cxxkit