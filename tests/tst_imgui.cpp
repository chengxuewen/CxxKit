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

// cxxkit::imgui headless tests — ImGuiHost lifecycle over fake backends.
#include <cxxkit/imgui/context.hpp>
#include <cxxkit/imgui/fake_backend.hpp>

#include <gtest/gtest.h>

#include <memory>

using namespace cxxkit;

namespace
{

/// ImGui context is a process-global singleton: one host at a time, torn down between tests.
struct HostFixture : public ::testing::Test
{
    FakePlatformBackend platform;
    FakeRendererBackend renderer;
    ImGuiHost host;

    HostFixture()
        : host(platform, renderer)
    {
    }

    ~HostFixture() override
    {
        // Belt-and-braces: every test path below already shuts down; keep it safe if one didn't.
        host.shutdown();
    }
};

} // namespace

TEST_F(HostFixture, LifecycleFullChain)
{
    EXPECT_TRUE(host.init());
    for (int i = 0; i < 3; ++i)
    {
        host.begin_frame();
        host.end_frame();
    }
    host.shutdown();
    EXPECT_EQ(platform.new_frame_count(), 3);
    EXPECT_EQ(renderer.render_count(), 3);
    EXPECT_TRUE(platform.is_shutdown_called());
    EXPECT_TRUE(renderer.is_shutdown_called());
}

TEST_F(HostFixture, FrameCounting)
{
    EXPECT_TRUE(host.init());
    EXPECT_EQ(platform.new_frame_count(), 0);
    EXPECT_EQ(renderer.render_count(), 0);
    for (int i = 0; i < 3; ++i)
    {
        host.begin_frame();
        host.end_frame();
    }
    EXPECT_EQ(platform.new_frame_count(), 3);
    EXPECT_EQ(renderer.render_count(), 3);
}

TEST_F(HostFixture, DrawDataContent)
{
    EXPECT_TRUE(host.init());
    // Warmup frame: the first Render() emits an empty-but-valid ImDrawData; vertices only
    // materialize from the second frame onward (upstream 1.92 viewport draw-data lifecycle).
    host.begin_frame();
    host.end_frame();
    host.begin_frame();
    ImGui::Text("hello");
    float speed = 1.0f;
    ImGui::SliderFloat("speed", &speed, 0.0f, 10.0f);
    host.end_frame();
    ASSERT_NE(renderer.last_draw_data(), nullptr);
    EXPECT_TRUE(renderer.last_draw_data()->Valid);
    EXPECT_GT(renderer.last_vertex_count(), 0);
    EXPECT_GT(renderer.last_index_count(), 0);
}

TEST_F(HostFixture, ShutdownIdempotent)
{
    EXPECT_TRUE(host.init());
    host.shutdown();
    host.shutdown(); // second call must be a no-op, never a double-destroy
    EXPECT_EQ(platform.shutdown_count(), 1);
    EXPECT_EQ(renderer.shutdown_count(), 1);
    EXPECT_TRUE(platform.is_shutdown_called());
    EXPECT_TRUE(renderer.is_shutdown_called());
}

TEST_F(HostFixture, DestructorCleansUp)
{
    // ASAN watchpoint: init + one frame, then destroy without an explicit shutdown —
    // the RAII fallback must leave zero leaks and no dangling context.
    FakePlatformBackend scopedPlatform;
    FakeRendererBackend scopedRenderer;
    {
        std::unique_ptr<ImGuiHost> scopedHost(new ImGuiHost(scopedPlatform, scopedRenderer));
        ASSERT_TRUE(scopedHost->init());
        scopedHost->begin_frame();
        scopedHost->end_frame();
    }
    EXPECT_EQ(scopedPlatform.shutdown_count(), 1);
    EXPECT_EQ(scopedRenderer.shutdown_count(), 1);
    EXPECT_EQ(scopedPlatform.new_frame_count(), 1);
}

TEST_F(HostFixture, ShowDemoWindowSmoke)
{
    EXPECT_TRUE(host.init());
    // Upstream window lifecycle: a window becomes Active the frame after it is first begun,
    // so vertices materialize from frame 2 onward (probed empirically: frames 1+ emit vtx).
    for (int i = 0; i < 2; ++i)
    {
        host.begin_frame();
        ImGui::ShowDemoWindow(); // imgui_demo.cpp compiled in — R5 evidence
        host.end_frame();
    }
    EXPECT_GT(renderer.last_vertex_count(), 0);
}

TEST(ImguiHost, InitBeforeBackendInitOrder)
{
    // CreateContext must precede GetIO(): a bare GetIO() here (no context) is the regression
    // this test pins. init() sequence: CreateContext -> backends -> ready.
    FakePlatformBackend platform;
    FakeRendererBackend renderer;
    ImGuiHost host(platform, renderer);
    EXPECT_TRUE(host.init());
    host.begin_frame();
    host.end_frame();
    host.shutdown();
    EXPECT_EQ(renderer.render_count(), 1);
}
