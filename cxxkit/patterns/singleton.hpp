/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present chengxuewen.
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

/** @brief Auto-lifetime singleton: instance() lazily constructs via function-static.
 * @tparam T Concrete singleton type (must derive from this base).
 * @see ManualSingleton, CXXKIT_DECLARE_SINGLETON
 */
template <typename T>
class Singleton<T, false, traits::enable_if_t<true>>
{
public:
    static constexpr bool UseManualLifetime = false;

/** @brief Return reference to the static singleton instance.
 * @return Singleton instance, lazily constructed (thread-safe in C++11).
 */
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
/** @brief Auto-lifetime singleton alias.
 * @tparam T Concrete singleton type.
 * @see Singleton, ManualSingleton
 */
template <typename T>
using AutoSingleton = Singleton<T, false>;

/** @brief Manual-lifetime singleton: instance() uses call_once; use destroy() at process exit.
 * @tparam T Concrete singleton type.
 * @see AutoSingleton, CXXKIT_DECLARE_SINGLETON
 */
template <typename T>
class Singleton<T, true, traits::enable_if_t<true>>
{
public:
    static constexpr bool UseManualLifetime = true;

/** @brief Return reference to the manually-managed singleton instance.
 * @return Singleton instance (created via std::call_once).
 */
    static T &instance()
    {
        std::call_once(mOnceFlag, create);
        CXXKIT_ASSERT(mInstance.load());
        return *mInstance.load();
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

/** @brief Transfer ownership out of the singleton scope.
 * @return Raw pointer to the instance, disassociated from internal scope.
 */
    T *detachScoped()
    {
        CXXKIT_ASSERT(mInstance.load());
        mScoped.release();
        return mInstance.exchange(nullptr);
    }

/** @brief Destroy the singleton instance and release all ownership.
 * @note Call during process tear-down; afterwards instance() is undefined.
 */
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
/** @brief Manual-lifetime singleton alias.
 * @tparam T Concrete singleton type.
 * @see Singleton, AutoSingleton
 */
template <typename T>
using ManualSingleton = Singleton<T, true>;

//! @cond INTERNAL

template <typename T>
std::once_flag Singleton<T, true>::mOnceFlag;
template <typename T>
std::atomic<T *> Singleton<T, true>::mInstance = nullptr;
template <typename T>
std::unique_ptr<T> Singleton<T, true>::mScoped = nullptr;

//! @endcond

CXXKIT_END_NAMESPACE

/** @brief Declare friendship with the Singleton specialization for the given class.
 * @param CLASS Concrete singleton class name.
 * @note Place inside the class definition so Singleton can call protected ctor/dtor.
 */
#define CXXKIT_DECLARE_SINGLETON(CLASS) friend class cxxkit::Singleton<CLASS, UseManualLifetime>;

