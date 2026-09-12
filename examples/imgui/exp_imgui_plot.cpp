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

// exp_imgui_plot: ImPlot 2D plotting — lines, histograms, in an SdlImGuiApplication window.
// The application owns the SDL lifecycle and the frame loop; the lambda is pure per-frame UI.
// Requires a display + a GL 3+ context to run; headless machines get rc=1 from exec().
#include <cmath>
#include <cstdio>
#include <random>

#include <cxxkit/3rdparty/implot/implot.h>
#include <cxxkit/imgui/sdl3/sdl_application.hpp>

int main()
{
    cxxkit::SdlImGuiApplication app("cxxkit exp_imgui_plot", 1280, 720);

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

    // ImPlot demo windows run inside the frame callback; the application owns the loop.
    // ImPlot context: CreateContext must follow the ImGui context creation (done by the
    // app constructor via ImGuiHost::init), DestroyContext must precede its destruction
    // (happens here, before the app destructor).
    ImPlot::CreateContext();
    const int rc = app.exec(
        [&]() -> bool
        {
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
            return false;
        });
    ImPlot::DestroyContext();
    return rc;
}
