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

// exp_imgui: SDL3 + OpenGL3 windowed walkthrough via SdlImGuiApplication.
// The application owns the SDL lifecycle and the frame loop (QUIT / window close / ESC are
// internal); the lambda below is pure per-frame UI. Requires a display + a GL 3+ context to
// run; headless machines get rc=1 from exec() (init-failed path).
#include <cstdio>

#include <cxxkit/imgui/sdl3/sdl_application.hpp>

int main()
{
    cxxkit::SdlImGuiApplication app("cxxkit exp_imgui", 1280, 720);
    const int rc = app.exec(
        []() -> bool
        {
            ImGui::Text("frame %d", ImGui::GetFrameCount());
            ImGui::Text("fps %.1f", ImGui::GetIO().Framerate);
            static float value = 0.5f;
            ImGui::SliderFloat("value", &value, 0.0f, 1.0f);
            ImGui::Button("click");
            return false;
        });
    return rc;
}
