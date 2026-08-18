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

#ifndef _CXXKIT_TYPES_HPP
#define _CXXKIT_TYPES_HPP

#include <cxxkit/base/macros.hpp>
#include <cxxkit/base/system.hpp>
#include <cxxkit/base/core_config.hpp>

#if defined(CXXKIT_OS_WIN)
#    include <windows.h>
#else
#    include <sys/types.h>
#endif
#include <vector>
#include <memory>
#include <cstdint>
#include <stdint.h>

CXXKIT_BEGIN_NAMESPACE

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using float_t = float;
using double_t = double;

using std::size_t;
using std::ptrdiff_t;
using uintptr_t = size_t;
using intptr_t = ptrdiff_t;
#if !CXXKIT_HAS_SSIZE_T && defined(CXXKIT_OS_WIN)
#    include <BaseTsd.h>
using ssize_t = SSIZE_T;
#else
using ssize_t = ssize_t;
#endif
#ifndef CXXKIT_SIZEOF_SSIZE_T
#    define CXXKIT_SIZEOF_SSIZE_T CXXKIT_SIZEOF_SIZE_T
#endif

using byte_t = uint8_t;
using uchar_t = unsigned char;
using ushort_t = unsigned short;
using uint_t = unsigned int;
using ulong_t = unsigned long;
using ulonglong_t = unsigned long long;

using handle_t = void *;
using pointer_t = void *;
using const_pointer_t = const void *;

using Binary = std::vector<byte_t>;
using TSBinary = std::pair<int64_t, Binary>;
using BinarySharedPtr = std::shared_ptr<Binary>;

template <typename T>
Binary makeBinary(const std::vector<T> &data)
{
    return {reinterpret_cast<const byte_t *>(data.data()), reinterpret_cast<const byte_t *>(data.data()) + data.size()};
}

struct None
{
};

/***********************************************************************************************************************
 * Integer conversion macro define
***********************************************************************************************************************/
#if defined(CXXKIT_OS_WIN) && !defined(CXXKIT_CC_GNU)
#    define CXXKIT_INT64_C(c)  c##i64  /* signed 64 bit constant */
#    define CXXKIT_UINT64_C(c) c##ui64 /* unsigned 64 bit constant */
#else
#    ifdef __cplusplus
#        define CXXKIT_INT64_C(c)  static_cast<long long>(c##LL)           /* signed 64 bit constant */
#        define CXXKIT_UINT64_C(c) static_cast<unsigned long long>(c##ULL) /* unsigned 64 bit constant */
#    else
#        define CXXKIT_INT64_C(c)  ((long long)(c##LL))           /* signed 64 bit constant */
#        define CXXKIT_UINT64_C(c) ((unsigned long long)(c##ULL)) /* unsigned 64 bit constant */
#    endif
#endif

/***********************************************************************************************************************
 * type format define
***********************************************************************************************************************/
#if CXXKIT_SIZEOF_SHORT == 2
#    define CXXKIT_INT16_MODIFIER "h"
#    define CXXKIT_INT16_FORMAT   "hi"
#    define CXXKIT_UINT16_FORMAT  "hu"
#elif CXXKIT_SIZEOF_INT == 2
#    define CXXKIT_INT16_MODIFIER ""
#    define CXXKIT_INT16_FORMAT   "i"
#    define CXXKIT_UINT16_FORMAT  "u"
#else
#    error "Compiler provides no native 16-bit integer type"
#endif

#if CXXKIT_SIZEOF_SHORT == 4
#    define CXXKIT_INT32_MODIFIER "h"
#    define CXXKIT_INT32_FORMAT   "hi"
#    define CXXKIT_UINT32_FORMAT  "hu"
#elif CXXKIT_SIZEOF_INT == 4
#    define CXXKIT_INT32_MODIFIER "h"
#    define CXXKIT_INT32_FORMAT   "i"
#    define CXXKIT_UINT32_FORMAT  "u"
#elif CXXKIT_SIZEOF_LONG == 4
#    define CXXKIT_INT32_MODIFIER "l"
#    define CXXKIT_INT32_FORMAT   "li"
#    define CXXKIT_UINT32_FORMAT  "lu"
#else
#    error "Compiler provides no native 32-bit integer type"
#endif

#if CXXKIT_SIZEOF_INT == 8
#    define CXXKIT_INT64_MODIFIER ""
#    define CXXKIT_INT64_FORMAT   "i"
#    define CXXKIT_UINT64_FORMAT  "u"
#elif (CXXKIT_SIZEOF_LONG == 8) && (CXXKIT_SIZEOF_LONG_LONG != CXXKIT_SIZEOF_LONG || CXXKIT_INT64_IS_LONG_TYPE)
#    define CXXKIT_INT64_MODIFIER "l"
#    define CXXKIT_INT64_FORMAT   "li"
#    define CXXKIT_UINT64_FORMAT  "lu"
#elif (CXXKIT_SIZEOF_LONG_LONG == 8) &&                                                                                \
    (CXXKIT_SIZEOF_LONG_LONG != CXXKIT_SIZEOF_LONG || CXXKIT_INT64_IS_LONG_LONG_TYPE)
