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
// Requires a display + a GL 3+ context to run; headless machines exit with rc=1 at SDL_Init.
// imgui_markdown is header-only: the include contract is <imgui.h> BEFORE <imgui_markdown.h>;
// both arrive through cxxkit::imgui_markdown's INTERFACE include tree (D6 namespaced paths).
#include <cstdio>

#include <cxxkit/3rdparty/imgui/imgui.h>
#include <cxxkit/3rdparty/imgui_markdown/imgui_markdown.h>
#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>

#include "sdl_host.hpp"

// Clicked-link sink: imgui_markdown calls it when a [link](url) is clicked (terminal print —
// an example has no browser to open). Registered via MarkdownConfig.linkCallback below.
static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data)
{
    printf("link clicked: %.*s\n", data.linkLength, data.link);
}

int main()
{
    // ---- SDL lifecycle block: the EXAMPLE is the host and owns every SDL call ----
    imgui_example::SdlHost host_sdl;
    if (!host_sdl.init("cxxkit exp_imgui_markdown", 1280, 720))
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
        ImGui::Begin("Markdown");
        ImGui::Markdown(markdown_text, strlen(markdown_text), markdown_config);
        ImGui::End();
        host.end_frame();
        SDL_GL_SwapWindow(host_sdl.window);
    }

    // ---- teardown: cxxkit side first (renderer + platform shutdown inside), then host SDL ----
    host.shutdown();
    host_sdl.shutdown();
    return 0;
}
