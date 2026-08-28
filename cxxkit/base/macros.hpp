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

#include <cxxkit/base/system.hpp>
#include <cxxkit/base/compiler.hpp>

#include <cstddef>
#include <cstring>

/***********************************************************************************************************************
 * version macro
***********************************************************************************************************************/
/** @brief Version number macros: `CXXKIT_VERSION` and `CXXKIT_VERSION_CHECK(major, minor, patch)`.
 * @see CXXKIT_VERSION_MAJOR, CXXKIT_VERSION_MINOR, CXXKIT_VERSION_PATCH
 */
// CXXKIT_VERSION is (major << 16) + (minor << 8) + patch.
#define CXXKIT_VERSION CXXKIT_VERSION_CHECK(CXXKIT_VERSION_MAJOR, CXXKIT_VERSION_MINOR, CXXKIT_VERSION_PATCH)
// can be used like #if (CXXKIT_VERSION >= CXXKIT_VERSION_CHECK(0, 3, 1))
#define CXXKIT_VERSION_CHECK(major, minor, patch) ((major << 16) | (minor << 8) | (patch))


/***********************************************************************************************************************
 * namespace macro
***********************************************************************************************************************/
#define CXXKIT_NAMESPACE               cxxkit
#define CXXKIT_PREPEND_NAMESPACE(name) ::CXXKIT_NAMESPACE::name
#define CXXKIT_USE_NAMESPACE           using namespace ::CXXKIT_NAMESPACE;
/** @brief Namespace macros: `CXXKIT_BEGIN_NAMESPACE` / `CXXKIT_END_NAMESPACE` and friends.
 * @see CXXKIT_NAMESPACE, CXXKIT_PREPEND_NAMESPACE, CXXKIT_FORWARD_DECLARE_CLASS
 */
#define CXXKIT_BEGIN_NAMESPACE                                                                                         \
    CXXKIT_WARNING_PUSH CXXKIT_WARNING_DISABLE_MSVC(4251) namespace CXXKIT_NAMESPACE                                   \
    {
#define CXXKIT_END_NAMESPACE                                                                                           \
    }                                                                                                                  \
    CXXKIT_WARNING_POP
