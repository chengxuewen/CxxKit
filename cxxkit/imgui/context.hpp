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
#include <cxxkit/imgui/backend.hpp>
#include <cxxkit/imgui/imgui_global.hpp>

namespace cxxkit
{

/// @brief Owns the Dear ImGui context and drives one frame lifecycle over host-injected backends.
///
/// Usage: construct with host-provided backends, init(), then per frame begin_frame()/end_frame(),
/// and shutdown() (or rely on the destructor's RAII fallback). The ImGui context is a process-global
/// singleton — do not nest or run two ImGuiHost instances concurrently.
class CXXKIT_IMGUI_API ImGuiHost
{
public:
    /// @param platform_backend Host-owned backend feeding window/input state; outlives *this.
    /// @param renderer_backend Host-owned backend rendering draw data; outlives *this.
    ImGuiHost(PlatformBackend &platform_backend, RendererBackend &renderer_backend);

    /// @brief RAII fallback: shuts down automatically if the user never did.
    ~ImGuiHost();

    /// @brief Create the ImGui context and initialize both backends.
    /// @return true on first successful init; false if already initialized (idempotent, state kept).
    bool init();

    /// @brief Start one frame: backend input pass, then ImGui::NewFrame().
    void begin_frame();

    /// @brief Finish one frame: ImGui::Render(), then hand the draw data to the renderer backend.
    void end_frame();

    /// @brief Tear down backends and destroy the ImGui context. Idempotent: a second call is a no-op.
    void shutdown();

private:
    CXXKIT_DISABLE_COPY_MOVE(ImGuiHost)

    PlatformBackend &mPlatformBackend;
    RendererBackend &mRendererBackend;
    bool mInitialized;
    bool mShutdown;
};

} // namespace cxxkit
