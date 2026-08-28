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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/network/http.hpp>
#include <cxxkit/tools/optional.hpp>

#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
#    include <cpr/cpr.h>
#endif

CXXKIT_BEGIN_NAMESPACE

namespace http
{

class ResponsePrivate
{
public:
    explicit ResponsePrivate(Response *p);
    virtual ~ResponsePrivate();

#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Response mResponse;
#endif

protected:
    CXXKIT_DEFINE_PPTR(Response)
    CXXKIT_DECLARE_PUBLIC(Response)
    CXXKIT_DISABLE_COPY_MOVE(ResponsePrivate)
};

class CookiePrivate
{
public:
    explicit CookiePrivate(Cookie *p);
    virtual ~CookiePrivate();

#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    Optional<cpr::Cookie> mCookie;
#endif

protected:
    CXXKIT_DEFINE_PPTR(Cookie)
    CXXKIT_DECLARE_PUBLIC(Cookie)
    CXXKIT_DISABLE_COPY_MOVE(CookiePrivate)
};

class AuthenticationPrivate
{
public:
    explicit AuthenticationPrivate(Authentication *p);
    virtual ~AuthenticationPrivate();

#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    static cpr::AuthMode to_cpr(Authentication::Mode mode);
    static Authentication::Mode from_cpr(cpr::AuthMode mode);
    cpr::Authentication mAuthentication{"", "", cpr::AuthMode::BASIC};
#endif

protected:
    CXXKIT_DEFINE_PPTR(Authentication)
    CXXKIT_DECLARE_PUBLIC(Authentication)
    CXXKIT_DISABLE_COPY_MOVE(AuthenticationPrivate)
};

class SessionPrivate
{
public:
    explicit SessionPrivate(Session *p);
    virtual ~SessionPrivate();

#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Session mSession;
#endif

protected:
    CXXKIT_DEFINE_PPTR(Session)
    CXXKIT_DECLARE_PUBLIC(Session)
    CXXKIT_DISABLE_COPY_MOVE(SessionPrivate)
};

} // namespace http

CXXKIT_END_NAMESPACE
