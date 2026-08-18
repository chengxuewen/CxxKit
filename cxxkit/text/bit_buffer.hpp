/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#pragma once

#include <cxxkit/numerics/safe_conversions.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/containers/array_view.hpp>
#include <cxxkit/units/data_size.hpp>

CXXKIT_BEGIN_NAMESPACE

// A class to parse sequence of bits. Byte order is assumed big-endian/network.
// This class is optimized for successful parsing and binary size.
// Individual calls to `Read` and `ConsumeBits` never fail. Instead they may
// change the class state into 'failure state'. User of this class should verify
// parsing by checking if class is in that 'failure state' by calling `Ok`.
// That verification can be done once after multiple reads.
class CXXKIT_CORE_API BitBufferReader
{
public:
    explicit BitBufferReader(ArrayView<const uint8_t> bytes CXXKIT_ATTRIBUTE_LIFETIME_BOUND);
    explicit BitBufferReader(StringView bytes CXXKIT_ATTRIBUTE_LIFETIME_BOUND);
    BitBufferReader(const BitBufferReader &) = default;
    BitBufferReader &operator=(const BitBufferReader &) = default;
    virtual ~BitBufferReader();

    // Return number of unread bits in the buffer, or negative number if there was a reading error.
    int RemainingBitCount() const;

    // Returns `true` iff all calls to `Read` and `ConsumeBits` were successful.
    bool Ok() const { return RemainingBitCount() >= 0; }

    // Sets `BitStream` into the failure state.
    void Invalidate() { remaining_bits_ = -1; }

    // Moves current read position forward. `bits` must be non-negative.
    void ConsumeBits(int bits);

    // Reads single bit. Returns 0 or 1.
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT int ReadBit();

    // Reads `bits` from the bitstream. `bits` must be in range [0, 64].
    // Returns an unsigned integer in range [0, 2^bits - 1].
    // On failure sets `BitStream` into the failure state and returns 0.
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT uint64_t ReadBits(int bits);

    // Reads unsigned integer of fixed width.
    template <typename T,
              typename std::enable_if<std::is_unsigned<T>::value && !std::is_same<T, bool>::value &&
                                      sizeof(T) <= 8>::type * = nullptr>
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT T Read()
    {
        return utils::dchecked_cast<T>(ReadBits(sizeof(T) * 8));
    }

    // Reads single bit as boolean.
    template <typename T, typename std::enable_if<std::is_same<T, bool>::value>::type * = nullptr>
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT bool Read()
    {
        return ReadBit() != 0;
    }

    // Reads value in range [0, `num_values` - 1].
    // This encoding is similar to ReadBits(val, Ceil(Log2(num_values)),
    // but reduces wastage incurred when encoding non-power of two value ranges
    // Non symmetric values are encoded as:
    // 1) n = bit_width(num_values)
    // 2) k = (1 << n) - num_values
    // Value v in range [0, k - 1] is encoded in (n-1) bits.
    // Value v in range [k, num_values - 1] is encoded as (v+k) in n bits.
    // https://aomediacodec.github.io/av1-spec/#nsn
    uint32_t ReadNonSymmetric(uint32_t num_values);

    // Reads exponential golomb encoded value.
    // On failure sets `BitStream` into the failure state and returns
    // unspecified value.
    // Exponential golomb values are encoded as:
    // 1) x = source val + 1
    // 2) In binary, write [bit_width(x) - 1] 0s, then x
    // To decode, we count the number of leading 0 bits, read that many + 1 bits,
    // and increment the result by 1.
    // Fails the parsing if the value wouldn't fit in a uint32_t.
    uint32_t ReadExponentialGolomb();

    // Reads signed exponential golomb values at the current offset. Signed
    // exponential golomb values are just the unsigned values mapped to the
    // sequence 0, 1, -1, 2, -2, etc. in order.
    // On failure sets `BitStream` into the failure state and returns
    // unspecified value.
    int ReadSignedExponentialGolomb();

    // Reads a LEB128 encoded value. The value will be considered invalid if it
    // can't fit into a uint64_t.
    uint64_t ReadLeb128();

    std::string ReadString(int num_bytes);

private:
    // Next byte with at least one unread bit.
    const uint8_t *bytes_;
    // Number of bits remained to read.
    int remaining_bits_;
    // Unused in release mode.
    mutable bool last_read_is_verified_ = true;
};


// A BitBuffer API for write operations. Supports symmetric write APIs to the
// reading APIs of BitstreamReader.
// Sizes/counts specify bits/bytes, for clarity.
// Byte order is assumed big-endian/network.
class CXXKIT_CORE_API BitBufferWriter
{
public:
    static constexpr DataSize kMaxLeb128Length = DataSize::Bytes(10);

    // Constructs a bit buffer for the writable buffer of `bytes`.
    BitBufferWriter(uint8_t *bytes, size_t byte_count);

    BitBufferWriter(const BitBufferWriter &) = delete;
    BitBufferWriter &operator=(const BitBufferWriter &) = delete;

    // Gets the current offset, in bytes/bits, from the start of the buffer. The
    // bit offset is the offset into the current byte, in the range [0,7].
    void GetCurrentOffset(size_t *out_byte_offset, size_t *out_bit_offset);

    // The remaining bits in the byte buffer.
    uint64_t RemainingBitCount() const;

    // Moves current position `byte_count` bytes forward. Returns false if
    // there aren't enough bytes left in the buffer.
    bool ConsumeBytes(size_t byte_count);
    // Moves current position `bit_count` bits forward. Returns false if
    // there aren't enough bits left in the buffer.
    bool ConsumeBits(size_t bit_count);

    // Sets the current offset to the provied byte/bit offsets. The bit
    // offset is from the given byte, in the range [0,7].
    bool Seek(size_t byte_offset, size_t bit_offset);

    // Writes byte-sized values from the buffer. Returns false if there isn't
    // enough data left for the specified type.
    bool WriteUInt8(uint8_t val);
    bool WriteUInt16(uint16_t val);
    bool WriteUInt32(uint32_t val);

    // Writes bit-sized values to the buffer. Returns false if there isn't enough
    // room left for the specified number of bits.
    bool WriteBits(uint64_t val, size_t bit_count);

    // Writes value in range [0, num_values - 1]
    // See ReadNonSymmetric documentation for the format,
    // Call SizeNonSymmetricBits to get number of bits needed to store the value.
    // Returns false if there isn't enough room left for the value.
    bool WriteNonSymmetric(uint32_t val, uint32_t num_values);
    // Returns number of bits required to store `val` with NonSymmetric encoding.
    static size_t SizeNonSymmetricBits(uint32_t val, uint32_t num_values);

    // Writes the exponential golomb encoded version of the supplied value.
    // Returns false if there isn't enough room left for the value.
    bool WriteExponentialGolomb(uint32_t val);
    // Writes the signed exponential golomb version of the supplied value.
    // Signed exponential golomb values are just the unsigned values mapped to the
    // sequence 0, 1, -1, 2, -2, etc. in order.
    bool WriteSignedExponentialGolomb(int32_t val);

    // Writes the Leb128 encoded value.
    bool WriteLeb128(uint64_t val);

    // Writes the string as bytes of data.
    bool WriteString(StringView data);

private:
    // The buffer, as a writable array.
    uint8_t *const writable_bytes_;
    // The total size of `bytes_`.
    const size_t byte_count_;
    // The current offset, in bytes, from the start of `bytes_`.
    size_t byte_offset_;
    // The current offset, in bits, into the current byte.
    size_t bit_offset_;
};

CXXKIT_END_NAMESPACE