#define CXXKIT_BEGIN_INCLUDE_NAMESPACE }
#define CXXKIT_END_INCLUDE_NAMESPACE                                                                                   \
    namespace CXXKIT_NAMESPACE                                                                                         \
    {
#define CXXKIT_FORWARD_DECLARE_CLASS(name)                                                                             \
    CXXKIT_BEGIN_NAMESPACE class name;                                                                                 \
    CXXKIT_END_NAMESPACE                                                                                               \
    using CXXKIT_PREPEND_NAMESPACE(name);

#define CXXKIT_FORWARD_DECLARE_STRUCT(name)                                                                            \
    CXXKIT_BEGIN_NAMESPACE struct name;                                                                                \
    CXXKIT_END_NAMESPACE                                                                                               \
    using CXXKIT_PREPEND_NAMESPACE(name);

#define CXXKIT_MANGLE_NAMESPACE0(x)    x
#define CXXKIT_MANGLE_NAMESPACE1(a, b) a##_##b
#define CXXKIT_MANGLE_NAMESPACE2(a, b) CXXKIT_MANGLE_NAMESPACE1(a, b)
#define CXXKIT_MANGLE_NAMESPACE(name)                                                                                  \
    CXXKIT_MANGLE_NAMESPACE2(CXXKIT_MANGLE_NAMESPACE0(name), CXXKIT_MANGLE_NAMESPACE0(CXXKIT_NAMESPACE))

namespace CXXKIT_NAMESPACE
{
} // namespace CXXKIT_NAMESPACE


/***********************************************************************************************************************
 * compiler cxx11 feature macro declare
***********************************************************************************************************************/
/** @brief C++ feature-ladder macros: `CXXKIT_NULLPTR`, `CXXKIT_CONSTEXPR`, `CXXKIT_CXX14_CONSTEXPR`,
 * `CXXKIT_CXX17_CONSTEXPR`, `CXXKIT_OVERRIDE`, `CXXKIT_FINAL`, `CXXKIT_NOEXCEPT`, `CXXKIT_ALIGN`, `CXXKIT_ALIGNOF`,
 * `CXXKIT_EQ_DEFAULT`, `CXXKIT_EQ_DELETE`.
 * @see cxxkit::base::compiler.hpp (CXXKIT_CC_FEATURE_*)
 */
#if CXXKIT_CC_FEATURE_NULLPTR
#    define CXXKIT_NULLPTR nullptr
#else
#    define CXXKIT_NULLPTR NULL
#endif

#if CXXKIT_CC_FEATURE_CONSTEXPR
#    define CXXKIT_CONSTEXPR          constexpr
#    define CXXKIT_CONSTEXPR_OR_CONST constexpr
#else
#    define CXXKIT_CONSTEXPR
#    define CXXKIT_CONSTEXPR_OR_CONST const
#endif
#if CXXKIT_CC_CPP14_OR_GREATER
#    define CXXKIT_CXX14_CONSTEXPR constexpr
#else
#    define CXXKIT_CXX14_CONSTEXPR
#endif
#if CXXKIT_CC_CPP17_OR_GREATER
#    define CXXKIT_CXX17_CONSTEXPR constexpr
#else
#    define CXXKIT_CXX17_CONSTEXPR
#endif
#if CXXKIT_CC_CPP20_OR_GREATER
#    define CXXKIT_CXX20_CONSTEXPR constexpr
#else
#    define CXXKIT_CXX20_CONSTEXPR
#endif
#if CXXKIT_CC_CPP23_OR_GREATER
#    define CXXKIT_CXX23_CONSTEXPR constexpr
#else
#    define CXXKIT_CXX23_CONSTEXPR
#endif

#if CXXKIT_CC_FEATURE_EXPLICIT_OVERRIDES
#    define CXXKIT_OVERRIDE override
#    define CXXKIT_FINAL    final
#else
#    define CXXKIT_OVERRIDE
#    define CXXKIT_FINAL
#endif

#if CXXKIT_CC_FEATURE_NOEXCEPT
#    define CXXKIT_NOEXCEPT         noexcept
#    define CXXKIT_NOEXCEPT_EXPR(x) noexcept(x)
#else
#    define CXXKIT_NOEXCEPT
#    define CXXKIT_NOEXCEPT_EXPR(x)
#endif
#define CXXKIT_NOTHROW CXXKIT_NOEXCEPT

#if CXXKIT_CC_FEATURE_DEFAULT_MEMBERS
#    define CXXKIT_EQ_DEFAULT      = default
#    define CXXKIT_EQ_DEFAULT_FUNC = default;
#else
#    define CXXKIT_EQ_DEFAULT
#    define CXXKIT_EQ_DEFAULT_FUNC                                                                                     \
        {                                                                                                              \
        }
#endif

#if CXXKIT_CC_FEATURE_DELETE_MEMBERS
#    define CXXKIT_EQ_DELETE      = delete
#    define CXXKIT_EQ_DELETE_FUNC = delete;
#else
#    define CXXKIT_EQ_DELETE
#    define CXXKIT_EQ_DELETE_FUNC                                                                                      \
        {                                                                                                              \
        }
#endif

#if CXXKIT_CC_FEATURE_ALIGNOF
#    define CXXKIT_ALIGNOF(x) alignof(x)
#else
#    define CXXKIT_ALIGNOF(x)
#endif

#if CXXKIT_CC_FEATURE_ALIGNAS
#    define CXXKIT_ALIGN(n) alignas(n)
#else
#    define CXXKIT_ALIGN(n)
#endif


/** @brief Copy/move inhibition macros: `CXXKIT_DECLARE_DISABLE_COPY`, `CXXKIT_DECLARE_DISABLE_MOVE`, `CXXKIT_DISABLE_COPY_MOVE`.
 */
/***********************************************************************************************************************
  * disable copy move macro declare
***********************************************************************************************************************/
#define CXXKIT_DECLARE_DISABLE_COPY(Class)                                                                             \
    Class(const Class &) CXXKIT_EQ_DELETE;                                                                             \
    Class &operator=(const Class &) CXXKIT_EQ_DELETE;

#if CXXKIT_CC_FEATURE_RVALUE_REFS
#    define CXXKIT_DECLARE_DISABLE_MOVE(Class)                                                                         \
        Class(Class &&) CXXKIT_EQ_DELETE;                                                                              \
        Class &operator=(Class &&) CXXKIT_EQ_DELETE;
#else
#    define CXXKIT_DECLARE_DISABLE_MOVE(Class)
#endif

#define CXXKIT_DISABLE_COPY_MOVE(Class)                                                                                \
    CXXKIT_DECLARE_DISABLE_COPY(Class)                                                                                 \
    CXXKIT_DECLARE_DISABLE_MOVE(Class)


/** @brief Static-constant macros: `CXXKIT_STATIC_CONSTANT_NUMBER`, `CXXKIT_STATIC_CONSTANT_STRING`.
 * @see CXXKIT_STATIC_CONSTANT_NUMBER
 */
/***********************************************************************************************************************
  * static variable macro
***********************************************************************************************************************/
#if CXXKIT_BUILD_CXX_STANDARD_11
#    define CXXKIT_STATIC_CONSTANT_NUMBER(name, number)                                                                \
        CXXKIT_WARNING_PUSH                                                                                            \
        CXXKIT_WARNING_DISABLE_CLANG("-Wdeprecated-anon-enum-enum-conversion")                                         \
        enum : decltype(number)                                                                                        \
        {                                                                                                              \
            name = static_cast<decltype(number)>(number)                                                               \
        };                                                                                                             \
        CXXKIT_WARNING_POP
#else
#    define CXXKIT_STATIC_CONSTANT_NUMBER(name, number) static constexpr decltype(number) name = number;
#endif
#define CXXKIT_STATIC_CONSTANT_STRING(name, string) static constexpr char name[] = string;


/** @brief Private-implementation (d-pointer) macros: `CXXKIT_DEFINE_DPTR`, `CXXKIT_DECLARE_PRIVATE`, `CXXKIT_D`,
 * plus p-pointer mirrors `CXXKIT_DEFINE_PPTR`, `CXXKIT_DECLARE_PUBLIC`, `CXXKIT_P`.
 * @see CXXKIT_CAST_IGNORE_ALIGN, CXXKIT_DECLARE_PRIVATE_D, CXXKIT_DECLARE_PUBLIC_P
 */
/***********************************************************************************************************************
 * class private implementation macro
***********************************************************************************************************************/
CXXKIT_BEGIN_NAMESPACE
template <typename T>
inline T *get_pointer_helper(T *ptr)
{
    return ptr;
}
template <typename Wrapper>
static inline typename Wrapper::pointer get_pointer_helper(const Wrapper &p)
{
    return p.get();
}
template <typename Wrapper>
static inline typename Wrapper::Pointer get_pointer_helper(const Wrapper &p)
{
    return p.data();
}
CXXKIT_END_NAMESPACE

// The body must be a statement:
#define CXXKIT_CAST_IGNORE_ALIGN(body)                                                                                 \
    CXXKIT_WARNING_PUSH                                                                                                \
    CXXKIT_WARNING_DISABLE_GCC("-Wcast-align")                                                                         \
    body CXXKIT_WARNING_POP

#define CXXKIT_DEFINE_DPTR(Class) std::unique_ptr<Class##Private> mDPtr;

#define CXXKIT_DECLARE_PRIVATE(Class)                                                                                  \
    inline Class##Private *d_func()                                                                                     \
    {                                                                                                                  \
        CXXKIT_CAST_IGNORE_ALIGN(return reinterpret_cast<Class##Private *>(cxxkit::get_pointer_helper(mDPtr));)          \
    }                                                                                                                  \
    inline const Class##Private *d_func() const                                                                         \
    {                                                                                                                  \
        CXXKIT_CAST_IGNORE_ALIGN(return reinterpret_cast<const Class##Private *>(cxxkit::get_pointer_helper(mDPtr));)    \
    }                                                                                                                  \
    friend class Class##Private;

#define CXXKIT_DECLARE_PRIVATE_D(DPtr, Class)                                                                          \
    inline Class##Private *d_func()                                                                                     \
    {                                                                                                                  \
        CXXKIT_CAST_IGNORE_ALIGN(return reinterpret_cast<Class##Private *>(cxxkit::get_pointer_helper(DPtr));)           \
    }                                                                                                                  \
    inline const Class##Private *d_func() const                                                                         \
    {                                                                                                                  \
        CXXKIT_CAST_IGNORE_ALIGN(return reinterpret_cast<const Class##Private *>(cxxkit::get_pointer_helper(DPtr));)     \
    }                                                                                                                  \
    friend class Class##Private;

#define CXXKIT_DEFINE_PPTR(Class) Class *const mPPtr;

#define CXXKIT_DECLARE_PUBLIC(Class)                                                                                   \
    inline Class *p_func()                                                                                              \
    {                                                                                                                  \
        return static_cast<Class *>(mPPtr);                                                                            \
    }                                                                                                                  \
    inline const Class *p_func() const                                                                                  \
    {                                                                                                                  \
        return static_cast<const Class *>(mPPtr);                                                                      \
    }                                                                                                                  \
    friend class Class;

#define CXXKIT_DECLARE_PUBLIC_P(PPtr, Class)                                                                           \
    inline Class *p_func()                                                                                              \
    {                                                                                                                  \
        return static_cast<Class *>(PPtr);                                                                             \
    }                                                                                                                  \
    inline const Class *p_func() const                                                                                  \
    {                                                                                                                  \
        return static_cast<const Class *>(PPtr);                                                                       \
    }                                                                                                                  \
    friend class Class;

#define CXXKIT_D(Class) Class##Private *const d = d_func()
#define CXXKIT_P(Class) Class *const p = p_func()


/** @brief Force-inline / no-inline / used attribute macros: `CXXKIT_FORCE_INLINE`, `CXXKIT_NO_INLINE`, `CXXKIT_USED`.
 */
/***********************************************************************************************************************
 * cxxkit force inline macro declare
***********************************************************************************************************************/
#if defined(CXXKIT_CC_MSVC)
#    define CXXKIT_FORCE_INLINE __forceinline
#    define CXXKIT_NO_INLINE    __declspec(noinline)
#    define CXXKIT_USED
#elif defined(CXXKIT_CC_GNU)
#    define CXXKIT_FORCE_INLINE inline __attribute__((always_inline))
#    define CXXKIT_NO_INLINE    __attribute__((noinline))
#    define CXXKIT_USED         __attribute__((used))
#elif defined(CXXKIT_CC_CLANG)
#    define CXXKIT_FORCE_INLINE inline __attribute__((always_inline))
#    define CXXKIT_NO_INLINE
#    define CXXKIT_USED __attribute__((used))
#else
#    define CXXKIT_FORCE_INLINE inline
#    define CXXKIT_NO_INLINE
#    define CXXKIT_USED
#endif


/** @brief Noreturn annotation macro: `CXXKIT_NORETURN`.
 */
/***********************************************************************************************************************
 * noreturn macro declare, annotate a function that will not return control flow to the caller.
***********************************************************************************************************************/
#if defined(CXXKIT_CC_MSVC)
#    define CXXKIT_NORETURN __declspec(noreturn)
#elif defined(CXXKIT_CC_GNU) || defined(CXXKIT_CC_CLANG)
#    define CXXKIT_NORETURN __attribute__((__noreturn__))
#else
#    define CXXKIT_NORETURN
#endif


/** @brief Path slash macro: `CXXKIT_PATH_SLASH` (platform-specific).
 */
/***********************************************************************************************************************
 * provide a path slash macro
***********************************************************************************************************************/
#if defined(CXXKIT_OS_WIN)
#    define CXXKIT_PATH_SLASH '\\'
#else
#    define CXXKIT_PATH_SLASH '/'
#endif


/** @brief 4CC / 8CC codec identifier macros: `CXXKIT_FOURCC`, `CXXKIT_EIGHTCC`.
 */
/***********************************************************************************************************************
 * number cc macro
***********************************************************************************************************************/
#define CXXKIT_FOURCC(c1, c2, c3, c4)                                                                                  \
    ((static_cast<uint32_t>(c1)) | (static_cast<uint32_t>(c2) << 8) | (static_cast<uint32_t>(c3) << 16) | /* NOLINT */ \
     (static_cast<uint32_t>(c4) << 24))                                                                   /* NOLINT */

#define CXXKIT_EIGHTCC(c1, c2, c3, c4, c5, c6, c7, c8)                                                                 \
    ((static_cast<uint64_t>(c1)) | (static_cast<uint64_t>(c2) << 8) | (static_cast<uint64_t>(c3) << 16) | /* NOLINT */ \
     (static_cast<uint64_t>(c4) << 24) | (static_cast<uint64_t>(c5) << 32) | (static_cast<uint64_t>(c6) << 40) |       \
     (static_cast<uint64_t>(c7) << 48) | (static_cast<uint64_t>(c8) << 56)) /* NOLINT */


/** @brief Exceptions-enabled detection macro: `CXXKIT_HAS_EXCEPTIONS` (0 or 1).
 */
/***********************************************************************************************************************
 * set exceptions flag macro
***********************************************************************************************************************/
#if defined(__cpp_exceptions) || defined(_CPPUNWIND) || defined(__EXCEPTIONS)
#    define CXXKIT_HAS_EXCEPTIONS 1
#else
#    define CXXKIT_HAS_EXCEPTIONS 0
#endif


/** @brief Source-location macros: `CXXKIT_STRFUNC`, `CXXKIT_STRFILE`, `CXXKIT_STRFILENAME`, `CXXKIT_STRFILELINE`,
 * `CXXKIT_STRFILELINE_W`, `CXXKIT_PATH_NAME`.
 */
/***********************************************************************************************************************
 * provide source location macro
***********************************************************************************************************************/
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define CXXKIT_STRFUNC ((const char *)(__func__))
#elif defined(__GNUC__) && defined(__cplusplus)
#    define CXXKIT_STRFUNC ((const char *)(__PRETTY_FUNCTION__))
#elif defined(__GNUC__) || (defined(_MSC_VER) && (_MSC_VER > 1300))
#    define CXXKIT_STRFUNC ((const char *)(__FUNCTION__))
#elif defined(CXXKIT_CC_MSVC)
#    define CXXKIT_STRFUNC ((const char *)(__FUNCSIG__))
#else
#    define CXXKIT_STRFUNC ((const char *)("???"))
#endif

#define CXXKIT_LINE    __LINE__
#define CXXKIT_STRFILE __FILE__

#define CXXKIT_PATH_NAME(path) (strrchr(path, CXXKIT_PATH_SLASH) ? strrchr(path, CXXKIT_PATH_SLASH) + 1 : path)
#ifdef __FILE_NAME__
#    define CXXKIT_STRFILENAME __FILE_NAME__
#else
#    define CXXKIT_STRFILENAME (strrchr(__FILE__, CXXKIT_PATH_SLASH) ? strrchr(__FILE__, CXXKIT_PATH_SLASH) + 1 : __FILE__))
#endif

#define CXXKIT_STRFILELINE   CXXKIT_STRFILENAME ":" CXXKIT_PP_STRINGIFY(__LINE__)
#define CXXKIT_STRFILELINE_W "(" CXXKIT_STRFILENAME ":" CXXKIT_PP_STRINGIFY(__LINE__) ")"


/** @brief Deprecation macros: `CXXKIT_DEPRECATED`, `CXXKIT_DEPRECATED_X(text)`.
 */
/***********************************************************************************************************************
 * deprecated macro
***********************************************************************************************************************/
#if __has_cpp_attribute(deprecated)
#    define CXXKIT_DEPRECATED         [[deprecated]]
#    define CXXKIT_DEPRECATED_X(text) [[deprecated(text)]]
#elif (CXXKIT_CC_GNU >= 405) || defined(CXXKIT_CC_CLANG) || CXXKIT_CC_HAS_ATTRIBUTE(__deprecated__)
#    define CXXKIT_DEPRECATED         __attribute__((__deprecated__))
#    define CXXKIT_DEPRECATED_X(text) __attribute__((__deprecated__(text)))
#elif defined(CXXKIT_CC_MSVC) && (CXXKIT_CC_MSVC >= 1300)
#    define CXXKIT_DEPRECATED         __declspec(deprecated)
#    define CXXKIT_DEPRECATED_X(text) __declspec(deprecated(text))
#elif defined(CXXKIT_CC_INTEL) && (__INTEL_COMPILER >= 1300) && !defined(__APPLE__)
#    define CXXKIT_DEPRECATED         __attribute__(__deprecated__)
#    define CXXKIT_DEPRECATED_X(text) __attribute__((__deprecated__(text)))
#else
#    define CXXKIT_DEPRECATED
#    define CXXKIT_DEPRECATED_X(text) CXXKIT_DEPRECATED
#endif


/** @brief Utility macros: `CXXKIT_UNUSED`, `CXXKIT_FOREVER`, `CXXKIT_PRAGMA`, `CXXKIT_STRINGIFY`, `CXXKIT_ZERO_INIT`,
 * `CXXKIT_ARRAY_SIZE`.
 */
/***********************************************************************************************************************
 * utils macro
***********************************************************************************************************************/
/* Avoid "unused parameter" warnings */
#define CXXKIT_UNUSED(x) (void)x;

#define CXXKIT_FOREVER for (;;)

/* Pragma keyword */
#if defined(_MSC_VER)
#    define CXXKIT_PRAGMA(X) __pragma(X)
#else
#    define CXXKIT_PRAGMA(X) _Pragma(#X)
#endif

/* Stringify macro or string */
#define CXXKIT_STRINGIFY(macro_or_string) CXXKIT_STRINGIFY_ARG(macro_or_string)
#define CXXKIT_STRINGIFY_ARG(contents)    #contents

#define CXXKIT_ZERO_INIT                                                                                               \
    {                                                                                                                  \
        0                                                                                                              \
    }

// Note: this internal template function declaration is used by ABSL_ARRAYSIZE.
// The function doesn't need a definition, as we only use its type.
template <typename T, size_t N>
auto cxxkit_array_size_helper(const T (&array)[N]) -> char (&)[N];

/**
 * @brief Returns the number of elements in an array as a compile-time constant,
 * which can be used in defining new arrays.
 * If you use this macro on a pointer by mistake, you will get a compile-time error.
 */
#define CXXKIT_ARRAY_SIZE(array) (sizeof(cxxkit_array_size_helper(array)))


/** @brief DLL export/import for static variables: `CXXKIT_EXTERN_VAR`.
 * @see CXXKIT_BUILD_SHARED, CXXKIT_BUILD_CORE_LIB
 */
/***********************************************************************************************************************
 * var exported in windows dlls macro define
***********************************************************************************************************************/
#ifdef CXXKIT_OS_WIN32
#    ifndef CXXKIT_BUILD_SHARED
#        define CXXKIT_EXTERN_VAR extern
#    else /* !CXXKIT_BUILD_SHARED */
#        ifdef CXXKIT_BUILD_CORE_LIB
#            define CXXKIT_EXTERN_VAR extern __declspec(dllexport)
#        else /* !CXXKIT_BUILD_CORE_LIB */
#            define CXXKIT_EXTERN_VAR extern __declspec(dllimport)
#        endif /* !CXXKIT_BUILD_CORE_LIB */
#    endif     /* !CXXKIT_BUILD_SHARED */
#else          /* !CXXKIT_OS_WIN32 */
#    define CXXKIT_EXTERN_VAR extern
#endif /* !CXXKIT_OS_WIN32 */


/** @brief Path/line limit macros: `CXXKIT_PATH_MAX`, `CXXKIT_LINE_MAX`.
 */
/***********************************************************************************************************************
 * limits macro
***********************************************************************************************************************/
/* # chars in a path name including nul */
#ifdef PATH_MAX
#    define CXXKIT_PATH_MAX PATH_MAX
#else
#    define CXXKIT_PATH_MAX (4096)
#endif
#ifdef LINE_MAX
#    define CXXKIT_LINE_MAX LINE_MAX
#else
#    define CXXKIT_LINE_MAX (4096)
#endif


/** @brief RTTI detection macro: `CXXKIT_RTTI_ENABLED` (0 or 1).
 */
/***********************************************************************************************************************
 * rtti macro
***********************************************************************************************************************/
#if defined(__GXX_RTTI) || defined(__cpp_rtti) || defined(_CPPRTTI)
#    define CXXKIT_RTTI_ENABLED 1
#else
#    define CXXKIT_RTTI_ENABLED 0
#endif


/** @brief Explicit template instantiation export macros: `CXXKIT_EXPORT_TEMPLATE_DECLARE`, `CXXKIT_EXPORT_TEMPLATE_DEFINE`.
 * Handles MSVC dllexport-at-definition requirement.
 */
/***********************************************************************************************************************
 * export template macro define
***********************************************************************************************************************/
/**
 * This header provides macros for using CXXKIT_EXPORT macros with explicit template instantiation declarations and
 * definitions.
 * Generally, the CXXKIT_EXPORT macros are used at declarations, and GCC requires them to be used at explicit
 * instantiation declarations, but MSVC requires __declspec(dllexport) to be used at the explicit instantiation
 * definitions instead.
 *
 * Usage
 * In a header file, write:
 *      extern template class CXXKIT_EXPORT_TEMPLATE_DECLARE(CXXKIT_EXPORT) foo<bar>;
 * In a source file, write:
 *      template class CXXKIT_EXPORT_TEMPLATE_DEFINE(CXXKIT_EXPORT) foo<bar>;
 *
 * Implementation notes
 * On Windows, when building when CXXKIT_EXPORT expands to __declspec(dllexport)), we want the two lines to expand to:
 *      extern template class foo<bar>;
 *      template class CXXKIT_EXPORT foo<bar>;
 *
 * In all other cases (non-Windows, and Windows when CXXKIT_EXPORT expands to __declspec(dllimport)), we want:
 *      extern template class CXXKIT_EXPORT foo<bar>;
 *      template class foo<bar>;
 *
 * The implementation of this header uses some subtle macro semantics to detect what the provided CXXKIT_EXPORT value
 * was defined as and then to dispatch to appropriate macro definitions.  Unfortunately, MSVC's C preprocessor is
 * rather non-compliant and requires special care to make it work.
 *
 * Issue 1.
 *      \#define F(x)
 *      F()
 *
 * MSVC emits warning C4003 ("not enough actual parameters for macro 'F'), even though it's a valid macro invocation.
 * This affects the macros below that take just an "export" parameter, because export may be empty.
 * As a workaround, we can add a dummy parameter and arguments:
 *      #define F(x,_)
 *      F(,)
 *
 * Issue 2.
 *      #define F(x) G##x
 *      #define Gj() ok
 *      F(j())
 *
 * The correct replacement for "F(j())" is "ok", but MSVC replaces it with "Gj()".
 * As a workaround, we can pass the result to an identity macro to force MSVC to look for replacements again.
 * (This is why CXXKIT_EXPORT_TEMPLATE_STYLE_3 exists.)
 */
#define CXXKIT_EXPORT_TEMPLATE_DECLARE(export)                                                                         \
    CXXKIT_EXPORT_TEMPLATE_INVOKE(DECLARE, CXXKIT_EXPORT_TEMPLATE_STYLE(export, ), export) // NOLINT
#define CXXKIT_EXPORT_TEMPLATE_DEFINE(export)                                                                          \
    CXXKIT_EXPORT_TEMPLATE_INVOKE(DEFINE, CXXKIT_EXPORT_TEMPLATE_STYLE(export, ), export) // NOLINT

/**
 * INVOKE is an internal helper macro to perform parameter replacements and token pasting to chain invoke another macro.
 * E.g., CXXKIT_EXPORT_TEMPLATE_INVOKE(DECLARE, DEFAULT, CXXKIT_EXPORT) will export to call
 * CXXKIT_EXPORT_TEMPLATE_DECLARE_DEFAULT(CXXKIT_EXPORT, ) (but with CXXKIT_EXPORT expanded too).
 */
#define CXXKIT_EXPORT_TEMPLATE_INVOKE(which, style, export)   CXXKIT_EXPORT_TEMPLATE_INVOKE_2(which, style, export)
#define CXXKIT_EXPORT_TEMPLATE_INVOKE_2(which, style, export) CXXKIT_EXPORT_TEMPLATE_##which##_##style(export, )

// Default style is to apply the CXXKIT_EXPORT macro at declaration sites.
#define CXXKIT_EXPORT_TEMPLATE_DECLARE_DEFAULT(export, _) export
#define CXXKIT_EXPORT_TEMPLATE_DEFINE_DEFAULT(export, _)

// The "MSVC hack" style is used when CXXKIT_EXPORT is defined as __declspec(dllexport), which MSVC requires to be
// used at definition sites instead.
#define CXXKIT_EXPORT_TEMPLATE_DECLARE_MSVC_HACK(export, _)
#define CXXKIT_EXPORT_TEMPLATE_DEFINE_MSVC_HACK(export, _) export

// CXXKIT_EXPORT_TEMPLATE_STYLE is an internal helper macro that identifies which
// export style needs to be used for the provided CXXKIT_EXPORT macro definition.
// "", "__attribute__(...)", and "__declspec(dllimport)" are mapped
// to "DEFAULT"; while "__declspec(dllexport)" is mapped to "MSVC_HACK".
//
// It's implemented with token pasting to transform the __attribute__ and
// __declspec annotations into macro invocations.  E.g., if CXXKIT_EXPORT is
// defined as "__declspec(dllimport)", it undergoes the following sequence of
// macro substitutions:
//     CXXKIT_EXPORT_TEMPLATE_STYLE(CXXKIT_EXPORT,)
//     CXXKIT_EXPORT_TEMPLATE_STYLE_2(__declspec(dllimport),)
//     CXXKIT_EXPORT_TEMPLATE_STYLE_3(
//         CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH__declspec(dllimport))
//     CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH__declspec(dllimport)
//     CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_DECLSPEC_dllimport
//     DEFAULT
#define CXXKIT_EXPORT_TEMPLATE_STYLE(export, _) CXXKIT_EXPORT_TEMPLATE_STYLE_2(export, )
#define CXXKIT_EXPORT_TEMPLATE_STYLE_2(export, _)                                                                      \
    CXXKIT_EXPORT_TEMPLATE_STYLE_3(CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_foj3FJo5StF0OvIzl7oMxA##export)
#define CXXKIT_EXPORT_TEMPLATE_STYLE_3(style) style

// Internal helper macros for CXXKIT_EXPORT_TEMPLATE_STYLE.
//
// XXX: C++ reserves all identifiers containing "__" for the implementation,
// but "__attribute__" and "__declspec" already contain "__" and the token-paste
// operator can only add characters; not remove them.  To minimize the risk of
// conflict with implementations, we include "foj3FJo5StF0OvIzl7oMxA" (a random
// 128-bit string, encoded in Base64) in the macro name.
#define CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_foj3FJo5StF0OvIzl7oMxA                   DEFAULT
#define CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_foj3FJo5StF0OvIzl7oMxA__attribute__(...) DEFAULT
#define CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_foj3FJo5StF0OvIzl7oMxA__declspec(arg)                                       \
    CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_DECLSPEC_##arg

// Internal helper macros for CXXKIT_EXPORT_TEMPLATE_STYLE.
#define CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_DECLSPEC_dllexport MSVC_HACK
#define CXXKIT_EXPORT_TEMPLATE_STYLE_MATCH_DECLSPEC_dllimport DEFAULT

// Sanity checks.
//
// CXXKIT_EXPORT_TEMPLATE_TEST uses the same macro invocation pattern as
// CXXKIT_EXPORT_TEMPLATE_DECLARE and CXXKIT_EXPORT_TEMPLATE_DEFINE do to check that
// they're working correctly. When they're working correctly, the sequence of
// macro replacements should go something like:
//
//     CXXKIT_EXPORT_TEMPLATE_TEST(DEFAULT, __declspec(dllimport));
//
//     static_assert(CXXKIT_EXPORT_TEMPLATE_INVOKE(TEST_DEFAULT,
//         CXXKIT_EXPORT_TEMPLATE_STYLE(__declspec(dllimport), ),
//         __declspec(dllimport)), "__declspec(dllimport)");
//
//     static_assert(CXXKIT_EXPORT_TEMPLATE_INVOKE(TEST_DEFAULT,
//         DEFAULT, __declspec(dllimport)), "__declspec(dllimport)");
//
//     static_assert(CXXKIT_EXPORT_TEMPLATE_TEST_DEFAULT_DEFAULT(
//         __declspec(dllimport)), "__declspec(dllimport)");
//
//     static_assert(true, "__declspec(dllimport)");
//
// When they're not working correctly, a syntax error should occur instead.
#define CXXKIT_EXPORT_TEMPLATE_TEST(want, export)                                                                      \
    static_assert(CXXKIT_EXPORT_TEMPLATE_INVOKE(TEST_##want, CXXKIT_EXPORT_TEMPLATE_STYLE(export, ), export),          \
                  #export) // NOLINT
#define CXXKIT_EXPORT_TEMPLATE_TEST_DEFAULT_DEFAULT(...)     true
#define CXXKIT_EXPORT_TEMPLATE_TEST_MSVC_HACK_MSVC_HACK(...) true

CXXKIT_EXPORT_TEMPLATE_TEST(DEFAULT, ); // NOLINT
CXXKIT_EXPORT_TEMPLATE_TEST(DEFAULT, __attribute__((visibility("default"))));
CXXKIT_EXPORT_TEMPLATE_TEST(MSVC_HACK, __declspec(dllexport));
CXXKIT_EXPORT_TEMPLATE_TEST(DEFAULT, __declspec(dllimport));

#undef CXXKIT_EXPORT_TEMPLATE_TEST
#undef CXXKIT_EXPORT_TEMPLATE_TEST_DEFAULT_DEFAULT
#undef CXXKIT_EXPORT_TEMPLATE_TEST_MSVC_HACK_MSVC_HACK


/** @brief Compiler feature-detection macros: `CXXKIT_CC_HAS_FEATURE`, `CXXKIT_CC_HAS_BUILTIN`, `CXXKIT_CC_HAS_EXTENSION`,
 * `CXXKIT_CC_HAS_ATTRIBUTE`, `CXXKIT_CC_HAS_INCLUDE`, `CXXKIT_CC_HAS_CPP_ATTRIBUTE`.
 */
/***********************************************************************************************************************
 * has feature macro define
***********************************************************************************************************************/
/*
 * Clang feature detection: http://clang.llvm.org/docs/LanguageExtensions.html These are not available on GCC, but since
 * the pre-processor doesn't do operator short-circuiting, we can't use it in a statement or we'll get:
 *
 * error: missing binary operator before token "(" So we define it to 0 to satisfy the pre-processor.
 */
#ifdef __has_feature
#    define CXXKIT_CC_HAS_FEATURE __has_feature
#else
#    define CXXKIT_CC_HAS_FEATURE(x) 0
#endif

#ifdef __has_builtin
#    define CXXKIT_CC_HAS_BUILTIN __has_builtin
#else
#    define CXXKIT_CC_HAS_BUILTIN(x) 0
#endif

#ifdef __has_extension
#    define CXXKIT_CC_HAS_EXTENSION __has_extension
#else
#    define CXXKIT_CC_HAS_EXTENSION(x) 0
#endif

/*
 * Attribute support detection. Works on clang and GCC >= 5
 * https://clang.llvm.org/docs/LanguageExtensions.html#has-attribute
 * https://gcc.gnu.org/onlinedocs/cpp/_005f_005fhas_005fattribute.html
 */
#ifdef __has_attribute
#    define CXXKIT_CC_HAS_ATTRIBUTE __has_attribute
#else
#    define CXXKIT_CC_HAS_ATTRIBUTE(X) 0
#endif

#ifdef __has_include
#    define CXXKIT_CC_HAS_INCLUDE __has_include
#else
#    define CXXKIT_CC_HAS_INCLUDE(X) 0
#endif

/**
 * @brief A function-like feature checking macro that accepts C++11 style attributes.
 * It's a wrapper around `__has_cpp_attribute`, defined by ISO C++ SD-6
 * (https://en.cppreference.com/w/cpp/experimental/feature_test).
 * If we don't find `__has_cpp_attribute`, will evaluate to 0.
 */
#if defined(__cplusplus) && defined(__has_cpp_attribute)
// NOTE: requiring __cplusplus above should not be necessary, but works around https://bugs.llvm.org/show_bug.cgi?id=23435.
#    define CXXKIT_CC_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#    define CXXKIT_CC_HAS_CPP_ATTRIBUTE(x) 0
#endif

/**
 * @brief A variable declaration annotated with the `ABSL_CONST_INIT` attribute will not compile
 * (on supported platforms) unless the variable has a constant initializer.
 * This is useful for variables with static and thread storage duration, because it guarantees that
 * they will not suffer from the so-called "static init order fiasco".
 *
 * This attribute must be placed on the initializing declaration of the variable. Some compilers will
 * give a -Wmissing-constinit warning when this attribute is placed on some other declaration but missing
 * from the initializing declaration.
 *
 * In some cases (notably with thread_local variables), `ABSL_CONST_INIT` can also be used in a non-initializing
 * declaration to tell the compiler that a variable is already initialized, reducing overhead that would otherwise be
 * incurred by a hidden guard variable. Thus annotating all declarations with this attribute is recommended to
 * potentially enhance optimization.
 *
 * Example:
 *
 * class MyClass
 * {
 *      public:
 *      CXXKIT_CONST_INIT static MyType my_var;
 * };
 *
 * CXXKIT_CONST_INIT MyType MyClass::my_var = MakeMyType(...);
 * For code or headers that are assured to only build with C++20 and up, prefer just using the
 * standard `constinit` keyword directly over this macro.
 *
 * Note that this attribute is redundant if the variable is declared constexpr.
 */
#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#    define CXXKIT_CONST_INIT constinit
#elif CXXKIT_CC_HAS_CPP_ATTRIBUTE(clang::require_constant_initialization)
#    define CXXKIT_CONST_INIT [[clang::require_constant_initialization]]
#else
#    define CXXKIT_CONST_INIT
#endif


/** @brief `CXXKIT_CONST_INIT`: guarantees static/thread-local variables have constant initializers.
 */
/***********************************************************************************************************************
 * CONSTRUCTORS DESTRUCTOR macro
***********************************************************************************************************************/
#if defined(__cplusplus)
#    define CXXKIT__CONSTRUCTOR_FUNCTION_WITH_ARGS(_func)                                                              \
        namespace                                                                                                      \
        {                                                                                                              \
        static const struct _func##_ctor_class_                                                                        \
        {                                                                                                              \
            inline _func##_ctor_class_()                                                                               \
            {                                                                                                          \
                _func();                                                                                               \
            }                                                                                                          \
        } _func##_ctor_instance_;                                                                                      \
        }
#    define CXXKIT_CONSTRUCTOR_FUNCTION(_func) CXXKIT__CONSTRUCTOR_FUNCTION_WITH_ARGS(_func)
#    define CXXKIT__DESTRUCTOR_FUNCTION_WITH_ARGS(_func)                                                               \
        namespace                                                                                                      \
        {                                                                                                              \
        static const struct _func##_dtor_class_                                                                        \
        {                                                                                                              \
            inline _func##_dtor_class_()                                                                               \
            {                                                                                                          \
            }                                                                                                          \
            inline ~_func##_dtor_class_()                                                                              \
            {                                                                                                          \
                _func();                                                                                               \
            }                                                                                                          \
        } _func##_dtor_instance_;                                                                                      \
        }
#    define CXXKIT_DESTRUCTOR_FUNCTION(_func) CXXKIT__DESTRUCTOR_FUNCTION_WITH_ARGS(_func)
#else
#    if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#        define CXXKIT_CONSTRUCTOR_FUNCTION(_func) static void __attribute__((constructor)) _func(void);
#        define CXXKIT_DESTRUCTOR_FUNCTION(_func)  static void __attribute__((destructor)) _func(void);
#    elif defined(_MSC_VER) && (_MSC_VER >= 1500)
#        include <stdlib.h>
//  Visual studio 2008 and later has _Pragma
//  We do some weird things to avoid the constructors being optimized away on VS2015 if WholeProgramOptimization is enabled.
//  First we make a reference to the array from the wrapper to make sure its references. Then we use a pragma to make sure
//  the wrapper function symbol is always included at the link side. Also, the symbols need to be extern (but not dllexport),
//  even though they are not really used from another object file.
//  We need to account for differences between the mangling of symbols for x86 and x64/ARM/ARM64 programs, as symbols on x86
//  are prefixed with an underscore but symbols on x64/ARM/ARM64 are not.
#        ifdef _M_IX86
#            define CXXKIT__MSVC_SYMBOL_PREFIX "_"
#        else
#            define CXXKIT__MSVC_SYMBOL_PREFIX ""
#        endif
#        define CXXKIT__MSVC_CTOR(_func, _sym_prefix)                                                                  \
            static void _func(void);                                                                                   \
            int _func##_wrapper(void);                                                                                 \
            int _func##_wrapper(void)                                                                                  \
            {                                                                                                          \
                _func();                                                                                               \
                return 0;                                                                                              \
            }                                                                                                          \
            CXXKIT_PRAGMA(comment(linker, "/include:" _sym_prefix #_func "_wrapper"))                                  \
            CXXKIT_PRAGMA(section(".CRT$XCU", read))                                                                   \
            __declspec(allocate(".CRT$XCU")) int (*_func##_wrapper_ptr)(void) = _func##_wrapper;
#        define CXXKIT_CONSTRUCTOR_FUNCTION(_func) CXXKIT__MSVC_CTOR(_func, CXXKIT__MSVC_SYMBOL_PREFIX)
#        define CXXKIT__MSVC_DTOR(_func, _sym_prefix)                                                                  \
            static void _func(void);                                                                                   \
            int _func##_constructor(void);                                                                             \
            int _func##_constructor(void)                                                                              \
            {                                                                                                          \
                atexit(_func);                                                                                         \
                return 0;                                                                                              \
            }                                                                                                          \
            CXXKIT_PRAGMA(comment(linker, "/include:" _sym_prefix #_func "_constructor"))                              \
            CXXKIT_PRAGMA(section(".CRT$XCU", read))                                                                   \
            __declspec(allocate(".CRT$XCU")) int (*_func##_constructor_ptr)(void) = _func##_constructor;
#        define CXXKIT_DESTRUCTOR_FUNCTION(_func) CXXKIT__MSVC_DTOR(_func, CXXKIT__MSVC_SYMBOL_PREFIX)
#    elif defined(_MSC_VER)
//  Pre Visual studio 2008 must use #pragma section
#        define CXXKIT__CONSTRUCTOR_FUNCTION_PRAGMA_ARGS(_func) section(".CRT$XCU", read)
#        define CXXKIT_CONSTRUCTOR_FUNCTION(_func)                                                                     \
            CXXKIT_PRAGMA(CXXKIT__CONSTRUCTOR_FUNCTION_PRAGMA_ARGS(_func))                                             \
            static void _func(void);                                                                                   \
            static int _func##_wrapper(void)                                                                           \
            {                                                                                                          \
                _func();                                                                                               \
                return 0;                                                                                              \
            }                                                                                                          \
            __declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func##_wrapper;
#        define CXXKIT__DESTRUCTOR_FUNCTION_PRAGMA_ARGS(_func) section(".CRT$XCU", read)
#        define CXXKIT_DESTRUCTOR_FUNCTION(_func)                                                                      \
            CXXKIT_PRAGMA(CXXKIT__DESTRUCTOR_FUNCTION_PRAGMA_ARGS(_func))                                              \
            static void _func(void);                                                                                   \
            static int _func##_constructor(void)                                                                       \
            {                                                                                                          \
                atexit(_func);                                                                                         \
                return 0;                                                                                              \
            }                                                                                                          \
            __declspec(allocate(".CRT$XCU")) static int (*_array##_func)(void) = _func##_constructor;
#    elif defined(__SUNPRO_C)
//  This is not tested, but i believe it should work, based on:
//  http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
#        define CXXKIT_DEFINE_CTOR_DTOR_NEEDS_PRAGMA
#        define CXXKIT__CONSTRUCTOR_FUNCTION_PRAGMA_ARGS(_func) init(_func)
#        define CXXKIT_CONSTRUCTOR_FUNCTION(_func)                                                                     \
            CXXKIT_PRAGMA(CXXKIT__CONSTRUCTOR_FUNCTION_PRAGMA_ARGS(_func))                                             \
            static void _func(void);
#        define CXXKIT__DESTRUCTOR_FUNCTION_PRAGMA_ARGS(_func) fini(_func)
#        define CXXKIT_DESTRUCTOR_FUNCTION(_func)                                                                      \
            CXXKIT_PRAGMA(CXXKIT__DESTRUCTOR_FUNCTION_PRAGMA_ARGS(_func))                                              \
            static void _func(void);
#    else
#        error "unimplemented constructor/destructor macro"
#    endif
#endif // #if defined(__cplusplus)


/** @brief Constructor/destructor callback macros: `CXXKIT_CONSTRUCTOR_FUNCTION`, `CXXKIT_DESTRUCTOR_FUNCTION`.
 * Portable across GCC/Clang/MSVC/SunPRO.
 */
/***********************************************************************************************************************
 * likely unlikely macro define
 * Enables the compiler to prioritize compilation using static analysis for likely paths within a boolean branch.
 * Example:
 * if (CXXKIT_LIKELY(expression)) {
 *      return result;                        // Faster if more likely
 * } else {
 *      return 0;
 * }
 * Compilers can use the information that a certain branch is not likely to be taken (for instance, a CHECK failure)
 * to optimize for the common case in the absence of better information (ie. compiling gcc with `-fprofile-arcs`).
 *
 * Recommendation: Modern CPUs dynamically predict branch execution paths, typically with accuracy greater than 97%.
 * As a result, annotating every branch in a codebase is likely counterproductive; however, annotating specific
 * branches that are both hot and consistently mispredicted is likely to yield performance improvements.
***********************************************************************************************************************/
#if (CXXKIT_CC_HAS_BUILTIN(__builtin_expect) || (defined(__GNUC__) && !defined(__clang__))) ||                         \
    (defined(CXXKIT_CC_GNU) && (CXXKIT_CC_GNU >= 200) && defined(__OPTIMIZE__))
#    define CXXKIT_LIKELY(expr)   __builtin_expect(!!(expr), true)
#    define CXXKIT_UNLIKELY(expr) __builtin_expect(!!(expr), false)
#else
#    define CXXKIT_LIKELY(expr)   (expr)
#    define CXXKIT_UNLIKELY(expr) (expr)
#endif

/** @brief Branch-likelihood hints: `CXXKIT_LIKELY`, `CXXKIT_UNLIKELY` (fall back to identity on unsupported compilers).
 */

/***********************************************************************************************************************
 * `CXXKIT_INTERNAL_IMMEDIATE_ABORT()` aborts the program in the fastest possible way, with no attempt at logging.
 * One use is to implement hardening aborts with ABSL_OPTION_HARDENED.  Since this is an internal symbol, it
 * should not be used directly outside of Abseil.
***********************************************************************************************************************/
#if CXXKIT_CC_HAS_BUILTIN(__builtin_trap) || (defined(__GNUC__) && !defined(__clang__))
#    define CXXKIT_INTERNAL_IMMEDIATE_ABORT() __builtin_trap()
#else
#    define CXXKIT_INTERNAL_IMMEDIATE_ABORT() abort()
#endif


/** @brief `CXXKIT_INTERNAL_IMMEDIATE_ABORT()`: trap/abort (no logging). Used internally.
 */
/***********************************************************************************************************************
 * `CXXKIT_INTERNAL_UNREACHABLE()` is the platform specific directive to indicate that a statement is unreachable,
 * and to allow the compiler to optimize accordingly. Clients should use `ABSL_UNREACHABLE()`, which is defined below.
***********************************************************************************************************************/
#if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#    define CXXKIT_INTERNAL_UNREACHABLE() std::unreachable()
#elif defined(__GNUC__) || CXXKIT_CC_HAS_BUILTIN(__builtin_unreachable)
#    define CXXKIT_INTERNAL_UNREACHABLE() __builtin_unreachable()
#elif CXXKIT_CC_HAS_BUILTIN(__builtin_assume)
#    define CXXKIT_INTERNAL_UNREACHABLE() __builtin_assume(false)
#elif defined(_MSC_VER)
#    define CXXKIT_INTERNAL_UNREACHABLE() __assume(false)
#else
#    define CXXKIT_INTERNAL_UNREACHABLE()
#endif


/** @brief `CXXKIT_INTERNAL_UNREACHABLE()`: marks code as unreachable for compiler optimization.
 * Public wrapper is `CXXKIT_CHECK_NOTREACHED`.
 */
/***********************************************************************************************************************
 * attribute macro define
***********************************************************************************************************************/
/** @brief Attribute family: `CXXKIT_ATTRIBUTE`, `CXXKIT_MAYBE_UNUSED`, plus lock/pure/format-family attributes.
 * @see CXXKIT_ATTRIBUTE_PURE, CXXKIT_ATTRIBUTE_FORMAT_PRINTF, CXXKIT_ATTRIBUTE_GUARDED_BY
 */
#if defined(__clang__) && (!defined(SWIG))
#    define CXXKIT_ATTRIBUTE(x) __attribute__((x))
#else
#    define CXXKIT_ATTRIBUTE(x) // no-op
#endif

#if CXXKIT_CC_HAS_ATTRIBUTE(maybe_unused) && CXXKIT_CC_CPP17_OR_GREATER
#    define CXXKIT_MAYBE_UNUSED [[maybe_unused]]
#else
#    define CXXKIT_MAYBE_UNUSED
#endif

/**
 * @brief It is used for declaring functions and arguments which may never be used.
 * It avoids possible compiler warnings.
 * For functions, place the attribute after the declaration, just before the semicolon.
 * It cannot go in the definition of a function, only the declaration. For arguments, place the attribute at the
 * beginning of the argument declaration.
 * @code
 * void my_unused_function(CXXKIT_ATTRIBUTE_UNUSED int unused_argument, int other_argument) CXXKIT_ATTRIBUTE_UNUSED;
 * @endcode
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(__unused__)
#    define CXXKIT_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#    define CXXKIT_ATTRIBUTE_UNUSED
#endif

/**
 * Declaring a function as `pure` enables better optimization of calls to the function.
 * A `pure` function has no effects except its return value and the return value depends only on the parameters
 * and/or global variables.
 * Place the attribute after the declaration, just before the semicolon.
 * @code
 * bool cxxkit_type_check_value(const Value *value) CXXKIT_ATTRIBUTE_PURE;
 * @endcode
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(__pure__)
#    define CXXKIT_ATTRIBUTE_PURE __attribute__((__pure__))
#else
#    define CXXKIT_ATTRIBUTE_PURE
#endif

/**
 * @param A the index of the argument corresponding to the format string (1-based)
 * @param B the index of the first of the format arguments, or 0 if there are none
 * This is used for declaring functions which take a variable number of arguments, with the same syntax as `printf()`.
 * It allows the compiler to type-check the arguments passed to the function.
 * Place the attribute after the function declaration, just before the semicolon.
 * @code
 * int my_snprintf(char *string, long n, char const *format, ...) CXXKIT_ATTRIBUTE_FORMAT_PRINTF(3, 4);
 * @endcode
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(__format__)
#    if !defined(__clang__) && CXXKIT_CC_GNU_CHECK_VERSION(4, 4)
#        define CXXKIT_ATTRIBUTE_FORMAT_PRINTF(format_idx, arg_idx)                                                    \
            __attribute__((__format__(gnu_printf, (format_idx), (arg_idx))))
#    else
#        define CXXKIT_ATTRIBUTE_FORMAT_PRINTF(format_idx, arg_idx)                                                    \
            __attribute__((__format__(__printf__, (format_idx), (arg_idx))))
#    endif
#elif (defined(CXXKIT_CC_GNU) || defined(CXXKIT_CC_CLANG)) && !defined(__INSURE__)
#    if defined(CXXKIT_CC_MINGW) && !defined(CXXKIT_CC_CLANG)
#        define CXXKIT_ATTRIBUTE_FORMAT_PRINTF(format_idx, arg_idx)                                                    \
            __attribute__((format(gnu_printf, (format_idx), (arg_idx))))
#    else
#        define CXXKIT_ATTRIBUTE_FORMAT_PRINTF(format_idx, arg_idx)                                                    \
            __attribute__((format(printf, (format_idx), (arg_idx))))
#    endif
#else
#    define CXXKIT_ATTRIBUTE_FORMAT_PRINTF(A, B)
#endif

/**
 * @brief Document if a shared variable/field needs to be protected by a lock.
 * GUARDED_BY allows the user to specify a particular lock that should be held when accessing the annotated variable.
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(guarded_by)
#    define CXXKIT_ATTRIBUTE_GUARDED_BY(x) __attribute__((guarded_by(x)))
#else
#    define CXXKIT_ATTRIBUTE_GUARDED_BY(x)
#endif

/**
 * Document the acquisition order between locks that can be held simultaneously by a thread. For any two locks that
 * need to be annotated to establish an acquisition order, only one of them needs the annotation.
 * (i.e. You don't have to annotate both locks with both acquired_after and acquired_before.)
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(acquired_after)
#    define CXXKIT_ATTRIBUTE_ACQUIRED_AFTER(x) __attribute__((acquired_after(x)))
#else
#    define CXXKIT_ATTRIBUTE_ACQUIRED_AFTER(x)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(acquired_before)
#    define CXXKIT_ATTRIBUTE_ACQUIRED_BEFORE(x) __attribute__((acquired_before(x)))
#else
#    define CXXKIT_ATTRIBUTE_ACQUIRED_BEFORE(x)
#endif

/**
 * @brief Document if the memory location pointed to by a pointer should be guarded by a lock when dereferencing the
 * pointer. Note that a pointer variable to a shared memory location could itself be a shared variable.
 * For example, if a shared global pointer q, which is guarded by mu1, points to a shared memory location that is
 * guarded by mu2, q should be annotated as follows:
 * @code
 * int *qGUARDED_BY(mu1) CXXKIT_ATTRIBUTE_PT_GUARDED_BY(mu2);
 * @endcode
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(pt_guarded_by)
#    define CXXKIT_ATTRIBUTE_PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))
#else
#    define CXXKIT_ATTRIBUTE_PT_GUARDED_BY(x)
#endif

/**
 * @brief Document the lock the annotated function returns without acquiring it.
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(lock_returned)
#    define CXXKIT_ATTRIBUTE_LOCK_RETURNED(x) __attribute__((lock_returned(x)))
#else
#    define CXXKIT_ATTRIBUTE_LOCK_RETURNED(x)
#endif

/**
 * @brief Document if a class/type is a lockable type (such as the mutex type).
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(lockable)
#    define CXXKIT_ATTRIBUTE_LOCKABLE __attribute__((lockable))
#else
#    define CXXKIT_ATTRIBUTE_LOCKABLE
#endif

/**
 * @brief Document if a class is a scoped lockable type (such as the MutexLock class).
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(scoped_lockable)
#    define CXXKIT_ATTRIBUTE_SCOPED_LOCKABLE __attribute__((scoped_lockable))
#else
#    define CXXKIT_ATTRIBUTE_SCOPED_LOCKABLE
#endif

// The following annotations specify lock and unlock primitives.
#if CXXKIT_CC_HAS_ATTRIBUTE(exclusive_lock_function)
#    define CXXKIT_ATTRIBUTE_EXCLUSIVE_LOCK_FUNCTION(...) __attribute__((exclusive_lock_function(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_EXCLUSIVE_LOCK_FUNCTION(...)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(shared_lock_function)
#    define CXXKIT_ATTRIBUTE_SHARED_LOCK_FUNCTION(...) __attribute__((shared_lock_function(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_SHARED_LOCK_FUNCTION(...)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(exclusive_trylock_function)
#    define CXXKIT_ATTRIBUTE_EXCLUSIVE_TRYLOCK_FUNCTION(...) __attribute__((exclusive_trylock_function(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_EXCLUSIVE_TRYLOCK_FUNCTION(...)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(shared_trylock_function)
#    define CXXKIT_ATTRIBUTE_SHARED_TRYLOCK_FUNCTION(...) __attribute__((shared_trylock_function(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_SHARED_TRYLOCK_FUNCTION(...)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(unlock_function)
#    define CXXKIT_ATTRIBUTE_UNLOCK_FUNCTION(...) __attribute__((unlock_function(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_UNLOCK_FUNCTION(...)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(assert_exclusive_lock)
#    define CXXKIT_ATTRIBUTE_ASSERT_EXCLUSIVE_LOCK(...) __attribute__((assert_exclusive_lock(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_ASSERT_EXCLUSIVE_LOCK(...)
#endif
// Document if a function expects certain locks to be held before it is called
#if CXXKIT_CC_HAS_ATTRIBUTE(exclusive_locks_required)
#    define CXXKIT_ATTRIBUTE_EXCLUSIVE_LOCKS_REQUIRED(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_EXCLUSIVE_LOCKS_REQUIRED(...)
#endif
#if CXXKIT_CC_HAS_ATTRIBUTE(shared_locks_required)
#    define CXXKIT_ATTRIBUTE_SHARED_LOCKS_REQUIRED(...) __attribute__((shared_locks_required(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_SHARED_LOCKS_REQUIRED(...)
#endif
/**
 * @brief Documents the locks acquired in the body of the function.
 * These locks cannot be held when calling this function (as Abseil's `Mutex` locks are non-reentrant).
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(locks_excluded)
#    define CXXKIT_ATTRIBUTE_LOCKS_EXCLUDED(...) __attribute__((locks_excluded(__VA_ARGS__)))
#else
#    define CXXKIT_ATTRIBUTE_LOCKS_EXCLUDED(...)
#endif

/**
 * @brief An escape hatch for thread safety analysis to ignore the annotated function.
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(no_thread_safety_analysis)
#    define CXXKIT_ATTRIBUTE_NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#else
#    define CXXKIT_ATTRIBUTE_NO_THREAD_SAFETY_ANALYSIS
#endif

/**
 * @brief CXXKIT_NO_UNIQUE_ADDRESS is a portable annotation to tell the compiler that a data member need not have an
 * address distinct from all other non-static data members of its class.
 * It allows empty types to actually occupy zero bytes as class members, instead of occupying at least one byte just
 * so that they get their own address.
 * There is almost never any reason not to use it on class members that could possibly be empty.
 * The macro expands to [[no_unique_address]] if the compiler supports the attribute, it expands to nothing otherwise.
 * Clang should supports this attribute since C++11, while other compilers should add support for it starting from
 * C++20. Among clang compilers, clang-cl doesn't support it yet and support is unclear also when the target platform
 * is iOS.
 */
#if CXXKIT_CC_HAS_ATTRIBUTE(no_unique_address)
#    define CXXKIT_ATTRIBUTE_NO_UNIQUE_ADDRESS [[no_unique_address]] // NOLINTNEXTLINE(whitespace/braces)
#else
#    define CXXKIT_ATTRIBUTE_NO_UNIQUE_ADDRESS
#endif

#if CXXKIT_CC_HAS_CPP_ATTRIBUTE(clang::annotate)
#    define CXXKIT_CPP_ATTRIBUTE_CLANG_ANNOTATE(x) [[clang::annotate(x)]]
#else
#    define CXXKIT_CPP_ATTRIBUTE_CLANG_ANNOTATE(x)
#endif

/**
 * CXXKIT_ATTRIBUTE_LIFETIME_BOUND indicates that a resource owned by a function parameter or implicit object parameter
 * is retained by the return value of the annotated function (or, for a parameter of a constructor, in the value of the
 * constructed object). This attribute causes warnings to be produced if a temporary object does not live long enough.
 *
 * When applied to a reference parameter, the referenced object is assumed to be retained by the return value of the
 * function. When applied to a non-reference parameter (for example, a pointer or a class type), all temporaries
 * referenced by the parameter are assumed to be retained by the return value of the function.
 *
 * See also the upstream documentation:
 * https://clang.llvm.org/docs/AttributeReference.html#lifetimebound
 */
#if CXXKIT_CC_HAS_CPP_ATTRIBUTE(clang::lifetimebound)
#    define CXXKIT_ATTRIBUTE_LIFETIME_BOUND [[clang::lifetimebound]]
#elif CXXKIT_CC_HAS_ATTRIBUTE(lifetimebound)
#    define CXXKIT_ATTRIBUTE_LIFETIME_BOUND __attribute__((lifetimebound))
#else
#    define CXXKIT_ATTRIBUTE_LIFETIME_BOUND
#endif

/**
 * Tells the compiler to warn about unused results.
 *
 * For code or headers that are assured to only build with C++17 and up, prefer just using the standard `[[nodiscard]]`
 * directly over this macro.
 *
 * When annotating a function, it must appear as the first part of the declaration or definition.
 * The compiler will warn if the return value from such a function is unused:
 *
 *      CXXKIT_ATTRIBUTE_MUST_USE_RESULT Sprocket* AllocateSprocket();
 *      AllocateSprocket();  // Triggers a warning.
 *
 * When annotating a class, it is equivalent to annotating every function which returns an instance.
 *
 *      class CXXKIT_ATTRIBUTE_MUST_USE_RESULT Sprocket {};
 *      Sprocket();  // Triggers a warning.
 *
 *      Sprocket MakeSprocket();
 *      MakeSprocket();  // Triggers a warning.
 *
 * Note that references and pointers are not instances:
 *
 *      Sprocket* SprocketPointer();
 *      SprocketPointer();  // Does *not* trigger a warning.
 *
 * CXXKIT_ATTRIBUTE_MUST_USE_RESULT allows using cast-to-void to suppress the unused result warning.
 * For that, warn_unused_result is used only for clang but not for gcc.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425
 *
 *  Note: past advice was to place the macro after the argument list.
 *
 *   TODO(b/176172494): Use ABSL_HAVE_CPP_ATTRIBUTE(nodiscard) when all code is compliant with the stricter [[nodiscard]].
 */
#if defined(__clang__) && CXXKIT_CC_HAS_ATTRIBUTE(warn_unused_result)
#    define CXXKIT_ATTRIBUTE_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#    define CXXKIT_ATTRIBUTE_MUST_USE_RESULT
#endif

