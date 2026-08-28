/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
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

#include <cxxkit/text/bit_buffer.hpp>
#include <cxxkit/tools/optional.hpp>
#include <cxxkit/numerics/bits.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

using ::testing::ElementsAre;

TEST(BitBufferReaderTest, InDebugModeRequiresToCheckOkStatusBeforeDestruction)
{
    const uint8_t bytes[32] = {};
    Optional<BitBufferReader> reader(utils::in_place, bytes);

    EXPECT_GE(reader->ReadBits(7), 0u);
#if CXXKIT_DCHECK_IS_ON && GTEST_HAS_DEATH_TEST && !defined(CXXKIT_OS_ANDROID)
    EXPECT_DEATH(reader = utils::nullopt, "");
#endif
    EXPECT_TRUE(reader->Ok());
    reader = utils::nullopt;
}

TEST(BitBufferReaderTest, InDebugModeMayCheckRemainingBitsInsteadOfOkStatus)
{
    const uint8_t bytes[32] = {};
    Optional<BitBufferReader> reader(utils::in_place, bytes);

    EXPECT_GE(reader->ReadBit(), 0);
#if CXXKIT_DCHECK_IS_ON && GTEST_HAS_DEATH_TEST && !defined(CXXKIT_OS_ANDROID)
    EXPECT_DEATH(reader = utils::nullopt, "");
#endif
    EXPECT_GE(reader->RemainingBitCount(), 0);
    reader = utils::nullopt;
}

TEST(BitBufferReaderTest, ConsumeBits)
{
    const uint8_t bytes[32] = {};
    BitBufferReader reader(bytes);

    int total_bits = 32 * 8;
    EXPECT_EQ(reader.RemainingBitCount(), total_bits);
    reader.ConsumeBits(3);
    total_bits -= 3;
    EXPECT_EQ(reader.RemainingBitCount(), total_bits);
    reader.ConsumeBits(3);
    total_bits -= 3;
    EXPECT_EQ(reader.RemainingBitCount(), total_bits);
    reader.ConsumeBits(15);
    total_bits -= 15;
    EXPECT_EQ(reader.RemainingBitCount(), total_bits);
    reader.ConsumeBits(67);
    total_bits -= 67;
    EXPECT_EQ(reader.RemainingBitCount(), total_bits);
    EXPECT_TRUE(reader.Ok());

    reader.ConsumeBits(32 * 8);
    EXPECT_FALSE(reader.Ok());
    EXPECT_LT(reader.RemainingBitCount(), 0);
}

