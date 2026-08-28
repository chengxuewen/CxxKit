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
#include <cxxkit/media/video_frame_buffer.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace {

TEST(I420Buffer, Create) {
    auto buf = cxxkit::I420Buffer::Create(16, 16);
    ASSERT_TRUE(buf);
    EXPECT_EQ(buf->width(), 16);
    EXPECT_EQ(buf->height(), 16);
    EXPECT_EQ(buf->type(), cxxkit::VideoType::kI420);
}

TEST(I420Buffer, Copy) {
    auto src = cxxkit::I420Buffer::Create(4, 4);
    src->SetBlack();  // Y=0, U=128, V=128 for black
    auto dst = cxxkit::I420Buffer::Copy(*src);
    ASSERT_TRUE(dst);
    EXPECT_EQ(dst->width(), 4);
    EXPECT_EQ(dst->height(), 4);
    // 校验 Y 平面为 0
    const uint8_t* y = dst->GetDataY();
    for (int i = 0; i < 4 * 4; ++i) EXPECT_EQ(y[i], 0);
}

TEST(I420Buffer, SetBlack) {
    auto buf = cxxkit::I420Buffer::Create(4, 4);
    buf->SetBlack();
    const uint8_t* y = buf->GetDataY();
    for (int i = 0; i < 16; ++i) EXPECT_EQ(y[i], 0);  // Y=0
    // U/V 平面为 128
    const uint8_t* u = buf->GetDataU();
    const uint8_t* v = buf->GetDataV();
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(u[i], 128);
        EXPECT_EQ(v[i], 128);
    }
}

TEST(I420Buffer, DataLayout) {
    auto buf = cxxkit::I420Buffer::Create(4, 4);
    // I420: Y 平面 w*h, U/V 平面 w/2*h/2，各 stride
    EXPECT_EQ(buf->StrideY(), 4);
    EXPECT_EQ(buf->StrideU(), 2);
    EXPECT_EQ(buf->StrideV(), 2);
}

TEST(I420Buffer, MutableDataWritable) {
    auto buf = cxxkit::I420Buffer::Create(4, 4);
    uint8_t* y = buf->MutableDataY();
    for (int i = 0; i < 16; ++i) y[i] = 42;
    EXPECT_EQ(buf->GetDataY()[0], 42);
}

TEST(I420Buffer, Rotate90) {
    auto src = cxxkit::I420Buffer::Create(4, 2);
    src->InitializeData();
    // 填 Y 平面行号，验证 90° 旋转后宽高交换
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 4; ++x) src->MutableDataY()[y * 4 + x] = static_cast<uint8_t>(y);
    auto rotated = cxxkit::I420Buffer::Rotate(*src, cxxkit::VideoRotation::kVideoRotation_90);
    ASSERT_TRUE(rotated);
    EXPECT_EQ(rotated->width(), 2);
    EXPECT_EQ(rotated->height(), 4);
    // 旋转后每一行都是原第 1 行（行 0 被旋转到最上）
    for (int y = 0; y < 4; ++y) EXPECT_EQ(rotated->GetDataY()[y * 2], 1);
}

}  // namespace
