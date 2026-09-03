/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright 2004 The WebRTC Project Authors. All rights reserved.
**
** License: MIT License
**
** This file contains code ported from the WebRTC project (https://webrtc.org), originally governed
** by a BSD-style license (WebRTC source tree LICENSE file). Modified for CxxKit.
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

#include <cxxkit/numerics/byte_order.hpp>

#include <gtest/gtest.h>

#include <cstdint>

// Tests for the byte-order load/store primitives (numerics/byte_order.hpp),
// ported from WebRTC rtc_base/byte_order.h.
namespace
{

using cxxkit::get8;
using cxxkit::set8;
using cxxkit::load_be16;
using cxxkit::load_be32;
using cxxkit::load_be64;
using cxxkit::load_le16;
using cxxkit::load_le32;
using cxxkit::load_le64;
using cxxkit::store_be16;
using cxxkit::store_be32;
using cxxkit::store_be64;
using cxxkit::store_le16;
using cxxkit::store_le32;
using cxxkit::store_le64;

TEST(ByteOrderTest, LoadBeKnownByteSequence)
{
    // Fixed-value pins: byte at offset 0 is the most significant for BE.
    const uint8_t bytes4[] = {0x12, 0x34, 0x56, 0x78};
    EXPECT_EQ(0x12345678u, load_be32(bytes4));
    EXPECT_EQ(0x1234u, load_be16(bytes4));

    const uint8_t bytes8[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    EXPECT_EQ(0x123456789ABCDEF0ull, load_be64(bytes8));
    EXPECT_EQ(0x12345678u, load_be32(bytes8));
}

TEST(ByteOrderTest, LoadLeKnownByteSequence)
{
    // Fixed-value pins: byte at offset 0 is the least significant for LE.
    const uint8_t bytes4[] = {0x12, 0x34, 0x56, 0x78};
    EXPECT_EQ(0x78563412u, load_le32(bytes4));
    EXPECT_EQ(0x3412u, load_le16(bytes4));

    const uint8_t bytes8[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    EXPECT_EQ(0xF0DEBC9A78563412ull, load_le64(bytes8));
    EXPECT_EQ(0x78563412u, load_le32(bytes8));
}

TEST(ByteOrderTest, StoreByteLayout)
{
    // Store must lay bytes out explicitly (not just round-trip self-consistently).
    uint8_t buf[8] = {0};

    store_be32(buf, 0x12345678u);
    EXPECT_EQ(0x12, buf[0]);
    EXPECT_EQ(0x34, buf[1]);
    EXPECT_EQ(0x56, buf[2]);
    EXPECT_EQ(0x78, buf[3]);

    store_le32(buf, 0x12345678u);
    EXPECT_EQ(0x78, buf[0]);
    EXPECT_EQ(0x56, buf[1]);
    EXPECT_EQ(0x34, buf[2]);
    EXPECT_EQ(0x12, buf[3]);

    store_be16(buf, 0xABCDu);
    EXPECT_EQ(0xAB, buf[0]);
    EXPECT_EQ(0xCD, buf[1]);

    store_le16(buf, 0xABCDu);
    EXPECT_EQ(0xCD, buf[0]);
    EXPECT_EQ(0xAB, buf[1]);

    store_be64(buf, 0x0123456789ABCDEFull);
    EXPECT_EQ(0x01, buf[0]);
    EXPECT_EQ(0x23, buf[1]);
    EXPECT_EQ(0x45, buf[2]);
    EXPECT_EQ(0x67, buf[3]);
    EXPECT_EQ(0x89, buf[4]);
    EXPECT_EQ(0xAB, buf[5]);
    EXPECT_EQ(0xCD, buf[6]);
    EXPECT_EQ(0xEF, buf[7]);

    store_le64(buf, 0x0123456789ABCDEFull);
    EXPECT_EQ(0xEF, buf[0]);
    EXPECT_EQ(0xCD, buf[1]);
    EXPECT_EQ(0xAB, buf[2]);
    EXPECT_EQ(0x89, buf[3]);
    EXPECT_EQ(0x67, buf[4]);
    EXPECT_EQ(0x45, buf[5]);
    EXPECT_EQ(0x23, buf[6]);
    EXPECT_EQ(0x01, buf[7]);
}

TEST(ByteOrderTest, StoreLoadRoundTrip)
{
    char buf[8];

    store_be16(buf, 0x0000u);
    EXPECT_EQ(0x0000u, load_be16(buf));
    store_be16(buf, 0xFFFFu);
    EXPECT_EQ(0xFFFFu, load_be16(buf));
    store_be16(buf, 0x1234u);
    EXPECT_EQ(0x1234u, load_be16(buf));

    store_le16(buf, 0x0000u);
    EXPECT_EQ(0x0000u, load_le16(buf));
    store_le16(buf, 0xFFFFu);
    EXPECT_EQ(0xFFFFu, load_le16(buf));
    store_le16(buf, 0x1234u);
    EXPECT_EQ(0x1234u, load_le16(buf));

    store_be32(buf, 0x00000000u);
    EXPECT_EQ(0x00000000u, load_be32(buf));
    store_be32(buf, 0xFFFFFFFFu);
    EXPECT_EQ(0xFFFFFFFFu, load_be32(buf));
    store_be32(buf, 0xDEADBEEFu);
    EXPECT_EQ(0xDEADBEEFu, load_be32(buf));

    store_le32(buf, 0x00000000u);
    EXPECT_EQ(0x00000000u, load_le32(buf));
    store_le32(buf, 0xFFFFFFFFu);
    EXPECT_EQ(0xFFFFFFFFu, load_le32(buf));
    store_le32(buf, 0xDEADBEEFu);
    EXPECT_EQ(0xDEADBEEFu, load_le32(buf));

    store_be64(buf, 0x0000000000000000ull);
    EXPECT_EQ(0x0000000000000000ull, load_be64(buf));
    store_be64(buf, 0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(0xFFFFFFFFFFFFFFFFull, load_be64(buf));
    store_be64(buf, 0x123456789ABCDEF0ull);
    EXPECT_EQ(0x123456789ABCDEF0ull, load_be64(buf));

    store_le64(buf, 0x0000000000000000ull);
    EXPECT_EQ(0x0000000000000000ull, load_le64(buf));
    store_le64(buf, 0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(0xFFFFFFFFFFFFFFFFull, load_le64(buf));
    store_le64(buf, 0x123456789ABCDEF0ull);
    EXPECT_EQ(0x123456789ABCDEF0ull, load_le64(buf));
}

TEST(ByteOrderTest, Set8Get8)
{
    uint8_t buf[8] = {0};

    set8(buf, 0, 0xA0);
    set8(buf, 3, 0xA3);
    set8(buf, 7, 0xA7);

    EXPECT_EQ(0xA0, get8(buf, 0));
    EXPECT_EQ(0x00, get8(buf, 1));
    EXPECT_EQ(0x00, get8(buf, 2));
    EXPECT_EQ(0xA3, get8(buf, 3));
    EXPECT_EQ(0x00, get8(buf, 4));
    EXPECT_EQ(0x00, get8(buf, 5));
    EXPECT_EQ(0x00, get8(buf, 6));
    EXPECT_EQ(0xA7, get8(buf, 7));

    // Overwrite an existing byte.
    set8(buf, 3, 0x5A);
    EXPECT_EQ(0x5A, get8(buf, 3));
}

TEST(ByteOrderTest, CrossOffsetReadWrite)
{
    char buf[16] = {0};

    // Operate at a non-zero, unaligned offset (per-byte access: no alignment requirement).
    store_be32(buf + 3, 0x11223344u);
    EXPECT_EQ(0x11223344u, load_be32(buf + 3));
    EXPECT_EQ(0x1122u, load_be16(buf + 3));
    EXPECT_EQ(0x3344u, load_be16(buf + 5));
    EXPECT_EQ(0x11, get8(buf, 3));
    EXPECT_EQ(0x44, get8(buf, 6));

    // Neighbouring bytes untouched by the 4-byte store at offset 3.
    EXPECT_EQ(0x00, get8(buf, 2));
    EXPECT_EQ(0x00, get8(buf, 7));

    // Single-byte write does not disturb the surrounding multi-byte value.
    set8(buf + 3, 0, 0xFF);
    EXPECT_EQ(0xFF223344u, load_be32(buf + 3));

    store_le64(buf + 5, 0xA1B2C3D4E5F60718ull);
    EXPECT_EQ(0xA1B2C3D4E5F60718ull, load_le64(buf + 5));
    EXPECT_EQ(0x18, get8(buf, 5));
    EXPECT_EQ(0xA1, get8(buf, 12));
}

} // namespace
