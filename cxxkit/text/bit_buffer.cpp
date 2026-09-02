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

#include <cxxkit/text/bit_buffer.hpp>
#include <cxxkit/numerics/bits.hpp>

#include <stdint.h>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
void set_last_read_is_verified(bool &verified, bool value)
{
#ifdef CXXKIT_DCHECK_IS_ON
    verified = value;
#endif
}

// Returns the highest byte of `val` in a uint8_t.
uint8_t highest_byte(uint64_t val)
{
    return static_cast<uint8_t>(val >> 56);
}

// Returns the result of writing partial data from `source`, of
// `source_bit_count` size in the highest bits, to `target` at
// `target_bit_offset` from the highest bit.
uint8_t write_partial_byte(uint8_t source, size_t source_bit_count, uint8_t target, size_t target_bit_offset)
{
    CXXKIT_DCHECK(target_bit_offset < 8);
    CXXKIT_DCHECK(source_bit_count < 9);
    CXXKIT_DCHECK(source_bit_count <= (8 - target_bit_offset));
    // generate a mask for just the bits we're going to overwrite, so:
    uint8_t mask =
        // The number of bits we want, in the most significant bits...
        static_cast<uint8_t>(0xFF << (8 - source_bit_count))
        // ...shifted over to the target offset from the most signficant bit.
        >> target_bit_offset;

    // We want the target, with the bits we'll overwrite masked off, or'ed with
    // the bits from the source we want.
    return (target & ~mask) | (source >> target_bit_offset);
}
} // namespace detail

BitBufferReader::BitBufferReader(ArrayView<const uint8_t> bytes)
    : mBytes(bytes.data())
    , mRemainingBits(utils::checked_cast<int>(bytes.size() * 8))
{
}

BitBufferReader::BitBufferReader(StringView bytes)
    : mBytes(reinterpret_cast<const uint8_t *>(bytes.data()))
    , mRemainingBits(utils::checked_cast<int>(bytes.size() * 8))
{
}

BitBufferReader::~BitBufferReader()
{
    CXXKIT_DCHECK(mLastReadIsVerified) << "Latest calls to read or ConsumeBit "
                                          "were not checked with Ok function.";
}

int BitBufferReader::remaining_bit_count() const
{
    detail::set_last_read_is_verified(mLastReadIsVerified, true);
    return mRemainingBits;
}

uint64_t BitBufferReader::read_bits(int bits)
{
    CXXKIT_DCHECK_GE(bits, 0);
    CXXKIT_DCHECK_LE(bits, 64);
    detail::set_last_read_is_verified(mLastReadIsVerified, false);

    if (mRemainingBits < bits)
    {
        invalidate();
        return 0;
    }

    int remaining_bits_in_first_byte = mRemainingBits % 8;
    mRemainingBits -= bits;
    if (bits < remaining_bits_in_first_byte)
    {
        // Reading fewer bits than what's left in the current byte, just
        // return the portion of this byte that is needed.
        int offset = (remaining_bits_in_first_byte - bits);
        return ((*mBytes) >> offset) & ((1 << bits) - 1);
    }

    uint64_t result = 0;
    if (remaining_bits_in_first_byte > 0)
    {
        // read all bits that were left in the current byte and consume that byte.
        bits -= remaining_bits_in_first_byte;
        uint8_t mask = (1 << remaining_bits_in_first_byte) - 1;
        result = static_cast<uint64_t>(*mBytes & mask) << bits;
        ++mBytes;
    }

    // read as many full bytes as we can.
    while (bits >= 8)
    {
        bits -= 8;
        result |= uint64_t{*mBytes} << bits;
        ++mBytes;
    }
    // Whatever is left to read is smaller than a byte, so grab just the needed
    // bits and shift them into the lowest bits.
    if (bits > 0)
    {
        result |= (*mBytes >> (8 - bits));
    }
    return result;
}

int BitBufferReader::read_bit()
{
    detail::set_last_read_is_verified(mLastReadIsVerified, false);
    if (mRemainingBits <= 0)
    {
        invalidate();
        return 0;
    }
    --mRemainingBits;

    int bit_position = mRemainingBits % 8;
    if (bit_position == 0)
    {
        // read the last bit from current byte and move to the next byte.
        return (*mBytes++) & 0x01;
    }

    return (*mBytes >> bit_position) & 0x01;
}

void BitBufferReader::consume_bits(int bits)
{
    CXXKIT_DCHECK_GE(bits, 0);
    detail::set_last_read_is_verified(mLastReadIsVerified, false);
    if (mRemainingBits < bits)
    {
        invalidate();
        return;
    }

    int remaining_bytes = (mRemainingBits + 7) / 8;
    mRemainingBits -= bits;
    int new_remaining_bytes = (mRemainingBits + 7) / 8;
    mBytes += (remaining_bytes - new_remaining_bytes);
}

