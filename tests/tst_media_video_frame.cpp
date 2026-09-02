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

#include <cxxkit/media/video_frame.hpp>
#include <cxxkit/media/i420_buffer.hpp>

#include <gtest/gtest.h>

namespace
{

TEST(VideoFrame, Builder)
{
    auto buf = cxxkit::I420Buffer::create(16, 16);
    auto frame = cxxkit::VideoFrame::Builder().set_video_frame_buffer(buf).set_timestamp_us(1000).build();
    EXPECT_EQ(frame.video_frame_buffer(), buf);
    EXPECT_EQ(frame.timestamp_us(), 1000);
    EXPECT_EQ(frame.width(), 16);
    EXPECT_EQ(frame.height(), 16);
    EXPECT_EQ(frame.id(), cxxkit::VideoFrame::kNotSetId);
    EXPECT_EQ(frame.rotation(), cxxkit::VideoRotation::kVideoRotation_0);
}

TEST(VideoFrame, BuilderAllMetadata)
{
    auto buf = cxxkit::I420Buffer::create(16, 16);
    cxxkit::ColorSpace cs(cxxkit::ColorSpace::PrimaryID::kBT709,
                          cxxkit::ColorSpace::TransferID::kBT709,
                          cxxkit::ColorSpace::MatrixID::kBT709,
                          cxxkit::ColorSpace::RangeID::kLimited);
    auto frame = cxxkit::VideoFrame::Builder()
                     .set_video_frame_buffer(buf)
                     .set_id(7)
                     .set_timestamp_rtp(90000)
                     .set_timestamp_us(1234)
                     .set_rotation(cxxkit::VideoRotation::kVideoRotation_90)
                     .set_color_space(&cs)
                     .build();
    EXPECT_EQ(frame.id(), 7);
    EXPECT_EQ(frame.timestamp_rtp(), 90000u);
    EXPECT_EQ(frame.timestamp_us(), 1234);
    EXPECT_EQ(frame.rotation(), cxxkit::VideoRotation::kVideoRotation_90);
    ASSERT_TRUE(frame.color_space());
    EXPECT_EQ(*frame.color_space(), cs);
}

TEST(VideoFrame, UpdateRectUnion)
{
    cxxkit::VideoFrame::UpdateRect a{0, 0, 10, 10};
    cxxkit::VideoFrame::UpdateRect b{5, 5, 10, 10};
    auto u = a.Union(b);
    EXPECT_EQ(u.x, 0);
    EXPECT_EQ(u.y, 0);
    EXPECT_EQ(u.width, 15);
    EXPECT_EQ(u.height, 15);
    // 原矩形不被修改
    EXPECT_EQ(a.width, 10);
    EXPECT_EQ(a.x, 0);
}

TEST(VideoFrame, UpdateRectIntersect)
{
    cxxkit::VideoFrame::UpdateRect a{0, 0, 10, 10};
    cxxkit::VideoFrame::UpdateRect b{5, 5, 10, 10};
    auto i = a.intersect(b);
    EXPECT_EQ(i.x, 5);
    EXPECT_EQ(i.y, 5);
    EXPECT_EQ(i.width, 5);
    EXPECT_EQ(i.height, 5);
    // 不相交 → 空矩形
    cxxkit::VideoFrame::UpdateRect c{20, 20, 4, 4};
    EXPECT_TRUE(a.intersect(c).is_empty());
}

TEST(VideoFrame, UpdateRectScaleWithFrame)
{
    // 640x360 帧，中间 320x180 裁剪，缩放到 320x180：原更新区 (0,0,640,360)
    cxxkit::VideoFrame::UpdateRect r{0, 0, 640, 360};
    auto s = r.scale_with_frame(640, 360, 160, 90, 320, 180, 320, 180);
    EXPECT_EQ(s.x, 0);
    EXPECT_EQ(s.y, 0);
    EXPECT_EQ(s.width, 320);
    EXPECT_EQ(s.height, 180);
}

} // namespace
