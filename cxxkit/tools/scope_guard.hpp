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

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/assert.hpp>
#include <cxxkit/tools/type_traits.hpp>

#include <atomic>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{

template <typename Callback>
class ScopeGuardStorage
{
public:
    ScopeGuardStorage() = delete;
    explicit ScopeGuardStorage(Callback callback)
    {
        // Placement-new into a character buffer is used for eager destruction when
        // the cleanup is invoked or cancelled. To ensure this optimizes well, the
        // behavior is implemented locally instead of using an absl::optional.
        ::new (this->get_callback_buffer()) Callback(std::move(callback));
        mCallbackEngaged = true;
    }
    ScopeGuardStorage(ScopeGuardStorage &&other)
    {
        CXXKIT_HARDENING_ASSERT(other.is_callback_engaged());
        ::new (this->get_callback_buffer()) Callback(std::move(other.get_callback()));
        mCallbackEngaged = true;
        other.destroy_callback();
    }

    ScopeGuardStorage(const ScopeGuardStorage &other) = delete;
    ScopeGuardStorage &operator=(ScopeGuardStorage &&other) = delete;
    ScopeGuardStorage &operator=(const ScopeGuardStorage &other) = delete;

    void destroy_callback()
    {
        mCallbackEngaged = false;
        this->get_callback().~Callback();
    }
    bool is_callback_engaged() const { return mCallbackEngaged; }
    void *get_callback_buffer() { return static_cast<void *>(+mCallbackBuffer); }
    Callback &get_callback() { return *reinterpret_cast<Callback *>(this->get_callback_buffer()); }
    void invoke_callback() CXXKIT_ATTRIBUTE_NO_THREAD_SAFETY_ANALYSIS { std::move(this->get_callback())(); }

private:
    bool mCallbackEngaged;
    alignas(Callback) char mCallbackBuffer[sizeof(Callback)];
};
} // namespace detail

template <typename F>
class [[nodiscard]] scope_guard
{
    static_assert(ReturnsVoid<F>::value, "Callbacks that return values are not supported.");

public:
    scope_guard(F f) noexcept
        : mStorage(std::move(f))
    {
    }
    scope_guard(scope_guard &&other) = default;

    ~scope_guard() noexcept
    {
        if (mStorage.is_callback_engaged())
        {
            mStorage.invoke_callback();
            mStorage.destroy_callback();
        }
    }

    void invoke() &&
    {
        CXXKIT_HARDENING_ASSERT(mStorage.is_callback_engaged());
        mStorage.invoke_callback();
        mStorage.destroy_callback();
    }
    void cancel() && noexcept
    {
        CXXKIT_HARDENING_ASSERT(mStorage.is_callback_engaged());
        mStorage.destroy_callback();
    }

private:
    detail::ScopeGuardStorage<F> mStorage;
    CXXKIT_DECLARE_DISABLE_COPY(scope_guard)
};

#if CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION
template <typename F>
scope_guard(F (&)()) -> scope_guard<F (*)()>;
#endif

namespace utils
{
template <typename F>
[[nodiscard]] scope_guard<F> make_scope_guard(F f)
{
    return {std::move(f)};
}
template <typename FC, typename F>
[[nodiscard]] scope_guard<F> make_scope_guard(const FC &fc, F f)
{
    fc();
    return {std::move(f)};
}
} // namespace utils

CXXKIT_END_NAMESPACE
