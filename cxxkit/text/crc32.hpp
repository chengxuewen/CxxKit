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

/// @file crc32.hpp
/// @brief CRC-32 checksum (IEEE 802.3, reversed polynomial 0xEDB88320), zlib-compatible semantics.

#pragma once

#include <cxxkit/base/global.hpp>

#include <cxxkit/text/string_view.hpp>

#include <cstddef>
#include <cstdint>

CXXKIT_BEGIN_NAMESPACE

/// @brief Updates a CRC-32 checksum with @p len bytes from @p data (ported from WebRTC rtc_base/crc32.h).
///
/// zlib-compatible semantics: @p crc holds the checksum result from the previous update; pass 0 for
/// the first call. The init/final XOR is handled internally, so the result is directly comparable to
/// zlib's crc32(): `crc32(data2, len2, crc32(data1, len1)) == crc32(concat)`.
/// @param data Input bytes (may be null when @p len is 0).
/// @param len Number of bytes to process.
/// @param crc Previous checksum result, or 0 for a fresh run.
/// @return The updated checksum.
uint32_t crc32(const void *data, size_t len, uint32_t crc = 0);

/// @brief Computes a CRC-32 checksum over a string view.
/// @param text Input text.
/// @param crc Previous checksum result, or 0 for a fresh run.
/// @return The updated checksum.
uint32_t crc32(StringView text, uint32_t crc = 0);

CXXKIT_END_NAMESPACE
