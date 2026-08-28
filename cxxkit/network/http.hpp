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

#include <cxxkit/network/network_global.hpp>
#include <cxxkit/text/string_view.hpp>
#include <cxxkit/thread/thread_pool.hpp>
#include <cxxkit/memory/memory.hpp>

#include <map>
#include <string>
#include <future>
#include <chrono>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <initializer_list>

CXXKIT_BEGIN_NAMESPACE

namespace http
{

namespace detail
{
template <class T>
class Container
{
public:
    Container() = default;
    Container(const std::initializer_list<T> &containers)
        : mContainers(containers)
    {
    }

    void add(const std::initializer_list<T> &containers)
    {
        std::transform(containers.begin(),
                       containers.end(),
                       std::back_inserter(mContainers),
                       [](const T &element) { return std::move(element); });
    }
    void add(const T &element) { mContainers.push_back(std::move(element)); }
    const std::vector<T> &data() const { return mContainers; }

    bool is_encoded() const { return mEncode; }
    void set_encoded(bool encode) { mEncode = encode; }

    // const std::string GetContent(const CurlHolder &) const;

protected:
    std::vector<T> mContainers;
    bool mEncode{true}; // Enables or disables URL encoding for keys and values when calling GetContent(...).
};

template <uint32_t TypeId>
class StringHolder
{
public:
    using SelfType = StringHolder<TypeId>;

    StringHolder() = default;
    explicit StringHolder(const std::string &str)
        : mString(str)
    {
    }
    explicit StringHolder(std::string &&str)
        : mString(std::move(str))
    {
    }
    explicit StringHolder(StringView str)
        : mString(str)
    {
    }
    StringHolder(const std::initializer_list<std::string> args)
    {
        mString = std::accumulate(args.begin(), args.end(), mString);
    }
    StringHolder(const StringHolder &other) = default;
    StringHolder(StringHolder &&old) noexcept = default;
    virtual ~StringHolder() = default;

    StringHolder &operator=(StringHolder &&old) noexcept = default;

    StringHolder &operator=(const StringHolder &other) = default;

    explicit operator std::string() const { return mString; }

    StringHolder operator+(const char *rhs) const { return SelfType(mString + rhs); }

    StringHolder operator+(const std::string &rhs) const { return SelfType(mString + rhs); }

    StringHolder operator+(const SelfType &rhs) const { return SelfType(mString + rhs.mString); }

    void operator+=(const char *rhs) { mString += rhs; }
    void operator+=(const std::string &rhs) { mString += rhs; }
    void operator+=(const SelfType &rhs) { mString += rhs; }

    bool operator==(const char *rhs) const { return mString == rhs; }
    bool operator==(const std::string &rhs) const { return mString == rhs; }
    bool operator==(const SelfType &rhs) const { return mString == rhs.mString; }

    bool operator!=(const char *rhs) const { return mString.c_str() != rhs; }
    bool operator!=(const std::string &rhs) const { return mString != rhs; }
    bool operator!=(const SelfType &rhs) const { return mString != rhs.mString; }

    const std::string &string() { return mString; }
    const std::string &string() const { return mString; }
    const char *c_str() const { return mString.c_str(); }
    const char *data() const { return mString.data(); }

protected:
    std::string mString{};
};

template <uint32_t TypeId>
std::ostream &operator<<(std::ostream &os, const StringHolder<TypeId> &s)
{
    os << s.string();
    return os;
}

struct CaseInsensitiveCompare
{
    bool operator()(const std::string &a, const std::string &b) const noexcept
    {
        return std::lexicographical_compare(a.begin(),
                                            a.end(),
                                            b.begin(),
                                            b.end(),
                                            [](unsigned char ac, unsigned char bc)
                                            { return std::tolower(ac) < std::tolower(bc); });
    }
};

template <uint32_t TypeId>
class MillisecondsTime
{
public:
    MillisecondsTime(const std::chrono::milliseconds &duration)
        : ms{duration}
    {
    }
    MillisecondsTime(const std::int32_t &milliseconds)
        : MillisecondsTime{std::chrono::milliseconds(milliseconds)}
    {
    }

