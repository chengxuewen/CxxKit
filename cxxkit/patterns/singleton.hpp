/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present chengxuewen.
** Copyright 2016 The WebRTC Project Authors.
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

#include <atomic>
#include <mutex>

CXXKIT_BEGIN_NAMESPACE

template <typename T, bool UseManualLifetime, typename = void>
struct Singleton;

template <typename T>
class Singleton<T, false, traits::enable_if_t<true>>
{
public:
    static constexpr bool UseManualLifetime = false;

    static T &instance()
    {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
    CXXKIT_DISABLE_COPY_MOVE(Singleton)
};
template <typename T>
using AutoSingleton = Singleton<T, false>;

template <typename T>
class Singleton<T, true, traits::enable_if_t<true>>
{
public:
    static constexpr bool UseManualLifetime = true;

    static T &instance()
    {
        std::call_once(mOnceFlag, create);
        CXXKIT_ASSERT(mInstance.load());
        return *mInstance.load();
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

    T *detachScoped()
    {
        CXXKIT_ASSERT(mInstance.load());
        mScoped.release();
        return mInstance.exchange(nullptr);
    }

    void destroy() { delete this->detachScoped(); }

private:
    static void create()
    {
        mScoped.reset(new T);
        mInstance.store(mScoped.get());
    }

    static std::once_flag mOnceFlag;
    static std::atomic<T *> mInstance;
    static std::unique_ptr<T> mScoped;
    CXXKIT_DISABLE_COPY_MOVE(Singleton)
};
template <typename T>
using ManualSingleton = Singleton<T, true>;

template <typename T>
std::once_flag Singleton<T, true>::mOnceFlag;
template <typename T>
std::atomic<T *> Singleton<T, true>::mInstance = nullptr;
template <typename T>
std::unique_ptr<T> Singleton<T, true>::mScoped = nullptr;

CXXKIT_END_NAMESPACE

#define CXXKIT_DECLARE_SINGLETON(CLASS) friend class cxxkit::Singleton<CLASS, UseManualLifetime>;

