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

#include <cxxkit/tools/tools_global.hpp>

#include <cxxkit/base/global.hpp>

/***********************************************************************************************************************
   cxxkit assert macro
***********************************************************************************************************************/
CXXKIT_TOOLS_API void cxxkit_assert_x(const char *where, const char *what, const char *file, int line) CXXKIT_NOTHROW;
CXXKIT_TOOLS_API void cxxkit_assert(const char *assertion, const char *file, int line) CXXKIT_NOTHROW;
static inline void cxxkit_noop(void)
{
}
#if !defined(CXXKIT_ASSERT)
#    if defined(CXXKIT_NO_DEBUG) && !defined(CXXKIT_FORCE_ASSERTS)
#        define CXXKIT_ASSERT(cond)                                                                                    \
            do                                                                                                         \
            {                                                                                                          \
            } while ((false) && (cond))
#    else
#        define CXXKIT_ASSERT(cond) ((!(cond)) ? cxxkit_assert(#cond, __FILE__, __LINE__) : cxxkit_noop())
#    endif
#endif
#if !defined(CXXKIT_ASSERT_X)
#    if defined(CXXKIT_NO_DEBUG) && !defined(CXXKIT_FORCE_ASSERTS)
#        define CXXKIT_ASSERT_X(cond, where, what)                                                                     \
            do                                                                                                         \
            {                                                                                                          \
            } while ((false) && (cond))
#    else
#        define CXXKIT_ASSERT_X(cond, where, what)                                                                     \
            ((!(cond)) ? cxxkit_assert_x(where, what, __FILE__, __LINE__) : cxxkit_noop())
#    endif
#endif
#define CXXKIT_STATIC_ASSERT(Condition)            static_assert(bool(Condition), #Condition)
#define CXXKIT_STATIC_ASSERT_X(Condition, Message) static_assert(bool(Condition), Message)

/**
 * @brief `CXXKIT_INTERNAL_HARDENING_ABORT()` controls how `CXXKIT_HARDENING_ASSERT()` aborts the program in
 * release mode (when NDEBUG is defined).
 * The implementation should abort the program as quickly as possible and ideally it should not be possible
 * to ignore the abort request.
 */
#if (CXXKIT_CC_HAS_BUILTIN(__builtin_trap) && CXXKIT_CC_HAS_BUILTIN(__builtin_unreachable)) ||                         \
    (defined(__GNUC__) && !defined(__clang__))
#    define CXXKIT_INTERNAL_HARDENING_ABORT()                                                                          \
        do                                                                                                             \
        {                                                                                                              \
            __builtin_trap();                                                                                          \
            __builtin_unreachable();                                                                                   \
        } while (false)
#else
#    define CXXKIT_INTERNAL_HARDENING_ABORT() abort()
#endif

/**
 * @brief `CXXKIT_HARDENING_ASSERT()` is like `CXXKIT_ASSERT()`, but used to implement runtime assertions that should
 * be enabled in hardened builds even when `NDEBUG` is defined.
 *
 * When `NDEBUG` is not defined or CXXKIT_FEATURE_ENABLE_HARDENING_ASSERT set false, `CXXKIT_HARDENING_ASSERT()` is
 * identical to `ABSL_ASSERT()`.
 */
#if CXXKIT_FEATURE_ENABLE_HARDENING_ASSERT && defined(NDEBUG)
#    define CXXKIT_HARDENING_ASSERT(expr)                                                                              \
        (CXXKIT_LIKELY((expr)) ? static_cast<void>(0) : [] { CXXKIT_INTERNAL_HARDENING_ABORT(); }())
#    define CXXKIT_UNREACHABLE() CXXKIT_INTERNAL_HARDENING_ABORT()
#else
#    define CXXKIT_HARDENING_ASSERT(expr) CXXKIT_ASSERT(expr)
#    define CXXKIT_UNREACHABLE()          CXXKIT_ASSERT_X("CXXKIT_UNREACHABLE reached")
#endif

#if CXXKIT_CC_CPP14_OR_GREATER
#    define CXXKIT_CXX14_CONSTEXPR_ASSERT(Condition)            CXXKIT_STATIC_ASSERT(Condition)
#    define CXXKIT_CXX14_CONSTEXPR_ASSERT_X(Condition, Message) CXXKIT_STATIC_ASSERT_X(Condition, Message)
#else
#    define CXXKIT_CXX14_CONSTEXPR_ASSERT(Condition)            CXXKIT_ASSERT(Condition)
#    define CXXKIT_CXX14_CONSTEXPR_ASSERT_X(Condition, Message) CXXKIT_ASSERT_X(Condition, CXXKIT_STRFILELINE, Message)
#endif

#if CXXKIT_CC_CPP17_OR_GREATER
#    define CXXKIT_CXX17_CONSTEXPR_ASSERT(Condition)            CXXKIT_STATIC_ASSERT(Condition)
#    define CXXKIT_CXX17_CONSTEXPR_ASSERT_X(Condition, Message) CXXKIT_STATIC_ASSERT_X(Condition, Message)
#else
#    define CXXKIT_CXX17_CONSTEXPR_ASSERT(Condition)            CXXKIT_ASSERT(Condition)
#    define CXXKIT_CXX17_CONSTEXPR_ASSERT_X(Condition, Message) CXXKIT_ASSERT_X(Condition, CXXKIT_STRFILELINE, Message)
#endif

