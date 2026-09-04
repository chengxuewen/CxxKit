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

// exp_imgui: SDL3 + OpenGL3 windowed walkthrough — real window, real GPU pipeline.
// Requires a display + a GL 3+ context to run; headless machines exit with rc=1 at SDL_Init.
#ifndef EXAMPLES_IMGUI_SDL_HOST_HPP
#define EXAMPLES_IMGUI_SDL_HOST_HPP

#include <cstdio>

#include <SDL3/SDL.h>

namespace imgui_example
{

// Minimal RAII-ish host for the SDL3 window + GL context. The EXAMPLE is the SDL host and owns
// every SDL call — cxxkit never inits SDL or creates windows (architecture contract).
// Partial-failure teardown mirrors the original exp_imgui.cpp block, step by step.
struct SdlHost
{
    SDL_Window *window = nullptr;
    SDL_GLContext context = nullptr;

    bool init(const char *title, int width, int height)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            printf("SDL_Init failed: %s\n", SDL_GetError());
            return false;
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
            SDL_Quit();
            return false;
        }
        context = SDL_GL_CreateContext(window);
        if (context == nullptr)
        {
            printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            window = nullptr;
            SDL_Quit();
            return false;
        }
        if (!SDL_GL_MakeCurrent(window, context))
        {
            printf("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
            SDL_GL_DestroyContext(context);
            context = nullptr;
            SDL_DestroyWindow(window);
            window = nullptr;
            return false;
        }
        SDL_GL_SetSwapInterval(1); // vsync
        return true;
    }

    void shutdown()
    {
        if (context != nullptr)
        {
            SDL_GL_DestroyContext(context); // SDL3 name; SDL2's SDL_GL_DeleteContext is a renamed alias
            context = nullptr;
        }
        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();
    }
};

} // namespace imgui_example

#endif // EXAMPLES_IMGUI_SDL_HOST_HPP
