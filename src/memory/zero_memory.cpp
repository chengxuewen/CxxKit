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

#include <cxxkit/memory/zero_memory.hpp>
#include <cxxkit/base/system.hpp>
#include <cxxkit/tools/checks.hpp>

#if defined(CXXKIT_OS_WIN)
#    include <windows.h>
#else
#    include <string.h>
#endif

CXXKIT_BEGIN_NAMESPACE
// Code and comment taken from "OPENSSL_cleanse" of BoringSSL.
void ExplicitZeroMemory(void *ptr, size_t len)
{
    CXXKIT_DCHECK(ptr || !len);
#if defined(CXXKIT_OS_WIN)
    SecureZeroMemory(ptr, len);
#else
    memset(ptr, 0, len);
#    if !defined(__pnacl__)
    /* As best as we can tell, this is sufficient to break any optimisations that
       might try to eliminate "superfluous" memsets. If there's an easy way to
       detect memset_s, it would be better to use that. */
    __asm__ __volatile__("" : : "r"(ptr) : "memory"); // NOLINT
#    endif
#endif // !CXXKIT_OS_WIN
}
CXXKIT_END_NAMESPACE
