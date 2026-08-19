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
   Profiling macros (Tracy backend, opt-in)

   The sublibrary is always available as a no-op; profiling activates when the consuming target links
   cxxkit::profiling AND the library was built with CXXKIT_ENABLE_LIB_TRACY=ON (which injects
   CXXKIT_PROFILING_ENABLED + TRACY_ENABLE through the target's interface compile definitions).
***********************************************************************************************************************/
#if defined(CXXKIT_PROFILING_ENABLED)
#    include <cxxkit/3rdparty/tracy/tracy/Tracy.hpp>

// Scoped zone: instruments the current scope, named `name` in the profiler UI.
// Tracy v0.13+: named zones use ZoneScopedN (ZoneScopedS is the static-zone variant).
#    define CXXKIT_PROFILE_SCOPE(name) ZoneScopedN(name)
// Marks the end of one frame in the profiler's frame timeline.
#    define CXXKIT_PROFILE_FRAME() FrameMark
#else
#    define CXXKIT_PROFILE_SCOPE(name) ((void)0)
#    define CXXKIT_PROFILE_FRAME() ((void)0)
#endif
