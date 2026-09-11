# ImGuiApplication 计划：抽象基类 + SDL 后端（D40）

日期：2026-09-12 ｜ 依据：2026-09-12 团队两轮调研裁定（librarian 流派普查 + Metis 房规裁决收敛）
前置：D29（imgui 子库 + host-owns-SDL 契约）、D38/D39（引擎入口先例 + 契约修订口径）、D36 T7（kernel::Application 漏斗不变量参考）

## 一、动因与范式

imgui 子库现状"使用不方便"：SDL 生命周期（~100 行 + 错误回滚）躺在 examples/sdl_host.hpp 未安装，帧循环逐例手写。参考 **OpenCTK ImGuiApplication**（.refinfo 实证：抽象基类 + 后端子类 + Factory 注册 + set 三函数）与 **DearPyGui**（context/viewport/单帧/循环糖四分离），取其继承体系、弃其 YAGNI 件（Properties/Factory 注册表/set 三函数/SpinLock——团队裁定见 decisions D40 附录）。

**终态用户面**：
```cpp
cxxkit::SdlImGuiApplication app("demo", 1280, 720);
std::thread producer([&] { while (!app.is_finished()) { ++count; } });
int rc = app.exec([&] { ImGui::Text("count=%d", count); return ImGui::Button("Quit"); });
producer.join();
```

## 二、架构裁定（已定，不再重开）

| 裁定 | 内容 |
|---|---|
| R1 继承体系 | 抽象基类 `ImGuiApplication`（FrameCallback + 纯虚 exec + is_finished/atomic mFinished）+ 后端子类。**GLFW 留继承位不实现**（无 vendored wrap，YAGNI） |
| R2 不继承 kernel | imgui 零 kernel 依赖保住（D32 双 opt-in 解耦）；帧环 ≠ kernel 事件环；组合路径文档化（frame 回调里 `loop.process_events(16ms)`） |
| R3 入口 = A 案 | `virtual int exec(const FrameCallback&) = 0`；frame 返 bool（true=quit）；**无 setter/无 SpinLock/无存储**；`is_finished()` 基类 atomic |
| R4 配置 = C2 案 | `explicit SdlImGuiApplication(const std::string &title, int width, int height, bool vsync = true)`——位置参数，vsync 默认 true（对齐 sdl_host.hpp:79 硬编码现状） |
| R5 D29 契约修订 | kernel + imgui 核心（ImGuiHost/双 backend 契约）永不开窗**不变**；`cxxkit/imgui/sdl3/` 平台角落显式拥有 SDL 生命周期（Qt 分层同构：QtCore 无头 / QtGui 拥有 display） |

## 三、文件与内容

| # | 文件 | 内容 |
|---|---|---|
| F1 | `cxxkit/imgui/application.hpp`（新） | 抽象基类：`using FrameCallback = std::function<bool()>` + `virtual int exec(const FrameCallback&) = 0` + `bool is_finished() const`（atomic mFinished protected）+ virtual dtor。atomic 天然禁拷贝，不加 DISABLE_COPY_MOVE（冗余）。CXXKIT_IMGUI_API、箱式横幅、doxygen 英文 @since 0.2、`#pragma once` |
| F2 | `cxxkit/imgui/sdl3/sdl_application.hpp/.cpp`（新） | `SdlImGuiApplication : public ImGuiApplication`——ctor(title,w,h,vsync=true) 执行 sdl_host.hpp 同款生命周期（SDL_Init(VIDEO) → GL 属性(3.0 Core 硬编码，无 ES2 分支——Momus 修复 3) → CreateWindow(RESIZABLE) → CreateContext → MakeCurrent → SetSwapInterval(vsync)）；exec(frame)：SDL_PollEvent 排空（QUIT/WINDOW_CLOSE_REQUESTED/ESC→quit）→ `host.begin_frame()` → `frame()` 返 true 则 quit → `host.end_frame()` → SwapWindow；**ctor 失败语义（Momus 修复 1，T4 可测性关键）**：任一步失败**不 fatal 不抛**——照搬回滚序列（MakeCurrent 分支补齐 sdl_host 漏掉的 SDL_Quit()）后存 `mInitFailed` 标志，exec() 入口检测 → 置 mFinished=true 返 1；公共访问器 `SDL_Window *window()`（Momus 修复 2，前向声明保头文件 SDL 面最小）。teardown 逆序。私有成员：SDL_Window*/SDL_GLContext 前向声明（.cpp 含 SDL3/SDL.h）+ 双 backend + ImGuiHost **值成员（backend 声明先于 host——host 持引用）**。实现 exec 返回前 `mFinished.store(true)` |
| F3 | examples 迁移 | `examples/imgui/sdl_host.hpp` 删除；`exp_imgui.cpp`（窗口例）改用 SdlImGuiApplication（~10 行）；headless 例**零改动**（fake backend 路径，ImGuiHost 契约未动）；其余窗口例（plot/gizmo/file_dialog/markdown/nodes/plot3d）同款迁移——**gizmo 依赖 F2 新增的 window() 访问器**（每帧 SDL_GetWindowSize 算投影，Momus 修复 2） |
| F4 | 文档 | README imgui 行 + "ImGuiApplication (SDL backend)"; docs/imgui-smoke.md 窗口化段落改新用法；sdl3_backend.hpp 顶部契约注释同步 R5 措辞；decisions.md 追加 D40（R1-R5 + 流派普查结论 + OpenCTK 对照）+ status.md |