    std::chrono::milliseconds ms;
};
} // namespace detail

class WriteCallback
{
public:
    WriteCallback() = default;

    WriteCallback(std::function<bool(std::string data, intptr_t userdata)> p_callback, intptr_t p_userdata = 0)
        : userdata(p_userdata)
        , callback(std::move(p_callback))
    {
    }
    bool operator()(std::string data) const { return callback(std::move(data), userdata); }

    intptr_t userdata{};
    std::function<bool(std::string data, intptr_t userdata)> callback;
};

using Url = detail::StringHolder<CXXKIT_FOURCC('u', 'r', 'l', ' ')>;
using Body = detail::StringHolder<CXXKIT_FOURCC('b', 'o', 'd', 'y')>;
using Bearer = detail::StringHolder<CXXKIT_FOURCC('b', 'a', 'e', 'r')>;
using Timeout = detail::MillisecondsTime<CXXKIT_FOURCC('t', 'o', ' ', ' ')>;
using ConnectTimeout = detail::MillisecondsTime<CXXKIT_FOURCC('c', 't', 'o', ' ')>;
using Header = std::map<std::string, std::string, detail::CaseInsensitiveCompare>;

struct Parameter
{
    Parameter(const std::string &p_key, const std::string &p_value)
        : key{p_key}
        , value{p_value}
    {
    }
    Parameter(std::string &&p_key, std::string &&p_value)
        : key{std::move(p_key)}
        , value{std::move(p_value)}
    {
    }

    std::string key;
    std::string value;
};

class Parameters : public detail::Container<Parameter>
{
public:
    Parameters() = default;
    Parameters(const std::initializer_list<Parameter> &parameters)
        : detail::Container<Parameter>(parameters)
    {
    }
};

struct Pair
{
    Pair(const std::string &p_key, const std::string &p_value)
        : key(p_key)
        , value(p_value)
    {
    }
    Pair(std::string &&p_key, std::string &&p_value)
        : key(std::move(p_key))
        , value(std::move(p_value))
    {
    }

    std::string key;
    std::string value;
};

class Payload : public detail::Container<Pair>
{
public:
    template <class It>
    Payload(const It begin, const It end)
    {
        for (It pair = begin; pair != end; ++pair)
        {
            Add(*pair);
        }
    }
    Payload(const std::initializer_list<Pair> &pairs)
        : detail::Container<Pair>(pairs)
    {
    }
};

class CookiePrivate;
class CXXKIT_NETWORK_API Cookie
{
public:
    using SharedPtr = SharedPointer<Cookie>;

    struct Initializer
    {
        Initializer(StringView p_name,
                    StringView p_value,
                    StringView p_domain = "",
                    bool p_isIncludingSubdomains = false,
                    StringView p_path = "/",
                    bool p_isHttpsOnly = false,
                    std::chrono::system_clock::time_point p_expires = std::chrono::system_clock::from_time_t(0))
            : name(p_name)
            , value(p_value)
            , domain(p_domain)
            , is_including_subdomains(p_isIncludingSubdomains)
            , path(p_path)
            , is_https_only(p_isHttpsOnly)
            , expires(p_expires)
        {
        }

        StringView name;
        StringView value;
        StringView domain;
        bool is_including_subdomains;
        StringView path;
        bool is_https_only;
        std::chrono::system_clock::time_point expires;
    };

    explicit Cookie();
    Cookie(const Initializer &initializer);
    virtual ~Cookie();

    bool is_including_subdomains() const;
    bool is_https_only() const;

    std::chrono::system_clock::time_point get_expires() const;
    std::string get_expires_string() const;
    std::string get_domain() const;
    std::string get_value() const;
    std::string get_path() const;
    std::string get_name() const;

protected:
    friend class Session;
    CXXKIT_DEFINE_DPTR(Cookie)
    CXXKIT_DECLARE_PRIVATE(Cookie)
    CXXKIT_DISABLE_COPY_MOVE(Cookie)
};
class Cookies : public detail::Container<Cookie::SharedPtr>
{
public:
    using BaseType = detail::Container<Cookie::SharedPtr>;

