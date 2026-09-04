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

// exp_imgui_gizmo: ImGuizmo transform manipulator over a hand-built view/projection pair.
// Requires a display + a GL 3+ context to run; headless machines exit with rc=1 at SDL_Init.
#include <cmath>
#include <cstdio>

#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/sdl3/sdl3_backend.hpp>
// ImGuizmo.h does NOT include imgui.h itself (upstream contract, same as markdown):
// imgui headers must come first or ImDrawList etc. are undeclared.
#include <cxxkit/3rdparty/imguizmo/ImGuizmo.h>

#include "sdl_host.hpp"

// All matrices below are float[16] in COLUMN-MAJOR order (OpenGL convention): the element at
// (row r, column c) lives at m[c * 4 + r], so the translation vector occupies m[12], m[13], m[14].
// Perspective projection: symmetric vertical fov_y, aspect = width / height, OpenGL clip z in [-1, 1].
static void make_perspective(float fov_y_rad, float aspect, float z_near, float z_far, float m[16])
{
    const float f = 1.0f / tanf(fov_y_rad * 0.5f);
    for (int i = 0; i < 16; ++i)
    {
        m[i] = 0.0f;
    }
    m[0] = f / aspect;                                // column 0
    m[5] = f;                                         // column 1
    m[10] = (z_far + z_near) / (z_near - z_far);      // column 2, row 2
    m[11] = -1.0f;                                    // column 2, row 3: perspective divide term
    m[14] = 2.0f * z_far * z_near / (z_near - z_far); // column 3, row 2
}

// View matrix from eye looking at target: the camera basis (side, up, -forward) forms the ROTATION
// ROWS, stored transposed into column-major m; the last column carries -basis . eye. ImGuizmo
// follows the same convention as gluLookAt — camera looks down its local -z.
static void make_lookat(const float eye[3], const float target[3], const float up[3], float m[16])
{
    float fwd[3] = {target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
    const float fwd_len = sqrtf(fwd[0] * fwd[0] + fwd[1] * fwd[1] + fwd[2] * fwd[2]);
    fwd[0] /= fwd_len;
    fwd[1] /= fwd_len;
    fwd[2] /= fwd_len;
    const float side[3] = {fwd[1] * up[2] - fwd[2] * up[1],
                           fwd[2] * up[0] - fwd[0] * up[2],
                           fwd[0] * up[1] - fwd[1] * up[0]}; // fwd x up, unit: both are unit/orthogonal enough here
    const float cam_up[3] = {side[1] * fwd[2] - side[2] * fwd[1],
                             side[2] * fwd[0] - side[0] * fwd[2],
                             side[0] * fwd[1] - side[1] * fwd[0]}; // side x fwd
    m[0] = side[0];
    m[4] = side[1];
    m[8] = side[2];
    m[12] = -(side[0] * eye[0] + side[1] * eye[1] + side[2] * eye[2]);
    m[1] = cam_up[0];
    m[5] = cam_up[1];
    m[9] = cam_up[2];
    m[13] = -(cam_up[0] * eye[0] + cam_up[1] * eye[1] + cam_up[2] * eye[2]);
    m[2] = -fwd[0];
    m[6] = -fwd[1];
    m[10] = -fwd[2];
    m[14] = fwd[0] * eye[0] + fwd[1] * eye[1] + fwd[2] * eye[2];
    m[3] = 0.0f;
    m[7] = 0.0f;
    m[11] = 0.0f;
    m[15] = 1.0f;
}

int main()
{
    // ---- SDL lifecycle block: the EXAMPLE is the host and owns every SDL call ----
    imgui_example::SdlHost host_sdl;
    if (!host_sdl.init("cxxkit exp_imgui_gizmo", 1280, 720))
    {
        return 1;
    }

    cxxkit::Sdl3PlatformBackend platform;
    cxxkit::Sdl3RendererBackend renderer;
    cxxkit::ImGuiHost host(platform, renderer);
    if (!host.init())
    {
        printf("imgui host init failed: %s\n", SDL_GetError());
        host_sdl.shutdown();
        return 1;
    }

    // Object transform the gizmo edits — identity + a translation in front of the camera.
    static float matrix[16];
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
    matrix[12] = 0.5f;
    matrix[13] = 0.0f;
    matrix[14] = -2.0f;

    // Hand-built camera (no glm): look at the origin from (0, 1, 4).
    const float eye[3] = {0.0f, 1.0f, 4.0f};
    const float target[3] = {0.0f, 0.0f, 0.0f};
    const float up[3] = {0.0f, 1.0f, 0.0f};
    float view[16];
    make_lookat(eye, target, up, view);
    float projection[16];

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
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
        host.begin_frame();     // backend drains + feeds events, ImGui::NewFrame()
        ImGuizmo::BeginFrame(); // upstream contract: right after ImGui_XXXX_NewFrame()

        int width = 0;
        int height = 0;
        SDL_GetWindowSize(host_sdl.window, &width, &height);
        make_perspective(1.0472f /* 60 degrees */,
                         float(width) / float(height > 0 ? height : 1),
                         0.1f,
                         100.0f,
                         projection);

        ImGui::Begin("Gizmo");
        ImGui::Text("translation: %.2f, %.2f, %.2f", matrix[12], matrix[13], matrix[14]);
        if (ImGui::Button("translate"))
        {
            operation = ImGuizmo::TRANSLATE;
        }
        ImGui::SameLine();
        if (ImGui::Button("rotate"))
        {
            operation = ImGuizmo::ROTATE;
        }
        ImGui::SameLine();
        if (ImGui::Button("scale"))
        {
            operation = ImGuizmo::SCALE;
        }
        // Draw + edit inside this window's rect (upstream sample pattern: SetRect, then Manipulate).
        ImGuizmo::SetRect(ImGui::GetWindowPos().x,
                          ImGui::GetWindowPos().y,
                          ImGui::GetWindowWidth(),
                          ImGui::GetWindowHeight());
        ImGuizmo::Manipulate(view, projection, operation, ImGuizmo::LOCAL, matrix);
        ImGui::End();

        host.end_frame(); // ImGui::Render() + renderer.render(draw_data)
        SDL_GL_SwapWindow(host_sdl.window);
    }

    host.shutdown();
    host_sdl.shutdown();
    return 0;
}