#    define CXXKIT_INT64_MODIFIER "ll"
#    define CXXKIT_INT64_FORMAT   "lli"
#    define CXXKIT_UINT64_FORMAT  "llu"
#else
#    error "Compiler provides no native 64-bit integer type"
#endif

#if CXXKIT_SIZET_IS_SHORT_TYPE
#    define CXXKIT_SIZE_MODIFIER  "h"
#    define CXXKIT_SSIZE_MODIFIER "h"
#    define CXXKIT_SIZE_FORMAT    "hu"
#    define CXXKIT_SSZIE_FORMAT   "hi"
#elif CXXKIT_SIZET_IS_INT_TYPE
#    define CXXKIT_SIZE_MODIFIER  ""
#    define CXXKIT_SSIZE_MODIFIER ""
#    define CXXKIT_SIZE_FORMAT    "u"
#    define CXXKIT_SSZIE_FORMAT   "i"
#elif CXXKIT_SIZET_IS_LONG_TYPE
#    define CXXKIT_SIZE_MODIFIER  "l"
#    define CXXKIT_SSIZE_MODIFIER "l"
#    define CXXKIT_SIZE_FORMAT    "lu"
#    define CXXKIT_SSZIE_FORMAT   "li"
#elif CXXKIT_SIZET_IS_LONG_LONG_TYPE
#    define CXXKIT_SIZE_MODIFIER  "ll"
#    define CXXKIT_SSIZE_MODIFIER "ll"
#    define CXXKIT_SIZE_FORMAT    "llu"
#    define CXXKIT_SSZIE_FORMAT   "lli"
#elif CXXKIT_SIZEOF_SIZE_T == 8
#    define CXXKIT_SIZE_MODIFIER  "l"
#    define CXXKIT_SSIZE_MODIFIER "l"
#    define CXXKIT_SIZE_FORMAT    "lu"
#    define CXXKIT_SSZIE_FORMAT   "li"
#elif CXXKIT_SIZEOF_SIZE_T == 4
#    define CXXKIT_SIZE_MODIFIER  ""
#    define CXXKIT_SSIZE_MODIFIER ""
#    define CXXKIT_SIZE_FORMAT    "u"
#    define CXXKIT_SSZIE_FORMAT   "i"
#else
#    error "Could not determine size of size_t."
#endif

#if CXXKIT_SIZEOF_VOID_P == CXXKIT_SIZEOF_INT
#    define CXXKIT_INTPTR_MODIFIER ""
#    define CXXKIT_INTPTR_FORMAT   "i"
#    define CXXKIT_UINTPTR_FORMAT  "u"
#elif CXXKIT_SIZEOF_VOID_P == CXXKIT_SIZEOF_LONG
#    define CXXKIT_INTPTR_MODIFIER "l"
#    define CXXKIT_INTPTR_FORMAT   "li"
#    define CXXKIT_UINTPTR_FORMAT  "lu"
#elif CXXKIT_SIZEOF_VOID_P == CXXKIT_SIZEOF_LONG_LONG
#    define CXXKIT_INTPTR_MODIFIER "ll"
#    define CXXKIT_INTPTR_FORMAT   "lli"
#    define CXXKIT_UINTPTR_FORMAT  "llu"
#else
#    error "Could not determine size of void *"
#endif

#if CXXKIT_SIZEOF_VOID_P == CXXKIT_SIZEOF_INT
#    define CXXKIT_INTPTR_MODIFIER ""
#    define CXXKIT_INTPTR_FORMAT   "i"
#    define CXXKIT_UINTPTR_FORMAT  "u"
#elif CXXKIT_SIZEOF_VOID_P == CXXKIT_SIZEOF_LONG
#    define CXXKIT_INTPTR_MODIFIER "l"
#    define CXXKIT_INTPTR_FORMAT   "li"
#    define CXXKIT_UINTPTR_FORMAT  "lu"
#elif CXXKIT_SIZEOF_VOID_P == CXXKIT_SIZEOF_LONG_LONG
#    define CXXKIT_INTPTR_MODIFIER "ll"
#    define CXXKIT_INTPTR_FORMAT   "lli"
#    define CXXKIT_UINTPTR_FORMAT  "llu"
#else
#    error "Could not determine size of void *"
#endif

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_TYPES_HPP
