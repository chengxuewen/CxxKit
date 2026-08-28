/***********************************************************************************************************************
**
** Library: CxxKit
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

#include <cxxkit/text/text_global.hpp>

#include <cxxkit/numerics/safe_conversions.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/containers/array_view.hpp>
#include <cxxkit/units/data_size.hpp>

CXXKIT_BEGIN_NAMESPACE

// A class to parse sequence of bits. Byte order is assumed big-endian/network.
// This class is optimized for successful parsing and binary size.
// Individual calls to `read` and `consume_bits` never fail. Instead they may
// change the class state into 'failure state'. User of this class should verify
// parsing by checking if class is in that 'failure state' by calling `Ok`.
// That verification can be done once after multiple reads.
class CXXKIT_TEXT_API BitBufferReader
{
public:
    explicit BitBufferReader(ArrayView<const uint8_t> bytes CXXKIT_ATTRIBUTE_LIFETIME_BOUND);
    explicit BitBufferReader(StringView bytes CXXKIT_ATTRIBUTE_LIFETIME_BOUND);
    BitBufferReader(const BitBufferReader &) = default;
    BitBufferReader &operator=(const BitBufferReader &) = default;
    virtual ~BitBufferReader();

    // Return number of unread bits in the buffer, or negative number if there was a reading error.
    int remaining_bit_count() const;

    // Returns `true` iff all calls to `read` and `consume_bits` were successful.
    bool Ok() const { return remaining_bit_count() >= 0; }

    // Sets `BitStream` into the failure state.
    void invalidate() { mRemainingBits = -1; }

    // Moves current read position forward. `bits` must be non-negative.
    void consume_bits(int bits);

    // Reads single bit. Returns 0 or 1.
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT int read_bit();

    // Reads `bits` from the bitstream. `bits` must be in range [0, 64].
    // Returns an unsigned integer in range [0, 2^bits - 1].
    // On failure sets `BitStream` into the failure state and returns 0.
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT uint64_t read_bits(int bits);

    // Reads unsigned integer of fixed width.
    template <typename T,
              typename std::enable_if<std::is_unsigned<T>::value && !std::is_same<T, bool>::value &&
                                      sizeof(T) <= 8>::type * = nullptr>
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT T read()
    {
        return utils::dchecked_cast<T>(read_bits(sizeof(T) * 8));
    }

    // Reads single bit as boolean.
    template <typename T, typename std::enable_if<std::is_same<T, bool>::value>::type * = nullptr>
    CXXKIT_ATTRIBUTE_MUST_USE_RESULT bool read()
    {
        return read_bit() != 0;
    }

    // Reads value in range [0, `num_values` - 1].
    // This encoding is similar to read_bits(val, Ceil(Log2(num_values)),
    // but reduces wastage incurred when encoding non-power of two value ranges
    // Non symmetric values are encoded as:
    // 1) n = bit_width(num_values)
    // 2) k = (1 << n) - num_values
    // Value v in range [0, k - 1] is encoded in (n-1) bits.
    // Value v in range [k, num_values - 1] is encoded as (v+k) in n bits.
    // https://aomediacodec.github.io/av1-spec/#nsn
    uint32_t read_non_symmetric(uint32_t num_values);

    // Reads exponential golomb encoded value.
    // On failure sets `BitStream` into the failure state and returns
    // unspecified value.
    // exponential golomb values are encoded as:
    // 1) x = source val + 1
    // 2) In binary, write [bit_width(x) - 1] 0s, then x
    // To decode, we count the number of leading 0 bits, read that many + 1 bits,
    // and increment the result by 1.
    // Fails the parsing if the value wouldn't fit in a uint32_t.
    uint32_t read_exponential_golomb();

    // Reads signed exponential golomb values at the current offset. Signed
    // exponential golomb values are just the unsigned values mapped to the
    // sequence 0, 1, -1, 2, -2, etc. in order.
    // On failure sets `BitStream` into the failure state and returns
    // unspecified value.
    int read_signed_exponential_golomb();

    // Reads a LEB128 encoded value. The value will be considered invalid if it
    // can't fit into a uint64_t.
    uint64_t read_leb128();

    std::string read_string(int num_bytes);

private:
    // Next byte with at least one unread bit.
    const uint8_t *mBytes;
    // Number of bits remained to read.
    int mRemainingBits;
    // Unused in release mode.
    mutable bool mLastReadIsVerified = true;
};


// A BitBuffer API for write operations. Supports symmetric write APIs to the
// reading APIs of BitstreamReader.
// Sizes/counts specify bits/bytes, for clarity.
// Byte order is assumed big-endian/network.
class CXXKIT_TEXT_API BitBufferWriter
{
public:

    // Constructs a bit buffer for the writable buffer of `bytes`.
    BitBufferWriter(uint8_t *bytes, size_t byte_count);

    BitBufferWriter(const BitBufferWriter &) = delete;
    BitBufferWriter &operator=(const BitBufferWriter &) = delete;

    // Gets the current offset, in bytes/bits, from the start of the buffer. The
    // bit offset is the offset into the current byte, in the range [0,7].
    void get_current_offset(size_t *out_byte_offset, size_t *out_bit_offset);

    // The remaining bits in the byte buffer.
    uint64_t remaining_bit_count() const;

    // Moves current position `byte_count` bytes forward. Returns false if
    // there aren't enough bytes left in the buffer.
    bool consume_bytes(size_t byte_count);
    // Moves current position `bit_count` bits forward. Returns false if
    // there aren't enough bits left in the buffer.
    bool consume_bits(size_t bit_count);

    // Sets the current offset to the provied byte/bit offsets. The bit
    // offset is from the given byte, in the range [0,7].
    bool seek(size_t byte_offset, size_t bit_offset);

    // Writes byte-sized values from the buffer. Returns false if there isn't
    // enough data left for the specified type.
    bool write_u_int8(uint8_t val);
    bool write_u_int16(uint16_t val);
    bool write_u_int32(uint32_t val);

    // Writes bit-sized values to the buffer. Returns false if there isn't enough
    // room left for the specified number of bits.
    bool write_bits(uint64_t val, size_t bit_count);

    // Writes value in range [0, num_values - 1]
    // See read_non_symmetric documentation for the format,
    // Call size_non_symmetric_bits to get number of bits needed to store the value.
    // Returns false if there isn't enough room left for the value.
    bool write_non_symmetric(uint32_t val, uint32_t num_values);
    // Returns number of bits required to store `val` with NonSymmetric encoding.
    static size_t size_non_symmetric_bits(uint32_t val, uint32_t num_values);

    // Writes the exponential golomb encoded version of the supplied value.
    // Returns false if there isn't enough room left for the value.
    bool write_exponential_golomb(uint32_t val);
    // Writes the signed exponential golomb version of the supplied value.
    // Signed exponential golomb values are just the unsigned values mapped to the
    // sequence 0, 1, -1, 2, -2, etc. in order.
    bool write_signed_exponential_golomb(int32_t val);

    // Writes the Leb128 encoded value.
    bool write_leb128(uint64_t val);

    // Writes the string as bytes of data.
    bool write_string(StringView data);

private:
    // The buffer, as a writable array.
    uint8_t *const mWritableBytes;
    // The total size of `mBytes`.
    const size_t mByteCount;
    // The current offset, in bytes, from the start of `mBytes`.
    size_t mByteOffset;
    // The current offset, in bits, into the current byte.
    size_t mBitOffset;
};

CXXKIT_END_NAMESPACE