/*
 *  Copyright 2026 The CxxKit Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by the MIT license
 *  that can be found in the LICENSE file in the root of the source tree.
 */

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