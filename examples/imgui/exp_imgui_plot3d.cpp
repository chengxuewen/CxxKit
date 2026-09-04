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

// exp_imgui_plot3d: ImPlot3D — 3D surface/scatter in an SDL3+GL window.
// Requires a display + a GL 3+ context to run; headless machines exit with rc=1 at SDL_Init.
#include <cmath>
#include <cstdio>

#include <cxxkit/3rdparty/implot3d/implot3d.h>
#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>

#include "sdl_host.hpp"

int main()
{
    // ---- SDL lifecycle block: the EXAMPLE is the host and owns every SDL call ----
    imgui_example::SdlHost host_sdl;
    if (!host_sdl.init("cxxkit exp_imgui_plot3d", 1280, 720))
    {
        return 1;
    }

    // ---- cxxkit imgui block: ImGuiHost + ImPlot3D context ----
    // implot3d.h contract: CreateContext after ImGui::CreateContext (host.init does it),
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
    ImPlot3D::CreateContext();

    // ---- deterministic demo data: 32x32 ripple surface, z = sin(sqrt(x^2 + y^2)) ----
    // PlotSurface grid contract (see implot3d.h): x_count * y_count vertices per array,
    // xs constant along rows, ys constant along columns, idx = i * y_count + j.
    enum
    {
        kGrid = 32
    };
    float xs[kGrid * kGrid], ys[kGrid * kGrid], zs[kGrid * kGrid];
    for (int i = 0; i < kGrid; ++i)
    {
        for (int j = 0; j < kGrid; ++j)
        {
            const int idx = i * kGrid + j;
            xs[idx] = -3.0f + 6.0f * j / static_cast<float>(kGrid - 1);
            ys[idx] = -3.0f + 6.0f * i / static_cast<float>(kGrid - 1);
            zs[idx] = std::sqrt(xs[idx] * xs[idx] + ys[idx] * ys[idx]);
            zs[idx] = std::sin(zs[idx]);
        }
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
        ImGui::Begin("ImPlot3D demo");
        if (ImPlot3D::BeginPlot("ripple"))
        {
            ImPlot3D::SetupAxes("x", "y", "z");
            ImPlot3D::SetupAxesLimits(-3, 3, -3, 3, -1.5, 1.5);
            ImPlot3D::PlotSurface("z = sin(sqrt(x^2+y^2))", xs, ys, zs, kGrid, kGrid);
            ImPlot3D::EndPlot();
        }
        ImGui::Text("drag to rotate, scroll to zoom, right-drag to pan");
        ImGui::Text("frame %d", ImGui::GetFrameCount());
        ImGui::End();
        host.end_frame();
        SDL_GL_SwapWindow(host_sdl.window);
    }

    // ---- teardown: ImPlot3D first, then cxxkit imgui, then host SDL ----
    ImPlot3D::DestroyContext();
    host.shutdown();
    host_sdl.shutdown();
    return 0;
}
