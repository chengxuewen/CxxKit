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

#include <cxxkit/media/frame_generator.hpp>
#include <cxxkit/media/frame_generator_capturer.hpp>
#include <cxxkit/media/video_frame_buffer.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

TEST(FrameGenerator, SlideShow) {
    auto gen = cxxkit::FrameGenerator::CreateSlideShow(
        std::vector<std::string>{}, cxxkit::FrameGenerator::OutputType::kI420, 640, 480, 1);
    ASSERT_TRUE(gen);
    EXPECT_EQ(gen->width(), 640);
    EXPECT_EQ(gen->height(), 480);
    auto frame = gen->GetNextFrame();
    ASSERT_TRUE(frame);
    EXPECT_EQ(frame->width(), 640);
    EXPECT_EQ(frame->height(), 480);
    EXPECT_EQ(frame->type(), cxxkit::VideoType::kI420);
}

TEST(FrameGeneratorCapturer, GenerateOneFrameInvokesCallback) {
    int captured = 0;
    auto cap = cxxkit::CreateFrameGeneratorCapturer(
        30.0, 32, 32, [&captured](const cxxkit::VideoFrame& frame) {
            ++captured;
            EXPECT_TRUE(frame.video_frame_buffer());
            EXPECT_EQ(frame.width(), 32);
            EXPECT_EQ(frame.height(), 32);
            EXPECT_EQ(frame.timestamp_us(), 1000);  // 1,000,000 ns → 1000 us
        });
    ASSERT_TRUE(cap);
    cap->GenerateOneFrame(1000000);
    EXPECT_EQ(captured, 1);
}

TEST(FrameGeneratorCapturer, FramerateThrottleDropsFrames) {
    int captured = 0;
    auto cap = cxxkit::CreateFrameGeneratorCapturer(
        30.0, 32, 32, [&captured](const cxxkit::VideoFrame&) { ++captured; });
    ASSERT_TRUE(cap);
    cap->GenerateOneFrame(0);         // 首帧保留
    cap->GenerateOneFrame(0);         // 同一时间戳未到间隔 → 丢
    cap->GenerateOneFrame(40000000);  // +40ms > 33.3ms 帧间隔 → 保留
    EXPECT_EQ(captured, 2);
}

}  // namespace