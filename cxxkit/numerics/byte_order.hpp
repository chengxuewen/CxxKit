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

/** @file
 * @brief Byte-order load/store primitives, ported from WebRTC `rtc_base/byte_order.h`.
 *
 * Clean C++11 implementation: pure shift/mask composition via get8/set8. Host-endian-agnostic,
 * no memcpy, no alignment assumptions, no htons/htonl platform macro swamp.
 */

#pragma once

#include <cxxkit/base/global.hpp>

#include <cstddef>
#include <cstdint>

CXXKIT_BEGIN_NAMESPACE

/** @brief Reads the byte at `offset` from `memory`. Matches WebRTC `Get8`. */
inline uint8_t get8(const void *memory, size_t offset)
{
    return static_cast<const uint8_t *>(memory)[offset];
}

/** @brief Writes `v` into `memory` at `offset`. Matches WebRTC `Set8`. */
inline void set8(void *memory, size_t offset, uint8_t v)
{
    static_cast<uint8_t *>(memory)[offset] = v;
}

/** @brief Reads a big-endian 16-bit integer from the first 2 bytes of `memory`. */
inline uint16_t load_be16(const void *memory)
{
    uint16_t v = static_cast<uint16_t>(get8(memory, 0)) << 8;
    v |= get8(memory, 1);
    return v;
}

/** @brief Reads a big-endian 32-bit integer from the first 4 bytes of `memory`. */
inline uint32_t load_be32(const void *memory)
{
    uint32_t v = static_cast<uint32_t>(get8(memory, 0)) << 24;
    v |= static_cast<uint32_t>(get8(memory, 1)) << 16;
    v |= static_cast<uint32_t>(get8(memory, 2)) << 8;
    v |= get8(memory, 3);
    return v;
}

/** @brief Reads a big-endian 64-bit integer from the first 8 bytes of `memory`. */
inline uint64_t load_be64(const void *memory)
{
    uint64_t v = static_cast<uint64_t>(get8(memory, 0)) << 56;
    v |= static_cast<uint64_t>(get8(memory, 1)) << 48;
    v |= static_cast<uint64_t>(get8(memory, 2)) << 40;
    v |= static_cast<uint64_t>(get8(memory, 3)) << 32;
    v |= static_cast<uint64_t>(get8(memory, 4)) << 24;
    v |= static_cast<uint64_t>(get8(memory, 5)) << 16;
    v |= static_cast<uint64_t>(get8(memory, 6)) << 8;
    v |= get8(memory, 7);
    return v;
}

/** @brief Reads a little-endian 16-bit integer from the first 2 bytes of `memory`. */
inline uint16_t load_le16(const void *memory)
{
    uint16_t v = get8(memory, 0);
    v |= static_cast<uint16_t>(get8(memory, 1)) << 8;
    return v;
}

/** @brief Reads a little-endian 32-bit integer from the first 4 bytes of `memory`. */
inline uint32_t load_le32(const void *memory)
{
    uint32_t v = get8(memory, 0);
    v |= static_cast<uint32_t>(get8(memory, 1)) << 8;
    v |= static_cast<uint32_t>(get8(memory, 2)) << 16;
    v |= static_cast<uint32_t>(get8(memory, 3)) << 24;
    return v;
}

/** @brief Reads a little-endian 64-bit integer from the first 8 bytes of `memory`. */
inline uint64_t load_le64(const void *memory)
{
    uint64_t v = get8(memory, 0);
    v |= static_cast<uint64_t>(get8(memory, 1)) << 8;
    v |= static_cast<uint64_t>(get8(memory, 2)) << 16;
    v |= static_cast<uint64_t>(get8(memory, 3)) << 24;
    v |= static_cast<uint64_t>(get8(memory, 4)) << 32;
    v |= static_cast<uint64_t>(get8(memory, 5)) << 40;
    v |= static_cast<uint64_t>(get8(memory, 6)) << 48;
    v |= static_cast<uint64_t>(get8(memory, 7)) << 56;
    return v;
}

/** @brief Stores `v` big-endian into the first 2 bytes of `memory`. */
inline void store_be16(void *memory, uint16_t v)
{
    set8(memory, 0, static_cast<uint8_t>(v >> 8));
    set8(memory, 1, static_cast<uint8_t>(v));
}

/** @brief Stores `v` big-endian into the first 4 bytes of `memory`. */
inline void store_be32(void *memory, uint32_t v)
{
    set8(memory, 0, static_cast<uint8_t>(v >> 24));
    set8(memory, 1, static_cast<uint8_t>(v >> 16));
    set8(memory, 2, static_cast<uint8_t>(v >> 8));
    set8(memory, 3, static_cast<uint8_t>(v));
}

/** @brief Stores `v` big-endian into the first 8 bytes of `memory`. */
inline void store_be64(void *memory, uint64_t v)
{
    set8(memory, 0, static_cast<uint8_t>(v >> 56));
    set8(memory, 1, static_cast<uint8_t>(v >> 48));
    set8(memory, 2, static_cast<uint8_t>(v >> 40));
    set8(memory, 3, static_cast<uint8_t>(v >> 32));
    set8(memory, 4, static_cast<uint8_t>(v >> 24));
    set8(memory, 5, static_cast<uint8_t>(v >> 16));
    set8(memory, 6, static_cast<uint8_t>(v >> 8));
    set8(memory, 7, static_cast<uint8_t>(v));
}

/** @brief Stores `v` little-endian into the first 2 bytes of `memory`. */
inline void store_le16(void *memory, uint16_t v)
{
    set8(memory, 0, static_cast<uint8_t>(v));
    set8(memory, 1, static_cast<uint8_t>(v >> 8));
}

/** @brief Stores `v` little-endian into the first 4 bytes of `memory`. */
inline void store_le32(void *memory, uint32_t v)
{
    set8(memory, 0, static_cast<uint8_t>(v));
    set8(memory, 1, static_cast<uint8_t>(v >> 8));
    set8(memory, 2, static_cast<uint8_t>(v >> 16));
    set8(memory, 3, static_cast<uint8_t>(v >> 24));
}

/** @brief Stores `v` little-endian into the first 8 bytes of `memory`. */
inline void store_le64(void *memory, uint64_t v)
{
    set8(memory, 0, static_cast<uint8_t>(v));
    set8(memory, 1, static_cast<uint8_t>(v >> 8));
    set8(memory, 2, static_cast<uint8_t>(v >> 16));
    set8(memory, 3, static_cast<uint8_t>(v >> 24));
    set8(memory, 4, static_cast<uint8_t>(v >> 32));
    set8(memory, 5, static_cast<uint8_t>(v >> 40));
    set8(memory, 6, static_cast<uint8_t>(v >> 48));
    set8(memory, 7, static_cast<uint8_t>(v >> 56));
}

CXXKIT_END_NAMESPACE
