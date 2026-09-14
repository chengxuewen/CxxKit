/***
Library: CxxKit
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

#include <cxxkit/network/socket_error.hpp>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

std::string to_string(SocketError error)
{
    switch (error)
    {
        case SocketError::kNone: return "none";
        case SocketError::kUnknown: return "unknown";
        case SocketError::kConnectionRefused: return "connection refused";
        case SocketError::kConnectionReset: return "connection reset";
        case SocketError::kTimedOut: return "timed out";
        case SocketError::kHostUnreachable: return "host unreachable";
        case SocketError::kNetworkUnreachable: return "network unreachable";
        case SocketError::kAddrNotAvailable: return "address not available";
        case SocketError::kBrokenPipe: return "broken pipe";
        case SocketError::kEof: return "end of file";
        case SocketError::kTlsHandshakeFailed: return "tls handshake failed";
        case SocketError::kTlsCertificateError: return "tls certificate error";
        case SocketError::kTlsPeerClosed: return "tls peer closed";
        case SocketError::kTlsProtocolError: return "tls protocol error";
    }
    return "unknown"; // unreachable for valid enumerators; silences -Wreturn-type on exotic compilers
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