TEST(BitBufferReaderTest, ConsumeLotsOfBits)
{
    const uint8_t bytes[1] = {};
    BitBufferReader reader(bytes);

    reader.ConsumeBits(std::numeric_limits<int>::max());
    reader.ConsumeBits(std::numeric_limits<int>::max());
    EXPECT_GE(reader.ReadBit(), 0);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBit)
{
    const uint8_t bytes[] = {0b0100'0001, 0b1011'0001};
    BitBufferReader reader(bytes);
    // First byte.
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 1);
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 0);

    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 1);

    // Second byte.
    EXPECT_EQ(reader.ReadBit(), 1);
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 1);
    EXPECT_EQ(reader.ReadBit(), 1);

    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_EQ(reader.ReadBit(), 1);

    EXPECT_TRUE(reader.Ok());
    // Try to read beyound the buffer.
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBoolConsumesSingleBit)
{
    const uint8_t bytes[] = {0b1010'1010};
    BitBufferReader reader(bytes);
    ASSERT_EQ(reader.RemainingBitCount(), 8);
    EXPECT_TRUE(reader.Read<bool>());
    EXPECT_EQ(reader.RemainingBitCount(), 7);
}

TEST(BitBufferReaderTest, ReadBytesAligned)
{
    const uint8_t bytes[] = {0x0A, //
                             0xBC, //
                             0xDE,
                             0xF1, //
                             0x23,
                             0x45,
                             0x67,
                             0x89};
    BitBufferReader reader(bytes);
    EXPECT_EQ(reader.Read<uint8_t>(), 0x0Au);
    EXPECT_EQ(reader.Read<uint8_t>(), 0xBCu);
    EXPECT_EQ(reader.Read<uint16_t>(), 0xDEF1u);
    EXPECT_EQ(reader.Read<uint32_t>(), 0x23456789u);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBytesOffset4)
{
    const uint8_t bytes[] = {0x0A, 0xBC, 0xDE, 0xF1, 0x23, 0x45, 0x67, 0x89, 0x0A};
    BitBufferReader reader(bytes);
    reader.ConsumeBits(4);

    EXPECT_EQ(reader.Read<uint8_t>(), 0xABu);
    EXPECT_EQ(reader.Read<uint8_t>(), 0xCDu);
    EXPECT_EQ(reader.Read<uint16_t>(), 0xEF12u);
    EXPECT_EQ(reader.Read<uint32_t>(), 0x34567890u);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBytesOffset3)
{
    // The pattern we'll check against is counting down from 0b1111. It looks
    // weird here because it's all offset by 3.
    // Byte pattern is:
    //    56701234
    //  0b00011111,
    //  0b11011011,
    //  0b10010111,
    //  0b01010011,
    //  0b00001110,
    //  0b11001010,
    //  0b10000110,
    //  0b01000010
    //       xxxxx <-- last 5 bits unused.

    // The bytes. It almost looks like counting down by two at a time, except the
    // jump at 5->3->0, since that's when the high bit is turned off.
    const uint8_t bytes[] = {0x1F, 0xDB, 0x97, 0x53, 0x0E, 0xCA, 0x86, 0x42};

    BitBufferReader reader(bytes);
    reader.ConsumeBits(3);
    EXPECT_EQ(reader.Read<uint8_t>(), 0xFEu);
    EXPECT_EQ(reader.Read<uint16_t>(), 0xDCBAu);
    EXPECT_EQ(reader.Read<uint32_t>(), 0x98765432u);
    EXPECT_TRUE(reader.Ok());

    // 5 bits left unread. Not enough to read a uint8_t.
    EXPECT_EQ(reader.RemainingBitCount(), 5);
    EXPECT_EQ(reader.Read<uint8_t>(), 0);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBits)
{
    const uint8_t bytes[] = {0b010'01'101, 0b0011'00'1'0};
    BitBufferReader reader(bytes);
    EXPECT_EQ(reader.ReadBits(3), 0b010u);
    EXPECT_EQ(reader.ReadBits(2), 0b01u);
    EXPECT_EQ(reader.ReadBits(7), 0b101'0011u);
    EXPECT_EQ(reader.ReadBits(2), 0b00u);
    EXPECT_EQ(reader.ReadBits(1), 0b1u);
    EXPECT_EQ(reader.ReadBits(1), 0b0u);
    EXPECT_TRUE(reader.Ok());

    EXPECT_EQ(reader.ReadBits(1), 0u);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadZeroBits)
{
    BitBufferReader reader(ArrayView<const uint8_t>(nullptr, 0));

    EXPECT_EQ(reader.ReadBits(0), 0u);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBitFromEmptyArray)
{
    BitBufferReader reader(ArrayView<const uint8_t>(nullptr, 0));

    // Trying to read from the empty array shouldn't dereference the pointer,
    // i.e. shouldn't crash.
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBitsFromEmptyArray)
{
    BitBufferReader reader(ArrayView<const uint8_t>(nullptr, 0));

    // Trying to read from the empty array shouldn't dereference the pointer,
    // i.e. shouldn't crash.
    EXPECT_EQ(reader.ReadBits(1), 0u);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadBits64)
{
    const uint8_t bytes[] =
        {0x4D, 0x32, 0xAB, 0x54, 0x00, 0xFF, 0xFE, 0x01, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89};
    BitBufferReader reader(bytes);

    EXPECT_EQ(reader.ReadBits(33), 0x4D32AB5400FFFE01u >> (64 - 33));

    constexpr uint64_t kMask31Bits = (1ull << 32) - 1;
    EXPECT_EQ(reader.ReadBits(31), 0x4D32AB5400FFFE01ull & kMask31Bits);

    EXPECT_EQ(reader.ReadBits(64), 0xABCDEF0123456789ull);
    EXPECT_TRUE(reader.Ok());

    // Nothing more to read.
    EXPECT_EQ(reader.ReadBit(), 0);
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferReaderTest, CanPeekBitsUsingCopyConstructor)
{
    // BitBufferReader doesn't have peek function. To simulate it, user may use
    // cheap BitBufferReader copy constructor.
    const uint8_t bytes[] = {0x0A, 0xBC};
    BitBufferReader reader(bytes);
    reader.ConsumeBits(4);
    ASSERT_EQ(reader.RemainingBitCount(), 12);

    BitBufferReader peeker = reader;
    EXPECT_EQ(peeker.ReadBits(8), 0xABu);
    EXPECT_EQ(peeker.RemainingBitCount(), 4);

    EXPECT_EQ(reader.RemainingBitCount(), 12);
    // Can resume reading from before peeker was created.
    EXPECT_EQ(reader.ReadBits(4), 0xAu);
    EXPECT_EQ(reader.RemainingBitCount(), 8);
}

TEST(BitBufferReaderTest, ReadNonSymmetricSameNumberOfBitsWhenNumValuesPowerOf2)
{
    const uint8_t bytes[2] = {0xf3, 0xa0};
    BitBufferReader reader(bytes);

    ASSERT_EQ(reader.RemainingBitCount(), 16);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/1 << 4), 0xfu);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/1 << 4), 0x3u);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/1 << 4), 0xau);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/1 << 4), 0x0u);
    EXPECT_EQ(reader.RemainingBitCount(), 0);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadNonSymmetricOnlyValueConsumesZeroBits)
{
    const uint8_t bytes[2] = {};
    BitBufferReader reader(bytes);

    ASSERT_EQ(reader.RemainingBitCount(), 16);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/1), 0u);
    EXPECT_EQ(reader.RemainingBitCount(), 16);
}

