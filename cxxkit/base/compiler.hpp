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

#ifndef _CXXKIT_COMPILER_HPP
#define _CXXKIT_COMPILER_HPP

#include "cxxkit/base/preprocessor.hpp"
#include "cxxkit/base/system.hpp"

/***********************************************************************************************************************
    cxxkit compiler type version macro define
***********************************************************************************************************************/
/*
    The compiler, must be one of: (CXXKIT_CC_x)

    GNU             - GNU C++ (MinGW-GCC for win)
    MINGW           - MinGW C++ (GCC for win)
    MSVC            - Microsoft Visual C/C++, Intel C++ for Windows
    CLANG           - C++ front-end for the LLVM compiler
    INTEL           - Intel C++ for Linux, Intel C++ for Windows
    BOR             - Borland/Turbo C++
    EMSCRIPTEN      - asm.js C++ for wasm

    Should be sorted most to least authoritative.
*/

/*GNU*/
#if defined(__GNUC__)
#    define CXXKIT_CC_GNU (100 * __GNUC__ + __GNUC_MINOR__)
#    define CXXKIT_CC_GNU_CHECK_VERSION(major, minor)                                                                    \
        ((__GNUC__ > (major)) || ((__GNUC__ == (major)) && (__GNUC_MINOR__ >= (minor))))
#else
#    define CXXKIT_CC_GNU_CHECK_VERSION(major, minor) 0
#endif

/*MINGW*/
#if defined(__MINGW32__)
#    define CXXKIT_CC_MINGW
#endif

/*MSVC*/
#if defined(_MSC_VER)
// MSVC++  6.0  _MSC_VER == 1200   (Visual Studio 6.0)
// MSVC++  7.0  _MSC_VER == 1300   (Visual Studio .NET 2002)
// MSVC++  7.1  _MSC_VER == 1310   (Visual Studio .NET 2003)
// MSVC++  8.0  _MSC_VER == 1400   (Visual Studio 2005)
// MSVC++  9.0  _MSC_VER == 1500   (Visual Studio 2008)
// MSVC++ 10.0  _MSC_VER == 1600   (Visual Studio 2010)
// MSVC++ 11.0  _MSC_VER == 1700   (Visual Studio 2012)
// MSVC++ 12.0  _MSC_VER == 1800   (Visual Studio 2013)
// MSVC++ 14.0  _MSC_VER == 1900   (Visual Studio 2015)
// MSVC++ 14.1  _MSC_VER >= 1910   (Visual Studio 2017)
// MSVC++ 14.2  _MSC_VER >= 1920   (Visual Studio 2019)
#    define CXXKIT_CC_MSVC (_MSC_VER)
#    define CXXKIT_CC_MSVC_NET
#    if _MSC_FULL_VER > 100000000
#        define CXXKIT_MSVC_FULL_VER _MSC_FULL_VER
#    else
#        define CXXKIT_MSVC_FULL_VER (_MSC_FULL_VER * 10)
#    endif
#endif

/*CLANG*/
#if defined(__clang__)
#    if defined(__apple_build_version__)
#        /* http://en.wikipedia.org/wiki/Xcode#Toolchain_Versions */

#        if __apple_build_version__ >= 8020041
#            define CXXKIT_CC_CLANG 309
#        elif __apple_build_version__ >= 8000038
#            define CXXKIT_CC_CLANG 308
#        elif __apple_build_version__ >= 7000053
#            define CXXKIT_CC_CLANG 306
#        elif __apple_build_version__ >= 6000051
#            define CXXKIT_CC_CLANG 305
#        elif __apple_build_version__ >= 5030038
#            define CXXKIT_CC_CLANG 304
#        elif __apple_build_version__ >= 5000275
#            define CXXKIT_CC_CLANG 303
#        elif __apple_build_version__ >= 4250024
#            define CXXKIT_CC_CLANG 302
#        elif __apple_build_version__ >= 3180045
#            define CXXKIT_CC_CLANG 301
#        elif __apple_build_version__ >= 2111001
#            define CXXKIT_CC_CLANG 300
#        else
#            error "Unknown Apple Clang version"
#        endif
#    else
#        define CXXKIT_CC_CLANG (100 * __clang_major__ + __clang_minor__)
#    endif
#endif

/*INTEL*/
#if defined(__INTEL_COMPILER)
#    define CXXKIT_CC_INTEL __INTEL_COMPILER
#endif

/*BOR*/
#if defined(__BORLANDC__) || defined(__TURBOC__)
#    define CXXKIT_CC_BOR
#    if __BORLANDC__ < 0x502
#        error "Compiler not supported with BORLANDC less than 0x502"
#    endif
#endif

/*EMSCRIPTEN*/
#ifdef __EMSCRIPTEN__
#    define CXXKIT_CC_EMSCRIPTEN
#endif

/***********************************************************************************************************************
    cxxkit compiler cplusplus std value macro define
***********************************************************************************************************************/
#if defined(_MSVC_LANG) && !defined(__clang__)
#    define CXXKIT_CC_CPLUSPLUS_VERSION (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#    define CXXKIT_CC_CPLUSPLUS_VERSION __cplusplus
#endif

#define CXXKIT_CC_CPP14_OR_GREATER (CXXKIT_CC_CPLUSPLUS_VERSION >= 201402L)
#define CXXKIT_CC_CPP17_OR_GREATER (CXXKIT_CC_CPLUSPLUS_VERSION >= 201703L)
#define CXXKIT_CC_CPP20_OR_GREATER (CXXKIT_CC_CPLUSPLUS_VERSION >= 202002L)
#define CXXKIT_CC_CPP23_OR_GREATER (CXXKIT_CC_CPLUSPLUS_VERSION >= 202300L)


