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

// exp_imgui_plot: ImPlot 2D plotting — lines, histograms, in an SDL3+GL window.
// Requires a display + a GL 3+ context to run; headless machines exit with rc=1 at SDL_Init.
#include <cmath>
#include <cstdio>
#include <random>
#include <cstdio>

#include <cxxkit/3rdparty/implot/implot.h>
#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>

#include "sdl_host.hpp"

int main()
{
    // ---- SDL lifecycle block: the EXAMPLE is the host and owns every SDL call ----
    imgui_example::SdlHost host_sdl;
    if (!host_sdl.init("cxxkit exp_imgui_plot", 1280, 720))
    {
        return 1;
    }

    // ---- cxxkit imgui block: ImGuiHost + ImPlot context ----
    // implot.h contract: CreateContext after ImGui::CreateContext (host.init does it),
    // DestroyContext before ImGui::DestroyContext (host.shutdown does it).
    cxxkit::Sdl3PlatformBackend platform;
    cxxkit::Sdl3RendererBackend renderer;
    cxxkit::ImGuiHost host(platform, renderer);
    if (!host.init())
    {
        printf("imgui host init failed: %s\n", SDL_GetError());
        host_sdl.shutdown();
        return 1;
    }
    ImPlot::CreateContext();

    // ---- deterministic demo data: fixed arrays, fixed seed (runs identically every frame/run) ----
    enum
    {
        kCount = 128
    };
    float xs[kCount], sine[kCount], cosine[kCount];
    for (int i = 0; i < kCount; ++i)
    {
        xs[i] = static_cast<float>(i);
        sine[i] = std::sin(i * 0.1);
        cosine[i] = std::cos(i * 0.1);
    }
    float samples[kCount];
    std::mt19937 rng(42u);
    std::normal_distribution<float> gauss(0.0f, 1.0f);
    for (int i = 0; i < kCount; ++i)
    {
        samples[i] = gauss(rng);
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
        host.begin_frame();
        ImGui::Begin("ImPlot demo");
        if (ImPlot::BeginPlot("signals"))
        {
            ImPlot::PlotLine("sin", xs, sine, kCount);
            ImPlot::PlotLine("cos", xs, cosine, kCount);
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("histogram"))
        {
            ImPlot::PlotHistogram("gauss", samples, kCount);
            ImPlot::EndPlot();
        }
        ImGui::End();
        host.end_frame();
        SDL_GL_SwapWindow(host_sdl.window);
    }

    // ---- teardown: ImPlot first, then cxxkit imgui, then host SDL ----
    ImPlot::DestroyContext();
    host.shutdown();
    host_sdl.shutdown();
    return 0;
}
