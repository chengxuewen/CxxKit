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

// exp_imgui_plot3d: ImPlot3D — 3D surface/scatter in an SdlImGuiApplication window.
// The application owns the SDL lifecycle and the frame loop; the lambda is pure per-frame UI.
// Requires a display + a GL 3+ context to run; headless machines get rc=1 from exec().
#include <cmath>
#include <cstdio>

#include <cxxkit/3rdparty/implot3d/implot3d.h>
#include <cxxkit/imgui/sdl3/sdl_application.hpp>

int main()
{
    cxxkit::SdlImGuiApplication app("cxxkit exp_imgui_plot3d", 1280, 720);

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

    // ImPlot3D context: CreateContext must follow the ImGui context creation (done by the
    // app constructor via ImGuiHost::init), DestroyContext must precede its destruction
    // (happens here, before the app destructor).
    ImPlot3D::CreateContext();
    const int rc = app.exec(
        [&]() -> bool
        {
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
            return false;
        });
    ImPlot3D::DestroyContext();
    return rc;
}