    Cookies() = default;
    Cookies(bool p_encode = true)
        : encode{p_encode}
    {
    }
    Cookies(const std::initializer_list<Cookie::Initializer> &initializers, bool p_encode = true)
        : encode{p_encode}
    {
        for (auto &item : initializers)
        {
            mContainers.push_back(utils::make_shared<Cookie>(item));
        }
    }
    Cookies(const Cookie::Initializer &initializer, bool p_encode = true)
        : encode{p_encode}
    {
        mContainers.push_back(utils::make_shared<Cookie>(initializer));
    }

    bool encode{true};
};

class ResponsePrivate;
/**
 * @brief HTTP response: status, headers, body, cookies and elapsed time.
 */
class CXXKIT_NETWORK_API Response
{
public:
    using SharedPtr = SharedPointer<Response>;

    explicit Response();
    virtual ~Response();

    long status_code() const;
    Cookies cookies() const;
    std::string text() const;
    std::string reason() const;
    std::string header(StringView key) const;

protected:
    friend class Session;
    CXXKIT_DEFINE_DPTR(Response)
    CXXKIT_DECLARE_PRIVATE(Response)
    CXXKIT_DISABLE_COPY_MOVE(Response)
};
using AsyncResponse = std::future<Response::SharedPtr>;

class AuthenticationPrivate;
/**
 * @brief HTTP authentication settings (basic/digest/ntlm) applied per request.
 */
class CXXKIT_NETWORK_API Authentication
{
public:
    enum class Mode
    {
        kBASIC,
        kDIGEST,
        kNTLM
    };

    explicit Authentication();
    Authentication(StringView username, StringView password, Mode auth_mode);
    virtual ~Authentication() noexcept;

    const char *auth_string() const noexcept;
    Mode auth_mode() const noexcept;

protected:
    friend class Session;
    CXXKIT_DEFINE_DPTR(Authentication)
    CXXKIT_DECLARE_PRIVATE(Authentication)
    CXXKIT_DISABLE_COPY_MOVE(Authentication)
};


class SessionPrivate;
/**
 * @brief HTTP session: holds connection state (URL, headers, auth, timeout)
 *        and executes requests (sync/async) via the cpr backend.
 */
class CXXKIT_NETWORK_API Session
{
public:
    explicit Session();
    virtual ~Session();

    void set_url(const Url &url);
    void set_parameters(const Parameters &parameters);
    void set_header(const Header &header);
    void update_header(const Header &header);
    void set_timeout(const Timeout &timeout);
    void set_connect_timeout(const ConnectTimeout &timeout);
    void set_auth(const Authentication &auth);
    void set_body(const Body &body);
    void set_bearer(const Bearer &bearer);
    void set_payload(const Payload &payload);
    void set_cookies(const Cookies &cookies);

    void set_option(const Url &url) { this->set_url(url); }
    void set_option(const Parameters &parameters) { this->set_parameters(parameters); }
    void set_option(const Header &header) { this->set_header(header); }
    void set_option(const Timeout &timeout) { this->set_timeout(timeout); }
    void set_option(const ConnectTimeout &timeout) { this->set_connect_timeout(timeout); }
    void set_option(const Authentication &auth) { this->set_auth(auth); }
    void set_option(const Body &body) { this->set_body(body); }
    void set_option(const Bearer &bearer) { this->set_bearer(bearer); }
    void set_option(const Payload &payload) { this->set_payload(payload); }
    void set_option(const Cookies &cookies) { this->set_cookies(cookies); }

