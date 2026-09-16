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

#include <system_error>

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
        case UV_EMSGSIZE: return SocketError::kMessageTooLarge; /// UDP: datagram exceeds the max size
        case UV_EADDRINUSE: return SocketError::kAddressInUse;  /// UDP: bind conflict (port already bound)
        default: return SocketError::kUnknown;
    }
}

/// Maps an asio backend status (std::error_code value) onto the SAME public SocketError set
/// (Task 4, D41): the errno-category pairs mirror map_transport_error, so both backends
/// surface identical public errors. EOF is NOT mapped here — asio reports eof via
/// asio::error::eof and the read site normalizes it onto the kBackendEof sentinel exactly
/// like uv does (UV_EOF path). Needs <system_error>; included by the asio backend's TU.
inline SocketError map_asio_error(const std::error_code &ec)
{
#    ifndef _WIN32
    switch (ec.value())
    {
        case ECONNREFUSED: return SocketError::kConnectionRefused;
        case ECONNRESET: return SocketError::kConnectionReset;
        case ETIMEDOUT: return SocketError::kTimedOut;
        case EHOSTUNREACH: return SocketError::kHostUnreachable;
        case ENETUNREACH: return SocketError::kNetworkUnreachable;
        case EADDRNOTAVAIL: return SocketError::kAddrNotAvailable;
        case EPIPE: return SocketError::kBrokenPipe;
        default: return SocketError::kUnknown;
    }
#    else
    (void)ec;
    return SocketError::kUnknown;
#    endif
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
