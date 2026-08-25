/*
 *  Copyright 2026 The CxxKit Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by the MIT license
 *  that can be found in the LICENSE file in the root of the source tree.
 */

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
