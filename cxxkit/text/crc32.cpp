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

CXXKIT_BEGIN_NAMESPACE

namespace
{

// This implementation is based on the sample implementation in RFC 1952 (WebRTC rtc_base/crc32.cc).
// CRC32 polynomial, in reversed form.
const uint32_t kCrc32Polynomial = 0xEDB88320;

/// Returns the lazily generated 256-entry CRC table. Function-local static: initialized on first
/// use, thread-safe since C++11.
const uint32_t *loadCrc32Table()
{
    static uint32_t table[256];
    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j)
        {
            if (c & 1)
            {
                c = kCrc32Polynomial ^ (c >> 1);
            }
            else
            {
                c >>= 1;
            }
        }
        table[i] = c;
    }
    return table;
}

} // namespace

uint32_t crc32(const void *data, size_t len, uint32_t crc)
{
    static const uint32_t *const kTable = loadCrc32Table();

    uint32_t c = crc ^ 0xFFFFFFFF;
    const uint8_t *bytes = static_cast<const uint8_t *>(data);
    for (size_t i = 0; i < len; ++i)
    {
        c = kTable[(c ^ bytes[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

uint32_t crc32(StringView text, uint32_t crc)
{
    return crc32(text.data(), text.size(), crc);
}

CXXKIT_END_NAMESPACE
