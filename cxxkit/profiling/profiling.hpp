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
// Tracy v0.13+: named zones use zone_scoped_n (ZoneScopedS is the static-zone variant).
/** @brief Start a named profiling zone for the current scope (no-op when profiling disabled).
 * @param name Human-readable zone name shown in the Tracy UI.
 * @see CXXKIT_PROFILE_FRAME, Tracy
 * @note Emits `((void)0)` when CXXKIT_PROFILING_ENABLED is not defined (zero-cost).
 */
#    define CXXKIT_PROFILE_SCOPE(name) zone_scoped_n(name)
// Marks the end of one frame in the profiler's frame timeline.
/** @brief Mark the end of a logical frame in the profiler timeline (no-op when disabled).
 * @see CXXKIT_PROFILE_SCOPE, Tracy
 */
#    define CXXKIT_PROFILE_FRAME() FrameMark
#else
// No-op variant when profiling is disabled
#    define CXXKIT_PROFILE_SCOPE(name) ((void)0)
// No-op variant when profiling is disabled
#    define CXXKIT_PROFILE_FRAME() ((void)0)
#endif
