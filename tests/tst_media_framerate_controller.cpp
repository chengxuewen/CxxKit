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

#include <cxxkit/media/framerate_controller.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

namespace {

TEST(FramerateController, SetGet) {
    cxxkit::FramerateController fc;
    fc.SetFrameRate(30.0);
    EXPECT_DOUBLE_EQ(fc.GetFrameRate(), 30.0);
}

TEST(FramerateController, ShouldDropFrame) {
    cxxkit::FramerateController fc(30.0);  // 30fps → 帧间隔 ~33.3ms
    // 同一时间戳连续查询：第二帧必然丢帧（未到间隔）
    EXPECT_FALSE(fc.ShouldDropFrame(0));
    EXPECT_TRUE(fc.ShouldDropFrame(0));  // 距上帧 0ms < 33ms → 丢
    // 时间推进到下一帧间隔后：不丢
    EXPECT_FALSE(fc.ShouldDropFrame(40000000));  // +40ms > 33.3ms → 保留
}

TEST(FramerateController, NoThrottleByDefault) {
    // 默认 maxdouble → 永不丢帧
    cxxkit::FramerateController fc;
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FALSE(fc.ShouldDropFrame(static_cast<int64_t>(i) * 1000));
    }
}

TEST(FramerateController, Reset) {
    cxxkit::FramerateController fc(30.0);
    EXPECT_FALSE(fc.ShouldDropFrame(0));
    EXPECT_TRUE(fc.ShouldDropFrame(0));
    fc.Reset();
    EXPECT_EQ(fc.GetFrameRate(), std::numeric_limits<double>::max());
    EXPECT_FALSE(fc.ShouldDropFrame(0));  // reset 后首帧不丢
}

TEST(FramerateController, BelowMinFpsDropsAll) {
    cxxkit::FramerateController fc(0.1);  // < kMinFramerate(0.5)
    EXPECT_TRUE(fc.ShouldDropFrame(0));
    EXPECT_TRUE(fc.ShouldDropFrame(1000000000));
}

}  // namespace
