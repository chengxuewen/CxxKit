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
#include <cxxkit/imgui/imgui_global.hpp>

#include <atomic>
#include <functional>

namespace cxxkit
{

/// @brief Abstract base for Dear ImGui applications: one frame callback, one exec loop.
///
/// Subclasses own the windowing/GL boilerplate; the base owns only the run-state contract:
/// exec() drives the frame callback until it returns true (quit) or the subclass's own exit
/// condition fires, then marks the application finished. is_finished() is safe to poll from
/// other threads (atomic), so a producer thread can outlive the loop.
/// @since 0.2
class CXXKIT_IMGUI_API ImGuiApplication
{
public:
    /// @brief One application frame; return true to quit the exec loop.
    using FrameCallback = std::function<bool()>;

    virtual ~ImGuiApplication() = default;

    /// @brief Run the frame loop; returns the application exit code (0 = normal).
    /// @param frame Called once per loop iteration; returning true stops the loop.
    virtual int exec(const FrameCallback &frame) = 0;

    /// @brief Whether the application has finished running (set after exec returns).
    bool is_finished() const { return mFinished.load(); }

protected:
    std::atomic<bool> mFinished{false};
};

} // namespace cxxkit
