/*
 *  Copyright 2026 The CxxKit Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by the MIT license
 *  that can be found in the LICENSE file in the root of the source tree.
 */

#include <cxxkit/media/color_space.hpp>
#include <cxxkit/media/video_rotation.hpp>
#include <cxxkit/media/video_types.hpp>

#include <gtest/gtest.h>

namespace {

using cxxkit::ColorSpace;

TEST(ColorSpace, MemberDefaultsAreUnspecified) {
    ColorSpace cs;
    EXPECT_EQ(cs.primaries(), ColorSpace::PrimaryID::kUnspecified);
    EXPECT_EQ(cs.transfer(), ColorSpace::TransferID::kUnspecified);
    EXPECT_EQ(cs.matrix(), ColorSpace::MatrixID::kUnspecified);
    EXPECT_EQ(cs.range(), ColorSpace::RangeID::kInvalid);
}

TEST(ColorSpace, ParamCtor) {
    ColorSpace cs(ColorSpace::PrimaryID::kBT709,
                  ColorSpace::TransferID::kBT709,
                  ColorSpace::MatrixID::kBT709,
                  ColorSpace::RangeID::kLimited);
    EXPECT_EQ(cs.primaries(), ColorSpace::PrimaryID::kBT709);
    EXPECT_EQ(cs.transfer(), ColorSpace::TransferID::kBT709);
    EXPECT_EQ(cs.matrix(), ColorSpace::MatrixID::kBT709);
    EXPECT_EQ(cs.range(), ColorSpace::RangeID::kLimited);
}

TEST(ColorSpace, Equality) {
    ColorSpace a(ColorSpace::PrimaryID::kBT709,
                 ColorSpace::TransferID::kBT709,
                 ColorSpace::MatrixID::kBT709,
                 ColorSpace::RangeID::kLimited);
    ColorSpace b(ColorSpace::PrimaryID::kBT709,
                 ColorSpace::TransferID::kBT709,
                 ColorSpace::MatrixID::kBT709,
                 ColorSpace::RangeID::kLimited);
    ColorSpace c(ColorSpace::PrimaryID::kBT2020,
                 ColorSpace::TransferID::kBT709,
                 ColorSpace::MatrixID::kBT709,
                 ColorSpace::RangeID::kLimited);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(ColorSpace, SetFromUint8) {
    ColorSpace cs;
    EXPECT_TRUE(cs.set_primaries_from_uint8(9));    // kBT2020
    EXPECT_TRUE(cs.set_matrix_from_uint8(9));       // kBT2020_NCL
    EXPECT_TRUE(cs.set_range_from_uint8(2));        // kFull
    EXPECT_EQ(cs.primaries(), ColorSpace::PrimaryID::kBT2020);
    EXPECT_EQ(cs.matrix(), ColorSpace::MatrixID::kBT2020_NCL);
    EXPECT_EQ(cs.range(), ColorSpace::RangeID::kFull);
    // 非法值返回 false
    EXPECT_FALSE(cs.set_primaries_from_uint8(255));
}

TEST(VideoType, CalcBufferSize) {
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 2, 2), 6u);   // 1.5 * w * h
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kNV12, 2, 2), 6u);
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kARGB, 2, 2), 16u);  // 4 * w * h
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kRGB24, 2, 2), 12u); // 3 * w * h
    EXPECT_EQ(cxxkit::CalcBufferSize(cxxkit::VideoType::kI420, 0, 2), 0u);   // 无效尺寸
}

TEST(VideoType, RotationValues) {
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_0), 0);
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_90), 90);
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_180), 180);
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_270), 270);
}

}  // namespace