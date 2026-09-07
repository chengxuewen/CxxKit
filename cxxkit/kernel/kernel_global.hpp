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

// kernel sublibrary is unconditionally configured and built (top-level add_subdirectory has no gate).
// The CXXKIT_FEATURE_ENABLE_KERNEL guard is an OpenCTK migration leftover that nothing defines —
// it turned the whole sublibrary into dead preprocessor code. Activate it here so every kernel
// header (which includes this file first) sees it.
#ifndef CXXKIT_FEATURE_ENABLE_KERNEL
#    define CXXKIT_FEATURE_ENABLE_KERNEL 1
#endif

/***********************************************************************************************************************
   cxxkit Compiler specific cmds for export and import code to DLL
***********************************************************************************************************************/
#ifdef CXXKIT_BUILD_SHARED_KERNEL     // compiled as a dynamic lib.
#    ifdef CXXKIT_BUILDING_KERNEL_LIB // defined if we are building the lib
#        define CXXKIT_KERNEL_API CXXKIT_DECLARE_EXPORT
#    else
#        define CXXKIT_KERNEL_API CXXKIT_DECLARE_IMPORT
#    endif
#    define CXXKIT_KERNEL_HIDDEN CXXKIT_DECLARE_HIDDEN
#else // compiled as a static lib.
#    define CXXKIT_KERNEL_API
#    define CXXKIT_KERNEL_HIDDEN
#endif