    Response::SharedPtr get();
    Response::SharedPtr put();
    Response::SharedPtr post();
    Response::SharedPtr download(std::ofstream &file);
    Response::SharedPtr download(const WriteCallback &write);

protected:
    CXXKIT_DEFINE_DPTR(Session)
    CXXKIT_DECLARE_PRIVATE(Session)
    CXXKIT_DISABLE_COPY_MOVE(Session)
};

namespace detail
{
template <bool processed_header, typename CurrentType>
void set_option_internal(Session &session, CurrentType &&current_option)
{
    session.set_option(std::forward<CurrentType>(current_option));
}

template <>
inline void set_option_internal<true, Header>(Session &session, Header &&current_option)
{
    // Header option was already provided -> Update previous header
    session.update_header(std::forward<Header>(current_option));
}

template <bool processed_header, typename CurrentType, typename... Ts>
void set_option_internal(Session &session, CurrentType &&current_option, Ts &&...ts)
{
    set_option_internal<processed_header, CurrentType>(session, std::forward<CurrentType>(current_option));

    if (std::is_same<CurrentType, Header>::value)
    {
        set_option_internal<true, Ts...>(session, std::forward<Ts>(ts)...);
    }
    else
    {
        set_option_internal<processed_header, Ts...>(session, std::forward<Ts>(ts)...);
    }
}

template <typename... Ts>
void set_option(Session &session, Ts &&...ts)
{
    set_option_internal<false, Ts...>(session, std::forward<Ts>(ts)...);
}

template <class Fn, class... Args>
auto async(Fn &&fn, Args &&...args) -> std::future<decltype(fn(args...))>
{
    return ThreadPool::default_instance()->start(std::forward<Fn>(fn), std::forward<Args>(args)...);
}
} // namespace detail

/**
 * Get methods
 * @tparam Ts
 * @param ts
 * @return
 */
template <typename... Ts>
Response::SharedPtr Get(Ts &&...ts)
{
    Session session;
    detail::set_option(session, std::forward<Ts>(ts)...);
    return session.get();
}

/**
 * Get async methods
 * @tparam Ts
 * @param ts
 * @return
 */
template <typename... Ts>
AsyncResponse async_get(Ts... ts)
{
    return detail::async([](Ts... ts_inner) { return get(std::move(ts_inner)...); }, std::move(ts)...);
}

/**
 * Put methods
 * @tparam Ts
 * @param ts
 * @return
 */
template <typename... Ts>
Response::SharedPtr put(Ts &&...ts)
{
    Session session;
    detail::set_option(session, std::forward<Ts>(ts)...);
    return session.put();
}

/**
 * Put async methods
 * @tparam Ts
 * @param ts
 * @return
 */
template <typename... Ts>
AsyncResponse async_put(Ts... ts)
{
    return detail::async([](Ts... ts_inner) { return put(std::move(ts_inner)...); }, std::move(ts)...);
}

/**
 * Post methods
 * @tparam Ts
 * @param ts
 * @return
 */
template <typename... Ts>
Response::SharedPtr post(Ts &&...ts)
{
    Session session;
    detail::set_option(session, std::forward<Ts>(ts)...);
    return session.post();
}

/**
 * Post async methods
 * @tparam Ts
 * @param ts
 * @return
 */
template <typename... Ts>
AsyncResponse async_post(Ts... ts)
{
    return detail::async([](Ts... ts_inner) { return post(std::move(ts_inner)...); }, std::move(ts)...);
}

/**
 * Download with user callback
 * @tparam Ts
 * @param write
 * @param ts
 * @return
 */
template <typename... Ts>
Response::SharedPtr download(const WriteCallback &write, Ts &&...ts)
{
    Session session;
    detail::set_option(session, std::forward<Ts>(ts)...);
    return session.download(write);
}

/**
 * Download methods
 * @tparam Ts
 * @param file
 * @param ts
 * @return
 */
template <typename... Ts>
Response::SharedPtr download(std::ofstream &file, Ts &&...ts)
{
    Session session;
    detail::set_option(session, std::forward<Ts>(ts)...);
    return session.download(file);
}

/**
 * Download async method
 * @tparam Ts
 * @param local_path
 * @param ts
 * @return
 */
template <typename... Ts>
AsyncResponse async_download(std::string local_path, Ts... ts)
{
    return std::async(
        std::launch::async,
        [](std::string local_path, Ts... ts)
        {
            std::ofstream f(local_path);
            return download(f, std::move(ts)...);
        },
        std::move(local_path),
        std::move(ts)...);
}

} // namespace http

CXXKIT_END_NAMESPACE