## 四、TDD 测试设计（先写测试再实现——本计划红绿顺序）

`tst_imgui_application.cpp`（gtest，CXXKIT_ENABLE_LIB_IMGUI 守卫，链接 imgui+imgui_sdl3）：

| 用例 | 断言 | 环境 |
|---|---|---|
| T1 基类契约：test-double 子类 exec 计数 N 帧后 mFinished=true，is_finished() 反映 | exec 返回值 + is_finished 序列（前 false/后 true） | 无显示 |
| T2 frame 返 true 即停 | frame 调 1 次即返，退出码 0 | 无显示（double） |
| T3 frame 返 false 继续 | 帧计数精确 =N | 无显示（double） |
| T4 headless SDL 失败路径 | SdlImGuiApplication::exec 返非零（SDL_Init no-video-device）+ is_finished()==true（失败也要置位）；**开头 GTEST_SKIP 护栏（Momus 修复 1）**：探测到显示环境（SDL_Init(VIDEO) 成功或 DISPLAY/WAYLAND_DISPLAY 非空）则 skip——dev 机带显示时 exec 正常返 0 非断言的非零 |
| T5 退出码语义 | exec 返回值透传（设计：返 0 正常；mInitFailed 返 1） | 无显示 |

**e2e（人工门禁，沿用 imgui-smoke.md 分层）**：有显示环境 `exp_imgui` rc=0 + 窗口渲染 + Quit 按钮/关窗/ESC 三路退出；`exp_imgui_headless` rc=0 零回归。CI 无显示环境跑 T1-T5（全部无显示可跑）。

## 五、任务分解（SDD）

### T1 — F1+F2 基类与 SDL 后端（TDD：先落 tst_imgui_application 的 T1-T5 RED → 实现 GREEN）
- 依赖：无（全新文件）
- 门禁：主树 ctest 全绿（新套件 +83→84）+ ASAN 全量零诊断 + format 干净 + headless 例 rc=0
- 提交：`feat(imgui): ImGuiApplication abstract base + SdlImGuiApplication backend (D40)`

### T2 — F3+F4 例子迁移与文档（依赖 T1）
- 门禁：主树全绿 + BuildInstall 新头在位 + format；有显示环境人工 smoke 交用户
- 提交：`refactor(imgui): examples adopt ImGuiApplication; docs sync (D40)`

### T3 — 收口（依赖 T2）
- decisions.md D40 + status.md；SDD workspace 清理
- 提交：`docs(memory): D40 records (imgui application)`

## 六、验收门禁（总）

1. `ctest --test-dir build` 全绿（含新 tst_imgui_application；83→84 套件）
2. ASAN 全量零诊断（正跑法）
3. `grep -rn "sdl_host" examples/ cxxkit/` → 清零（生命周期收编完成态）
4. headless 例 rc=0；有显示环境窗口例 rc=0（人工）
5. clang-format 全触碰文件干净；C18 英文提交/注释
6. 无显示环境 CI 矩阵：T1-T5 全部可跑全绿（真实 Sdl 失败路径 T4 也是无显示可测）

## 七、风险与备案

- **SDL_Window*/SDL_GLContext 前向声明**：SDL3 头 `struct SDL_Window` 前向声明合法（.cpp include 完整头）；GLContext 是 void* 别名——私有成员用 `void *mGlContext` 或前置声明形态，执行时按 SDL3 头实测定
- **ESC 语义保留**：现 exp_imgui 的 ESC→quit 约定随 exec 内部轮询保留（windowed 例既有行为零回归）
- **其他窗口例（plot 等）迁移**：机械替换，单提交内完成；若个别例有特殊 SDL 用法（如 file_dialog 的原生对话框），保留其宿主代码并在报告备案
- **vsync=false 路径**：SDL_GL_SetSwapInterval(0)，无测试覆盖（无显示环境测不了 swap 行为）——备案
EOF