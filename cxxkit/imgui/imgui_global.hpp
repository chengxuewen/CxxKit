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

#pragma once

#include <cxxkit/base/global.hpp>

/***********************************************************************************************************************
   cxxkit Compiler specific cmds for export and import code to DLL
***********************************************************************************************************************/
#ifdef CXXKIT_BUILD_SHARED_IMGUI     // compiled as a dynamic lib.
#    ifdef CXXKIT_BUILDING_IMGUI_LIB // defined if we are building the lib
#        define CXXKIT_IMGUI_API CXXKIT_DECLARE_EXPORT
#    else
#        define CXXKIT_IMGUI_API CXXKIT_DECLARE_IMPORT
#    endif
#    define CXXKIT_IMGUI_HIDDEN CXXKIT_DECLARE_HIDDEN
#else // compiled as a static lib.
#    define CXXKIT_IMGUI_API
#    define CXXKIT_IMGUI_HIDDEN
#endif

// IMGUI_API must be defined BEFORE <cxxkit/3rdparty/imgui/imgui.h> is included: upstream guards its
// empty default with `#ifndef IMGUI_API` (imgui.h L85-86), so the first definition wins. Keeping the
// include inside this header makes the ordering discipline structural — any TU that includes
// imgui_global.hpp automatically gets CXXKIT_IMGUI_API first, and the upstream 5 TUs receive
// IMGUI_API=CXXKIT_IMGUI_API via the target COMPILE_DEFINITIONS instead.
#ifndef IMGUI_API
#    define IMGUI_API CXXKIT_IMGUI_API
#endif

#include <cxxkit/3rdparty/imgui/imgui.h>
