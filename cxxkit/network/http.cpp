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

#include <cxxkit/network/detail/http_p.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace http
{
CookiePrivate::CookiePrivate(Cookie *p)
    : mPPtr(p)
{
}

CookiePrivate::~CookiePrivate()
{
}

Cookie::Cookie()
    : mDPtr(new CookiePrivate(this))
{
}

Cookie::Cookie(const Initializer &initializer)
    : mDPtr(new CookiePrivate(this))
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    mDPtr->mCookie = utils::make_optional(cpr::Cookie(initializer.name.data(),
                                                      initializer.value.data(),
                                                      initializer.domain.data(),
                                                      initializer.is_including_subdomains,
                                                      initializer.path.data(),
                                                      initializer.is_https_only,
                                                      initializer.expires));
#endif
}

Cookie::~Cookie()
{
}

bool Cookie::is_including_subdomains() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->is_including_subdomains() : false;
#endif
}

bool Cookie::is_https_only() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->is_https_only() : false;
#endif
}

std::chrono::system_clock::time_point Cookie::get_expires() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->get_expires() : std::chrono::system_clock::time_point();
#endif
}

std::string Cookie::get_expires_string() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->get_expires_string() : "";
#endif
}

std::string Cookie::get_domain() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->get_domain() : "";
#endif
}

std::string Cookie::get_value() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->get_value() : "";
#endif
}

std::string Cookie::get_path() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->get_path() : "";
#endif
}

std::string Cookie::get_name() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->get_name() : "";
#endif
}

ResponsePrivate::ResponsePrivate(Response *p)
    : mPPtr(p)
{
}

ResponsePrivate::~ResponsePrivate()
{
}

Response::Response()
    : mDPtr(new ResponsePrivate(this))
{
}

Response::~Response()
{
}

long Response::status_code() const
{
    CXXKIT_D(const Response);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mResponse.status_code;
#endif
}

Cookies Response::cookies() const
{
    CXXKIT_D(const Response);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    const auto &aprCookies = d->mResponse.cookies;
    Cookies cookies(aprCookies.encode);
    for (auto &item : aprCookies)
    {
        cookies.add(utils::make_shared<Cookie>(Cookie::Initializer{item.get_name(),
                                                                   item.get_value(),
                                                                   item.get_domain(),
                                                                   item.is_including_subdomains(),
                                                                   item.get_path(),
                                                                   item.is_https_only(),
                                                                   item.get_expires()}));
    }
    return cookies;
#endif
}

std::string Response::text() const
{
    CXXKIT_D(const Response);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mResponse.text;
#endif
}

std::string Response::reason() const
{
    CXXKIT_D(const Response);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mResponse.reason;
#endif
}

std::string Response::header(StringView key) const
{
    CXXKIT_D(const Response);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    const auto iter = d->mResponse.header.find(key.data());
    return d->mResponse.header.end() != iter ? iter->second : "";
#endif
}

AuthenticationPrivate::AuthenticationPrivate(Authentication *p)
    : mPPtr(p)
{
}

AuthenticationPrivate::~AuthenticationPrivate()
{
}

#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
cpr::AuthMode AuthenticationPrivate::to_cpr(Authentication::Mode mode)
{
    switch (mode)
    {
        case Authentication::Mode::kBASIC: return cpr::AuthMode::BASIC;
        case Authentication::Mode::kDIGEST: return cpr::AuthMode::DIGEST;
        case Authentication::Mode::kNTLM: return cpr::AuthMode::NTLM;
        default: break;
    }
    return cpr::AuthMode::BASIC;
}

Authentication::Mode AuthenticationPrivate::from_cpr(cpr::AuthMode mode)
{
    switch (mode)
    {
        case cpr::AuthMode::BASIC: return Authentication::Mode::kBASIC;
        case cpr::AuthMode::DIGEST: return Authentication::Mode::kDIGEST;
        case cpr::AuthMode::NTLM: return Authentication::Mode::kNTLM;
        default: break;
    }
    return Authentication::Mode::kBASIC;
}
#endif

Authentication::Authentication()
    : mDPtr(new AuthenticationPrivate(this))
{
}

Authentication::Authentication(StringView username, StringView password, Mode auth_mode)
    : mDPtr(new AuthenticationPrivate(this))
{
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    mDPtr->mAuthentication = std::move(
        cpr::Authentication{username.data(), password.data(), AuthenticationPrivate::to_cpr(auth_mode)});
#endif
}

Authentication::~Authentication() noexcept
{
}

const char *Authentication::auth_string() const noexcept
{
    CXXKIT_D(const Authentication);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mAuthentication.get_auth_string();
#endif
}

Authentication::Mode Authentication::auth_mode() const noexcept
{
    CXXKIT_D(const Authentication);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return AuthenticationPrivate::from_cpr(d->mAuthentication.get_auth_mode());
#endif
    return Mode::kBASIC;
}

SessionPrivate::SessionPrivate(Session *p)
    : mPPtr(p)
{
}

SessionPrivate::~SessionPrivate()
{
}

Session::Session()
    : mDPtr(new SessionPrivate(this))
{
}

Session::~Session()
{
}

void Session::set_url(const Url &url)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_url(url.data());
#endif
}

void Session::set_parameters(const Parameters &parameters)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Parameters cprParameters;
    for (const auto &item : parameters.data())
    {
        cprParameters.add({item.key, item.value});
    }
    d->mSession.set_parameters(std::move(cprParameters));
#endif
}

void Session::set_header(const Header &header)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_header(cpr::Header{header.begin(), header.end()});
#endif
}

void Session::update_header(const Header &header)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.update_header(cpr::Header{header.begin(), header.end()});
#endif
}

void Session::set_timeout(const Timeout &timeout)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_timeout({timeout.ms});
#endif
}

void Session::set_connect_timeout(const ConnectTimeout &timeout)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_connect_timeout(timeout.ms);
#endif
}

void Session::set_auth(const Authentication &auth)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_auth(auth.d_func()->mAuthentication);
#endif
}

void Session::set_body(const Body &body)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_body(body.string());
#endif
}

void Session::set_bearer(const Bearer &bearer)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.set_bearer(bearer.string());
#endif
}

void Session::set_payload(const Payload &payload)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Payload cprPayload{};
    for (const auto &item : payload.data())
    {
        cprPayload.add({item.key, item.value});
    }
    d->mSession.set_payload(std::move(cprPayload));
#endif
}

void Session::set_cookies(const Cookies &cookies)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Cookies cprCookies{};
    for (const auto &item : cookies.data())
    {
        cprCookies.push_back({item->get_name(),
                              item->get_value(),
                              item->get_domain(),
                              item->is_including_subdomains(),
                              item->get_path(),
                              item->is_https_only(),
                              item->get_expires()});
    }
    d->mSession.set_cookies(std::move(cprCookies));
#endif
}

Response::SharedPtr Session::get()
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->d_func()->mResponse = std::move(d->mSession.get());
#endif
    return response;
}

Response::SharedPtr Session::put()
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->d_func()->mResponse = std::move(d->mSession.put());
#endif
    return response;
}

Response::SharedPtr Session::post()
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->d_func()->mResponse = std::move(d->mSession.post());
#endif
    return response;
}

Response::SharedPtr Session::download(std::ofstream &file)
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->d_func()->mResponse = std::move(d->mSession.download(file));
#endif
    return response;
}

Response::SharedPtr Session::download(const WriteCallback &write)
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->d_func()->mResponse = std::move(d->mSession.download({write.callback, write.userdata}));
#endif
    return response;
}

} // namespace http

CXXKIT_END_NAMESPACE