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

#include <cxxkit/media/video_frame_buffer_pool.hpp>

#include <gtest/gtest.h>

namespace {

TEST(VideoFrameBufferPool, ReusesBuffer) {
    cxxkit::VideoFrameBufferPool pool;
    {
        auto b1 = pool.create_i420_buffer(16, 16);
        EXPECT_TRUE(b1);
        EXPECT_EQ(b1->width(), 16);
        EXPECT_EQ(b1->height(), 16);
    }  // b1 析构，refcount==1 → 回池复用
    auto b2 = pool.create_i420_buffer(16, 16);
    EXPECT_TRUE(b2);
    auto b3 = pool.create_i420_buffer(16, 16);
    EXPECT_TRUE(b3);
}

TEST(VideoFrameBufferPool, DifferentSizeNoReuse) {
    cxxkit::VideoFrameBufferPool pool;
    void* first = nullptr;
    {
        auto b1 = pool.create_i420_buffer(16, 16);
        ASSERT_TRUE(b1);
        first = b1->mutable_data_y();
    }
    auto b2 = pool.create_i420_buffer(32, 32);  // 不同尺寸 → 旧缓冲被清除，不复用
    ASSERT_TRUE(b2);
    EXPECT_NE(b2->mutable_data_y(), first);
}

TEST(VideoFrameBufferPool, ResizeAndRelease) {
    cxxkit::VideoFrameBufferPool pool;
    {
        auto b1 = pool.create_i420_buffer(8, 8);
        ASSERT_TRUE(b1);
        auto b2 = pool.create_i420_buffer(8, 8);
        ASSERT_TRUE(b2);
        // 两个缓冲仍在用（refcount > 1），无法 resize 到 1。
        EXPECT_FALSE(pool.resize(1));
    }  // 均回池
    EXPECT_TRUE(pool.resize(1));
    pool.release();
    auto b = pool.create_i420_buffer(8, 8);
    EXPECT_TRUE(b);
}

}  // namespace