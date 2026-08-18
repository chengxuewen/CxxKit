/***********************************************************************************************************************
**
** Library: cxxkit
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

#include <cxxkit/tools/id_registry.hpp>

#include <set>

CXXKIT_BEGIN_NAMESPACE

class IdRegistryPrivate
{
    CXXKIT_DEFINE_PPTR(IdRegistry)
    CXXKIT_DECLARE_PUBLIC(IdRegistry)
    CXXKIT_DISABLE_COPY_MOVE(IdRegistryPrivate)
public:
    explicit IdRegistryPrivate(IdRegistry *p);
    virtual ~IdRegistryPrivate();

    int64_t mCounter;
    std::set<int64_t> mFreeIds;
    std::set<int64_t> mUsedIds;
};

IdRegistryPrivate::IdRegistryPrivate(IdRegistry *p)
    : mPPtr(p)
{
}

IdRegistryPrivate::~IdRegistryPrivate() { }

IdRegistry::IdRegistry()
    : mDPtr(new IdRegistryPrivate(this))
{
}

IdRegistry::~IdRegistry() { }

int64_t IdRegistry::registeredIdCount() const
{
    CXXKIT_D(const IdRegistry);
    return d->mUsedIds.size();
}
bool IdRegistry::isIdRegistered(int64_t id) const
{
    CXXKIT_D(const IdRegistry);
    return d->mUsedIds.find(id) != d->mUsedIds.cend();
}

int64_t IdRegistry::requestId()
{
    CXXKIT_D(IdRegistry);
    int64_t id = d->mFreeIds.empty() ? ++d->mCounter : *d->mFreeIds.erase(d->mFreeIds.begin());
    while (d->mFreeIds.empty() && d->mUsedIds.find(id) != d->mUsedIds.end())
    {
        id = ++d->mCounter;
    }
    this->registerId(id);
    return id;
}

void IdRegistry::registerId(int64_t id)
{
    CXXKIT_D(IdRegistry);
    if (d->mFreeIds.find(id) != d->mFreeIds.end())
    {
        d->mFreeIds.erase(id);
    }
    if (d->mUsedIds.find(id) == d->mUsedIds.end())
    {
        d->mUsedIds.insert(id);
    }
}

void IdRegistry::unregisterId(int64_t id)
{
    CXXKIT_D(IdRegistry);
    if (d->mFreeIds.find(id) == d->mFreeIds.end())
    {
        d->mFreeIds.insert(id);
    }
    if (d->mUsedIds.find(id) != d->mUsedIds.end())
    {
        d->mUsedIds.erase(id);
    }
}

CXXKIT_END_NAMESPACE