/***********************************************************************************************************************
    cxxkit compiler cxx feature macro define
***********************************************************************************************************************/
/*CXXKIT_CC_GNU*/
#if defined(CXXKIT_CC_GNU) && !defined(CXXKIT_CC_INTEL) && !defined(CXXKIT_CC_CLANG)
#    define CXXKIT_CC_FEATURE_RESTRICTED_VLA     1
#    define CXXKIT_CC_FEATURE_THREADSAFE_STATICS 1
#    if CXXKIT_CC_GNU >= 403
// GCC supports binary literals in C, C++98 and C++11 modes
#        define CXXKIT_CC_FEATURE_BINARY_LITERALS 1
#    endif
#    if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) ||                                            \
        (defined(__cplusplus) && (__cplusplus >= 201103L)) ||                                                          \
        (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
// Variadic macros are supported for gnu++98, c++11, C99 ... since forever (gcc 2.97)
#        define CXXKIT_CC_FEATURE_VARIADIC_MACROS 1
#    endif
#    if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#        if CXXKIT_CC_GNU >= 403
/* C++11 features supported in GCC 4.3: */
#            define CXXKIT_CC_FEATURE_DECLTYPE      1
#            define CXXKIT_CC_FEATURE_RVALUE_REFS   1
#            define CXXKIT_CC_FEATURE_STATIC_ASSERT 1
#        endif
#        if CXXKIT_CC_GNU >= 404
/* C++11 features supported in GCC 4.4: */
#            define CXXKIT_CC_FEATURE_AUTO_FUNCTION      1
#            define CXXKIT_CC_FEATURE_AUTO_TYPE          1
#            define CXXKIT_CC_FEATURE_EXTERN_TEMPLATES   1
#            define CXXKIT_CC_FEATURE_UNIFORM_INIT       1
#            define CXXKIT_CC_FEATURE_UNICODE_STRINGS    1
#            define CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES 1
#        endif
#        if CXXKIT_CC_GNU >= 405
/* C++11 features supported in GCC 4.5: */
#            define CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS 1
/* GCC 4.4 implements initializer_list but does not define typedefs required
            * by the standard. */
#            define CXXKIT_CC_FEATURE_INITIALIZER_LISTS 1
#            define CXXKIT_CC_FEATURE_LAMBDA            1
#            define CXXKIT_CC_FEATURE_RAW_STRINGS       1
#            define CXXKIT_CC_FEATURE_CLASS_ENUM        1
#        endif
#        if CXXKIT_CC_GNU >= 406
/* Pre-4.6 compilers implement a non-final snapshot of N2346, hence default and delete
            * functions are supported only if they are public. Starting from 4.6, GCC handles
            * final version - the access modifier is not relevant. */
#            define CXXKIT_CC_FEATURE_DEFAULT_MEMBERS 1
#            define CXXKIT_CC_FEATURE_DELETE_MEMBERS  1
/* C++11 features supported in GCC 4.6: */
#            define CXXKIT_CC_FEATURE_NULLPTR             1
#            define CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS 1
#            define CXXKIT_CC_FEATURE_RANGE_FOR           1
#        endif
#        if CXXKIT_CC_GNU >= 407
/* GCC 4.4 implemented <atomic> and std::atomic using its old intrinsics.
            * However, the implementation is incomplete for most platforms until GCC 4.7:
            * instead, std::atomic would use an external lock. Since we need an std::atomic
            * that is behavior-compatible with QBasicAtomic, we only enable it here */
#            define CXXKIT_CC_FEATURE_ATOMICS 1
/* GCC 4.6.x has problems dealing with noexcept expressions,
            * so turn the feature on for 4.7 and above, only */
#            define CXXKIT_CC_FEATURE_NOEXCEPT 1
/* C++11 features supported in GCC 4.7: */
#            define CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT   1
#            define CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS 1
#            define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES      1
#            define CXXKIT_CC_FEATURE_TEMPLATE_ALIAS          1
#            define CXXKIT_CC_FEATURE_UDL                     1
#        endif
#        if CXXKIT_CC_GNU >= 408
#            define CXXKIT_CC_FEATURE_ATTRIBUTES              1
#            define CXXKIT_CC_FEATURE_ALIGNAS                 1
#            define CXXKIT_CC_FEATURE_ALIGNOF                 1
#            define CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS 1
#            define CXXKIT_CC_FEATURE_THREAD_LOCAL            1
#            if CXXKIT_CC_GNU > 408 || __GNUC_PATCHLEVEL__ >= 1
#                define CXXKIT_CC_FEATURE_REF_QUALIFIERS 1
#            endif
#        endif
#        if CXXKIT_CC_GNU >= 500
/* GCC 4.6 introduces constexpr, but it's bugged (at least) in the whole
            * 4.x series, see e.g. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57694 */
#            define CXXKIT_CC_FEATURE_CONSTEXPR 1
#        endif
#    endif
#    if __cplusplus > 201103L
#        if CXXKIT_CC_GNU >= 409
/* C++1y features in GCC 4.9 - deprecated, do not update this list */
// define CXXKIT_CC_FEATURE_BINARY_LITERALS
// already supported since GCC 4.3 as an extension
#            define CXXKIT_CC_FEATURE_LAMBDA_CAPTURES       1
#            define CXXKIT_CC_FEATURE_RETURN_TYPE_DEDUCTION 1
#        endif
#    endif
#    if defined(__STDC_VERSION__) && __STDC_VERSION__ > 199901L
#        if CXXKIT_CC_GNU >= 407
/* C11 features supported in GCC 4.7: */
#            define CXXKIT_CC_FEATURE_STATIC_ASSERT 1
#        endif
#        if CXXKIT_CC_GNU >= 409 && defined(__has_include)
/* C11 features supported in GCC 4.9: */
#            if __has_include(<threads.h>)
#                define CXXKIT_CC_FEATURE_THREAD_LOCAL 1
#            endif
#        endif
#    endif
#    if CXXKIT_CC_GNU >= 408
#        define CXXKIT_CC_FEATURE_BUILTIN_SOURCE_LOCATION 1
#    endif
#endif

/*CXXKIT_CC_MSVC*/
#if defined(CXXKIT_CC_MSVC)
#    if _MSC_VER >= 1400 // C++11 features supported in VC8 = VC2005
#        define CXXKIT_CC_FEATURE_VARIADIC_MACROS 1
/* 2005 supports the CXXKIT_OVERRIDE and final contextual keywords, in
        the same positions as the C++11 variants, but 'final' is
        called 'sealed' instead:
        http://msdn.microsoft.com/en-us/library/0w2w91tf%28v=vs.80%29.aspx
        The behavior is slightly different in C++/CLI, which requires the
        "virtual" keyword to be present too, so don't define for that.
        So don't define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES (since it's not
        the same as the C++11 version), but define the CXXKIT_DECLARE_* flags
        accordingly. */
#    endif
#    if _MSC_VER >= 1500 // C++11 features supported in VC9 = VC2008
#    endif
#    if _MSC_VER >= 1600 // C++11 features supported in VC10 = VC2010
#        define CXXKIT_CC_FEATURE_AUTO_FUNCTION    1
#        define CXXKIT_CC_FEATURE_AUTO_TYPE        1
#        define CXXKIT_CC_FEATURE_DECLTYPE         1
#        define CXXKIT_CC_FEATURE_EXTERN_TEMPLATES 1
#        define CXXKIT_CC_FEATURE_LAMBDA           1
#        define CXXKIT_CC_FEATURE_NULLPTR          1
#        define CXXKIT_CC_FEATURE_RVALUE_REFS      1
#        define CXXKIT_CC_FEATURE_STATIC_ASSERT    1
#    else
#        define CXXKIT_CC_FEATURE_NO_TEMPLATE_FRIENDS 1
#    endif
#    if _MSC_VER >= 1700 // C++11 features supported in VC11 = VC2012
#        define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES 1
#        define CXXKIT_CC_FEATURE_CLASS_ENUM         1
#        define CXXKIT_CC_FEATURE_ATOMICS            1
#    endif
#    if _MSC_VER >= 1800 // C++11 features supported in VC12 = VC2013
#        define CXXKIT_CC_FEATURE_DELETE_MEMBERS          1
#        define CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS 1
#        define CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS    1
#        define CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT   1
#        define CXXKIT_CC_FEATURE_RAW_STRINGS             1
#        define CXXKIT_CC_FEATURE_TEMPLATE_ALIAS          1
#        define CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES      1
#        define CXXKIT_CC_FEATURE_INITIALIZER_LISTS       1 // VC 12 SP 2 RC
#    endif
#    if _MSC_VER >= 1900 // C++11 features supported in VC14 = VC2015
#        define CXXKIT_CC_FEATURE_DEFAULT_MEMBERS         1
#        define CXXKIT_CC_FEATURE_ALIGNAS                 1
#        define CXXKIT_CC_FEATURE_ALIGNOF                 1
#        define CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS 1
#        define CXXKIT_CC_FEATURE_NOEXCEPT                1
#        define CXXKIT_CC_FEATURE_RANGE_FOR               1
#        define CXXKIT_CC_FEATURE_REF_QUALIFIERS          1
#        define CXXKIT_CC_FEATURE_THREAD_LOCAL            1
#        define CXXKIT_CC_FEATURE_UDL                     1
#        define CXXKIT_CC_FEATURE_UNICODE_STRINGS         1
#        define CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS     1
#    endif
#    if _MSC_VER >= 1910 // C++11 features supported in VC14.1 = VC2017
#        define CXXKIT_CC_FEATURE_CONSTEXPR 1
#    endif
#    if _MSC_VER >= 1920 // C++11 features supported in VC14.2 = VC2019
#    endif
#    if _MSC_FULL_VER >= 190023419
#        define CXXKIT_CC_FEATURE_ATTRIBUTES         1
#        define CXXKIT_CC_FEATURE_THREADSAFE_STATICS 1
#        define CXXKIT_CC_FEATURE_UNIFORM_INIT       1
#    endif
#    if !defined(__INTEL_COMPILER) && (_MSC_VER >= 1926)
#        define CXXKIT_CC_FEATURE_BUILTIN_SOURCE_LOCATION 1
#        define CXXKIT_CC_FEATURE_BUILTIN_COLUMN          1
#    endif
#endif

/*CXXKIT_CC_CLANG*/
#if defined(CXXKIT_CC_CLANG) && !defined(CXXKIT_CC_INTEL) && !defined(CXXKIT_CC_MSVC)
/* General C++ features */
#    define CXXKIT_CC_FEATURE_RESTRICTED_VLA     1
#    define CXXKIT_CC_FEATURE_THREADSAFE_STATICS 1
// Clang supports binary literals in C, C++98 and C++11 modes
// It's been supported "since the dawn of time itself" (cf. commit 179883)
#    if __has_extension(cxx_binary_literals)
#        define CXXKIT_CC_FEATURE_BINARY_LITERALS 1
#    endif
// Variadic macros are supported for gnu++98, c++11, c99 ... since 2.9
#    if CXXKIT_CC_CLANG >= 209
#        if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) ||                                        \
            (defined(__cplusplus) && (__cplusplus >= 201103L)) ||                                                      \
            (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#            define CXXKIT_CC_FEATURE_VARIADIC_MACROS 1
#        endif
#    endif
/* C++11 features, see http://clang.llvm.org/cxx_status.html */
#    if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
/* Detect C++ features using __has_feature(), see http://clang.llvm.org/docs/LanguageExtensions.html#cxx11 */
#        if __has_feature(cxx_alignas)
#            define CXXKIT_CC_FEATURE_ALIGNAS 1
#            define CXXKIT_CC_FEATURE_ALIGNOF 1
#        endif
#        if __has_feature(cxx_atomic) && __has_include(<atomic>)
#            define CXXKIT_CC_FEATURE_ATOMICS 1
#        endif
#        if __has_feature(cxx_attributes)
#            define CXXKIT_CC_FEATURE_ATTRIBUTES 1
#        endif
#        if __has_feature(cxx_auto_type)
#            define CXXKIT_CC_FEATURE_AUTO_FUNCTION 1
#            define CXXKIT_CC_FEATURE_AUTO_TYPE     1
#        endif
#        if __has_feature(cxx_strong_enums)
#            define CXXKIT_CC_FEATURE_CLASS_ENUM 1
#        endif
#        if __has_feature(cxx_constexpr) && CXXKIT_CC_CLANG > 302 /* CLANG 3.2 has bad/partial support */
#            define CXXKIT_CC_FEATURE_CONSTEXPR 1
#        endif
#        if __has_feature(cxx_decltype) /* && __has_feature(cxx_decltype_incomplete_return_types) */
#            define CXXKIT_CC_FEATURE_DECLTYPE 1
#        endif
#        if __has_feature(cxx_defaulted_functions)
#            define CXXKIT_CC_FEATURE_DEFAULT_MEMBERS 1
#        endif
#        if __has_feature(cxx_deleted_functions)
#            define CXXKIT_CC_FEATURE_DELETE_MEMBERS 1
#        endif
#        if __has_feature(cxx_delegating_constructors)
#            define CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS 1
#        endif
#        if __has_feature(cxx_explicit_conversions)
#            define CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS 1
#        endif
#        if __has_feature(cxx_override_control)
#            define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES 1
#        endif
#        if __has_feature(cxx_inheriting_constructors)
#            define CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS 1
#        endif
#        if __has_feature(cxx_generalized_initializers)
#            define CXXKIT_CC_FEATURE_INITIALIZER_LISTS 1
#            define CXXKIT_CC_FEATURE_UNIFORM_INIT      1 /* both covered by this feature macro, according to docs */
#        endif
#        if __has_feature(cxx_lambdas)
#            define CXXKIT_CC_FEATURE_LAMBDA 1
#        endif
#        if __has_feature(cxx_noexcept)
#            define CXXKIT_CC_FEATURE_NOEXCEPT 1
#        endif
#        if __has_feature(cxx_nonstatic_member_init)
#            define CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT 1
#        endif
#        if __has_feature(cxx_nullptr)
#            define CXXKIT_CC_FEATURE_NULLPTR 1
#        endif
#        if __has_feature(cxx_range_for)
#            define CXXKIT_CC_FEATURE_RANGE_FOR 1
#        endif
#        if __has_feature(cxx_raw_string_literals)
#            define CXXKIT_CC_FEATURE_RAW_STRINGS 1
#        endif
#        if __has_feature(cxx_reference_qualified_functions)
#            define CXXKIT_CC_FEATURE_REF_QUALIFIERS 1
#        endif
#        if __has_feature(cxx_rvalue_references)
#            define CXXKIT_CC_FEATURE_RVALUE_REFS 1
#        endif
#        if __has_feature(cxx_static_assert)
#            define CXXKIT_CC_FEATURE_STATIC_ASSERT 1
#        endif
#        if __has_feature(cxx_alias_templates)
#            define CXXKIT_CC_FEATURE_TEMPLATE_ALIAS 1
#        endif
#        if __has_feature(cxx_thread_local)
#            if !defined(__FreeBSD__) /* FreeBSD clang fails on __cxa_thread_atexit */
#                define CXXKIT_CC_FEATURE_THREAD_LOCAL 1
#            endif
#        endif
#        if __has_feature(cxx_user_literals)
#            define CXXKIT_CC_FEATURE_UDL 1
#        endif
#        if __has_feature(cxx_unicode_literals)
#            define CXXKIT_CC_FEATURE_UNICODE_STRINGS 1
#        endif
#        if __has_feature(cxx_unrestricted_unions)
#            define CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS 1
#        endif
#        if __has_feature(cxx_variadic_templates)
#            define CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES 1
#        endif
/* Features that have no __has_feature() check */
#        if CXXKIT_CC_CLANG >= 209 /* since clang 2.9 */
#            define CXXKIT_CC_FEATURE_EXTERN_TEMPLATES 1
#        endif
#    endif
/* C++1y features, deprecated macros. Do not update this list. */
#    if __cplusplus > 201103L
#        if __has_feature(cxx_binary_literals)
#            define CXXKIT_CC_FEATURE_BINARY_LITERALS 1 // see above
#        endif
#        if __has_feature(cxx_generic_lambda)
#            define CXXKIT_CC_FEATURE_GENERIC_LAMBDA 1
#        endif
#        if __has_feature(cxx_init_capture)
#            define CXXKIT_CC_FEATURE_LAMBDA_CAPTURES 1
#        endif
#        if __has_feature(cxx_relaxed_constexpr)
#            define CXXKIT_CC_FEATURE_RELAXED_CONSTEXPR_FUNCTIONS 1
#        endif
#        if __has_feature(cxx_decltype_auto) && __has_feature(cxx_return_type_deduction)
#            define CXXKIT_CC_FEATURE_RETURN_TYPE_DEDUCTION 1
#        endif
#        if __has_feature(cxx_variable_templates)
#            define CXXKIT_CC_FEATURE_VARIABLE_TEMPLATES 1
#        endif
#        if __has_feature(cxx_runtime_array)
#            define CXXKIT_CC_FEATURE_VLA 1
#        endif
#    endif
#    if defined(__STDC_VERSION__)
#        if __has_feature(c_static_assert)
#            define CXXKIT_CC_FEATURE_STATIC_ASSERT 1
#        endif
#        if __has_feature(c_thread_local) && __has_include(<threads.h>)
#            if !defined(__FreeBSD__) /* FreeBSD clang fails on __cxa_thread_atexit */
#                define CXXKIT_CC_FEATURE_THREAD_LOCAL 1
#            endif
#        endif
#    endif
#    if !defined(__apple_build_version__) && (__clang_major__ >= 9)
#        define CXXKIT_CC_FEATURE_BUILTIN_SOURCE_LOCATION 1
#        define CXXKIT_CC_FEATURE_BUILTIN_COLUMN          1
#    elif defined(__apple_build_version__) &&                                                                          \
        (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__ % 100) >= 110003
#        define CXXKIT_CC_FEATURE_BUILTIN_SOURCE_LOCATION 1
#        define CXXKIT_CC_FEATURE_BUILTIN_COLUMN          1
#    endif
#elif defined(CXXKIT_CC_INTEL) && !defined(CXXKIT_CC_MSVC)
#    define CXXKIT_CC_FEATURE_RESTRICTED_VLA     1
#    define CXXKIT_CC_FEATURE_VARIADIC_MACROS    1 // C++11 feature supported as an extension in other modes, too
#    define CXXKIT_CC_FEATURE_THREADSAFE_STATICS 1
#    if __INTEL_COMPILER < 1200
#        define CXXKIT_CC_FEATURE_NO_TEMPLATE_FRIENDS 1
#    endif
#    if __INTEL_COMPILER >= 1310 && !defined(_WIN32)
//    ICC supports C++14 binary literals in C, C++98, and C++11 modes
//    at least since 13.1, but I can't test further back
#        define CXXKIT_CC_FEATURE_BINARY_LITERALS 1
#    endif
#    if __cplusplus >= 201103L || defined(__INTEL_CXX11_MODE__)
#        if __INTEL_COMPILER >= 1200
#            define CXXKIT_CC_FEATURE_AUTO_TYPE        1
#            define CXXKIT_CC_FEATURE_CLASS_ENUM       1
#            define CXXKIT_CC_FEATURE_DECLTYPE         1
#            define CXXKIT_CC_FEATURE_DEFAULT_MEMBERS  1
#            define CXXKIT_CC_FEATURE_DELETE_MEMBERS   1
#            define CXXKIT_CC_FEATURE_EXTERN_TEMPLATES 1
#            define CXXKIT_CC_FEATURE_LAMBDA           1
#            define CXXKIT_CC_FEATURE_RVALUE_REFS      1
#            define CXXKIT_CC_FEATURE_STATIC_ASSERT    1
#            define CXXKIT_CC_FEATURE_VARIADIC_MACROS  1
#        endif
#        if __INTEL_COMPILER >= 1210
#            define CXXKIT_CC_FEATURE_ATTRIBUTES     1
#            define CXXKIT_CC_FEATURE_AUTO_FUNCTION  1
#            define CXXKIT_CC_FEATURE_NULLPTR        1
#            define CXXKIT_CC_FEATURE_TEMPLATE_ALIAS 1
#            ifndef _CHAR16T // MSVC headers
#                define CXXKIT_CC_FEATURE_UNICODE_STRINGS 1
#            endif
#            define CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES 1
#        endif
#        if __INTEL_COMPILER >= 1300
#            define CXXKIT_CC_FEATURE_ATOMICS
//#define CXXKIT_CC_FEATURE_CONSTEXPR 1 // constexpr support is only partial
#            define CXXKIT_CC_FEATURE_INITIALIZER_LISTS 1
#            define CXXKIT_CC_FEATURE_UNIFORM_INIT      1
#            define CXXKIT_CC_FEATURE_NOEXCEPT          1
#        endif
#        if __INTEL_COMPILER >= 1400
//#define CXXKIT_CC_FEATURE_CONSTEXPR 1 // constexpr support is only partial
#            define CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS 1
#            define CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS    1
#            define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES      1
#            define CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT   1
#            define CXXKIT_CC_FEATURE_RANGE_FOR               1
#            define CXXKIT_CC_FEATURE_RAW_STRINGS             1
#            define CXXKIT_CC_FEATURE_REF_QUALIFIERS          1
#            define CXXKIT_CC_FEATURE_UNICODE_STRINGS         1
#            define CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS     1
#        endif
#        if __INTEL_COMPILER >= 1500
#            if __INTEL_COMPILER * 100 + __INTEL_COMPILER_UPDATE >= 150001
#                define CXXKIT_CC_FEATURE_CONSTEXPR 1 // the bug mentioned above is fixed in 15.0.1
#            endif
#            define CXXKIT_CC_FEATURE_ALIGNAS                 1
#            define CXXKIT_CC_FEATURE_ALIGNOF                 1
#            define CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS 1
#            define CXXKIT_CC_FEATURE_THREAD_LOCAL            1
#            define CXXKIT_CC_FEATURE_UDL                     1
#        endif
#    elif defined(__STDC_VERSION__) && __STDC_VERSION__ > 199901L
// C11 features supported. Only tested with ICC 17 and up.
#        define CXXKIT_CC_FEATURE_STATIC_ASSERT 1
#        if __has_include(<threads.h>)
#            define CXXKIT_CC_FEATURE_THREAD_LOCAL 1
#        endif
#    endif
#endif

/*CXXKIT_CC_INTEL*/
#if defined(CXXKIT_CC_INTEL) && !defined(CXXKIT_CC_MSVC)
#    define CXXKIT_CC_FEATURE_RESTRICTED_VLA     1
#    define CXXKIT_CC_FEATURE_VARIADIC_MACROS    1 // C++11 feature supported as an extension in other modes, too
#    define CXXKIT_CC_FEATURE_THREADSAFE_STATICS 1
#    if __INTEL_COMPILER < 1200
#        define CXXKIT_CC_FEATURE_NO_TEMPLATE_FRIENDS 1
#    endif
#    if __INTEL_COMPILER >= 1310 && !defined(_WIN32)
//    ICC supports C++14 binary literals in C, C++98, and C++11 modes
//    at least since 13.1, but I can't test further back
#        define CXXKIT_CC_FEATURE_BINARY_LITERALS 1
#    endif
#    if __cplusplus >= 201103L || defined(__INTEL_CXX11_MODE__)
#        if __INTEL_COMPILER >= 1200
#            define CXXKIT_CC_FEATURE_AUTO_TYPE        1
#            define CXXKIT_CC_FEATURE_CLASS_ENUM       1
#            define CXXKIT_CC_FEATURE_DECLTYPE         1
#            define CXXKIT_CC_FEATURE_DEFAULT_MEMBERS  1
#            define CXXKIT_CC_FEATURE_DELETE_MEMBERS   1
#            define CXXKIT_CC_FEATURE_EXTERN_TEMPLATES 1
#            define CXXKIT_CC_FEATURE_LAMBDA           1
#            define CXXKIT_CC_FEATURE_RVALUE_REFS      1
#            define CXXKIT_CC_FEATURE_STATIC_ASSERT    1
#            define CXXKIT_CC_FEATURE_VARIADIC_MACROS  1
#        endif
#        if __INTEL_COMPILER >= 1210
#            define CXXKIT_CC_FEATURE_ATTRIBUTES     1
#            define CXXKIT_CC_FEATURE_AUTO_FUNCTION  1
#            define CXXKIT_CC_FEATURE_NULLPTR        1
#            define CXXKIT_CC_FEATURE_TEMPLATE_ALIAS 1
#            ifndef _CHAR16T // MSVC headers
#                define CXXKIT_CC_FEATURE_UNICODE_STRINGS 1
#            endif
#            define CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES 1
#        endif
#        if __INTEL_COMPILER >= 1300
#            define CXXKIT_CC_FEATURE_ATOMICS
//#define CXXKIT_CC_FEATURE_CONSTEXPR 1 // constexpr support is only partial
#            define CXXKIT_CC_FEATURE_INITIALIZER_LISTS 1
#            define CXXKIT_CC_FEATURE_UNIFORM_INIT      1
#            define CXXKIT_CC_FEATURE_NOEXCEPT          1
#        endif
#        if __INTEL_COMPILER >= 1400
//#define CXXKIT_CC_FEATURE_CONSTEXPR 1
#            define CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS 1
#            define CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS    1
#            define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES      1
#            define CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT   1
#            define CXXKIT_CC_FEATURE_RANGE_FOR               1
#            define CXXKIT_CC_FEATURE_RAW_STRINGS             1
#            define CXXKIT_CC_FEATURE_REF_QUALIFIERS          1
#            define CXXKIT_CC_FEATURE_UNICODE_STRINGS         1
#            define CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS     1
#        endif
#        if __INTEL_COMPILER >= 1500
#            if __INTEL_COMPILER * 100 + __INTEL_COMPILER_UPDATE >= 150001
#                define CXXKIT_CC_FEATURE_CONSTEXPR 1 //the bug mentioned above is fixed in 15.0.1
#            endif
#            define CXXKIT_CC_FEATURE_ALIGNAS                 1
#            define CXXKIT_CC_FEATURE_ALIGNOF                 1
#            define CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS 1
#            define CXXKIT_CC_FEATURE_THREAD_LOCAL            1
#            define CXXKIT_CC_FEATURE_UDL                     1
#        endif
#    elif defined(__STDC_VERSION__) && __STDC_VERSION__ > 199901L
//   C11 features supported. Only tested with ICC 17 and up.
#        define CXXKIT_CC_FEATURE_STATIC_ASSERT 1
#        if __has_include(<threads.h>)
#            define CXXKIT_CC_FEATURE_THREAD_LOCAL 1
#        endif
#    endif
#endif

#ifndef CXXKIT_CC_FEATURE_RESTRICTED_VLA
#    define CXXKIT_CC_FEATURE_RESTRICTED_VLA 0
#endif
#ifndef CXXKIT_CC_FEATURE_THREADSAFE_STATICS
#    define CXXKIT_CC_FEATURE_THREADSAFE_STATICS 0
#endif
#ifndef CXXKIT_CC_FEATURE_NO_TEMPLATE_FRIENDS
#    define CXXKIT_CC_FEATURE_NO_TEMPLATE_FRIENDS 0
#endif
#ifndef CXXKIT_CC_FEATURE_BINARY_LITERALS
#    define CXXKIT_CC_FEATURE_BINARY_LITERALS 0
#endif
#ifndef CXXKIT_CC_FEATURE_AUTO_TYPE
#    define CXXKIT_CC_FEATURE_AUTO_TYPE 0
#endif
#ifndef CXXKIT_CC_FEATURE_DECLTYPE
#    define CXXKIT_CC_FEATURE_DECLTYPE 0
#endif
#ifndef CXXKIT_CC_FEATURE_ALIGNAS
#    define CXXKIT_CC_FEATURE_ALIGNAS 0
#endif
#ifndef CXXKIT_CC_FEATURE_ALIGNAS
#    define CXXKIT_CC_FEATURE_ALIGNAS 0
#endif
#ifndef CXXKIT_CC_FEATURE_ALIGNOF
#    define CXXKIT_CC_FEATURE_ALIGNOF 0
#endif
#ifndef CXXKIT_CC_FEATURE_ATOMICS
#    define CXXKIT_CC_FEATURE_ATOMICS 0
#endif
#ifndef CXXKIT_CC_FEATURE_ATTRIBUTES
#    define CXXKIT_CC_FEATURE_ATTRIBUTES 0
#endif
#ifndef CXXKIT_CC_FEATURE_AUTO_FUNCTION
#    define CXXKIT_CC_FEATURE_AUTO_FUNCTION 0
#endif
#ifndef CXXKIT_CC_FEATURE_CLASS_ENUM
#    define CXXKIT_CC_FEATURE_CLASS_ENUM 0
#endif
#ifndef CXXKIT_CC_FEATURE_DEFAULT_MEMBERS
#    define CXXKIT_CC_FEATURE_DEFAULT_MEMBERS 0
#endif
#ifndef CXXKIT_CC_FEATURE_DELETE_MEMBERS
#    define CXXKIT_CC_FEATURE_DELETE_MEMBERS 0
#endif
#ifndef CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS
#    define CXXKIT_CC_FEATURE_DELEGATING_CONSTRUCTORS 0
#endif
#ifndef CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS
#    define CXXKIT_CC_FEATURE_EXPLICIT_CONVERSIONS 0
#endif
#ifndef CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES
#    define CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES 0
#endif
#ifndef CXXKIT_CC_FEATURE_EXTERN_TEMPLATES
#    define CXXKIT_CC_FEATURE_EXTERN_TEMPLATES 0
#endif
#ifndef CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS
#    define CXXKIT_CC_FEATURE_INHERITING_CONSTRUCTORS 0
#endif
#ifndef CXXKIT_CC_FEATURE_INITIALIZER_LISTS
#    define CXXKIT_CC_FEATURE_INITIALIZER_LISTS 0
#endif
#ifndef CXXKIT_CC_FEATURE_LAMBDA
#    define CXXKIT_CC_FEATURE_LAMBDA 0
#endif
#ifndef CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT
#    define CXXKIT_CC_FEATURE_NONSTATIC_MEMBER_INIT 0
#endif
#ifndef CXXKIT_CC_FEATURE_NOEXCEPT
#    define CXXKIT_CC_FEATURE_NOEXCEPT 0
#endif
#ifndef CXXKIT_CC_FEATURE_NULLPTR
#    define CXXKIT_CC_FEATURE_NULLPTR 0
#endif
#ifndef CXXKIT_CC_FEATURE_CONSTEXPR
#    define CXXKIT_CC_FEATURE_CONSTEXPR 0
#endif
#ifndef CXXKIT_CC_FEATURE_RANGE_FOR
#    define CXXKIT_CC_FEATURE_RANGE_FOR 0
#endif
#ifndef CXXKIT_CC_FEATURE_RAW_STRINGS
#    define CXXKIT_CC_FEATURE_RAW_STRINGS 0
#endif
#ifndef CXXKIT_CC_FEATURE_REF_QUALIFIERS
#    define CXXKIT_CC_FEATURE_REF_QUALIFIERS 0
#endif
#ifndef CXXKIT_CC_FEATURE_RVALUE_REFS
#    define CXXKIT_CC_FEATURE_RVALUE_REFS 0
#endif
#ifndef CXXKIT_CC_FEATURE_STATIC_ASSERT
#    define CXXKIT_CC_FEATURE_STATIC_ASSERT 0
#endif
#ifndef CXXKIT_CC_FEATURE_TEMPLATE_ALIAS
#    define CXXKIT_CC_FEATURE_TEMPLATE_ALIAS 0
#endif
#ifndef CXXKIT_CC_FEATURE_THREAD_LOCAL
#    define CXXKIT_CC_FEATURE_THREAD_LOCAL 0
#endif
#ifndef CXXKIT_CC_FEATURE_UDL
#    define CXXKIT_CC_FEATURE_UDL 0
#endif
#ifndef CXXKIT_CC_FEATURE_UNICODE_STRINGS
#    define CXXKIT_CC_FEATURE_UNICODE_STRINGS 0
#endif
#ifndef CXXKIT_CC_FEATURE_UNIFORM_INIT
#    define CXXKIT_CC_FEATURE_UNIFORM_INIT 0
#endif
#ifndef CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS
#    define CXXKIT_CC_FEATURE_UNRESTRICTED_UNIONS 0
#endif
#ifndef CXXKIT_CC_FEATURE_VARIADIC_MACROS
#    define CXXKIT_CC_FEATURE_VARIADIC_MACROS 0
#endif
#ifndef CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES
#    define CXXKIT_CC_FEATURE_VARIADIC_TEMPLATES 0
#endif
#ifndef CXXKIT_CC_FEATURE_BUILTIN_SOURCE_LOCATION
// __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE()
#    define CXXKIT_CC_FEATURE_BUILTIN_SOURCE_LOCATION 0
#endif
#ifndef CXXKIT_CC_FEATURE_BUILTIN_COLUMN
// __builtin_COLUMN()
#    define CXXKIT_CC_FEATURE_BUILTIN_COLUMN 0
#endif
#if defined(__cpp_deduction_guides)
#    define CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION 1
#else
#    define CXXKIT_CC_FEATURE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION 0
#endif


/***********************************************************************************************************************
    cxxkit compiler Warning/diagnostic handling macro define
***********************************************************************************************************************/
#define CXXKIT_DO_PRAGMA(text) _Pragma(#text)
#if defined(CXXKIT_CC_INTEL) && defined(CXXKIT_CC_MSVC)
/* icl.exe: Intel compiler on Windows */
#    undef CXXKIT_DO_PRAGMA /* not needed */
#    define CXXKIT_WARNING_PUSH __pragma(warning(push))
#    define CXXKIT_WARNING_POP  __pragma(warning(pop))
#    define CXXKIT_WARNING_DISABLE_MSVC(number)
#    define CXXKIT_WARNING_DISABLE_INTEL(number) __pragma(warning(disable : number))
#    define CXXKIT_WARNING_DISABLE_CLANG(text)
#    define CXXKIT_WARNING_DISABLE_GCC(text)
#    define CXXKIT_WARNING_DISABLE_DEPRECATED    CXXKIT_WARNING_DISABLE_INTEL(1478 1786)
#    define CXXKIT_WARNING_DISABLE_FLOAT_COMPARE CXXKIT_WARNING_DISABLE_INTEL(1572)
#    define CXXKIT_WARNING_DISABLE_INVALID_OFFSETOF
#elif defined(CXXKIT_CC_INTEL)
/* icc: Intel compiler on Linux or OS X */
#    define CXXKIT_WARNING_PUSH                  CXXKIT_DO_PRAGMA(warning(push))
#    define CXXKIT_WARNING_POP                   CXXKIT_DO_PRAGMA(warning(pop))
#    define CXXKIT_WARNING_DISABLE_INTEL(number) CXXKIT_DO_PRAGMA(warning(disable : number))
#    define CXXKIT_WARNING_DISABLE_MSVC(number)
#    define CXXKIT_WARNING_DISABLE_CLANG(text)
#    define CXXKIT_WARNING_DISABLE_GCC(text)
#    define CXXKIT_WARNING_DISABLE_DEPRECATED    CXXKIT_WARNING_DISABLE_INTEL(1478 1786)
#    define CXXKIT_WARNING_DISABLE_FLOAT_COMPARE CXXKIT_WARNING_DISABLE_INTEL(1572)
#    define CXXKIT_WARNING_DISABLE_INVALID_OFFSETOF
#elif defined(CXXKIT_CC_MSVC) && !defined(CXXKIT_CC_CLANG)
#    undef CXXKIT_DO_PRAGMA /* not needed */
#    define CXXKIT_WARNING_PUSH                 __pragma(warning(push))
#    define CXXKIT_WARNING_POP                  __pragma(warning(pop))
#    define CXXKIT_WARNING_DISABLE_MSVC(number) __pragma(warning(disable : number))
#    define CXXKIT_WARNING_DISABLE_INTEL(number)
#    define CXXKIT_WARNING_DISABLE_CLANG(text)
#    define CXXKIT_WARNING_DISABLE_GCC(text)
#    define CXXKIT_WARNING_DISABLE_DEPRECATED CXXKIT_WARNING_DISABLE_MSVC(4996)
#    define CXXKIT_WARNING_DISABLE_FLOAT_COMPARE
#    define CXXKIT_WARNING_DISABLE_INVALID_OFFSETOF
#elif defined(CXXKIT_CC_CLANG)
#    define CXXKIT_WARNING_PUSH                CXXKIT_DO_PRAGMA(clang diagnostic push)
#    define CXXKIT_WARNING_POP                 CXXKIT_DO_PRAGMA(clang diagnostic pop)
#    define CXXKIT_WARNING_DISABLE_CLANG(text) CXXKIT_DO_PRAGMA(clang diagnostic ignored text)
#    define CXXKIT_WARNING_DISABLE_GCC(text)
#    define CXXKIT_WARNING_DISABLE_INTEL(number)
#    define CXXKIT_WARNING_DISABLE_MSVC(number)
#    define CXXKIT_WARNING_DISABLE_DEPRECATED       CXXKIT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
#    define CXXKIT_WARNING_DISABLE_FLOAT_COMPARE    CXXKIT_WARNING_DISABLE_CLANG("-Wfloat-equal")
#    define CXXKIT_WARNING_DISABLE_INVALID_OFFSETOF CXXKIT_WARNING_DISABLE_CLANG("-Winvalid-offsetof")
#elif defined(CXXKIT_CC_GNU) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406)
#    define CXXKIT_WARNING_PUSH              CXXKIT_DO_PRAGMA(GCC diagnostic push)
#    define CXXKIT_WARNING_POP               CXXKIT_DO_PRAGMA(GCC diagnostic pop)
#    define CXXKIT_WARNING_DISABLE_GCC(text) CXXKIT_DO_PRAGMA(GCC diagnostic ignored text)
#    define CXXKIT_WARNING_DISABLE_CLANG(text)
#    define CXXKIT_WARNING_DISABLE_INTEL(number)
#    define CXXKIT_WARNING_DISABLE_MSVC(number)
#    define CXXKIT_WARNING_DISABLE_DEPRECATED       CXXKIT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
#    define CXXKIT_WARNING_DISABLE_FLOAT_COMPARE    CXXKIT_WARNING_DISABLE_GCC("-Wfloat-equal")
#    define CXXKIT_WARNING_DISABLE_INVALID_OFFSETOF CXXKIT_WARNING_DISABLE_GCC("-Winvalid-offsetof")
#else // All other compilers, GCC < 4.6 and MSVC < 2008
#    define CXXKIT_WARNING_DISABLE_GCC(text)
#    define CXXKIT_WARNING_PUSH
#    define CXXKIT_WARNING_POP
#    define CXXKIT_WARNING_DISABLE_INTEL(number)
#    define CXXKIT_WARNING_DISABLE_MSVC(number)
#    define CXXKIT_WARNING_DISABLE_CLANG(text)
#    define CXXKIT_WARNING_DISABLE_GCC(text)
#    define CXXKIT_WARNING_DISABLE_DEPRECATED
#    define CXXKIT_WARNING_DISABLE_FLOAT_COMPARE
#endif

#ifndef CXXKIT_IGNORE_DEPRECATIONS
#    define CXXKIT_IGNORE_DEPRECATIONS(statement)                                                                        \
        CXXKIT_WARNING_PUSH                                                                                              \
        CXXKIT_WARNING_DISABLE_DEPRECATED                                                                                \
        statement CXXKIT_WARNING_POP
#endif

#endif // _CXXKIT_COMPILER_HPP