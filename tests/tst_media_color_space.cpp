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

#include <cxxkit/media/color_space.hpp>
#include <cxxkit/media/video_rotation.hpp>
#include <cxxkit/media/video_types.hpp>

#include <gtest/gtest.h>

namespace
{

using cxxkit::ColorSpace;

TEST(ColorSpace, MemberDefaultsAreUnspecified)
{
    ColorSpace cs;
    EXPECT_EQ(cs.primaries(), ColorSpace::PrimaryID::kUnspecified);
    EXPECT_EQ(cs.transfer(), ColorSpace::TransferID::kUnspecified);
    EXPECT_EQ(cs.matrix(), ColorSpace::MatrixID::kUnspecified);
    EXPECT_EQ(cs.range(), ColorSpace::RangeID::kInvalid);
}

TEST(ColorSpace, ParamCtor)
{
    ColorSpace cs(ColorSpace::PrimaryID::kBT709,
                  ColorSpace::TransferID::kBT709,
                  ColorSpace::MatrixID::kBT709,
                  ColorSpace::RangeID::kLimited);
    EXPECT_EQ(cs.primaries(), ColorSpace::PrimaryID::kBT709);
    EXPECT_EQ(cs.transfer(), ColorSpace::TransferID::kBT709);
    EXPECT_EQ(cs.matrix(), ColorSpace::MatrixID::kBT709);
    EXPECT_EQ(cs.range(), ColorSpace::RangeID::kLimited);
}

TEST(ColorSpace, Equality)
{
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

TEST(ColorSpace, set_from_uint8)
{
    ColorSpace cs;
    EXPECT_TRUE(cs.set_primaries_from_uint8(9)); // kBT2020
    EXPECT_TRUE(cs.set_matrix_from_uint8(9));    // kBT2020_NCL
    EXPECT_TRUE(cs.set_range_from_uint8(2));     // kFull
    EXPECT_EQ(cs.primaries(), ColorSpace::PrimaryID::kBT2020);
    EXPECT_EQ(cs.matrix(), ColorSpace::MatrixID::kBT2020_NCL);
    EXPECT_EQ(cs.range(), ColorSpace::RangeID::kFull);
    // 非法值返回 false
    EXPECT_FALSE(cs.set_primaries_from_uint8(255));
}

TEST(VideoType, calc_buffer_size)
{
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kI420, 2, 2), 6u); // 1.5 * w * h
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kNV12, 2, 2), 6u);
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kARGB, 2, 2), 16u);  // 4 * w * h
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kRGB24, 2, 2), 12u); // 3 * w * h
    EXPECT_EQ(cxxkit::calc_buffer_size(cxxkit::VideoType::kI420, 0, 2), 0u);   // 无效尺寸
}

TEST(VideoType, RotationValues)
{
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_0), 0);
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_90), 90);
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_180), 180);
    EXPECT_EQ(static_cast<int>(cxxkit::VideoRotation::kVideoRotation_270), 270);
}

} // namespace
// as_string 覆盖：所有枚举分支（+~70 行覆盖）
TEST(ColorSpace, AsStringAllEnums)
{
    using cxxkit::ColorSpace;
    // PrimaryID 各值
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kBT709,
                         ColorSpace::TransferID::kBT709,
                         ColorSpace::MatrixID::kBT709,
                         ColorSpace::RangeID::kLimited)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kBT470M,
                         ColorSpace::TransferID::kUnspecified,
                         ColorSpace::MatrixID::kUnspecified,
                         ColorSpace::RangeID::kInvalid)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kBT470BG,
                         ColorSpace::TransferID::kGAMMA22,
                         ColorSpace::MatrixID::kFCC,
                         ColorSpace::RangeID::kFull)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kSMPTE170M,
                         ColorSpace::TransferID::kGAMMA28,
                         ColorSpace::MatrixID::kBT470BG,
                         ColorSpace::RangeID::kDerived)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kSMPTE240M,
                         ColorSpace::TransferID::kSMPTE170M,
                         ColorSpace::MatrixID::kSMPTE170M,
                         ColorSpace::RangeID::kLimited)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kFILM,
                         ColorSpace::TransferID::kSMPTE240M,
                         ColorSpace::MatrixID::kSMPTE240M,
                         ColorSpace::RangeID::kFull)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kBT2020,
                         ColorSpace::TransferID::kLINEAR,
                         ColorSpace::MatrixID::kYCOCG,
                         ColorSpace::RangeID::kInvalid)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kSMPTEST428,
                         ColorSpace::TransferID::kLOG,
                         ColorSpace::MatrixID::kBT2020_NCL,
                         ColorSpace::RangeID::kLimited)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kSMPTEST431,
                         ColorSpace::TransferID::kLOG_SQRT,
                         ColorSpace::MatrixID::kBT2020_CL,
                         ColorSpace::RangeID::kFull)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kSMPTEST432,
                         ColorSpace::TransferID::kIEC61966_2_4,
                         ColorSpace::MatrixID::kSMPTE2085,
                         ColorSpace::RangeID::kDerived)
                  .as_string(),
              "");
    EXPECT_NE(ColorSpace(ColorSpace::PrimaryID::kJEDECP22,
                         ColorSpace::TransferID::kBT1361_ECG,
                         ColorSpace::MatrixID::kCDNCLS,
                         ColorSpace::RangeID::kLimited)
                  .as_string(),
              "");
}