std::array<uint8_t, 8> GolombEncoded(uint32_t val)
{
    int val_width = utils::bit_width(val + 1);
    int total_width = 2 * val_width - 1;
    uint64_t representation = (uint64_t{val} + 1) << (64 - total_width);
    std::array<uint8_t, 8> result;
    for (int i = 0; i < 8; ++i)
    {
        result[i] = representation >> (7 - i) * 8;
    }
    return result;
}

TEST(BitBufferReaderTest, GolombUint32Values)
{
    // Test over the uint32_t range with a large enough step that the test doesn't
    // take forever. Around 20,000 iterations should do.
    const int kStep = std::numeric_limits<uint32_t>::max() / 20000;
    for (uint32_t i = 0; i < std::numeric_limits<uint32_t>::max() - kStep; i += kStep)
    {
        std::array<uint8_t, 8> buffer = GolombEncoded(i);
        BitBufferReader reader(buffer);
        // Use assert instead of EXPECT to avoid spamming thousands of failed
        // expectation when this test fails.
        ASSERT_EQ(reader.ReadExponentialGolomb(), i);
        EXPECT_TRUE(reader.Ok());
    }
}

TEST(BitBufferReaderTest, SignedGolombValues)
{
    uint8_t golomb_bits[][1] = {
        {0b1'0000000},
        {0b010'00000},
        {0b011'00000},
        {0b00100'000},
        {0b00111'000},
    };
    int expected[] = {0, 1, -1, 2, -3};
    for (size_t i = 0; i < sizeof(golomb_bits); ++i)
    {
        BitBufferReader reader(golomb_bits[i]);
        EXPECT_EQ(reader.ReadSignedExponentialGolomb(), expected[i])
            << "Mismatch in expected/decoded value for golomb_bits[" << i
            << "]: " << static_cast<int>(golomb_bits[i][0]);
        EXPECT_TRUE(reader.Ok());
    }
}

TEST(BitBufferReaderTest, NoGolombOverread)
{
    const uint8_t bytes[] = {0x00, 0xFF, 0xFF};
    // Make sure the bit buffer correctly enforces byte length on golomb reads.
    // If it didn't, the above buffer would be valid at 3 bytes.
    BitBufferReader reader1(utils::make_array_view(bytes, 1));
    // When parse fails, `ReadExponentialGolomb` may return any number.
    reader1.ReadExponentialGolomb();
    EXPECT_FALSE(reader1.Ok());

    BitBufferReader reader2(utils::make_array_view(bytes, 2));
    reader2.ReadExponentialGolomb();
    EXPECT_FALSE(reader2.Ok());

    BitBufferReader reader3(bytes);
    // Golomb should have read 9 bits, so 0x01FF, and since it is golomb, the
    // result is 0x01FF - 1 = 0x01FE.
    EXPECT_EQ(reader3.ReadExponentialGolomb(), 0x01FEu);
    EXPECT_TRUE(reader3.Ok());
}

TEST(BitBufferReaderTest, ReadLeb128)
{
    const uint8_t bytes[] = {0xFF, 0x7F};
    BitBufferReader reader(bytes);
    EXPECT_EQ(reader.ReadLeb128(), 0x3FFFu);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferReaderTest, ReadLeb128Large)
{
    const uint8_t max_uint64[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1};
    BitBufferReader max_reader(max_uint64);
    EXPECT_EQ(max_reader.ReadLeb128(), std::numeric_limits<uint64_t>::max());
    EXPECT_TRUE(max_reader.Ok());

    const uint8_t overflow_unit64_t[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2};
    BitBufferReader overflow_reader(overflow_unit64_t);
    EXPECT_EQ(overflow_reader.ReadLeb128(), uint64_t{0});
    EXPECT_FALSE(overflow_reader.Ok());
}

TEST(BitBufferReaderTest, ReadLeb128NoEndByte)
{
    const uint8_t bytes[] = {0xFF, 0xFF};
    BitBufferReader reader(bytes);
    EXPECT_EQ(reader.ReadLeb128(), uint64_t{0});
    EXPECT_FALSE(reader.Ok());
}

TEST(BitBufferWriterTest, ConsumeBits)
{
    uint8_t bytes[64] = {0};
    BitBufferWriter buffer(bytes, 32);
    uint64_t total_bits = 32 * 8;
    EXPECT_EQ(total_bits, buffer.RemainingBitCount());
    EXPECT_TRUE(buffer.ConsumeBits(3));
    total_bits -= 3;
    EXPECT_EQ(total_bits, buffer.RemainingBitCount());
    EXPECT_TRUE(buffer.ConsumeBits(3));
    total_bits -= 3;
    EXPECT_EQ(total_bits, buffer.RemainingBitCount());
    EXPECT_TRUE(buffer.ConsumeBits(15));
    total_bits -= 15;
    EXPECT_EQ(total_bits, buffer.RemainingBitCount());
    EXPECT_TRUE(buffer.ConsumeBits(37));
    total_bits -= 37;
    EXPECT_EQ(total_bits, buffer.RemainingBitCount());

    EXPECT_FALSE(buffer.ConsumeBits(32 * 8));
    EXPECT_EQ(total_bits, buffer.RemainingBitCount());
}

TEST(BitBufferWriterDeathTest, SetOffsetValues)
{
    uint8_t bytes[4] = {0};
    BitBufferWriter buffer(bytes, 4);

    size_t byte_offset, bit_offset;
    // Bit offsets are [0,7].
    EXPECT_TRUE(buffer.Seek(0, 0));
    EXPECT_TRUE(buffer.Seek(0, 7));
    buffer.GetCurrentOffset(&byte_offset, &bit_offset);
    EXPECT_EQ(0u, byte_offset);
    EXPECT_EQ(7u, bit_offset);
    EXPECT_FALSE(buffer.Seek(0, 8));
    buffer.GetCurrentOffset(&byte_offset, &bit_offset);
    EXPECT_EQ(0u, byte_offset);
    EXPECT_EQ(7u, bit_offset);
    // Byte offsets are [0,length]. At byte offset length, the bit offset must be
    // 0.
    EXPECT_TRUE(buffer.Seek(0, 0));
    EXPECT_TRUE(buffer.Seek(2, 4));
    buffer.GetCurrentOffset(&byte_offset, &bit_offset);
    EXPECT_EQ(2u, byte_offset);
    EXPECT_EQ(4u, bit_offset);
    EXPECT_TRUE(buffer.Seek(4, 0));
    EXPECT_FALSE(buffer.Seek(5, 0));
    buffer.GetCurrentOffset(&byte_offset, &bit_offset);
    EXPECT_EQ(4u, byte_offset);
    EXPECT_EQ(0u, bit_offset);
    EXPECT_FALSE(buffer.Seek(4, 1));

// Disable death test on Android because it relies on fork() and doesn't play
// nicely.
#if GTEST_HAS_DEATH_TEST
#    if !defined(WEBRTC_ANDROID)
    // Passing a null out parameter is death.
    EXPECT_DEATH(buffer.GetCurrentOffset(&byte_offset, nullptr), "");
#    endif
#endif
}

TEST(BitBufferWriterTest, WriteNonSymmetricSameNumberOfBitsWhenNumValuesPowerOf2)
{
    uint8_t bytes[2] = {};
    BitBufferWriter writer(bytes, 2);

    ASSERT_EQ(writer.RemainingBitCount(), 16u);
    EXPECT_TRUE(writer.WriteNonSymmetric(0xf, /*num_values=*/1 << 4));
    ASSERT_EQ(writer.RemainingBitCount(), 12u);
    EXPECT_TRUE(writer.WriteNonSymmetric(0x3, /*num_values=*/1 << 4));
    ASSERT_EQ(writer.RemainingBitCount(), 8u);
    EXPECT_TRUE(writer.WriteNonSymmetric(0xa, /*num_values=*/1 << 4));
    ASSERT_EQ(writer.RemainingBitCount(), 4u);
    EXPECT_TRUE(writer.WriteNonSymmetric(0x0, /*num_values=*/1 << 4));
    ASSERT_EQ(writer.RemainingBitCount(), 0u);

    EXPECT_THAT(bytes, ElementsAre(0xf3, 0xa0));
}

TEST(BitBufferWriterTest, NonSymmetricReadsMatchesWrites)
{
    uint8_t bytes[2] = {};
    BitBufferWriter writer(bytes, 2);

    EXPECT_EQ(BitBufferWriter::SizeNonSymmetricBits(/*val=*/1, /*num_values=*/6), 2u);
    EXPECT_EQ(BitBufferWriter::SizeNonSymmetricBits(/*val=*/2, /*num_values=*/6), 3u);
    // Values [0, 1] can fit into two bit.
    ASSERT_EQ(writer.RemainingBitCount(), 16u);
    EXPECT_TRUE(writer.WriteNonSymmetric(/*val=*/0, /*num_values=*/6));
    ASSERT_EQ(writer.RemainingBitCount(), 14u);
    EXPECT_TRUE(writer.WriteNonSymmetric(/*val=*/1, /*num_values=*/6));
    ASSERT_EQ(writer.RemainingBitCount(), 12u);
    // Values [2, 5] require 3 bits.
    EXPECT_TRUE(writer.WriteNonSymmetric(/*val=*/2, /*num_values=*/6));
    ASSERT_EQ(writer.RemainingBitCount(), 9u);
    EXPECT_TRUE(writer.WriteNonSymmetric(/*val=*/3, /*num_values=*/6));
    ASSERT_EQ(writer.RemainingBitCount(), 6u);
    EXPECT_TRUE(writer.WriteNonSymmetric(/*val=*/4, /*num_values=*/6));
    ASSERT_EQ(writer.RemainingBitCount(), 3u);
    EXPECT_TRUE(writer.WriteNonSymmetric(/*val=*/5, /*num_values=*/6));
    ASSERT_EQ(writer.RemainingBitCount(), 0u);

    // Bit values are
    // 00.01.100.101.110.111 = 00011001|01110111 = 0x19|77
    EXPECT_THAT(bytes, ElementsAre(0x19, 0x77));

    BitBufferReader reader(bytes);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/6), 0u);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/6), 1u);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/6), 2u);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/6), 3u);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/6), 4u);
    EXPECT_EQ(reader.ReadNonSymmetric(/*num_values=*/6), 5u);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferWriterTest, WriteNonSymmetricOnlyValueConsumesNoBits)
{
    uint8_t bytes[2] = {};
    BitBufferWriter writer(bytes, 2);
    ASSERT_EQ(writer.RemainingBitCount(), 16u);

    EXPECT_TRUE(writer.WriteNonSymmetric(0, /*num_values=*/1));

    EXPECT_EQ(writer.RemainingBitCount(), 16u);
}

