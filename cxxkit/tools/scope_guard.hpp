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

#ifndef _CXXKIT_SCOPE_GUARD_HPP
#define _CXXKIT_SCOPE_GUARD_HPP

#include "cxxkit/base/global.hpp"
#include "cxxkit/tools/assert.hpp"
#include "cxxkit/tools/type_traits.hpp"

#include <atomic>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{

template <typename Callback> class ScopeGuardStorage
{
public:
    ScopeGuardStorage() = delete;
    explicit ScopeGuardStorage(Callback callback)
    {
        // Placement-new into a character buffer is used for eager destruction when
        // the cleanup is invoked or cancelled. To ensure this optimizes well, the
        // behavior is implemented locally instead of using an absl::optional.
        ::new (this->getCallbackBuffer()) Callback(std::move(callback));
        mCallbackEngaged = true;
    }
    ScopeGuardStorage(ScopeGuardStorage &&other)
    {
        CXXKIT_HARDENING_ASSERT(other.isCallbackEngaged());
        ::new (this->getCallbackBuffer()) Callback(std::move(other.getCallback()));
        mCallbackEngaged = true;
        other.destroyCallback();
    }

    ScopeGuardStorage(const ScopeGuardStorage &other) = delete;
    ScopeGuardStorage &operator=(ScopeGuardStorage &&other) = delete;
    ScopeGuardStorage &operator=(const ScopeGuardStorage &other) = delete;

    void destroyCallback()
    {
        mCallbackEngaged = false;
        this->getCallback().~Callback();
    }
    bool isCallbackEngaged() const { return mCallbackEngaged; }
    void *getCallbackBuffer() { return static_cast<void *>(+mCallbackBuffer); }
    Callback &getCallback() { return *reinterpret_cast<Callback *>(this->getCallbackBuffer()); }
    void invokeCallback() CXXKIT_ATTRIBUTE_NO_THREAD_SAFETY_ANALYSIS { std::move(this->getCallback())(); }

private:
    bool mCallbackEngaged;
    alignas(Callback) char mCallbackBuffer[sizeof(Callback)];
};
} // namespace detail

template <typename F> class [[nodiscard]] ScopeGuard
{
    static_assert(ReturnsVoid<F>::value, "Callbacks that return values are not supported.");

public:
    ScopeGuard(F f) noexcept
        : mStorage(std::move(f))
    {
    }
    ScopeGuard(ScopeGuard &&other) = default;

    ~ScopeGuard() noexcept
    {
        if (mStorage.isCallbackEngaged())
        {
            mStorage.invokeCallback();
            mStorage.destroyCallback();
        }
    }

    void invoke() &&
    {
        CXXKIT_HARDENING_ASSERT(mStorage.isCallbackEngaged());
        mStorage.invokeCallback();
        mStorage.destroyCallback();
    }
    void cancel() && noexcept
    {
        CXXKIT_HARDENING_ASSERT(mStorage.isCallbackEngaged());
        mStorage.destroyCallback();
    }

private:
    detail::ScopeGuardStorage<F> mStorage;
    CXXKIT_DECLARE_DISABLE_COPY(ScopeGuard)
};

#if CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
template <typename F> ScopeGuard(F (&)()) -> ScopeGuard<F (*)()>;
#endif

namespace utils
{
template <typename F> [[nodiscard]] ScopeGuard<F> makeScopeGuard(F f) { return {std::move(f)}; }
template <typename FC, typename F> [[nodiscard]] ScopeGuard<F> makeScopeGuard(const FC &fc, F f)
{
    fc();
    return {std::move(f)};
}
} // namespace utils

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_SCOPE_GUARD_HPP
