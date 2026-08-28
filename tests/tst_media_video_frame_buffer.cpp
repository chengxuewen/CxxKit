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

#include <cxxkit/media/video_frame_buffer.hpp>
#include <cxxkit/memory/ref_count.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

// 本地 test double：实现 I420BufferInterface（Task 4 才移植真实 I420Buffer）。
// VideoFrameBuffer 继承 RefCountInterface，故 double 自带最小引用计数。
class NativeI420Buffer final : public cxxkit::I420BufferInterface {
public:
    NativeI420Buffer(int w, int h)
        : data_(static_cast<size_t>(w) * h * 3 / 2), w_(w), h_(h), sy_(w), su_(w / 2), sv_(w / 2)
    {
    }

    void add_ref() const override { ref_count_.inc_ref(); }
    cxxkit::RefCountReleaseStatus Release() const override
    {
        if (ref_count_.DecRef() == cxxkit::RefCountReleaseStatus::kDroppedLastRef) {
            delete this;
            return cxxkit::RefCountReleaseStatus::kDroppedLastRef;
        }
        return cxxkit::RefCountReleaseStatus::kOtherRefsRemained;
    }

    int width() const override { return w_; }
    int height() const override { return h_; }
    const uint8_t* GetDataY() const override { return data_.data(); }
    const uint8_t* GetDataU() const override { return data_.data() + w_ * h_; }
    const uint8_t* GetDataV() const override { return data_.data() + w_ * h_ * 5 / 4; }
    int StrideY() const override { return sy_; }
    int StrideU() const override { return su_; }
    int StrideV() const override { return sv_; }
    cxxkit::VideoType type() const override { return cxxkit::VideoType::kI420; }

private:
    ~NativeI420Buffer() override = default;

    mutable cxxkit::detail::RefCounter ref_count_{0};
    std::vector<uint8_t> data_;
    int w_, h_, sy_, su_, sv_;
};

TEST(VideoFrameBuffer, InterfaceType) {
    cxxkit::SharedRefPtr<NativeI420Buffer> buf(new NativeI420Buffer(2, 2));
    EXPECT_EQ(buf->width(), 2);
    EXPECT_EQ(buf->height(), 2);
    EXPECT_EQ(buf->type(), cxxkit::VideoType::kI420);
    EXPECT_EQ(buf->StrideY(), 2);
    EXPECT_EQ(buf->ChromaWidth(), 1);
    EXPECT_EQ(buf->ChromaHeight(), 1);
}

TEST(VideoFrameBuffer, WrapI420Buffer) {
    std::vector<uint8_t> mem(6);
    bool released = false;
    auto wrapped = cxxkit::WrapI420Buffer(
        2, 2, mem.data(), 2, mem.data() + 4, 1, mem.data() + 5, 1,
        [&released]() { released = true; });
    EXPECT_TRUE(wrapped);
    EXPECT_EQ(wrapped->width(), 2);
    EXPECT_EQ(wrapped->type(), cxxkit::VideoType::kI420);
    wrapped = nullptr;
    EXPECT_TRUE(released);
}

TEST(VideoFrameBuffer, ToI420ReturnsSelf) {
    cxxkit::SharedRefPtr<NativeI420Buffer> buf(new NativeI420Buffer(2, 2));
    auto converted = buf->ToI420();
    ASSERT_TRUE(converted);
    EXPECT_EQ(converted.get(), buf.get());
}

}  // namespace
