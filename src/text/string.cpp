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

#include <cxxkit/text/string.hpp>

#include <limits>
#include <cstdlib>

CXXKIT_BEGIN_NAMESPACE

class StringPrivate
{
    CXXKIT_STATIC_CONSTANT_NUMBER(kBufferSize, 48);

public:
    explicit StringPrivate(String *p);
    virtual ~StringPrivate();

    char mBuffer[kBufferSize];
    char *mDynamic{nullptr};
    size_t mLength{0};

protected:
    CXXKIT_DEFINE_PPTR(String)
    CXXKIT_DECLARE_PUBLIC(String)
    CXXKIT_DISABLE_COPY_MOVE(StringPrivate)
};

StringPrivate::StringPrivate(String *p)
    : mPPtr(p)
{
}

StringPrivate::~StringPrivate()
{
}

String::String(StringPrivate *d)
    : mDPtr(d ? d : new StringPrivate(this))
{
}

String::~String()
{
    this->destroy();
}

size_t String::size() const
{
    CXXKIT_D(const String);
    return d->mLength;
}

bool String::isDynamic() const
{
    CXXKIT_D(const String);
    return d->mDynamic != nullptr;
}

const char *String::c_str() const
{
    CXXKIT_D(const String);
    return nullptr != d->mDynamic ? d->mDynamic : d->mBuffer;
}

void String::init(const char *str, size_t len)
{
    CXXKIT_D(String);
    this->destroy();
    d->mLength = len;
    if (len < StringPrivate::kBufferSize)
    {
        this->strncpy_s(d->mBuffer, StringPrivate::kBufferSize, str, len);
        d->mDynamic = 0;
    }
    else
    {
        d->mDynamic = new char[len + 1];
        this->strncpy_s(d->mDynamic, len + 1, str, len);
    }
}

void String::destroy()
{
    CXXKIT_D(String);
    if (nullptr != d->mDynamic)
    {
        delete[] d->mDynamic;
        d->mDynamic = nullptr;
    }
}

char *String::strndup(const char *str, size_t n)
{
    char *newStr = nullptr;
    if (str)
    {
        size_t end = std::min(strlen(str), n);
        newStr = (char *)std::malloc(n + 1);
        memset(newStr, 0, n + 1);
        memcpy(newStr, str, end);
    }
    return newStr;
}

#ifndef _TRUNCATE
#    define _TRUNCATE ((size_t)-1)
#endif // _TRUNCATE
int String::strncpy_s(char *dst, size_t nElements, const char *src, size_t count)
{
#ifdef _MSC_VER
    return strncpy_s(dst, nElements, src, count);
#else
    if (!count)
    {
        return 0;
    }
    if (!dst || !src || !nElements)
    {
        return -1;
    }
    size_t end = count != _TRUNCATE && count < nElements ? count : nElements - 1;
    size_t i = 0;
    for (; i < end && src[i]; ++i)
    {
        dst[i] = src[i];
    }
    if (!src[i] || end == count || count == _TRUNCATE)
    {
        dst[i] = '\0';
        return 0;
    }
    dst[0] = '\0';
    return -1;
#endif
}

CXXKIT_END_NAMESPACE
