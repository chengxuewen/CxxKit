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

#include <cxxkit/imgui/application.hpp>
#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/imgui_global.hpp>
#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>

#include <string>

struct SDL_Window;

namespace cxxkit
{

/// @brief ImGuiApplication over an owned SDL3 window + OpenGL context (the "platform corner").
///
/// Unlike ImGuiHost/Sdl3PlatformBackend/Sdl3RendererBackend — which never touch the SDL lifecycle
/// (host owns SDL) — this class IS the SDL host: the constructor performs the full
/// SDL_Init -> GL attributes -> CreateWindow -> CreateContext -> MakeCurrent -> SetSwapInterval
/// sequence (GL 3.0 Core), and the destructor tears it down in reverse. The kernel+imgui-core
/// never-opens-a-window contract is unchanged; window ownership is explicit and opt-in here.
///
/// Failure semantics: any setup step that fails rolls back what it created and records an
/// mInitFailed flag — the constructor never aborts the process. exec() on a failed construction
/// marks the application finished and returns nonzero. On display-less machines (CI) the
/// constructor degrades to this flag path, so headless runs are safe.
/// @since 0.2
class CXXKIT_IMGUI_API SdlImGuiApplication : public ImGuiApplication
{
public:
    /// @param title Window title. @param width @param height Initial window size in pixels.
    /// @param vsync Swap interval (1 = vsync on, the common default).
    explicit SdlImGuiApplication(const std::string &title, int width, int height, bool vsync = true);
    ~SdlImGuiApplication() override;

    int exec(const FrameCallback &frame) override;

    /// @brief The owned window (null when construction failed); exposed for callers that need
    /// raw window metrics (e.g. SDL_GetWindowSize for viewport math in editor overlays).
    SDL_Window *window();

private:
    bool mInitFailed{false};
    SDL_Window *mWindow{nullptr};
    void *mGlContext{nullptr};
    // Declaration order matters: the host holds references to both backends.
    Sdl3PlatformBackend mPlatformBackend;
    Sdl3RendererBackend mRendererBackend;
    ImGuiHost mHost;
};

} // namespace cxxkit
