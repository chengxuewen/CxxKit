/*
 *  Copyright 2026 The CxxKit Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by the MIT license
 *  that can be found in the LICENSE file in the root of the source tree.
 */

#include <cxxkit/media/i420_buffer.hpp>
#include <cxxkit/media/video_frame.hpp>
#include <cxxkit/media/webrtc_libyuv.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

TEST(WebRtcLibyuv, CalcBufferSize) {
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 2, 2), 6u);
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kNV12, 2, 2), 6u);
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kARGB, 2, 2), 16u);
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 0, 4), 0u);
}

TEST(WebRtcLibyuv, ExtractBuffer) {
    auto src = cxxkit::I420Buffer::Create(4, 4);
    src->SetBlack();
    const size_t size = cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 4, 4);
    std::vector<uint8_t> buf(size);
    const int ret = cxxkit::ExtractBuffer(*src, size, buf.data());
    EXPECT_EQ(ret, static_cast<int>(size));
    // Y plane all black (0), U/V planes 128.
    for (size_t i = 0; i < 16; ++i) EXPECT_EQ(buf[i], 0);
    for (size_t i = 16; i < size; ++i) EXPECT_EQ(buf[i], 128);
}

TEST(WebRtcLibyuv, ExtractBufferTooSmall) {
    auto src = cxxkit::I420Buffer::Create(4, 4);
    uint8_t tiny = 0;
    EXPECT_LT(cxxkit::ExtractBuffer(*src, 4, &tiny), 0);
}

TEST(WebRtcLibyuv, ConvertFromI420ToArgb) {
    auto src = cxxkit::I420Buffer::Create(4, 4);
    src->SetBlack();
    auto frame = cxxkit::VideoFrame::Builder()
                     .set_video_frame_buffer(src)
                     .set_timestamp_us(1000)
                     .build();
    const size_t size = cxxkit::CalcBufferSize(cxxkit::VideoType::kARGB, 4, 4);
    std::vector<uint8_t> dst(size);
    const int ret = cxxkit::ConvertFromI420(frame, cxxkit::VideoType::kARGB, 4, 4, dst.data());
    EXPECT_EQ(ret, 0);
    // Black I420 → black ARGB: RGB=0, alpha=0xFF.
    for (size_t i = 0; i < size; i += 4) {
        EXPECT_EQ(dst[i], 0);
        EXPECT_EQ(dst[i + 1], 0);
        EXPECT_EQ(dst[i + 2], 0);
        EXPECT_EQ(dst[i + 3], 0xFF);
    }
}

TEST(WebRtcLibyuv, ScaleVideoFrameBuffer) {
    auto src = cxxkit::I420Buffer::Create(16, 16);
    src->SetBlack();
    auto scaled = cxxkit::ScaleVideoFrameBuffer(*src, 8, 8);
    ASSERT_TRUE(scaled);
    EXPECT_EQ(scaled->width(), 8);
    EXPECT_EQ(scaled->height(), 8);
    EXPECT_EQ(scaled->type(), cxxkit::VideoType::kI420);
}

TEST(WebRtcLibyuv, PSNRSameFrame) {
    auto a = cxxkit::I420Buffer::Create(8, 8);
    a->SetBlack();
    const double psnr = cxxkit::I420Psnr(*a, *a);
    // 相同帧 ⇒ mse=0 ⇒ libyuv 返回 128，被 kPerfectPSNR(48) 截断。
    EXPECT_DOUBLE_EQ(psnr, cxxkit::kPerfectPSNR);
}

TEST(WebRtcLibyuv, SSIMSameFrame) {
    auto a = cxxkit::I420Buffer::Create(32, 32);
    // 用渐变内容避免恒定平面（恒定平面方差 0 → libyuv SSIM 为 NaN）。
    for (int i = 0; i < 32 * 32; ++i) a->MutableDataY()[i] = static_cast<uint8_t>(i * 3);
    const double ssim = cxxkit::I420Ssim(*a, *a);
    EXPECT_NEAR(ssim, 1.0, 0.001);  // 相同帧 SSIM == 1.0
}

}  // namespace