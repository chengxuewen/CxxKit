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

#include <cxxkit/imgui/sdl3/sdl_application.hpp>

#include <SDL3/SDL.h>

namespace cxxkit
{

SdlImGuiApplication::SdlImGuiApplication(const std::string &title, int width, int height, bool vsync)
    : mPlatformBackend()
    , mRendererBackend()
    , mHost(mPlatformBackend, mRendererBackend)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        mInitFailed = true;
        return;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    mWindow = SDL_CreateWindow(title.c_str(),
                               width,
                               height,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (mWindow == nullptr)
    {
        SDL_Quit();
        mInitFailed = true;
        return;
    }
    mGlContext = SDL_GL_CreateContext(mWindow);
    if (mGlContext == nullptr)
    {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
        SDL_Quit();
        mInitFailed = true;
        return;
    }
    if (!SDL_GL_MakeCurrent(mWindow, static_cast<SDL_GLContext>(mGlContext)))
    {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(mGlContext));
        mGlContext = nullptr;
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
        SDL_Quit();
        mInitFailed = true;
        return;
    }
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

SdlImGuiApplication::~SdlImGuiApplication()
{
    mHost.shutdown();
    if (mGlContext != nullptr)
    {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(mGlContext));
        mGlContext = nullptr;
    }
    if (mWindow != nullptr)
    {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    SDL_Quit();
}

int SdlImGuiApplication::exec(const FrameCallback &frame)
{
    if (mInitFailed)
    {
        // Construction failed (e.g. display-less machine): report it without touching SDL state.
        mFinished.store(true);
        return 1;
    }
    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                quit = true;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                quit = true;
            }
        }
        mHost.begin_frame();
        const bool want_quit = frame();
        mHost.end_frame();
        SDL_GL_SwapWindow(mWindow);
        if (quit || want_quit)
        {
            break;
        }
    }
    mFinished.store(true);
    return 0;
}

SDL_Window *SdlImGuiApplication::window()
{
    return mWindow;
}

} // namespace cxxkit
