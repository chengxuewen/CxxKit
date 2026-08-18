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

#ifndef _CXXKIT_GLOBAL_HPP
#define _CXXKIT_GLOBAL_HPP

#include <cxxkit/base/core_config.hpp>
#include <cxxkit/base/compiler.hpp>
#include <cxxkit/base/system.hpp>
#include <cxxkit/base/macros.hpp>
#include <cxxkit/base/types.hpp>

#include <stdarg.h>

/***********************************************************************************************************************
 * compiler dll visibility macro declare
***********************************************************************************************************************/
#if defined(CXXKIT_CC_GNU) && (CXXKIT_CC_GNU > 400)
#    define CXXKIT_DECLARE_EXPORT __attribute__((visibility("default")))
#    define CXXKIT_DECLARE_IMPORT __attribute__((visibility("default")))
#    define CXXKIT_DECLARE_HIDDEN __attribute__((visibility("hidden")))
#elif defined(CXXKIT_CC_MINGW) || defined(CXXKIT_CC_MSVC)
#    define CXXKIT_DECLARE_EXPORT __declspec(dllexport)
#    define CXXKIT_DECLARE_IMPORT __declspec(dllimport)
#    define CXXKIT_DECLARE_HIDDEN
#elif defined(CXXKIT_CC_CLANG)
#    define CXXKIT_DECLARE_EXPORT __attribute__((visibility("default")))
#    define CXXKIT_DECLARE_IMPORT __attribute__((visibility("default")))
#    define CXXKIT_DECLARE_HIDDEN __attribute__((visibility("hidden")))
#endif

#ifndef CXXKIT_DECLARE_EXPORT
#    define CXXKIT_DECLARE_EXPORT
#endif
#ifndef CXXKIT_DECLARE_IMPORT
#    define CXXKIT_DECLARE_IMPORT
#endif
#ifndef CXXKIT_DECLARE_HIDDEN
#    define CXXKIT_DECLARE_HIDDEN
#endif

/***********************************************************************************************************************
 * compiler specific cmds for export and import code to DLL and declare namespace
***********************************************************************************************************************/
#ifdef CXXKIT_BUILD_SHARED          // compiled as a dynamic lib.
#    ifdef CXXKIT_BUILDING_CORE_LIB // defined if we are building the lib
#        define CXXKIT_CORE_API CXXKIT_DECLARE_EXPORT
#    else
#        define CXXKIT_CORE_API CXXKIT_DECLARE_IMPORT
#    endif
#    define CXXKIT_CORE_HIDDEN CXXKIT_DECLARE_HIDDEN
#else // compiled as a static lib.
#    define CXXKIT_CORE_API
#    define CXXKIT_CORE_HIDDEN
#endif

#endif // _CXXKIT_GLOBAL_HPP
