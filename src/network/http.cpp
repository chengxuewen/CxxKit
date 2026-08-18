/***********************************************************************************************************************
**
** Library: cxxkit
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
                                                      initializer.isIncludingSubdomains,
                                                      initializer.path.data(),
                                                      initializer.isHttpsOnly,
                                                      initializer.expires));
#endif
}

Cookie::~Cookie()
{
}

bool Cookie::isIncludingSubdomains() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->IsIncludingSubdomains() : false;
#endif
}

bool Cookie::isHttpsOnly() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->IsHttpsOnly() : false;
#endif
}

std::chrono::system_clock::time_point Cookie::getExpires() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->GetExpires() : std::chrono::system_clock::time_point();
#endif
}

std::string Cookie::getExpiresString() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->GetExpiresString() : "";
#endif
}

std::string Cookie::getDomain() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->GetDomain() : "";
#endif
}

std::string Cookie::getValue() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->GetValue() : "";
#endif
}

std::string Cookie::getPath() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->GetPath() : "";
#endif
}

std::string Cookie::getName() const
{
    CXXKIT_D(const Cookie);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mCookie.has_value() ? d->mCookie->GetName() : "";
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

long Response::statusCode() const
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
        cookies.add(utils::make_shared<Cookie>(Cookie::Initializer{item.GetName(),
                                                                   item.GetValue(),
                                                                   item.GetDomain(),
                                                                   item.IsIncludingSubdomains(),
                                                                   item.GetPath(),
                                                                   item.IsHttpsOnly(),
                                                                   item.GetExpires()}));
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
cpr::AuthMode AuthenticationPrivate::toCpr(Authentication::Mode mode)
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

Authentication::Mode AuthenticationPrivate::fromCpr(cpr::AuthMode mode)
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
        cpr::Authentication{username.data(), password.data(), AuthenticationPrivate::toCpr(auth_mode)});
#endif
}

Authentication::~Authentication() noexcept
{
}

const char *Authentication::authString() const noexcept
{
    CXXKIT_D(const Authentication);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return d->mAuthentication.GetAuthString();
#endif
}

Authentication::Mode Authentication::authMode() const noexcept
{
    CXXKIT_D(const Authentication);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    return AuthenticationPrivate::fromCpr(d->mAuthentication.GetAuthMode());
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

void Session::setUrl(const Url &url)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetUrl(url.data());
#endif
}

void Session::setParameters(const Parameters &parameters)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Parameters cprParameters;
    for (const auto &item : parameters.data())
    {
        cprParameters.Add({item.key, item.value});
    }
    d->mSession.SetParameters(std::move(cprParameters));
#endif
}

void Session::setHeader(const Header &header)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetHeader(cpr::Header{header.begin(), header.end()});
#endif
}

void Session::updateHeader(const Header &header)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.UpdateHeader(cpr::Header{header.begin(), header.end()});
#endif
}

void Session::setTimeout(const Timeout &timeout)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetTimeout({timeout.ms});
#endif
}

void Session::setConnectTimeout(const ConnectTimeout &timeout)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetConnectTimeout(timeout.ms);
#endif
}

void Session::setAuth(const Authentication &auth)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetAuth(auth.dFunc()->mAuthentication);
#endif
}

void Session::setBody(const Body &body)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetBody(body.string());
#endif
}

void Session::setBearer(const Bearer &bearer)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    d->mSession.SetBearer(bearer.string());
#endif
}

void Session::setPayload(const Payload &payload)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Payload cprPayload{};
    for (const auto &item : payload.data())
    {
        cprPayload.Add({item.key, item.value});
    }
    d->mSession.SetPayload(std::move(cprPayload));
#endif
}

void Session::setCookies(const Cookies &cookies)
{
    CXXKIT_D(Session);
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    cpr::Cookies cprCookies{};
    for (const auto &item : cookies.data())
    {
        cprCookies.push_back({item->getName(),
                              item->getValue(),
                              item->getDomain(),
                              item->isIncludingSubdomains(),
                              item->getPath(),
                              item->isHttpsOnly(),
                              item->getExpires()});
    }
    d->mSession.SetCookies(std::move(cprCookies));
#endif
}

Response::SharedPtr Session::get()
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->dFunc()->mResponse = std::move(d->mSession.Get());
#endif
    return response;
}

Response::SharedPtr Session::put()
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->dFunc()->mResponse = std::move(d->mSession.Put());
#endif
    return response;
}

Response::SharedPtr Session::post()
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->dFunc()->mResponse = std::move(d->mSession.Post());
#endif
    return response;
}

Response::SharedPtr Session::download(std::ofstream &file)
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->dFunc()->mResponse = std::move(d->mSession.Download(file));
#endif
    return response;
}

Response::SharedPtr Session::download(const WriteCallback &write)
{
    CXXKIT_D(Session);
    auto response = utils::make_shared<Response>();
#if CXXKIT_FEATURE_USE_BOOST_BACKEND

#else
    response->dFunc()->mResponse = std::move(d->mSession.Download({write.callback, write.userdata}));
#endif
    return response;
}

} // namespace http

CXXKIT_END_NAMESPACE