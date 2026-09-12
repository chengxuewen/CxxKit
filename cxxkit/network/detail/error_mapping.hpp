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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/network/socket_error.hpp>

#include <cxxkit/3rdparty/libuv/uv.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/// Maps a backend transport status onto the public SocketError set (Qt alignment: backends
/// translate). The concrete status space is the uv errno table.
inline SocketError map_transport_error(int status)
{
    switch (status)
    {
        case UV_ECONNREFUSED: return SocketError::kConnectionRefused;
        case UV_ECONNRESET: return SocketError::kConnectionReset;
        case UV_ETIMEDOUT: return SocketError::kTimedOut;
        case UV_EHOSTUNREACH: return SocketError::kHostUnreachable;
        case UV_ENETUNREACH: return SocketError::kNetworkUnreachable;
        case UV_EADDRNOTAVAIL: return SocketError::kAddrNotAvailable;
        case UV_EPIPE: return SocketError::kBrokenPipe;
        default: return SocketError::kUnknown;
    }
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
