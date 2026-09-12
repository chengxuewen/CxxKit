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

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/kernel/kernel_global.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#ifdef _WIN32
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/// IPv4/IPv6 auto-detecting sockaddr filler (dual-stack; shared by all backends).
/// Returns false when @p ip parses as neither family.
inline bool fill_sockaddr(const std::string &ip, uint16_t port, sockaddr_storage *out)
{
    std::memset(out, 0, sizeof(*out));
    sockaddr_in *v4 = reinterpret_cast<sockaddr_in *>(out);
    if (inet_pton(AF_INET, ip.c_str(), &v4->sin_addr) == 1)
    {
        v4->sin_family = AF_INET;
        v4->sin_port = htons(port);
        return true;
    }
    sockaddr_in6 *v6 = reinterpret_cast<sockaddr_in6 *>(out);
    if (inet_pton(AF_INET6, ip.c_str(), &v6->sin6_addr) == 1)
    {
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(port);
        return true;
    }
    return false;
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