TEST(BitBufferWriterTest, SymmetricReadWrite)
{
    uint8_t bytes[16] = {0};
    BitBufferWriter buffer(bytes, 4);

    // Write some bit data at various sizes.
    EXPECT_TRUE(buffer.WriteBits(0x2u, 3));
    EXPECT_TRUE(buffer.WriteBits(0x1u, 2));
    EXPECT_TRUE(buffer.WriteBits(0x53u, 7));
    EXPECT_TRUE(buffer.WriteBits(0x0u, 2));
    EXPECT_TRUE(buffer.WriteBits(0x1u, 1));
    EXPECT_TRUE(buffer.WriteBits(0x1ABCDu, 17));
    // That should be all that fits in the buffer.
    EXPECT_FALSE(buffer.WriteBits(1, 1));

    BitBufferReader reader(utils::make_array_view(bytes, 4));
    EXPECT_EQ(reader.ReadBits(3), 0x2u);
    EXPECT_EQ(reader.ReadBits(2), 0x1u);
    EXPECT_EQ(reader.ReadBits(7), 0x53u);
    EXPECT_EQ(reader.ReadBits(2), 0x0u);
    EXPECT_EQ(reader.ReadBits(1), 0x1u);
    EXPECT_EQ(reader.ReadBits(17), 0x1ABCDu);
    // And there should be nothing left.
    EXPECT_EQ(reader.RemainingBitCount(), 0);
}

