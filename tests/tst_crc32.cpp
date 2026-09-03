/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
** Copyright 2012 The WebRTC Project Authors. All rights reserved.
**
** License: MIT License
**
** This file contains code ported from the WebRTC project (https://webrtc.org), originally governed
** by a BSD-style license (WebRTC source tree LICENSE file). Modified for CxxKit.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <cxxkit/text/crc32.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

// Tests for crc32 (text/crc32.hpp), ported from WebRTC rtc_base/crc32.h.
namespace
{

using cxxkit::crc32;

TEST(Crc32Test, IeeeCheckValue)
{
    // IEEE 802.3 standard check value for "123456789" (RFC 1952 / zlib).
    const char data[] = "123456789";
    EXPECT_EQ(0xCBF43926u, crc32(data, 9));
}

TEST(Crc32Test, EmptyInput)
{
    const char data[] = "anything";
    EXPECT_EQ(0u, crc32(data, 0));
}

TEST(Crc32Test, IncrementalEqualsOneShot)
{
    const std::string data = "The quick brown fox jumps over the lazy dog";
    const size_t half = data.size() / 2;
    const uint32_t incremental = crc32(data.data() + half, data.size() - half, crc32(data.data(), half));
    EXPECT_EQ(crc32(data.data(), data.size()), incremental);
    // Chained calls must match zlib semantics.
    EXPECT_EQ(0xCBF43926u, crc32("789", 3, crc32("123456", 6)));
}

TEST(Crc32Test, StringViewOverload)
{
    const char data[] = "123456789";
    EXPECT_EQ(crc32(data, 9), crc32(cxxkit::StringView(data)));
    EXPECT_EQ(crc32(std::string("hello world")), crc32("hello world", 11));
    EXPECT_EQ(0u, crc32(cxxkit::StringView()));
}

TEST(Crc32Test, SingleNulByte)
{
    const char data[] = {'\0', 'x'}; // only the first byte is fed
    EXPECT_EQ(0xD202EF8Du, crc32(data, 1));
}

} // namespace
