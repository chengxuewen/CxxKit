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

#include <cxxkit/imgui/imgui_global.hpp>

/***********************************************************************************************************************
   Host-injected backends: cxxkit never creates windows or GL contexts and never links a windowing or
   rendering library. The embedding application owns its platform window and renderer; it hands cxxkit
   an already-initialized native window handle (via PlatformBackend::init) and renders the draw data
   ImGui produces (via RendererBackend::render) with its own graphics API. Concrete backends live in
   the host application (or bind upstream imgui_impl_* files); cxxkit/imgui only defines the contract.
***********************************************************************************************************************/

namespace cxxkit
{

class CXXKIT_IMGUI_API PlatformBackend
{
public:
    virtual ~PlatformBackend() = default;

    /// @brief Attach to an existing native window created by the host (e.g. SDL_Window*, HWND, NSWindow*).
    /// @param native_window Host-owned native window handle, opaque to cxxkit.
    /// @return true if the backend initialized successfully.
    virtual bool init(void *native_window) = 0;

    /// @brief Feed per-frame input/display state; must leave io.DisplaySize/io.DeltaTime valid
    ///        before ImGui::NewFrame() is called (headless: the backend guarantees the semantics).
    virtual void new_frame() = 0;

    /// @brief Detach from the window; must be idempotent-safe when paired with init/shutdown.
    virtual void shutdown() = 0;
};

class CXXKIT_IMGUI_API RendererBackend
{
public:
    virtual ~RendererBackend() = default;

    /// @brief Prepare the renderer-side state (device objects, shaders) for drawing ImGui draw lists.
    /// @return true if the backend initialized successfully.
    virtual bool init() = 0;

    /// @brief Render one frame of draw data produced by ImGui::Render() using the host's graphics API.
    /// @param draw_data Draw lists owned by the ImGui context; valid only within this call.
    virtual void render(ImDrawData *draw_data) = 0;

    /// @brief Release renderer-side state.
    virtual void shutdown() = 0;
};

} // namespace cxxkit