// from_uint8 覆盖所有枚举分支
TEST(ColorSpace, SetFromUint8AllValid)
{
    cxxkit::ColorSpace cs;
    // PrimaryID 合法值 1..12 + 22
    EXPECT_TRUE(cs.set_primaries_from_uint8(1));
    EXPECT_TRUE(cs.set_primaries_from_uint8(2));
    EXPECT_TRUE(cs.set_primaries_from_uint8(4));
    EXPECT_TRUE(cs.set_primaries_from_uint8(5));
    EXPECT_TRUE(cs.set_primaries_from_uint8(6));
    EXPECT_TRUE(cs.set_primaries_from_uint8(7));
    EXPECT_TRUE(cs.set_primaries_from_uint8(8));
    EXPECT_TRUE(cs.set_primaries_from_uint8(9));
    EXPECT_TRUE(cs.set_primaries_from_uint8(10));
    EXPECT_TRUE(cs.set_primaries_from_uint8(11));
    EXPECT_TRUE(cs.set_primaries_from_uint8(12));
    EXPECT_TRUE(cs.set_primaries_from_uint8(22));
    EXPECT_FALSE(cs.set_primaries_from_uint8(3)); // 非法
    EXPECT_FALSE(cs.set_primaries_from_uint8(255));
    // TransferID 合法值 1..18
    EXPECT_TRUE(cs.set_transfer_from_uint8(1));
    EXPECT_TRUE(cs.set_transfer_from_uint8(4));
    EXPECT_TRUE(cs.set_transfer_from_uint8(5));
    EXPECT_TRUE(cs.set_transfer_from_uint8(6));
    EXPECT_TRUE(cs.set_transfer_from_uint8(7));
    EXPECT_TRUE(cs.set_transfer_from_uint8(8));
    EXPECT_TRUE(cs.set_transfer_from_uint8(9));
    EXPECT_TRUE(cs.set_transfer_from_uint8(10));
    EXPECT_TRUE(cs.set_transfer_from_uint8(11));
    EXPECT_TRUE(cs.set_transfer_from_uint8(12));
    EXPECT_TRUE(cs.set_transfer_from_uint8(13));
    EXPECT_TRUE(cs.set_transfer_from_uint8(14));
    EXPECT_TRUE(cs.set_transfer_from_uint8(15));
    EXPECT_TRUE(cs.set_transfer_from_uint8(16));
    EXPECT_TRUE(cs.set_transfer_from_uint8(17));
    EXPECT_TRUE(cs.set_transfer_from_uint8(18));
    EXPECT_FALSE(cs.set_transfer_from_uint8(3));
    // MatrixID 合法值
    EXPECT_TRUE(cs.set_matrix_from_uint8(0)); // kRGB
    EXPECT_TRUE(cs.set_matrix_from_uint8(1));
    EXPECT_TRUE(cs.set_matrix_from_uint8(4));
    EXPECT_TRUE(cs.set_matrix_from_uint8(5));
    EXPECT_TRUE(cs.set_matrix_from_uint8(6));
    EXPECT_TRUE(cs.set_matrix_from_uint8(7));
    EXPECT_TRUE(cs.set_matrix_from_uint8(8));
    EXPECT_TRUE(cs.set_matrix_from_uint8(9));
    EXPECT_TRUE(cs.set_matrix_from_uint8(10));
    EXPECT_TRUE(cs.set_matrix_from_uint8(11));
    EXPECT_TRUE(cs.set_matrix_from_uint8(12));
    EXPECT_TRUE(cs.set_matrix_from_uint8(13));
    EXPECT_TRUE(cs.set_matrix_from_uint8(14));
    EXPECT_FALSE(cs.set_matrix_from_uint8(3));
    // RangeID
    EXPECT_TRUE(cs.set_range_from_uint8(1));
    EXPECT_TRUE(cs.set_range_from_uint8(2));
    EXPECT_TRUE(cs.set_range_from_uint8(3));
    EXPECT_FALSE(cs.set_range_from_uint8(4));
}

