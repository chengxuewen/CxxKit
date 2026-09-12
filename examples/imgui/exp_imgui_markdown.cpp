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

// exp_imgui_markdown: single-header markdown rendering — headings, lists, links, code.
// The application owns the SDL lifecycle and the frame loop; the lambda is pure per-frame UI.
// Requires a display + a GL 3+ context to run; headless machines get rc=1 from exec().
// imgui_markdown is header-only: the include contract is <imgui.h> BEFORE <imgui_markdown.h>;
// both arrive through cxxkit::imgui_markdown's INTERFACE include tree (D6 namespaced paths).
#include <cstdio>

#include <cxxkit/3rdparty/imgui/imgui.h>
#include <cxxkit/3rdparty/imgui_markdown/imgui_markdown.h>
#include <cxxkit/imgui/sdl3/sdl_application.hpp>

// Clicked-link sink: imgui_markdown calls it when a [link](url) is clicked (terminal print —
// an example has no browser to open). Registered via MarkdownConfig.linkCallback below.
static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data)
{
    printf("link clicked: %.*s\n", data.linkLength, data.link);
}

int main()
{
    cxxkit::SdlImGuiApplication app("cxxkit exp_imgui_markdown", 1280, 720);

    // Fixed demo document: H1/H2, bold+italic, list, code, link — everything defaultMarkdownFormatCallback styles.
    // static so the linkCallback can outlive this frame's stack (upstream demo uses file/static scope too).
    static const char *markdown_text = u8R"(
# CxxKit Markdown
A **single-header** renderer for *Dear ImGui* — vendored via `cxxkit::imgui_markdown`.

- headings and separators
- *italic*, **bold**, ***both***
- inline `code` spans

Click a link — it prints here:
[cxxkit on github](https://github.com)
)";
    static ImGui::MarkdownConfig markdown_config;
    markdown_config.linkCallback = &markdown_link_callback;

    const int rc = app.exec(
        []() -> bool
        {
            ImGui::Begin("Markdown");
            ImGui::Markdown(markdown_text, strlen(markdown_text), markdown_config);
            ImGui::End();
            return false;
        });
    return rc;
}
