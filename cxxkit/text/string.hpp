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

#pragma once

#include <cxxkit/text/string_view.hpp>

#include <cstring>

CXXKIT_BEGIN_NAMESPACE

class StringPrivate;
class CXXKIT_CORE_API String
{
public:
    explicit String(StringPrivate *d = nullptr);
    String(const char *str, int len = -1)
        : String()
    {
        this->init(str, len < 0 ? strlen(str) : len);
    }
    String(const std::string &str)
        : String()
    {
        this->init(str.c_str(), str.length());
    }
    String(const StringView &str)
        : String()
    {
        this->init(str.data(), str.length());
    }
    String(const String &other)
        : String()
    {
        this->init(other.c_str(), other.size());
    }
    String(String &&other)
        : String()
    {
        std::swap(mDPtr, other.mDPtr);
    }
    virtual ~String();

    String &operator=(const std::string &str)
    {
        this->destroy();
        this->init(str.c_str(), str.length());
        return *this;
    }
    String &operator=(const String &other)
    {
        this->destroy();
        this->init(other.c_str(), other.size());
        return *this;
    }
    String &operator=(String &&other)
    {
        std::swap(mDPtr, other.mDPtr);
        return *this;
    }

    size_t size() const;
    size_t length() const { return this->size(); }

    bool isDynamic() const;

    const char *c_str() const;
    const char *c_string() const { return this->c_str(); }

    static std::string to_std_string(const String &str) { return str.std_string(); }
    std::string std_string() const { return std::string(this->c_str(), this->size()); }

    /**
     * @brief Duplicates the first @n bytes of a String, returning a newly-allocated buffer @n + 1 bytes long which
     * will always be nul-terminated.
     * If @a str is less than @n bytes long the buffer is padded with nuls.
     * If @a str is %NULL it returns %NULL. The returned value should be freed when no longer needed.
     *
     * @param str   the String to duplicate
     * @param n     the maximum number of bytes to copy from @a str
     * @return a newly-allocated buffer containing the first @n bytes of @a str, nul-terminated
     */
    static char *strndup(const char *str, size_t n);

    /**
     * @brief Duplicates a String. If @a str is %NULL it returns %NULL.
     * The returned String should be freed with free() when no longer needed.
     * @param str The String to duplicate
     * @return A newly-allocated copy of @a str
     */
    static char *strdup(const char *str) { return str ? String::strndup(str, strlen(str) + 1) : nullptr; }

    static int strncpy_s(char *dst, size_t nElements, const char *src, size_t count);

protected:
    void init(const char *str, size_t len);
    void destroy();

private:
    CXXKIT_DEFINE_DPTR(String)
    CXXKIT_DECLARE_PRIVATE(String)
};

static CXXKIT_FORCE_INLINE bool operator<(const String &lhs, const String &rhs) noexcept
{
    return StringView(lhs.c_str()) < StringView(rhs.c_str());
}

static CXXKIT_FORCE_INLINE bool operator>(const String &lhs, const String &rhs) noexcept
{
    return rhs < lhs;
}

static CXXKIT_FORCE_INLINE bool operator<=(const String &lhs, const String &rhs) noexcept
{
    return !(rhs < lhs);
}

static CXXKIT_FORCE_INLINE bool operator>=(const String &lhs, const String &rhs) noexcept
{
    return !(lhs < rhs);
}

CXXKIT_END_NAMESPACE