TEST(BitBufferWriterTest, SymmetricBytesMisaligned)
{
    uint8_t bytes[16] = {0};
    BitBufferWriter buffer(bytes, 16);

    // Offset 3, to get things misaligned.
    EXPECT_TRUE(buffer.ConsumeBits(3));
    EXPECT_TRUE(buffer.WriteUInt8(0x12u));
    EXPECT_TRUE(buffer.WriteUInt16(0x3456u));
    EXPECT_TRUE(buffer.WriteUInt32(0x789ABCDEu));

    BitBufferReader reader(bytes);
    reader.ConsumeBits(3);
    EXPECT_EQ(reader.Read<uint8_t>(), 0x12u);
    EXPECT_EQ(reader.Read<uint16_t>(), 0x3456u);
    EXPECT_EQ(reader.Read<uint32_t>(), 0x789ABCDEu);
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferWriterTest, SymmetricGolomb)
{
    char test_string[] = "my precious";
    uint8_t bytes[64] = {0};
    BitBufferWriter buffer(bytes, 64);
    for (size_t i = 0; i < CXXKIT_ARRAY_SIZE(test_string); ++i)
    {
        EXPECT_TRUE(buffer.WriteExponentialGolomb(test_string[i]));
    }
    BitBufferReader reader(bytes);
    for (size_t i = 0; i < CXXKIT_ARRAY_SIZE(test_string); ++i)
    {
        EXPECT_EQ(int64_t{reader.ReadExponentialGolomb()}, int64_t{test_string[i]});
    }
    EXPECT_TRUE(reader.Ok());
}

