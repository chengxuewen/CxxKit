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

#include <cxxkit/media/i420_buffer.hpp>
#include <vector>
#include <cxxkit/media/video_frame.hpp>
#include <cxxkit/media/webrtc_libyuv.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

TEST(WebRtcLibyuv, calc_buffer_size) {
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kI420, 2, 2), 6u);
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kNV12, 2, 2), 6u);
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kARGB, 2, 2), 16u);
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kI420, 0, 4), 0u);
}

TEST(WebRtcLibyuv, extract_buffer) {
    auto src = cxxkit::I420Buffer::create(4, 4);
    src->set_black();
    const size_t size = cxxkit::calc_buffer_size(cxxkit::VideoType::kI420, 4, 4);
    std::vector<uint8_t> buf(size);
    const int ret = cxxkit::extract_buffer(*src, size, buf.data());
    EXPECT_EQ(ret, static_cast<int>(size));
    // Y plane all black (0), U/V planes 128.
    for (size_t i = 0; i < 16; ++i) EXPECT_EQ(buf[i], 0);
    for (size_t i = 16; i < size; ++i) EXPECT_EQ(buf[i], 128);
}

TEST(WebRtcLibyuv, ExtractBufferTooSmall) {
    auto src = cxxkit::I420Buffer::create(4, 4);
    uint8_t tiny = 0;
    EXPECT_LT(cxxkit::extract_buffer(*src, 4, &tiny), 0);
}

TEST(WebRtcLibyuv, ConvertFromI420ToArgb) {
    auto src = cxxkit::I420Buffer::create(4, 4);
    src->set_black();
    auto frame = cxxkit::VideoFrame::Builder()
                     .set_video_frame_buffer(src)
                     .set_timestamp_us(1000)
                     .build();
    const size_t size = cxxkit::calc_buffer_size(cxxkit::VideoType::kARGB, 4, 4);
    std::vector<uint8_t> dst(size);
    const int ret = cxxkit::convert_from_i420(frame, cxxkit::VideoType::kARGB, 4, 4, dst.data());
    EXPECT_EQ(ret, 0);
    // Black I420 → black ARGB: RGB=0, alpha=0xFF.
    for (size_t i = 0; i < size; i += 4) {
        EXPECT_EQ(dst[i], 0);
        EXPECT_EQ(dst[i + 1], 0);
        EXPECT_EQ(dst[i + 2], 0);
        EXPECT_EQ(dst[i + 3], 0xFF);
    }
}

TEST(WebRtcLibyuv, scale_video_frame_buffer) {
    auto src = cxxkit::I420Buffer::create(16, 16);
    src->set_black();
    auto scaled = cxxkit::scale_video_frame_buffer(*src, 8, 8);
    ASSERT_TRUE(scaled);
    EXPECT_EQ(scaled->width(), 8);
    EXPECT_EQ(scaled->height(), 8);
    EXPECT_EQ(scaled->type(), cxxkit::VideoType::kI420);
}

TEST(WebRtcLibyuv, PSNRSameFrame) {
    auto a = cxxkit::I420Buffer::create(8, 8);
    a->set_black();
    const double psnr = cxxkit::I420Psnr(*a, *a);
    // 相同帧 ⇒ mse=0 ⇒ libyuv 返回 128，被 kPerfectPSNR(48) 截断。
    EXPECT_DOUBLE_EQ(psnr, cxxkit::kPerfectPSNR);
}

TEST(WebRtcLibyuv, SSIMSameFrame) {
    auto a = cxxkit::I420Buffer::create(32, 32);
    // 用渐变内容避免恒定平面（恒定平面方差 0 → libyuv SSIM 为 NaN）。
    for (int i = 0; i < 32 * 32; ++i) a->mutable_data_y()[i] = static_cast<uint8_t>(i * 3);
    const double ssim = cxxkit::I420Ssim(*a, *a);
    EXPECT_NEAR(ssim, 1.0, 0.001);  // 相同帧 SSIM == 1.0
}

}  // namespace
// sample_size 全分支覆盖：NV12/UYVY/RGB24/RGB565 + ARGB（已有）
TEST(WebRtcLibyuv, ConvertFromI420AllFormats) {
    auto src = cxxkit::I420Buffer::create(4, 4);
    src->set_black();
    auto frame = cxxkit::VideoFrame::Builder()
                     .set_video_frame_buffer(src)
                     .set_timestamp_us(1000)
                     .build();
    const cxxkit::VideoType formats[] = {
        cxxkit::VideoType::kNV12, cxxkit::VideoType::kUYVY,
        cxxkit::VideoType::kRGB24, cxxkit::VideoType::kRGB565,
    };
    for (const auto fmt : formats) {
        // calc_buffer_size 对 RGB565 返回 0（video_types 未口径）；此处用 4*4*2 兜底
        const size_t size = fmt == cxxkit::VideoType::kRGB565
                                ? 4u * 4 * 2
                                : cxxkit::calc_buffer_size(fmt, 4, 4);
        ASSERT_GT(size, 0u) << "format size unknown";
        std::vector<uint8_t> dst(size);
        const int ret = cxxkit::convert_from_i420(frame, fmt, 4, 4, dst.data());
        EXPECT_EQ(ret, 0) << "convert_from_i420 failed for " << static_cast<int>(fmt);
    }
}

// 不同内容帧 → PSNR 有限值（非完美）
TEST(WebRtcLibyuv, PsnrDifferentFrames) {
    auto a = cxxkit::I420Buffer::create(16, 16);
    auto b = cxxkit::I420Buffer::create(16, 16);
    for (int i = 0; i < 16 * 16; ++i) {
        a->mutable_data_y()[i] = static_cast<uint8_t>(i);
        b->mutable_data_y()[i] = static_cast<uint8_t>(i + 100);
    }
    const double psnr = cxxkit::I420Psnr(*a, *b);
    EXPECT_GT(psnr, 0.0);
    EXPECT_LT(psnr, cxxkit::kPerfectPSNR);  // 有损 → 低于完美值
}
