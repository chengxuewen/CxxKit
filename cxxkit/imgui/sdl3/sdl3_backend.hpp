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
** The upstream impl files are the property of the dear imgui project (MIT), (c) Omar Cornut.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#ifndef CXXKIT_IMGUI_SDL3_BACKEND_HPP
#define CXXKIT_IMGUI_SDL3_BACKEND_HPP

#include <cxxkit/imgui/imgui_global.hpp>
#include <cxxkit/imgui/backend.hpp>

namespace cxxkit
{

/***********************************************************************************************************************
   SDL3 + OpenGL3 backend: binds upstream imgui_impl_sdl3/imgui_impl_opengl3 to the cxxkit backend contract.
   The HOST owns SDL: it must have called SDL_Init(SDL_INIT_VIDEO) and created an SDL_GLWindow with a current GL
   context before init(); cxxkit never calls SDL_Init/SDL_Quit and never creates windows or contexts.
***********************************************************************************************************************/
class CXXKIT_IMGUI_API Sdl3PlatformBackend : public PlatformBackend
{
public:
    Sdl3PlatformBackend();
    ~Sdl3PlatformBackend() override;

    /// @brief native_window is the host's SDL_Window* (cast to void*); the GL context is taken from SDL_GL_GetCurrentContext().
    bool init(void *native_window) override;
    /// @brief Pumps SDL_PollEvent and feeds every event to ImGui_ImplSDL3_ProcessEvent, then ImGui_ImplSDL3_NewFrame().
    void new_frame() override;
    /// @brief ImGui_ImplSDL3_Shutdown(); never SDL_Quit (host owns the SDL lifecycle). Idempotent.
    void shutdown() override;

private:
    void *mWindow;
    bool mInitialized;
};

class CXXKIT_IMGUI_API Sdl3RendererBackend : public RendererBackend
{
public:
    Sdl3RendererBackend();
    ~Sdl3RendererBackend() override;

    /// @brief ImGui_ImplOpenGL3_Init(nullptr) — default GLSL version (imgui's bundled loader resolves GL procs at runtime).
    bool init() override;
    /// @brief ImGui_ImplOpenGL3_RenderDrawData(draw_data).
    void render(ImDrawData *draw_data) override;
    /// @brief ImGui_ImplOpenGL3_Shutdown(). Idempotent.
    void shutdown() override;

private:
    bool mInitialized;
};

} // namespace cxxkit

#endif // CXXKIT_IMGUI_SDL3_BACKEND_HPP
