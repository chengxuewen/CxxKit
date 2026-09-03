/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

// exp_base: version/compiler-feature report and house macros (pimpl, non-copyable).

#include <iostream>
#include <memory>

#include <cxxkit/base/macros.hpp>
#include <cxxkit/base/core_config.hpp>
#include <cxxkit/base/compiler.hpp>

/*!
 * ****************************************************************************************************************
 * \brief Section 2 demo: copy/move inhibition. CXXKIT_DISABLE_COPY_MOVE deletes the copy ctor/assign and the move
 *        ctor/assign at compile time, so any attempt to copy or move a NonCopyable fails to compile.
 * ****************************************************************************************************************
 */
class NonCopyable
{
public:
    NonCopyable() { }
    CXXKIT_DISABLE_COPY_MOVE(NonCopyable)
};

/*!
 * ****************************************************************************************************************
 * \brief Section 3 demo: pimpl idiom. CXXKIT_DEFINE_DPTR declares `std::unique_ptr<WidgetPrivate> mDPtr;`;
 *        CXXKIT_DECLARE_PRIVATE declares `d_func()` accessors (+ friend WidgetPrivate); CXXKIT_D is the
 *        shorthand inside member functions. The token-joined `WidgetPrivate` must live in the same scope
 *        as `Widget` — no detail/ namespace wrapper in a single-file demo.
 */
CXXKIT_BEGIN_NAMESPACE
struct WidgetPrivate
{
    int mValue;
};

class Widget
{
public:
    Widget()
        : mDPtr(new WidgetPrivate)
    {
        d_func()->mValue = 42;
    }
    int value() const
    {
        CXXKIT_D(const Widget); // `const WidgetPrivate *const d = d_func()`
        return d->mValue;
    }
    CXXKIT_DISABLE_COPY_MOVE(Widget)
    CXXKIT_DECLARE_PRIVATE(Widget)
    CXXKIT_DEFINE_DPTR(Widget) // unique_ptr member: private impl freed automatically, no new/delete leak
};
CXXKIT_END_NAMESPACE

int main()
{
    // Section 1: version + compiler-feature report.
    std::cout << "== version & compiler features ==" << std::endl;
    std::cout << "CxxKit version: " << CXXKIT_VERSION_NAME << std::endl;
#if defined(CXXKIT_CC_GNU) && !defined(CXXKIT_CC_CLANG)
    std::cout << "Compiler: GNU C++ " << (CXXKIT_CC_GNU / 100) << "." << (CXXKIT_CC_GNU % 100) << std::endl;
#elif defined(CXXKIT_CC_CLANG)
    std::cout << "Compiler: Clang " << (CXXKIT_CC_CLANG / 100) << "." << (CXXKIT_CC_CLANG % 100) << std::endl;
#elif defined(CXXKIT_CC_MSVC)
    std::cout << "Compiler: MSVC " << CXXKIT_CC_MSVC << std::endl;
#endif
    std::cout << "__cplusplus = " << CXXKIT_CC_CPLUSPLUS_VERSION << std::endl;
#if CXXKIT_CC_CPP14_OR_GREATER
    std::cout << "C++14 or greater: yes" << std::endl;
#else
    std::cout << "C++14 or greater: no" << std::endl;
#endif
#if CXXKIT_CC_CPP17_OR_GREATER
    std::cout << "C++17 or greater: yes" << std::endl;
#else
    std::cout << "C++17 or greater: no" << std::endl;
#endif
#if CXXKIT_CC_CPP23_OR_GREATER
    std::cout << "C++23 or greater: yes" << std::endl;
#else
    std::cout << "C++23 or greater: no" << std::endl;
#endif

    // Section 2: copy/move inhibition (the deleted calls below would not compile):
    //   NonCopyable a; NonCopyable b(a);            // error: copy ctor deleted
    //   b = a;                                      // error: copy assign deleted
    //   NonCopyable c(std::move(a));                // error: move ctor deleted
    std::cout << "\n== CXXKIT_DISABLE_COPY_MOVE ==" << std::endl;
    NonCopyable non_copyable;
    (void)non_copyable;
    std::cout << "NonCopyable compiled; copy/move are deleted (see commented-out calls above)" << std::endl;

    // Section 3: pimpl via CXXKIT_DEFINE_DPTR.
    std::cout << "\n== CXXKIT_DEFINE_DPTR pimpl ==" << std::endl;
    cxxkit::Widget widget;
    std::cout << "Widget::value() = " << widget.value() << std::endl;
    return 0;
}
