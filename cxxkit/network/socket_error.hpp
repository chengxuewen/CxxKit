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
#include <cxxkit/kernel/kernel_global.hpp> // defines the kernel guard this header gates on

#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Backend-neutral socket error codes (Qt alignment — phase-3 backends map onto this set).
 *
 * Produced by @ref TcpSocket::set_on_error and readable via @ref TcpSocket::last_error. The uv
 * mapping lives in network/detail (error_mapping.hpp); other backends map their native codes onto
 * the same enumerators.
 */
enum class SocketError
{
    kNone,                /// no error has occurred (initial last_error)
    kUnknown,             /// unmapped backend error
    kConnectionRefused,   /// peer refused the connection (ECONNREFUSED)
    kConnectionReset,     /// connection reset by peer (ECONNRESET)
    kTimedOut,            /// connection attempt or operation timed out (ETIMEDOUT)
    kHostUnreachable,     /// no route to the host (EHOSTUNREACH)
    kNetworkUnreachable,  /// no route to the network (ENETUNREACH)
    kAddrNotAvailable,    /// local address not available (EADDRNOTAVAIL)
    kBrokenPipe,          /// write on a broken pipe (EPIPE)
    kEof,                 /// end of file: peer closed the stream cleanly (UV_EOF)
    kTlsHandshakeFailed,  /// TLS handshake failed (alert, protocol mismatch, bad input)
    kTlsCertificateError, /// TLS certificate verification/parse failure (X.509)
    kTlsPeerClosed,       /// TLS peer closed the connection cleanly (PEER_CLOSE_NOTIFY)
    kTlsProtocolError,    /// unmapped TLS protocol-level error (other MBEDTLS_ERR_SSL_*)
    kMessageTooLarge,     /// datagram exceeds the path MTU / max message size (EMSGSIZE — UDP send)
    kAddressInUse         /// local address/port already bound by another socket (EADDRINUSE — UDP bind)
};

/** @brief Readable name for a @ref SocketError (full switch, all enumerators). */
std::string to_string(SocketError error);

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
