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

#include <cxxkit/3rdparty/mbedtls/ssl.h>
#include <cxxkit/3rdparty/mbedtls/x509.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/**
 * @brief mbedTLS return-code → public @ref SocketError mapping (D42 T1).
 *
 * Callers distinguish retryable (non-blocking) I/O via @ref tls_want_retry first: WANT_READ /
 * WANT_WRITE are control-flow, not errors. Clean peer shutdown arrives as
 * MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY → kTlsPeerClosed.
 */
inline SocketError map_tls_error(int mbedtls_ret)
{
    switch (mbedtls_ret)
    {
        case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY: return SocketError::kTlsPeerClosed;
        case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED: return SocketError::kTlsCertificateError;
        case MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE: return SocketError::kTlsHandshakeFailed;
        case MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE: return SocketError::kTlsHandshakeFailed;
        case MBEDTLS_ERR_SSL_BAD_INPUT_DATA: return SocketError::kTlsHandshakeFailed;
        default: break;
    }
    if (mbedtls_ret <= -0x2000 &&
        mbedtls_ret >= -0x3000) // MBEDTLS_ERR_X509_* block (-0x2000..-0x3000, includes X509_FATAL_ERROR at -0x3000)
    {
        return SocketError::kTlsCertificateError;
    }
    if (mbedtls_ret <= -0x1000) // remaining MBEDTLS_ERR_SSL_* block (-0x1000..-0x7FFF)
    {
        return SocketError::kTlsProtocolError;
    }
    return SocketError::kUnknown;
}

/// WANT_READ / WANT_WRITE are retryable control-flow returns, not failures (non-blocking BIO).
inline bool tls_want_retry(int mbedtls_ret)
{
    return mbedtls_ret == MBEDTLS_ERR_SSL_WANT_READ || mbedtls_ret == MBEDTLS_ERR_SSL_WANT_WRITE;
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
