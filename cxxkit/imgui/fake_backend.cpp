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
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <cxxkit/imgui/fake_backend.hpp>

namespace cxxkit
{

FakePlatformBackend::FakePlatformBackend()
    : mNativeWindow(nullptr)
    , mNewFrameCount(0)
    , mShutdownCount(0)
{
}

bool FakePlatformBackend::init(void *native_window)
{
    // Headless: no real window exists. Record a non-null sentinel so tests can distinguish an
    // uninitialized backend from an initialized one via native_window().
    mNativeWindow = (native_window != nullptr) ? native_window : this;
    return true;
}

void FakePlatformBackend::new_frame()
{
    ++mNewFrameCount;
    // Upstream NewFrame() asserts io.DisplaySize/io.DeltaTime were refreshed since the previous
    // frame. With no real window, the fake platform pass guarantees that contract here.
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    // Legacy path (renderer without ImGuiBackendFlags_RendererHasTextures): NewFrame() asserts the
    // font atlas is built; the fake backend builds it once up front.
    if (!io.Fonts->IsBuilt())
    {
        unsigned char *pixels = nullptr;
        int width = 0, height = 0, bytes_per_pixel = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
    }
}

void FakePlatformBackend::shutdown()
{
    // Idempotent by counting, not early-returning: double shutdown is observable, never harmful.
    ++mShutdownCount;
}

FakeRendererBackend::FakeRendererBackend()
    : mInitOk(false)
    , mRenderCount(0)
    , mShutdownCount(0)
    , mLastDrawData(nullptr)
    , mLastVertexCount(0)
    , mLastIndexCount(0)
{
}

bool FakeRendererBackend::init()
{
    mInitOk = true;
    return mInitOk;
}

void FakeRendererBackend::render(ImDrawData *draw_data)
{
    ++mRenderCount;
    mLastDrawData = draw_data;
    if (draw_data != nullptr && draw_data->Valid)
    {
        // Sum across all draw lists (imgui precomputes these on Render()).
        mLastVertexCount = draw_data->TotalVtxCount;
        mLastIndexCount = draw_data->TotalIdxCount;
    }
}

void FakeRendererBackend::shutdown()
{
    // Idempotent by counting, same discipline as the platform side.
    ++mShutdownCount;
}

} // namespace cxxkit