uint32_t BitBufferReader::read_non_symmetric(uint32_t num_values)
{
    CXXKIT_DCHECK_GT(num_values, 0);
    CXXKIT_DCHECK_LE(num_values, uint32_t{1} << 31);

    int width = utils::bit_width(num_values);
    uint32_t num_min_bits_values = (uint32_t{1} << width) - num_values;

    uint64_t val = read_bits(width - 1);
    if (val < num_min_bits_values)
    {
        return val;
    }
    return (val << 1) + read_bit() - num_min_bits_values;
}

uint32_t BitBufferReader::read_exponential_golomb()
{
    // Count the number of leading 0.
    int zero_bit_count = 0;
    while (read_bit() == 0)
    {
        if (++zero_bit_count >= 32)
        {
            // Golob value won't fit into 32 bits of the return value. Fail the parse.
            invalidate();
            return 0;
        }
    }

    // The bit count of the value is the number of zeros + 1.
    // However the first '1' was already read above.
    return (uint32_t{1} << zero_bit_count) + utils::dchecked_cast<uint32_t>(read_bits(zero_bit_count)) - 1;
}

int BitBufferReader::read_signed_exponential_golomb()
{
    uint32_t unsigned_val = read_exponential_golomb();
    if ((unsigned_val & 1) == 0)
    {
        return -static_cast<int>(unsigned_val / 2);
    }
    else
    {
        return (unsigned_val + 1) / 2;
    }
}

uint64_t BitBufferReader::read_leb128()
{
    uint64_t decoded = 0;
    size_t i = 0;
    uint8_t byte;
    // A LEB128 value can in theory be arbitrarily large, but for convenience sake
    // consider it invalid if it can't fit in an uint64_t.
    do
    {
        byte = read<uint8_t>();
        decoded += (static_cast<uint64_t>(byte & 0x7f) << static_cast<uint64_t>(7 * i));
        ++i;
    } while (i < 10 && (byte & 0x80));

    // The first 9 bytes represent the first 63 bits. The tenth byte can therefore
    // not be larger than 1 as it would overflow an uint64_t.
    if (i == 10 && byte > 1)
    {
        invalidate();
    }

    return Ok() ? decoded : 0;
}

std::string BitBufferReader::read_string(int num_bytes)
{
    std::string res;
    res.reserve(num_bytes);
    for (int i = 0; i < num_bytes; ++i)
    {
        res += read<uint8_t>();
    }

    return Ok() ? res : std::string();
}

BitBufferWriter::BitBufferWriter(uint8_t *bytes, size_t byte_count)
    : mWritableBytes(bytes)
    , mByteCount(byte_count)
    , mByteOffset()
    , mBitOffset()
{
    CXXKIT_DCHECK(static_cast<uint64_t>(mByteCount) <= std::numeric_limits<uint32_t>::max());
}

uint64_t BitBufferWriter::remaining_bit_count() const
{
    return (static_cast<uint64_t>(mByteCount) - mByteOffset) * 8 - mBitOffset;
}

bool BitBufferWriter::consume_bytes(size_t byte_count)
{
    return consume_bits(byte_count * 8);
}

bool BitBufferWriter::consume_bits(size_t bit_count)
{
    if (bit_count > remaining_bit_count())
    {
        return false;
    }

    mByteOffset += (mBitOffset + bit_count) / 8;
    mBitOffset = (mBitOffset + bit_count) % 8;
    return true;
}

void BitBufferWriter::get_current_offset(size_t *out_byte_offset, size_t *out_bit_offset)
{
    CXXKIT_CHECK(out_byte_offset != nullptr);
    CXXKIT_CHECK(out_bit_offset != nullptr);
    *out_byte_offset = mByteOffset;
    *out_bit_offset = mBitOffset;
}

bool BitBufferWriter::seek(size_t byte_offset, size_t bit_offset)
{
    if (byte_offset > mByteCount || bit_offset > 7 || (byte_offset == mByteCount && bit_offset > 0))
    {
        return false;
    }
    mByteOffset = byte_offset;
    mBitOffset = bit_offset;
    return true;
}

bool BitBufferWriter::write_u_int8(uint8_t val)
{
    return write_bits(val, sizeof(uint8_t) * 8);
}

bool BitBufferWriter::write_u_int16(uint16_t val)
{
    return write_bits(val, sizeof(uint16_t) * 8);
}

bool BitBufferWriter::write_u_int32(uint32_t val)
{
    return write_bits(val, sizeof(uint32_t) * 8);
}

