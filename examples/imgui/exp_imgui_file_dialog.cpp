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
// The application owns the SDL lifecycle and the frame loop; the lambda is pure per-frame UI.
// Requires a display + a GL 3+ context to run; headless machines get rc=1 from exec().
#include <cstdio>
#include <string>

// IGFD header relies on imgui.h being included first (upstream contract).
#include <cxxkit/3rdparty/imgui_file_dialog/ImGuiFileDialog.h>
#include <cxxkit/imgui/sdl3/sdl_application.hpp>

int main()
{
    cxxkit::SdlImGuiApplication app("cxxkit exp_imgui_file_dialog", 1280, 720);

    // ---- IGFD block: modal open-file dialog; default flags already include Modal ----
    static std::string picked_path;
    const int rc = app.exec(
        []() -> bool
        {
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
            return false;
        });
    return rc;
}
