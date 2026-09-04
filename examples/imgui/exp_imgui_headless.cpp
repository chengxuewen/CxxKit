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

// exp_imgui_headless: fake-backend 3-frame walkthrough — runs on headless CI (rc=0).

#include <iostream>

#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/fake_backend.hpp>

int main()
{
    cxxkit::FakePlatformBackend platform;
    cxxkit::FakeRendererBackend renderer;
    cxxkit::ImGuiHost host(platform, renderer);
    if (!host.init())
    {
        std::cerr << "imgui host init failed" << std::endl;
        return 1;
    }

    // The UI block below is byte-for-byte what a real rendering backend (SDL3) would run —
    // the ImGui::* calls never touch the backend; UI and backend are fully decoupled.
    for (int frame = 1; frame <= 3; ++frame)
    {
        host.begin_frame();
        ImGui::Text("frame %d", frame);
        static float value = 0.5f;
        ImGui::SliderFloat("value", &value, 0.0f, 1.0f);
        ImGui::Button("click");
        host.end_frame();
        std::cout << "frame " << frame << ": vertices=" << renderer.last_vertex_count() << std::endl;
    }

    std::cout << "Real-window rendering lives in exp_imgui (SDL3 backend). This example proves the "
                 "UI code path is backend-decoupled."
              << std::endl;
    return 0;
}
