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

// exp_imgui_nodes: imnodes minimal graph — two nodes, one link.
// The application owns the SDL lifecycle and the frame loop; the lambda is pure per-frame UI.
// Requires a display + a GL 3+ context to run; headless machines get rc=1 from exec().
#include <cstdio>

#include <cxxkit/3rdparty/imnodes/imnodes.h>
#include <cxxkit/imgui/sdl3/sdl_application.hpp>

int main()
{
    cxxkit::SdlImGuiApplication app("cxxkit exp_imgui_nodes", 1280, 720);

    // ImNodes context: CreateContext must follow the ImGui context creation (done by the app
    // constructor via ImGuiHost::init — imnodes Initialize() calls ImGui::MemAlloc and every
    // frame reads ImGui::GetIO); DestroyContext must precede ImGui teardown (app destructor).
    ImNodes::CreateContext();

    // ---- one static graph, drawn identically every frame; the application owns the loop ----
    const int rc = app.exec(
        []() -> bool
        {
            ImGui::Begin("Node graph");
            ImNodes::BeginNodeEditor();

            // Node 1: source (static attribute 1)
            ImNodes::BeginNode(1);
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("output");
            ImNodes::EndNodeTitleBar();
            ImNodes::BeginStaticAttribute(1);
            ImGui::TextUnformatted("source");
            ImNodes::EndStaticAttribute();
            ImNodes::EndNode();

            // Node 2: sink (static attribute 2)
            ImNodes::BeginNode(2);
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("input");
            ImNodes::EndNodeTitleBar();
            ImNodes::BeginStaticAttribute(2);
            ImGui::TextUnformatted("sink");
            ImNodes::EndStaticAttribute();
            ImNodes::EndNode();

            ImNodes::Link(1, 1, 2); // link id 1: attribute 1 -> attribute 2

            ImNodes::EndNodeEditor();

            // Hover report lives AFTER EndNodeEditor (imnodes.h: hover queries are post-editor).
            int hovered = -1;
            if (ImNodes::IsNodeHovered(&hovered))
            {
                ImGui::Text("hovering node %d", hovered);
            }
            else
            {
                ImGui::Text("drag nodes / pan with the middle mouse button");
            }
            ImGui::End();
            return false;
        });
    ImNodes::DestroyContext();
    return rc;
}
