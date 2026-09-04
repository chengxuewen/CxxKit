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

#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>

#include <SDL3/SDL.h>
#include <cxxkit/3rdparty/imgui/imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

namespace cxxkit
{

Sdl3PlatformBackend::Sdl3PlatformBackend()
    : mWindow(nullptr)
    , mInitialized(false)
{
}

Sdl3PlatformBackend::~Sdl3PlatformBackend()
{
    shutdown();
}

bool Sdl3PlatformBackend::init(void *native_window)
{
    if (mInitialized)
    {
        return true;
    }
    if (native_window == nullptr)
    {
        return false;
    }
    // InitForOpenGL with a null context pointer: the impl then queries SDL_GL_GetCurrentContext()
    // itself; the host must have made the context current before this call.
    if (!ImGui_ImplSDL3_InitForOpenGL(static_cast<SDL_Window *>(native_window), nullptr))
    {
        return false;
    }
    mWindow = native_window;
    mInitialized = true;
    return true;
}

void Sdl3PlatformBackend::new_frame()
{
    if (!mInitialized)
    {
        return;
    }
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
    ImGui_ImplSDL3_NewFrame();
}

void Sdl3PlatformBackend::shutdown()
{
    if (!mInitialized)
    {
        return;
    }
    ImGui_ImplSDL3_Shutdown();
    mWindow = nullptr;
    mInitialized = false;
}

Sdl3RendererBackend::Sdl3RendererBackend()
    : mInitialized(false)
{
}

Sdl3RendererBackend::~Sdl3RendererBackend()
{
    shutdown();
}

bool Sdl3RendererBackend::init()
{
    if (mInitialized)
    {
        return true;
    }
    if (!ImGui_ImplOpenGL3_Init(nullptr))
    {
        return false;
    }
    mInitialized = true;
    return true;
}

void Sdl3RendererBackend::render(ImDrawData *draw_data)
{
    if (!mInitialized)
    {
        return;
    }
    ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}

void Sdl3RendererBackend::shutdown()
{
    if (!mInitialized)
    {
        return;
    }
    ImGui_ImplOpenGL3_Shutdown();
    mInitialized = false;
}

} // namespace cxxkit