bool BitBufferWriter::write_bits(uint64_t val, size_t bit_count)
{
    if (bit_count > remaining_bit_count())
    {
        return false;
    }
    size_t total_bits = bit_count;

    // For simplicity, push the bits we want to read from val to the highest bits.
    val <<= (sizeof(uint64_t) * 8 - bit_count);

    uint8_t *bytes = mWritableBytes + mByteOffset;

    // The first byte is relatively special; the bit offset to write to may put us
    // in the middle of the byte, and the total bit count to write may require we
    // save the bits at the end of the byte.
    size_t remaining_bits_in_current_byte = 8 - mBitOffset;
    size_t bits_in_first_byte = std::min(bit_count, remaining_bits_in_current_byte);
    *bytes = detail::write_partial_byte(detail::highest_byte(val), bits_in_first_byte, *bytes, mBitOffset);
    if (bit_count <= remaining_bits_in_current_byte)
    {
        // Nothing left to write, so quit early.
        return consume_bits(total_bits);
    }

    // Subtract what we've written from the bit count, shift it off the value, and
    // write the remaining full bytes.
    val <<= bits_in_first_byte;
    bytes++;
    bit_count -= bits_in_first_byte;
    while (bit_count >= 8)
    {
        *bytes++ = detail::highest_byte(val);
        val <<= 8;
        bit_count -= 8;
    }

    // Last byte may also be partial, so write the remaining bits from the top of
    // val.
    if (bit_count > 0)
    {
        *bytes = detail::write_partial_byte(detail::highest_byte(val), bit_count, *bytes, 0);
    }

    // All done! Consume the bits we've written.
    return consume_bits(total_bits);
}

bool BitBufferWriter::write_non_symmetric(uint32_t val, uint32_t num_values)
{
    CXXKIT_DCHECK_LT(val, num_values);
    CXXKIT_DCHECK_LE(num_values, uint32_t{1} << 31);
    if (num_values == 1)
    {
        // When there is only one possible value, it requires zero bits to store it.
        // But write_bits doesn't support writing zero bits.
        return true;
    }
    size_t count_bits = utils::bit_width(num_values);
    uint32_t num_min_bits_values = (uint32_t{1} << count_bits) - num_values;

    return val < num_min_bits_values ? write_bits(val, count_bits - 1)
                                     : write_bits(val + num_min_bits_values, count_bits);
}

size_t BitBufferWriter::size_non_symmetric_bits(uint32_t val, uint32_t num_values)
{
    CXXKIT_DCHECK_LT(val, num_values);
    CXXKIT_DCHECK_LE(num_values, uint32_t{1} << 31);
    size_t count_bits = utils::bit_width(num_values);
    uint32_t num_min_bits_values = (uint32_t{1} << count_bits) - num_values;

    return val < num_min_bits_values ? (count_bits - 1) : count_bits;
}

bool BitBufferWriter::write_exponential_golomb(uint32_t val)
{
    // We don't support reading UINT32_MAX, because it doesn't fit in a uint32_t
    // when encoded, so don't support writing it either.
    if (val == std::numeric_limits<uint32_t>::max())
    {
        return false;
    }
    uint64_t val_to_encode = static_cast<uint64_t>(val) + 1;

    // We need to write bit_width(val+1) 0s and then val+1. Since val (as a
    // uint64_t) has leading zeros, we can just write the total golomb encoded
    // size worth of bits, knowing the value will appear last.
    return write_bits(val_to_encode, utils::bit_width(val_to_encode) * 2 - 1);
}

bool BitBufferWriter::write_signed_exponential_golomb(int32_t val)
{
    if (val == 0)
    {
        return write_exponential_golomb(0);
    }
    else if (val > 0)
    {
        uint32_t signed_val = val;
        return write_exponential_golomb((signed_val * 2) - 1);
    }
    else
    {
        if (val == std::numeric_limits<int32_t>::min())
            return false; // Not supported, would cause overflow.
        uint32_t signed_val = -val;
        return write_exponential_golomb(signed_val * 2);
    }
}

bool BitBufferWriter::write_leb128(uint64_t val)
{
    bool success = true;
    do
    {
        uint8_t byte = static_cast<uint8_t>(val & 0x7f);
        val >>= 7;
        if (val > 0)
        {
            byte |= 0x80;
        }
        success &= write_u_int8(byte);
    } while (val > 0);
    return success;
}

bool BitBufferWriter::write_string(StringView data)
{
    bool success = true;
    for (char c : data)
    {
        success &= write_u_int8(c);
    }
    return success;
}


CXXKIT_END_NAMESPACE