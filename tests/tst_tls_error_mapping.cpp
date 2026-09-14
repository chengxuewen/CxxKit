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

/// @file tst_tls_error_mapping.cpp
/// @brief mbedTLS → SocketError mapping coverage (D42 T1). map_tls_error / tls_want_retry are
///        inline header functions — these tests exercise them directly; no socket needed.

#include <cxxkit/base/global.hpp>

#include <cxxkit/network/detail/tls_error_mapping.hpp>
#include <cxxkit/network/socket_error.hpp>

#include <cxxkit/3rdparty/mbedtls/ssl.h>
#include <cxxkit/3rdparty/mbedtls/x509.h>

#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

//----------------------------------------------------------------------------------------------------------------------
TEST(tls_error_mapping, peer_close_notify_maps_to_peer_closed)
{
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY),
              cxxkit::SocketError::kTlsPeerClosed);
}

TEST(tls_error_mapping, cert_verify_failed_maps_to_certificate_error)
{
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(MBEDTLS_ERR_X509_CERT_VERIFY_FAILED),
              cxxkit::SocketError::kTlsCertificateError);
}

TEST(tls_error_mapping, x509_cert_block_maps_to_certificate_error)
{
    // Whole MBEDTLS_ERR_X509_CERT_* block (-0x1000..-0x1FFF), not just the verify constant.
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(MBEDTLS_ERR_X509_CERT_UNKNOWN_FORMAT),
              cxxkit::SocketError::kTlsCertificateError);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(-0x2780), cxxkit::SocketError::kTlsCertificateError);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(-0x2500), cxxkit::SocketError::kTlsCertificateError);
}

TEST(tls_error_mapping, fatal_alert_and_handshake_failures_map_to_handshake_failed)
{
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE),
              cxxkit::SocketError::kTlsHandshakeFailed);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(MBEDTLS_ERR_SSL_HANDSHAKE_FAILURE),
              cxxkit::SocketError::kTlsHandshakeFailed);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(MBEDTLS_ERR_SSL_BAD_INPUT_DATA),
              cxxkit::SocketError::kTlsHandshakeFailed);
}

TEST(tls_error_mapping, other_ssl_errors_map_to_protocol_error)
{
    // -0x3000 is MBEDTLS_ERR_X509_FATAL_ERROR (last X509 code, not SSL block).
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(-0x3000), cxxkit::SocketError::kTlsCertificateError);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(-0x7000), cxxkit::SocketError::kTlsProtocolError);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(-0x7F00), cxxkit::SocketError::kTlsProtocolError);
}

TEST(tls_error_mapping, want_read_write_are_retryable_not_errors)
{
    EXPECT_TRUE(cxxkit::network::detail::tls_want_retry(MBEDTLS_ERR_SSL_WANT_READ));
    EXPECT_TRUE(cxxkit::network::detail::tls_want_retry(MBEDTLS_ERR_SSL_WANT_WRITE));
    EXPECT_FALSE(cxxkit::network::detail::tls_want_retry(0));
    EXPECT_FALSE(cxxkit::network::detail::tls_want_retry(MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY));
}

TEST(tls_error_mapping, positive_and_zero_return_is_unknown)
{
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(0), cxxkit::SocketError::kUnknown);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(42), cxxkit::SocketError::kUnknown);
    EXPECT_EQ(cxxkit::network::detail::map_tls_error(-1), cxxkit::SocketError::kUnknown);
}

TEST(tls_error_mapping, to_string_names_match_table)
{
    EXPECT_EQ(cxxkit::to_string(cxxkit::SocketError::kTlsHandshakeFailed), "tls handshake failed");
    EXPECT_EQ(cxxkit::to_string(cxxkit::SocketError::kTlsCertificateError), "tls certificate error");
    EXPECT_EQ(cxxkit::to_string(cxxkit::SocketError::kTlsPeerClosed), "tls peer closed");
    EXPECT_EQ(cxxkit::to_string(cxxkit::SocketError::kTlsProtocolError), "tls protocol error");
}

//----------------------------------------------------------------------------------------------------------------------
#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
