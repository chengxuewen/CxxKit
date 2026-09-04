# cxxkit/imgui P2b 实施计划（examples/imgui 目录化 + 8 例全家桶）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** examples 矩阵的 imgui 家族从 1 例扩到 8 例（每 target 一例），目录化 `examples/imgui/`，共享 SDL3 宿主头消除 7 个窗口例的样板重复。

**Architecture:** 两层——① 共享宿主 `examples/imgui/sdl_host.hpp`（SDL_Init/CreateWindow/GL context/vsync + 失败路径 rc=1 + 退出清理，全部 60 行内 header-only，**example 内部共享件非库代码**）② 8 个 `exp_imgui_*.cpp` 各 30-80 行聚焦单一 API 面。headless 例独立存在（CI 可跑的 imgui 面）。编译期全绿 = 硬门禁；运行期 = 失败路径输出 + `docs/imgui-smoke.md` 人工门禁扩充。

**Tech Stack:** 同 P1/P2a（C++11 / vendored / pixi clang-format）。

**Spec:** 用户裁定全量 8 例目录化（2026-09-04 会话）。P2b = 本计划；P1 先例 T9 的 exp_imgui 结构直接复用。

**审核状态:** 轻量自审（同构流水线第四次重复；Momus 已审 P2a 同族先例）——派工前 controller 自查清单见文末。

## 决策与约束

| # | 约束 | 依据 |
|---|---|---|
| 1 | 目录化仅 imgui 家族（其余 16 例保持扁平）| 用户裁定；避免大面积迁移噪音 |
| 2 | 共享宿主头 `sdl_host.hpp` 是 **example 内部共享件**（不进 cxxkit/、不安装、不导出）| cxxkit-never-opens-windows 契约 |
| 3 | headless 例（fake backend）保留并独立成文件 | CI 无显示环境仍能跑 imgui 面 |
| 4 | exp_imgui 现名保留给窗口版（README 索引行同步）| T9 语义延续 |
| 5 | 每例 API 名 grep 各扩展头核实（ImPlot::/ImPlot3D::/ImGuizmo::/IGFD::/ImNodes::）| 本会话既定纪律 |
| 6 | ctest 72/72 不变（examples 不注册 ctest）；新增 example 不引入测试 | examples 惯例 |

## Task 14: 目录迁移 + 共享宿主头 + headless 拆分（重构批）

- [ ] 14.1 `git mv examples/exp_imgui.cpp examples/imgui/exp_imgui.cpp`；examples/CMakeLists.txt 的 SOURCES 路径改 `imgui/exp_imgui.cpp`（注册块其余不动）
- [ ] 14.2 从 exp_imgui.cpp 抽出 SDL 宿主块 → `examples/imgui/sdl_host.hpp`（header-only，命名空间 `imgui_example`，C++11；函数式接口：`struct SdlHost { SDL_Window *window; SDL_GLContext context; bool init(const char *title, int w, int h); void shutdown(); }`——grep SDL3 头核 API 真名；失败路径 printf SDL_GetError + 返回 false）
- [ ] 14.3 exp_imgui.cpp 改用 sdl_host（窗口块缩到 3 行）
- [ ] 14.4 新建 `examples/imgui/exp_imgui_headless.cpp`：现 exp_imgui 的 headless 三帧逻辑（P0 版内容）+ fake backend——CI 可跑；注册（无附加链接，仅 cxxkit::imgui）
- [ ] 14.5 验证：全链 build 0 error；exp_imgui 无显示 rc=1 行为不变；**exp_imgui_headless 本机 rc=0**（CI 语义）；ctest 72/72；clang-format 新文件

**验收**：迁移零行为变化 + headless 例本机真实可跑。

## Task 15: 五个窗口例 + markdown 例（扩展批，6 例）

前置：Task 14。每例结构 = sdl_host 块（3 行）+ imgui 帧循环 + **单一扩展 API 面演示**（30-60 行）+ README 索引行。**UI 代码与 P0 教学注释一脉相承**（"与真后端写法一致"）。