TEST(BitBufferWriterTest, WriteClearsBits)
{
    uint8_t bytes[] = {0xFF, 0xFF};
    BitBufferWriter buffer(bytes, 2);
    EXPECT_TRUE(buffer.ConsumeBits(3));
    EXPECT_TRUE(buffer.WriteBits(0, 1));
    EXPECT_EQ(0xEFu, bytes[0]);
    EXPECT_TRUE(buffer.WriteBits(0, 3));
    EXPECT_EQ(0xE1u, bytes[0]);
    EXPECT_TRUE(buffer.WriteBits(0, 2));
    EXPECT_EQ(0xE0u, bytes[0]);
    EXPECT_EQ(0x7F, bytes[1]);
}

TEST(BitBufferWriterTest, WriteLeb128)
{
    uint8_t small_number[2];
    BitBufferWriter small_buffer(small_number, sizeof(small_number));
    EXPECT_TRUE(small_buffer.WriteLeb128(129));
    EXPECT_THAT(small_number, ElementsAre(0x81, 0x01));

    uint8_t large_number[10];
    BitBufferWriter large_buffer(large_number, sizeof(large_number));
    EXPECT_TRUE(large_buffer.WriteLeb128(std::numeric_limits<uint64_t>::max()));
    EXPECT_THAT(large_number, ElementsAre(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01));
}

TEST(BitBufferWriterTest, WriteLeb128TooSmallBuffer)
{
    uint8_t bytes[1];
    BitBufferWriter buffer(bytes, sizeof(bytes));
    EXPECT_FALSE(buffer.WriteLeb128(12345));
}

TEST(BitBufferWriterTest, WriteString)
{
    uint8_t buffer[2];
    BitBufferWriter writer(buffer, sizeof(buffer));
    EXPECT_TRUE(writer.WriteString("ab"));
    EXPECT_THAT(buffer, ElementsAre('a', 'b'));
}

TEST(BitBufferWriterTest, WriteStringTooSmallBuffer)
{
    uint8_t buffer[2];
    BitBufferWriter writer(buffer, sizeof(buffer));
    EXPECT_FALSE(writer.WriteString("abc"));
}

CXXKIT_END_NAMESPACE
