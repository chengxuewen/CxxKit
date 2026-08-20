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

#pragma once

#include <cxxkit/tools/type_traits.hpp>
#include <cxxkit/thread/reference_counter.hpp>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
struct SharedDataRefCounter;
} // namespace detail

template <typename T, bool Explicitly = false, typename = void>
struct SharedDataPointer;

class SharedData
{
    mutable ReferenceCounter::Value mRefCount{0};
    friend class detail::SharedDataRefCounter;

public:
    inline SharedData() noexcept { }
    // used in SharedDataPointer::clone() must implement
    inline SharedData(const SharedData &) noexcept { }

    inline int refCount() const { return ReferenceCounter::loadAcquire(mRefCount); }

    // using the assignment operator would lead to corruption in the ref-counting
    SharedData &operator=(const SharedData &) = delete;
    ~SharedData() = default;
};

namespace detail
{
struct SharedDataRefCounter final
{
    template <typename T>
    SharedDataRefCounter(T *data) noexcept
        : mRefCount(dynamic_cast<SharedData *>(const_cast<typename std::remove_const<T>::type *>(data))->mRefCount)
    {
        static_assert(traits::is_base_of_v<SharedData, T>, "T must be derived from SharedData");
    }
    ReferenceCounter::Value &mRefCount;
    inline bool ref() noexcept { return ReferenceCounter::ref(mRefCount); }
    inline bool deref() noexcept { return ReferenceCounter::deref(mRefCount); }
    inline int load() const noexcept { return ReferenceCounter::load(mRefCount); }
    inline int loadAcquire() const noexcept { return ReferenceCounter::loadAcquire(mRefCount); }
};
} // namespace detail

/**
 * @brief Implicitly SharedDataPointer
 * @tparam T
 */
template <typename T>
class SharedDataPointer<T, false, traits::enable_if_t<true>>
{
public:
    using DataType = T;
    using Self = SharedDataPointer<T, false>;
    using RefCounter = detail::SharedDataRefCounter;

    inline void detach()
    {
        if (mData && RefCounter{mData}.loadAcquire() != 1)
        {
            this->detachHelper();
        }
    }
    inline T &operator*()
    {
        this->detach();
        return *mData;
    }
    inline const T &operator*() const { return *mData; }
    inline T *operator->()
    {
        this->detach();
        return mData;
    }
    inline const T *operator->() const { return mData; }
    inline operator T *()
    {
        this->detach();
        return mData;
    }
    inline operator const T *() const { return mData; }
    inline T *data()
    {
        this->detach();
        return mData;
    }
    inline const T *data() const { return mData; }
    inline const T *constData() const { return mData; }

    inline bool operator==(const Self &other) const { return mData == other.mData; }
    inline bool operator!=(const Self &other) const { return mData != other.mData; }

    inline SharedDataPointer() noexcept { }
    inline ~SharedDataPointer()
    {
        if (mData && !RefCounter{mData}.deref())
        {
            delete mData;
        }
    }

    explicit SharedDataPointer(T *data) noexcept
        : mData(data)
    {
        if (mData)
        {
            RefCounter{mData}.ref();
        }
    }
    inline SharedDataPointer(const Self &other)
        : mData(other.mData)
    {
        if (mData)
        {
            RefCounter{mData}.ref();
        }
    }
    inline Self &operator=(const Self &other)
    {
        if (other.mData != mData)
        {
            if (other.mData)
            {
                RefCounter{other.mData}.ref();
            }
            T *old = mData;
            mData = other.mData;
            if (old && !RefCounter{old}.deref())
            {
                delete old;
            }
        }
        return *this;
    }
    inline Self &operator=(T *data)
    {
        if (data != mData)
        {
            if (data)
            {
                RefCounter{data}.ref();
            }
            T *old = mData;
            mData = data;
            if (old && !RefCounter{old}.deref())
            {
                delete old;
            }
        }
        return *this;
    }
    SharedDataPointer(Self &&other) noexcept
        : mData(other.mData)
    {
        other.mData = nullptr;
    }
    inline Self &operator=(Self &&other) noexcept
    {
        Self moved(std::move(other));
        this->swap(moved);
        return *this;
    }

    inline bool operator!() const { return !mData; }

    inline void swap(Self &other) noexcept { std::swap(mData, other.mData); }

protected:
    T *clone() { return new T(*mData); }

private:
    void detachHelper()
    {
        T *data = this->clone();
        RefCounter{data}.ref();
        if (!RefCounter{mData}.deref())
        {
            delete mData;
        }
        mData = data;
    }