- [ ] 15.1 `exp_imgui_plot.cpp`：ImPlot 上下文（`ImPlot::CreateContext()` 在 host.init 后）+ 折线（`ImPlot::PlotLine` 双 sin/cos 数组）+ 直方图（`PlotHistogram`）+ 图例/坐标轴标签——grep implot.h 核 API 真名；**ImPlot context 创建/销毁与 ImGui context 的顺序**（implot demo 惯例：CreateContext 在 ImGui 之后、Destroy 在 ImGui 之前——以 implot.h 注释为准）
- [ ] 15.2 `exp_imgui_plot3d.cpp`：ImPlot3D context + 3D 散点/曲面（`ImPlot3D::PlotSurface` 若存在——grep implot3d.h；否则 PlotScatter 3D）+ 旋转视角提示
- [ ] 15.3 `exp_imgui_gizmo.cpp`：`ImGuizmo::BeginFrame()`（每帧必要）+ 视图/投影矩阵构造（glm 不引入——**手写 4x4 常量矩阵或 `ImGuizmo::` 的 Helpers**；grep ImGuizmo.h 看有没有矩阵工具）+ `Manipulate()` + 组件拖拽后矩阵回读打印（窗口内 Text 显示 translation）——**运行期才见真章，编译期保证 API 正确**
- [ ] 15.4 `exp_imgui_file_dialog.cpp`：`IGFD::ImGuiFileDialog::Instance()->OpenModal(...)` + `FileBrowser()` 渲染循环 + 选中路径 `GetFilePathName` 打印（窗口内）——grep ImGuiFileDialog.h 核 API；config 宏用默认
- [ ] 15.5 `exp_imgui_markdown.cpp`：按 Momus F4 契约（imgui.h 先 include）+ `ImGui::markdown(...)` 真名 grep（imgui_markdown.h 242 行 namespace ImGui）——渲染一段固定 markdown 文本（标题/列表/链接/代码块）
- [ ] 15.6 `exp_imgui_nodes.cpp`：`ImNodes::CreateContext` + BeginNodeEditor/BeginNode(1)/AddTitle/AddStaticAttribute/EndNode/EndNodeEditor 最小两节点一连线——grep imnodes.h 核 API
- [ ] 15.7 全部注册进 examples/CMakeLists.txt（IMGUI 门控块内，各自 `target_link_libraries` 链对应扩展 target：plot→cxxkit_imgui_plot、plot3d→…、gizmo→…、file_dialog→…、markdown→…、nodes→…）
- [ ] 15.8 验证：全链 build 0 error（编译期=API 正确性门禁）；7 窗口例本机 rc=1 失败路径输出各贴；headless 例 rc=0；ctest 72/72；clang-format 全部新文件

**验收**：8 例全编译 + headless 本机可跑 + 7 窗口例无显示行为诚实（rc=1 + SDL_GetError）。

## Task 16: 文档同步（收尾）

- [ ] 16.1 `docs/imgui-smoke.md` 扩充：8 例各自的可见验证点（plot 曲线可见/markdown 渲染样式/gizmo 拖拽后 translation 变化/file_dialog 选中路径打印/nodes 两节点一连线）
- [ ] 16.2 README Examples 表：exp_imgui 行改为指向 imgui/ 目录（8 例清单）
- [ ] 16.3 提交拆分建议：T14 一个（迁移+重构）、T15 一个（6 例）、T16 一个（docs）= 3 提交

## 自查清单（controller 派工前）

- [ ] sdl_host.hpp 的 SDL3 API 真名与 T9 已验证用法一致（SDL_GL_DestroyContext 等）
- [ ] 各扩展 context 创建顺序与上游 demo 惯例一致（grep 各头注释）
- [ ] IMGUI_API 契约：example 只 include D6 路径头 + cxxkit 头（不直连上游源根）
- [ ] PI 校验：无 sleep/无时序断言/输出确定性（headless）
