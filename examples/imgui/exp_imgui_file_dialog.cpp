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

// exp_imgui_file_dialog: IGFD modal file dialog — open a file, print the picked path.
// Requires a display + a GL 3+ context to run; headless machines exit with rc=1 at SDL_Init.
#include <cstdio>
#include <string>

#include <cxxkit/imgui/context.hpp>
// IGFD header relies on imgui.h being included first (upstream contract).
#include <cxxkit/3rdparty/imgui_file_dialog/ImGuiFileDialog.h>
#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>

#include "sdl_host.hpp"

int main()
{
    // ---- SDL lifecycle block: the EXAMPLE is the host and owns every SDL call ----
    imgui_example::SdlHost host_sdl;
    if (!host_sdl.init("cxxkit exp_imgui_file_dialog", 1280, 720))
    {
        return 1;
    }
    // ---- cxxkit imgui block: UI code below is identical to what any other backend would run ----
    cxxkit::Sdl3PlatformBackend platform;
    cxxkit::Sdl3RendererBackend renderer;
    cxxkit::ImGuiHost host(platform, renderer);
    if (!host.init())
    {
        printf("imgui host init failed: %s\n", SDL_GetError());
        host_sdl.shutdown();
        return 1;
    }

    // ---- IGFD block: modal open-file dialog; default flags already include Modal ----
    static std::string picked_path;

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
        ImGui::Begin("File dialog");
        if (ImGui::Button("Open..."))
        {
            // v0.6.8 API: filters are comma-separated extensions (".cpp,.h,.hpp" style); the
            // config struct carries path/selection settings — default flags imply a modal dialog.
            IGFD::FileDialogConfig config;
            config.path = ".";
            IGFD::FileDialog::Instance()->OpenDialog("PickFileDlg", "Pick a file", ".cpp,.hpp,.txt,.7z", config);
        }
        if (IGFD::FileDialog::Instance()->Display("PickFileDlg"))
        {
            if (IGFD::FileDialog::Instance()->IsOk())
            {
                picked_path = IGFD::FileDialog::Instance()->GetFilePathName();
            }
            IGFD::FileDialog::Instance()->Close();
        }
        ImGui::Text("picked: %s", picked_path.empty() ? "(none yet)" : picked_path.c_str());
        ImGui::End();
        host.end_frame();
        SDL_GL_SwapWindow(host_sdl.window);
    }

    // ---- teardown: cxxkit side first, then host SDL ----
    host.shutdown();
    host_sdl.shutdown();
    return 0;
}
