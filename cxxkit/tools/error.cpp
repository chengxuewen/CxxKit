/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
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

#include <cxxkit/tools/error.hpp>
#include <cxxkit/thread/spinlock.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
struct DomainData
{
    std::string type;
    std::string name;
    std::string description;
};
static SpinLock &id_domain_datas_map_spin_lock()
{
    static SpinLock spin_lock;
    return spin_lock;
}
static std::map<ErrorId, DomainData> *id_domain_datas_map()
{
    static std::map<ErrorId, DomainData> map;
    return &map;
}
static bool is_id_registered(ErrorId id)
{
    SpinLock::Locker locker(id_domain_datas_map_spin_lock());
    auto id_domain_datas_map = detail::id_domain_datas_map();
    return id_domain_datas_map->find(id) != id_domain_datas_map->end();
}
static ErrorId fnv1aHash(StringView name)
{
    constexpr uint32_t prime = 0x01000193; // 16777619
    uint32_t hash = 0x811C9DC5;            // 2166136261

    for (size_t i = 0; i < name.size(); ++i)
    {
        hash ^= static_cast<uint8_t>(name[i]);
        hash *= prime;
    }
    return static_cast<ErrorId>(hash);
}
} // namespace detail

ErrorId Error::Domain::Registry::register_domain(StringView type, StringView name, StringView description)
{
    SpinLock::Locker locker(detail::id_domain_datas_map_spin_lock());
    auto id = detail::fnv1aHash(type);
    auto map = detail::id_domain_datas_map();
    int retryCount = 0;
    const int maxRetries = 5;
    while (map->find(id) != map->end() && retryCount < maxRetries)
    {
        std::string modifiedType = std::string(type) + "_" + std::to_string(retryCount);
        id = detail::fnv1aHash(modifiedType);
        ++retryCount;
    }

    if (retryCount >= maxRetries)
    {
        return kInvalidId;
    }

    detail::DomainData domainData{std::string(type), std::string(name), std::string(description)};
    map->insert(std::make_pair(id, domainData));
    return id;
}

Error::Domain::Domain(ErrorId id)
    : mId(detail::is_id_registered(id) ? id : kInvalidId)
{
    SpinLock::Locker locker(detail::id_domain_datas_map_spin_lock());
    auto id_domain_datas_map = detail::id_domain_datas_map();
    const auto iter = id_domain_datas_map->find(mId);
    if (iter != id_domain_datas_map->end())
    {
        mType = iter->second.type;
        mName = iter->second.name;
        mDescription = iter->second.description;
    }
}

Error::Domain::Domain(Domain &&other)
{
    std::swap(mId, other.mId);
    std::swap(mType, other.mType);
    std::swap(mName, other.mName);
    std::swap(mDescription, other.mDescription);
}

Error::Domain::Domain(const Domain &other)
{
    mId = other.mId;
    mType = other.mType;
    mName = other.mName;
    mDescription = other.mDescription;
}

Error::Domain::~Domain()
{
}

Error::Error(const Domain &domain, ErrorId code, StringView message, const SharedDataPtr &cause)
    : mDomain(domain)
    , mCode(code)
    , mMessage(message)
    , mCause(cause)
{
}

Error::Error(StringView message, const SharedDataPtr &cause)
    : mDomain(invalid_domain())
    , mCode(kInvalidId)
    , mMessage(message)
    , mCause(cause)
{
}

Error::Error(const char *message, const SharedDataPtr &cause)
    : mDomain(invalid_domain())
    , mCode(kInvalidId)
    , mMessage(message)
    , mCause(cause)
{
}

Error::Error(const Error &other)
    : mDomain(other.mDomain)
    , mCode(other.mCode)
    , mMessage(other.mMessage)
    , mCause(other.mCause)
{
}

Error::~Error()
{
}

const Error::Domain &invalid_domain()
{
    static const Error::Domain domain;
    return domain;
}

CXXKIT_END_NAMESPACE
