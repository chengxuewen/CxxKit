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

// cxxkit::imgui application layer tests — ImGuiApplication base contract (T1-T3, test double, no
// display) and the SdlImGuiApplication headless failure path (T4, skips when a display is present).
#include <cxxkit/imgui/application.hpp>

#include <cxxkit/imgui/sdl3/sdl_application.hpp>

#include <SDL3/SDL.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdlib>
namespace
{

/// Deterministic double: runs exactly n_ frames (frame returns false) unless the frame callback
/// quits earlier; records the frame count and mirrors the base-class finished flag.
class TestDoubleApp : public cxxkit::ImGuiApplication
{
public:
    int exec(const FrameCallback &frame) override
    {
        while (mCalls < mFrames)
        {
            ++mCalls;
            if (frame())
            {
                break;
            }
        }
        mFinished.store(true);
        return 0;
    }

    int mFrames = 0;
    int mCalls = 0;
};

/// Probe for a usable display: T4 asserts the SDL_Init failure path, which is only reachable
/// without one (Momus fix 1: skip, never fail, on display-equipped dev machines).
bool has_display()
{
    const char *wayland = std::getenv("WAYLAND_DISPLAY");
    const char *x11 = std::getenv("DISPLAY");
    if ((wayland != nullptr && wayland[0] != '\0') || (x11 != nullptr && x11[0] != '\0'))
    {
        return true;
    }
    // Belt and braces: env vars can lie; ask SDL directly.
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Quit();
        return true;
    }
    return false;
}

} // namespace

TEST(imgui_application, base_contract_runs_frames_then_finishes)
{
    TestDoubleApp app;
    app.mFrames = 3;
    EXPECT_FALSE(app.is_finished());
    EXPECT_EQ(0, app.exec([] { return false; }));
    EXPECT_TRUE(app.is_finished());
    EXPECT_EQ(3, app.mCalls);
}

TEST(imgui_application, frame_returning_true_stops_after_one_call)
{
    TestDoubleApp app;
    app.mFrames = 5;
    EXPECT_EQ(0, app.exec([] { return true; }));
    EXPECT_EQ(1, app.mCalls);
    EXPECT_TRUE(app.is_finished());
}

TEST(imgui_application, frame_returning_false_runs_all_frames)
{
    TestDoubleApp app;
    app.mFrames = 3;
    // Capture-free lambda (FrameCallback is a copyable std::function): the double itself counts
    // calls and stops; this test pins that a never-quitting frame still runs exactly mFrames.
    EXPECT_EQ(0, app.exec([] { return false; }));
    EXPECT_EQ(3, app.mCalls);
}

TEST(imgui_application, sdl_headless_failure_path_sets_finished_and_returns_nonzero)
{
    if (has_display())
    {
        GTEST_SKIP() << "display present: SdlImGuiApplication constructs successfully here";
    }
    cxxkit::SdlImGuiApplication app("cxxkit tst", 100, 100);
    // Constructor must not fatal (Momus fix 1): failure is recorded, exec() reports it.
    const int rc = app.exec([] { return true; });
    EXPECT_NE(0, rc);
    EXPECT_TRUE(app.is_finished());
    EXPECT_EQ(nullptr, app.window());
}
