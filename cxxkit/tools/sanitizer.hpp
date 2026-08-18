/***********************************************************************************************************************
**
** Library: cxxkit
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

#pragma once

#include <cxxkit/tools/type_traits.hpp>

#include <stddef.h> // For size_t.

#if defined(__has_feature)
#    if __has_feature(address_sanitizer)
#        define CXXKIT_HAS_ASAN 1
#    endif
#    if __has_feature(memory_sanitizer)
#        define CXXKIT_HAS_MSAN 1
#    endif
#endif
#ifndef CXXKIT_HAS_ASAN
#    define CXXKIT_HAS_ASAN 0
#endif
#ifndef CXXKIT_HAS_MSAN
#    define CXXKIT_HAS_MSAN 0
#endif

#if CXXKIT_HAS_ASAN
#    include <sanitizer/asan_interface.h>
#endif
#if CXXKIT_HAS_MSAN
#    include <sanitizer/msan_interface.h>
#endif

#ifdef __has_attribute
#    if __has_attribute(no_sanitize) && defined(__clang__)
#        define CXXKIT_NO_SANITIZE(what) __attribute__((no_sanitize(what)))
#    endif
#endif
#ifndef CXXKIT_NO_SANITIZE
#    define CXXKIT_NO_SANITIZE(what)
#endif

// Ask ASan to mark the memory range [ptr, ptr + element_size * num_elements)
// as being unaddressable, so that reads and writes are not allowed. ASan may
// narrow the range to the nearest alignment boundaries.
static inline void cxxkit_asan_poison(const volatile void *ptr, size_t element_size, size_t num_elements)
{
#if CXXKIT_HAS_ASAN
    ASAN_POISON_MEMORY_REGION(ptr, element_size * num_elements);
#else
    // This is to prevent from the compiler raising a warning/error over unused
    // variables. We cannot use clang's annotation (`[[maybe_unused]]`) because
    // this file is also included from c files which doesn't support the
    // annotation till we switch to C23
    (void)ptr;
    (void)element_size;
    (void)num_elements;
#endif
}

// Ask ASan to mark the memory range [ptr, ptr + element_size * num_elements)
// as being addressable, so that reads and writes are allowed. ASan may widen
// the range to the nearest alignment boundaries.
static inline void cxxkit_asan_unpoison(const volatile void *ptr, size_t element_size, size_t num_elements)
{
#if CXXKIT_HAS_ASAN
    ASAN_UNPOISON_MEMORY_REGION(ptr, element_size * num_elements);
#else
    (void)ptr;
    (void)element_size;
    (void)num_elements;
#endif
}

// Ask MSan to mark the memory range [ptr, ptr + element_size * num_elements)
// as being uninitialized.
static inline void cxxkit_msan_mark_uninitialized(const volatile void *ptr, size_t element_size, size_t num_elements)
{
#if CXXKIT_HAS_MSAN
    __msan_poison(ptr, element_size * num_elements);
#else
    (void)ptr;
    (void)element_size;
    (void)num_elements;
#endif
}

// Force an MSan check (if any bits in the memory range [ptr, ptr +
// element_size * num_elements) are uninitialized the call will crash with an
// MSan report).
static inline void cxxkit_msan_check_initialized(const volatile void *ptr, size_t element_size, size_t num_elements)
{
#if CXXKIT_HAS_MSAN
    __msan_check_mem_is_initialized(ptr, element_size * num_elements);
#else
    (void)ptr;
    (void)element_size;
    (void)num_elements;
#endif
}

CXXKIT_BEGIN_NAMESPACE

template <typename T>
inline void AsanPoison(const T &mem)
{
    cxxkit_asan_poison(mem.data(), sizeof(mem.data()[0]), mem.size());
}

template <typename T>
inline void AsanUnpoison(const T &mem)
{
    cxxkit_asan_unpoison(mem.data(), sizeof(mem.data()[0]), mem.size());
}

template <typename T>
inline void MsanMarkUninitialized(const T &mem)
{
    cxxkit_msan_mark_uninitialized(mem.data(), sizeof(mem.data()[0]), mem.size());
}

template <typename T>
inline T MsanUninitialized(T t)
{
#if CXXKIT_HAS_MSAN
    // becomes available in downstream projects.
    static_assert(sanittraits::is_trivially_copyable_v<T>(), "");
#endif
    cxxkit_msan_mark_uninitialized(&t, sizeof(T), 1);
    return t;
}

template <typename T>
inline void MsanCheckInitialized(const T &mem)
{
    cxxkit_msan_check_initialized(mem.data(), sizeof(mem.data()[0]), mem.size());
}

CXXKIT_END_NAMESPACE