// ChromaSiting + 6 参数构造
TEST(ColorSpace, ChromaSitingCtor)
{
    cxxkit::ColorSpace cs(cxxkit::ColorSpace::PrimaryID::kBT709,
                          cxxkit::ColorSpace::TransferID::kBT709,
                          cxxkit::ColorSpace::MatrixID::kBT709,
                          cxxkit::ColorSpace::RangeID::kLimited,
                          cxxkit::ColorSpace::ChromaSiting::kCollocated,
                          cxxkit::ColorSpace::ChromaSiting::kHalf,
                          nullptr);
    EXPECT_EQ(cs.chroma_siting_horizontal(), cxxkit::ColorSpace::ChromaSiting::kCollocated);
    EXPECT_EQ(cs.chroma_siting_vertical(), cxxkit::ColorSpace::ChromaSiting::kHalf);
    EXPECT_TRUE(cs.set_chroma_siting_horizontal_from_uint8(0)); // kUnspecified
    EXPECT_TRUE(cs.set_chroma_siting_horizontal_from_uint8(1)); // kCollocated
    EXPECT_TRUE(cs.set_chroma_siting_horizontal_from_uint8(2)); // kHalf
    EXPECT_FALSE(cs.set_chroma_siting_horizontal_from_uint8(3));
    EXPECT_TRUE(cs.set_chroma_siting_vertical_from_uint8(1));
    EXPECT_FALSE(cs.set_chroma_siting_vertical_from_uint8(3));
}

// HdrMetadata validate + 比较
TEST(HdrMetadata, ValidateAndCompare)
{
    cxxkit::HdrMetadata md;
    EXPECT_TRUE(md.validate()); // 默认全 0 合法
    md.max_content_light_level = 1000;
    md.max_frame_average_light_level = 400;
    EXPECT_TRUE(md.validate());
    md.max_content_light_level = 999999; // 超范围
    EXPECT_FALSE(md.validate());
    md.max_content_light_level = 1000;
    EXPECT_TRUE(md.validate());
    // mastering metadata validate
    cxxkit::HdrMasteringMetadata mm;
    EXPECT_TRUE(mm.validate());
    mm.luminance_max = 10000; // 合法上限
    EXPECT_TRUE(mm.validate());
    mm.luminance_min = 10.0f; // 非法（>5）
    EXPECT_FALSE(mm.validate());
    // Chromaticity
    cxxkit::HdrMasteringMetadata::Chromaticity ch;
    EXPECT_TRUE(ch.validate());
    ch.x = 2.0f; // 非法
    EXPECT_FALSE(ch.validate());
    // operator==
    cxxkit::HdrMetadata other;      // 默认构造，全 0
    cxxkit::HdrMetadata md_default; // 对照
    EXPECT_EQ(md_default, other);   // 两个默认构造相等
    EXPECT_FALSE(md == other);      // md 已改 (1000/400) → 不等
}
