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

#include <cxxkit/imgui/context.hpp>

namespace cxxkit
{

ImGuiHost::ImGuiHost(PlatformBackend &platform_backend, RendererBackend &renderer_backend)
    : mPlatformBackend(platform_backend)
    , mRendererBackend(renderer_backend)
    , mInitialized(false)
    , mShutdown(false)
{
}

ImGuiHost::~ImGuiHost()
{
    // RAII fallback: upstream requires DestroyContext to balance CreateContext; do it once.
    shutdown();
}

bool ImGuiHost::init()
{
    if (mInitialized)
    {
        // Idempotent success: simplest contract, documented — callers may call init() defensively.
        return true;
    }

    // Upstream: CreateContext installs the process-global current context; io (GetIO()) is only
    // valid after it. DisplaySize/DeltaTime and all input state are intentionally left to the
    // backend to fill each frame (headless semantics guaranteed by the backend, not here).
    ImGui::CreateContext();

    if (!mPlatformBackend.init(nullptr) || !mRendererBackend.init())
    {
        // Partial failure: roll the context back so a later init() starts from a clean state.
        ImGui::DestroyContext();
        return false;
    }

    mInitialized = true;
    mShutdown = false;
    return true;
}

void ImGuiHost::begin_frame()
{
    if (!mInitialized)
    {
        return;
    }
    // Upstream: NewFrame() asserts that io.DisplaySize and io.DeltaTime have been filled since the
    // previous frame — the platform backend's new_frame() pass owns that contract.
    mPlatformBackend.new_frame();
    ImGui::NewFrame();
}

void ImGuiHost::end_frame()
{
    if (!mInitialized)
    {
        return;
    }
    // Upstream: Render() finalizes the frame into draw lists; GetDrawData() may return nullptr when
    // no context state was rendered (e.g. zero displays) — forward whatever it returns and let the
    // renderer backend decide (upstream backends skip null draw data).
    ImGui::Render();
    mRendererBackend.render(ImGui::GetDrawData());
}

void ImGuiHost::shutdown()
{
    if (mShutdown)
    {
        // Idempotent: destructor calls shutdown() too, so a user shutdown() + destruction must not
        // double-destroy the context.
        return;
    }
    if (mInitialized)
    {
        mRendererBackend.shutdown();
        mPlatformBackend.shutdown();
        ImGui::DestroyContext();
        mInitialized = false;
    }
    mShutdown = true;
}

} // namespace cxxkit
