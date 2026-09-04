# imgui smoke checklist (manual)

Windowed verification for `cxxkit::imgui` + the SDL3/OpenGL3 backend (`exp_imgui`). The CI has no
display, so compile-time gates run there; **this page is the runtime gate** — same honest layering as
crash/network examples.

## Prerequisites

- Displayed Linux (X11/Wayland) or macOS; Windows unverified (archived, same as the rest of the repo)
- GPU/driver with GL 3.0+ (upstream imgui_impl_opengl3 default GLSL)
- `CXXKIT_ENABLE_LIB_IMGUI=ON` configured build (pulls vendored imgui + SDL3 wraps)

## Steps

| # | Step | Expected |
|---|------|----------|
| 1 | `cmake -S . -B build -DCXXKIT_ENABLE_LIB_IMGUI=ON && cmake --build build --parallel` | configure + build 0 errors |
| 2 | `./build/examples/cxxkit_exp_imgui` | 1280x720 resizable window "cxxkit exp_imgui" appears |
| 3 | Look at the UI | ImGui default font renders crisply (no smear/overlap) |
| 4 | Drag the `value` slider | Thumb follows mouse, value animates smoothly |
| 5 | Read the `fps` line | ≈ display refresh rate (vsync on via `SDL_GL_SetSwapInterval(1)`; 60 on most panels) |
| 6 | Press ESC or click the window X | Window closes, process exits with rc=0 |
| 7 | Re-run with `2>stderr.log`; inspect log | No GL errors/warnings from the driver or imgui_impl_opengl3 |

## Known boundaries

- Headless/CI machines: the binary exits at the first gate with
  `SDL_Init failed: No available video device` (rc=1) — that is the designed failure path, not a bug.
- All SDL lifecycle calls (init/window/context/quit) live in the example — cxxkit never opens
  windows (host-owns-SDL contract, `cxxkit/imgui/sdl3/sdl3_backend.hpp`).
- `imgui` sublibrary unit tests (`cxxkit_tst_imgui`) cover the host/fake-backend contract only;
  the GL/SDL3 backend has no automated runtime test anywhere in this repo.
