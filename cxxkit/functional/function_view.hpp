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

#ifndef _CXXKIT_FUNCTION_VIEW_HPP
#define _CXXKIT_FUNCTION_VIEW_HPP

#include <cxxkit/tools/checks.hpp>

#include <cstddef>
#include <utility>
#include <type_traits>

CXXKIT_BEGIN_NAMESPACE

/**
 * @addtogroup core
 * @{
 * @addtogroup FunctionView
 * @brief
 * @{
 * @details
 * Just like std::function, FunctionView will wrap any callable and hide its actual type, exposing only its signature.
 * But unlike std::function, FunctionView doesn't own its callable---it just points to it.
 * Thus, it's a good choice mainly as a function argument when the callable argument will not be called again once the
 * function has returned.
 *
 * Its constructors are implicit, so that callers won't have to convert lambdas and other callables to
 * FunctionView<Blah(Blah, Blah)> explicitly.
 * This is safe because FunctionView is only a reference to the real callable.
 *
 * Example use:
 * @code
 *    void SomeFunction(FunctionView<int(int)> index_transform);
 *    ...
 *    SomeFunction([](int i) { return 2 * i + 1; });
 * @endcode
 *
 * Note: FunctionView is tiny (essentially just two pointers) and trivially copyable, so it's probably cheaper
 * to pass it by value than by const reference.
 */

template <typename T>
class FunctionView; // Undefined.

template <typename RetT, typename... ArgT>
class FunctionView<RetT(ArgT...)> final
{
public:
    /**
     * Constructor for lambdas and other callables;
     * it accepts every type of argument except those noted in its enable_if call.
     * @tparam F
     * @param f
     */
    template <
        typename F,
        typename std::enable_if<
            // Not for function pointers; we have another constructor for that below.
            !std::is_function<typename std::remove_pointer<typename std::remove_reference<F>::type>::type>::value &&
            // Not for nullptr; we have another constructor for that below.
            !std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value &&
            // Not for FunctionView objects; we have another constructor for that (the implicitly declared copy constructor).
            !std::is_same<FunctionView, typename std::remove_cv<typename std::remove_reference<F>::type>::type>::
                value>::type * = nullptr>
    FunctionView(F &&f)
        : mCall(CallVoidPtr<typename std::remove_reference<F>::type>)
    {
        mVoidUnion.voidPtr = &f;
    }

    /**
     * Constructor that accepts function pointers.
     * If the argument is null, the result is an empty FunctionView.
     * @tparam F
     * @param f
     */
    template <typename F,
              typename std::enable_if<std::is_function<typename std::remove_pointer<
                  typename std::remove_reference<F>::type>::type>::value>::type * = nullptr>
    FunctionView(F &&f)
        : mCall(f ? CallFunPtr<typename std::remove_pointer<F>::type> : nullptr)
    {
        mVoidUnion.funPtr = reinterpret_cast<void (*)()>(f);
    }

    /**
     * Constructor that accepts nullptr. It creates an empty FunctionView.
     * @tparam F
     */
    template <typename F,
              typename std::enable_if<std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value>::type * =
                  nullptr>
    FunctionView(F && /* f */)
        : mCall(nullptr)
    {
    }

    /**
     * Default constructor. Creates an empty FunctionView.
     */
    FunctionView()
        : mCall(nullptr)
    {
    }

    RetT operator()(ArgT... args) const
    {
        CXXKIT_DCHECK(mCall);
        return mCall(mVoidUnion, std::forward<ArgT>(args)...);
    }

    /**
     * Returns true if we have a function, false if we don't (i.e., we're null).
     */
    explicit operator bool() const { return !!mCall; }

private:
    union VoidUnion
    {
        void *voidPtr;
        void (*funPtr)();
    };

    template <typename F>
    static RetT CallVoidPtr(VoidUnion vu, ArgT... args)
    {
        return (*static_cast<F *>(vu.voidPtr))(std::forward<ArgT>(args)...);
    }
    template <typename F>
    static RetT CallFunPtr(VoidUnion vu, ArgT... args)
    {
        return (reinterpret_cast<typename std::add_pointer<F>::type>(vu.funPtr))(std::forward<ArgT>(args)...);
    }

    /**
     * A pointer to the callable thing, with type information erased.
     * It's a union because we have to use separate types depending on if the callable thing is a function pointer
     * or something else.
     */
    VoidUnion mVoidUnion;

    /**
     * Pointer to a dispatch function that knows the type of the callable thing that's stored in mVoidUnion,
     * and how to call it.
     * A FunctionView object is empty (null) iff mCall is null.
     */
    RetT (*mCall)(VoidUnion, ArgT...);
};

/**
 * @}
 * @}
 */

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_FUNCTION_VIEW_HPP
