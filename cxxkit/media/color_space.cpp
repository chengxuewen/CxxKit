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

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

namespace cxxkit {

namespace
{
// Try to convert `enum_value` into the enum class T. `enum_bitmask` is created
// by the funciton below. Returns true if conversion was successful, false
// otherwise.
template <typename T> bool set_from_uint8(uint8_t enum_value, uint64_t enum_bitmask, T *out)
{
    if ((enum_value < 64) && ((enum_bitmask >> enum_value) & 1))
    {
        *out = static_cast<T>(enum_value);
        return true;
    }
    return false;
}

// This function serves as an assert for the constexpr function below. It's on
// purpose not declared as constexpr so that it causes a build problem if enum
// values of 64 or above are used. The bitmask and the code generating it would
// have to be extended if the standard is updated to include enum values >= 64.
int enum_must_be_less_than64() { return -1; }

template <typename T, size_t N> constexpr int make_mask(const int index, const int length, T (&values)[N])
{
    return length > 1 ? (make_mask(index, 1, values) + make_mask(index + 1, length - 1, values))
                      : (static_cast<uint8_t>(values[index]) < 64 ? (uint64_t{1} << static_cast<uint8_t>(values[index]))
                                                                  : enum_must_be_less_than64());
}

// create a bitmask where each bit corresponds to one potential enum value.
// `values` should be an array listing all possible enum values. The bit is set
// to one if the corresponding enum exists. Only works for enums with values
// less than 64.
template <typename T, size_t N> constexpr uint64_t create_enum_bitmask(T (&values)[N]) { return make_mask(0, N, values); }

bool set_chroma_siting_from_uint8(uint8_t enum_value, ColorSpace::ChromaSiting *chroma_siting)
{
    constexpr ColorSpace::ChromaSiting kChromaSitings[] = {ColorSpace::ChromaSiting::kUnspecified,
                                                           ColorSpace::ChromaSiting::kCollocated,
                                                           ColorSpace::ChromaSiting::kHalf};
    constexpr uint64_t enum_bitmask = create_enum_bitmask(kChromaSitings);

    return set_from_uint8(enum_value, enum_bitmask, chroma_siting);
}
} // namespace

ColorSpace::ColorSpace() = default;
ColorSpace::ColorSpace(const ColorSpace &other) = default;
ColorSpace::ColorSpace(ColorSpace &&other) = default;
ColorSpace &ColorSpace::operator=(const ColorSpace &other) = default;

ColorSpace::ColorSpace(PrimaryID primaries, TransferID transfer, MatrixID matrix, RangeID range)
    : ColorSpace(primaries, transfer, matrix, range, ChromaSiting::kUnspecified, ChromaSiting::kUnspecified, nullptr)
{
}

ColorSpace::ColorSpace(PrimaryID primaries,
                       TransferID transfer,
                       MatrixID matrix,
                       RangeID range,
                       ChromaSiting chroma_siting_horz,
                       ChromaSiting chroma_siting_vert,
                       const HdrMetadata *hdr_metadata)
    : mPrimaries(primaries)
    , mTransfer(transfer)
    , mMatrix(matrix)
    , mRange(range)
    , mChromaSitingHorizontal(chroma_siting_horz)
    , mChromaSitingVertical(chroma_siting_vert)
    , mHdrMetadata(hdr_metadata ? utils::make_optional(*hdr_metadata) : utils::nullopt)
{
}

ColorSpace::~ColorSpace() { }

ColorSpace::PrimaryID ColorSpace::primaries() const { return mPrimaries; }

ColorSpace::TransferID ColorSpace::transfer() const { return mTransfer; }

ColorSpace::MatrixID ColorSpace::matrix() const { return mMatrix; }

ColorSpace::RangeID ColorSpace::range() const { return mRange; }

ColorSpace::ChromaSiting ColorSpace::chroma_siting_horizontal() const { return mChromaSitingHorizontal; }

ColorSpace::ChromaSiting ColorSpace::chroma_siting_vertical() const { return mChromaSitingVertical; }

const HdrMetadata *ColorSpace::hdr_metadata() const { return mHdrMetadata ? &*mHdrMetadata : nullptr; }

#define PRINT_ENUM_CASE(TYPE, NAME)                                                                                    \
    case TYPE::NAME: ss << #NAME; break;

std::string ColorSpace::as_string() const
{
    //    char buf[1024];
    //    rtc::SimpleStringBuilder ss(buf);
    std::stringstream ss;
    ss << "{primaries:";
    switch (mPrimaries)
    {
        PRINT_ENUM_CASE(PrimaryID, kBT709)
        PRINT_ENUM_CASE(PrimaryID, kUnspecified)
        PRINT_ENUM_CASE(PrimaryID, kBT470M)
        PRINT_ENUM_CASE(PrimaryID, kBT470BG)
        PRINT_ENUM_CASE(PrimaryID, kSMPTE170M)
        PRINT_ENUM_CASE(PrimaryID, kSMPTE240M)
        PRINT_ENUM_CASE(PrimaryID, kFILM)
        PRINT_ENUM_CASE(PrimaryID, kBT2020)
        PRINT_ENUM_CASE(PrimaryID, kSMPTEST428)
        PRINT_ENUM_CASE(PrimaryID, kSMPTEST431)
        PRINT_ENUM_CASE(PrimaryID, kSMPTEST432)
        PRINT_ENUM_CASE(PrimaryID, kJEDECP22)
    }
    ss << ", transfer:";
    switch (mTransfer)
    {
        PRINT_ENUM_CASE(TransferID, kBT709)
        PRINT_ENUM_CASE(TransferID, kUnspecified)
        PRINT_ENUM_CASE(TransferID, kGAMMA22)
        PRINT_ENUM_CASE(TransferID, kGAMMA28)
        PRINT_ENUM_CASE(TransferID, kSMPTE170M)
        PRINT_ENUM_CASE(TransferID, kSMPTE240M)
        PRINT_ENUM_CASE(TransferID, kLINEAR)
        PRINT_ENUM_CASE(TransferID, kLOG)
        PRINT_ENUM_CASE(TransferID, kLOG_SQRT)
        PRINT_ENUM_CASE(TransferID, kIEC61966_2_4)
        PRINT_ENUM_CASE(TransferID, kBT1361_ECG)
        PRINT_ENUM_CASE(TransferID, kIEC61966_2_1)
        PRINT_ENUM_CASE(TransferID, kBT2020_10)
        PRINT_ENUM_CASE(TransferID, kBT2020_12)
        PRINT_ENUM_CASE(TransferID, kSMPTEST2084)
        PRINT_ENUM_CASE(TransferID, kSMPTEST428)
        PRINT_ENUM_CASE(TransferID, kARIB_STD_B67)
    }
    ss << ", matrix:";
    switch (mMatrix)
    {
        PRINT_ENUM_CASE(MatrixID, kRGB)
        PRINT_ENUM_CASE(MatrixID, kBT709)
        PRINT_ENUM_CASE(MatrixID, kUnspecified)
        PRINT_ENUM_CASE(MatrixID, kFCC)
        PRINT_ENUM_CASE(MatrixID, kBT470BG)
        PRINT_ENUM_CASE(MatrixID, kSMPTE170M)
        PRINT_ENUM_CASE(MatrixID, kSMPTE240M)
        PRINT_ENUM_CASE(MatrixID, kYCOCG)
        PRINT_ENUM_CASE(MatrixID, kBT2020_NCL)
        PRINT_ENUM_CASE(MatrixID, kBT2020_CL)
        PRINT_ENUM_CASE(MatrixID, kSMPTE2085)
        PRINT_ENUM_CASE(MatrixID, kCDNCLS)
        PRINT_ENUM_CASE(MatrixID, kCDCLS)
        PRINT_ENUM_CASE(MatrixID, kBT2100_ICTCP)
    }

    ss << ", range:";
    switch (mRange)
    {
        PRINT_ENUM_CASE(RangeID, kInvalid)
        PRINT_ENUM_CASE(RangeID, kLimited)
        PRINT_ENUM_CASE(RangeID, kFull)
        PRINT_ENUM_CASE(RangeID, kDerived)
    }
    ss << "}";
    return ss.str();
}

#undef PRINT_ENUM_CASE

bool ColorSpace::set_primaries_from_uint8(uint8_t enum_value)
{
    constexpr PrimaryID kPrimaryIds[] = {PrimaryID::kBT709,
                                         PrimaryID::kUnspecified,
                                         PrimaryID::kBT470M,
                                         PrimaryID::kBT470BG,
                                         PrimaryID::kSMPTE170M,
                                         PrimaryID::kSMPTE240M,
                                         PrimaryID::kFILM,
                                         PrimaryID::kBT2020,
                                         PrimaryID::kSMPTEST428,
                                         PrimaryID::kSMPTEST431,
                                         PrimaryID::kSMPTEST432,
                                         PrimaryID::kJEDECP22};
    constexpr uint64_t enum_bitmask = create_enum_bitmask(kPrimaryIds);

    return set_from_uint8(enum_value, enum_bitmask, &mPrimaries);
}

bool ColorSpace::set_transfer_from_uint8(uint8_t enum_value)
{
    constexpr TransferID kTransferIds[] = {TransferID::kBT709,
                                           TransferID::kUnspecified,
                                           TransferID::kGAMMA22,
                                           TransferID::kGAMMA28,
                                           TransferID::kSMPTE170M,
                                           TransferID::kSMPTE240M,
                                           TransferID::kLINEAR,
                                           TransferID::kLOG,
                                           TransferID::kLOG_SQRT,
                                           TransferID::kIEC61966_2_4,
                                           TransferID::kBT1361_ECG,
                                           TransferID::kIEC61966_2_1,
                                           TransferID::kBT2020_10,
                                           TransferID::kBT2020_12,
                                           TransferID::kSMPTEST2084,
                                           TransferID::kSMPTEST428,
                                           TransferID::kARIB_STD_B67};
    constexpr uint64_t enum_bitmask = create_enum_bitmask(kTransferIds);

    return set_from_uint8(enum_value, enum_bitmask, &mTransfer);
}

bool ColorSpace::set_matrix_from_uint8(uint8_t enum_value)
{
    constexpr MatrixID kMatrixIds[] = {MatrixID::kRGB,
                                       MatrixID::kBT709,
                                       MatrixID::kUnspecified,
                                       MatrixID::kFCC,
                                       MatrixID::kBT470BG,
                                       MatrixID::kSMPTE170M,
                                       MatrixID::kSMPTE240M,
                                       MatrixID::kYCOCG,
                                       MatrixID::kBT2020_NCL,
                                       MatrixID::kBT2020_CL,
                                       MatrixID::kSMPTE2085,
                                       MatrixID::kCDNCLS,
                                       MatrixID::kCDCLS,
                                       MatrixID::kBT2100_ICTCP};
    constexpr uint64_t enum_bitmask = create_enum_bitmask(kMatrixIds);

    return set_from_uint8(enum_value, enum_bitmask, &mMatrix);
}

bool ColorSpace::set_range_from_uint8(uint8_t enum_value)
{
    constexpr RangeID kRangeIds[] = {RangeID::kInvalid, RangeID::kLimited, RangeID::kFull, RangeID::kDerived};
    constexpr uint64_t enum_bitmask = create_enum_bitmask(kRangeIds);

    return set_from_uint8(enum_value, enum_bitmask, &mRange);
}

bool ColorSpace::set_chroma_siting_horizontal_from_uint8(uint8_t enum_value)
{
    return set_chroma_siting_from_uint8(enum_value, &mChromaSitingHorizontal);
}

bool ColorSpace::set_chroma_siting_vertical_from_uint8(uint8_t enum_value)
{
    return set_chroma_siting_from_uint8(enum_value, &mChromaSitingVertical);
}

void ColorSpace::set_hdr_metadata(const HdrMetadata *hdr_metadata)
{
    mHdrMetadata = hdr_metadata ? utils::make_optional(*hdr_metadata) : utils::nullopt;
}
}  // namespace cxxkit