    T *mData{nullptr};
};
template <typename T>
using ImplicitlySharedDataPointer = SharedDataPointer<T, false>;

template <typename T>
inline bool operator==(std::nullptr_t p1, const ImplicitlySharedDataPointer<T> &p2)
{
    CXXKIT_UNUSED(p1);
    return !p2;
}

template <typename T>
inline bool operator==(const ImplicitlySharedDataPointer<T> &p1, std::nullptr_t p2)
{
    CXXKIT_UNUSED(p2);
    return !p1;
}

/**
 * @brief Explicitly SharedDataPointer
 * @tparam T
 */
template <typename T>
class SharedDataPointer<T, true, traits::enable_if_t<true>>
{
public:
    using DataType = T;
    using Self = SharedDataPointer<T, true>;
    using RefCounter = detail::SharedDataRefCounter;

    inline T &operator*() const { return *mData; }
    inline T *operator->() { return mData; }
    inline T *operator->() const { return mData; }
    inline T *data() const { return mData; }
    inline const T *constData() const { return mData; }

    inline T *take()
    {
        T *data = mData;
        mData = nullptr;
        return data;
    }

    inline void detach()
    {
        if (mData && RefCounter{mData}.loadAcquire() != 1)
        {
            this->detachHelper();
        }
    }

    inline void reset()
    {
        if (mData && !RefCounter{mData}.deref())
        {
            delete mData;
        }
        mData = nullptr;
    }

    inline operator bool() const { return mData != nullptr; }

    inline bool operator==(const Self &other) const { return mData == other.mData; }
    inline bool operator!=(const Self &other) const { return mData != other.mData; }
    inline bool operator==(const T *data) const { return mData == data; }
    inline bool operator!=(const T *data) const { return mData != data; }

    inline SharedDataPointer() noexcept { }
    inline ~SharedDataPointer()
    {
        if (mData && !RefCounter{mData}.deref())
        {
            delete mData;
        }
    }

    explicit SharedDataPointer(T *data) noexcept
        : mData(data)
    {
        if (mData)
        {
            RefCounter(mData).ref();
        }
    }
    inline SharedDataPointer(const Self &other)
        : mData(other.mData)
    {
        if (mData)
        {
            RefCounter{mData}.ref();
        }
    }

    template <typename X>
    inline SharedDataPointer(const SharedDataPointer<X, true> &other, bool useStaticCast = true)
        : mData(useStaticCast ? static_cast<T *>(other.mData) : other.mData)
    {
        if (mData)
        {
            RefCounter{mData}.ref();
        }
    }

    inline Self &operator=(const Self &other)
    {
        if (other.mData != mData)
        {
            if (other.mData)
            {
                RefCounter{other.mData}.ref();
            }
            T *data = mData;
            mData = other.mData;
            if (data && !RefCounter{data}.deref())
            {
                delete data;
            }
        }
        return *this;
    }
    inline Self &operator=(T *data)
    {
        if (data != mData)
        {
            if (data)
            {
                RefCounter{data}.ref();
            }
            T *old = mData;
            mData = data;
            if (old && !RefCounter{old}.deref())
            {
                delete old;
            }
        }
        return *this;
    }
    inline SharedDataPointer(Self &&other) noexcept
        : mData(other.mData)
    {
        other.mData = nullptr;
    }
    inline Self &operator=(Self &&other) noexcept
    {
        Self moved(std::move(other));
        this->swap(moved);
        return *this;
    }

    inline bool operator!() const { return !mData; }

    inline void swap(Self &other) noexcept { std::swap(mData, other.mData); }

protected:
    T *clone() { return new T(*mData); }

private:
    void detachHelper()
    {
        T *data = this->clone();
        RefCounter{data}.ref();
        if (!RefCounter{mData}.deref())
        {
            delete mData;
        }
        mData = data;
    }

    T *mData{nullptr};
};
template <typename T>
using ExplicitlySharedDataPointer = SharedDataPointer<T, true>;

template <typename T>
inline bool operator==(std::nullptr_t p1, const ExplicitlySharedDataPointer<T> &p2)
{
    CXXKIT_UNUSED(p1);
    return !p2;
}

template <typename T>
inline bool operator==(const ExplicitlySharedDataPointer<T> &p1, std::nullptr_t p2)
{
    CXXKIT_UNUSED(p2);
    return !p1;
}

CXXKIT_END_NAMESPACE

