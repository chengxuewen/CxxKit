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

#ifndef CXXKIT_IMGUI_FAKE_BACKEND_HPP
#define CXXKIT_IMGUI_FAKE_BACKEND_HPP

#include <cxxkit/imgui/backend.hpp>
#include <cxxkit/imgui/imgui_global.hpp>

namespace cxxkit
{

/// @brief Headless platform-backend test double: records init/new_frame/shutdown call counts.
///
/// Fills io.DisplaySize/io.DeltaTime each new_frame() — upstream NewFrame() asserts these were
/// refreshed since the last frame, and there is no real window to do it in headless runs.
class CXXKIT_IMGUI_API FakePlatformBackend : public PlatformBackend
{
public:
    FakePlatformBackend();

    // PlatformBackend
    bool init(void *native_window) override;
    void new_frame() override;
    void shutdown() override;

    // Inspectors (all const).
    void *native_window() const { return mNativeWindow; }
    int new_frame_count() const { return mNewFrameCount; }
    bool is_shutdown_called() const { return mShutdownCount > 0; }
    int shutdown_count() const { return mShutdownCount; }

private:
    void *mNativeWindow;
    int mNewFrameCount;
    int mShutdownCount;
};

/// @brief Headless renderer-backend test double: counts render() calls and snapshots draw-data stats.
class CXXKIT_IMGUI_API FakeRendererBackend : public RendererBackend
{
public:
    FakeRendererBackend();

    // RendererBackend
    bool init() override;
    void render(ImDrawData *draw_data) override;
    void shutdown() override;

    // Inspectors (all const).
    bool is_init_ok() const { return mInitOk; }
    int render_count() const { return mRenderCount; }
    bool is_shutdown_called() const { return mShutdownCount > 0; }
    int shutdown_count() const { return mShutdownCount; }
    ImDrawData *last_draw_data() const { return mLastDrawData; }
    int last_vertex_count() const { return mLastVertexCount; }
    int last_index_count() const { return mLastIndexCount; }

private:
    bool mInitOk;
    int mRenderCount;
    int mShutdownCount;
    ImDrawData *mLastDrawData;
    int mLastVertexCount;
    int mLastIndexCount;
};

} // namespace cxxkit

#endif // CXXKIT_IMGUI_FAKE_BACKEND_HPP
