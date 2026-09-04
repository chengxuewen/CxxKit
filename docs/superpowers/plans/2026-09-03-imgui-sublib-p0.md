# cxxkit/imgui 落地实施计划（P0：核心骨架 + fake backend）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 CxxKit 建立 `cxxkit/imgui` 子库——imgui 核心 vendored + 可扩展后端抽象 + headless 测试/示例，examples 矩阵 17/17 收官。

**Architecture:** 三层——① vendored imgui v1.92.9b（extract-only，上游 5 cpp 直编进 target）② wrapper（imgui_global 导出宏 + Platform/Renderer 双接口 + ImGuiHost 生命周期）③ fake backend（headless 测试桩）。opt-in 开关 `CXXKIT_ENABLE_LIB_IMGUI`（父开关全建，决策 3/7）。P1 加 SDL3 真后端 + implot/ImGuizmo 扩展；P2 按需（implot3d/imgui_markdown/FileDialog/imnodes）。

**Tech Stack:** C++11 库门禁（imgui core 官方 C++11 兼容——实证 imgui.h "now using C++11 standard version"）/ CMake cxxkit_add_library helper / vendored gtest 1.12.1 / clang-format 23.1.0（pixi）。

**Spec:** 团队调研 cxxkit-imgui-research（2026-09-03）+ 交互式裁定 7 项（见下表）。两个失联研究员职责由 lead 实证补位（GitHub API 直查）。

**审核状态:** Momus 审核（bg_ce6be0c5）结论 APPROVE-WITH-FIXES——F1（coverage.sh 排除清单+决策 8）/ F2（INSTALL_DIR+头暂存+PUBLIC_LIBRARIES）/ F3（COMPILE_DEFINITIONS 通道+imgui_global 收编 include+nm 断言）已全部修订进本版；F4/F5 顺手改；F6 备案 P1。

## 已裁定决策（7/7，2026-09-03 会话）

| # | 决策点 | 裁定 | 实证锚点 |
|---|---|---|---|
| 1 | wrap 形态 | **extract-only**（无 .a 中间产物，上游 few-files 直编 target）| imgui.h L85-86 `#ifndef IMGUI_API` 空默认守卫 → `IMGUI_API=CXXKIT_IMGUI_API` 重定义可行；行业先例 imgui_bundle/vcpkg（port 仅装头）全是直编派 |
| 2 | P0 范围 | **fake backend 先行**，真实平台层归 P1 | 零外部依赖三树全绿可断言；验证分层纪律（crash/network 同款） |
| 2b | P1 平台层 | **SDL3 主选**（完整 wrap——有自有构建系统走 libyuv 式）| sdl3/sdlrenderer3/sdlgpu3 三 backend 官方内置；dummy driver 无头 CI 可运行；GLFW 降 P2 |
| 3 | 扩展开关 | **父开关全建**（`CXXKIT_ENABLE_LIB_IMGUI` 单旋钮） | 2^N 矩阵是维护陷阱；cxxkit 现有 opt-in 全是库级 |
| 4 | 扩展梯队 | **P1: implot(6214★)+ImGuizmo(4013★) · P2: implot3d/imgui_markdown/FileDialog/imnodes · 按需: ColorTextEdit** | 8+2 仓库活跃度/许可全查（2026-09-03），全 MIT 除 markdown(Zlib) |
| 5 | 后端架构 | **Platform+Renderer 双接口 + ImGuiHost**（宿主注入，cxxkit 永不开窗）| imgui 上游 impl 文件本身分 platform/renderer；N+M 实现覆盖 N×M 组合 |
| 6 | imgui 版本 | **v1.92.9b** stable（2026-07-31），不用 docking | 最新 release；docking/master 双线活跃但无 docking 诉求，不钉巨面 API |
| 7 | C++ 标准 | 库 target 跟 D25 默认 C++11，零提升；未来重扩展子 target 单独抬 | imgui core C++11 兼容实证 |
| 8 | 上游覆盖率口径 | coverage.sh 排除清单追加 5 个上游 unit（imgui/imgui_draw/imgui_tables/imgui_widgets/imgui_demo.cpp）——extract-only 直编使 .gcda 落 cxxkit/imgui/ 下不被现有 \$BUILD_DIR/3rdparty/* 过滤命中；全口径门禁继续覆盖 cxxkit 自有 wrapper/fake | libyuv 先例：wrap 目录独立构建天然被路径排除；extract-only 破此假设（Momus F1）|

## 消费方依据

- B1 延后项正式解除（media 半边已落地，imgui 半边本计划承接）
- examples 矩阵 16/17 唯一 N/A（imgui 空目录）——本计划 P0 即转正
- 远程控制宿主项目的 GUI 面需求（-imgui 系工具链是桌面工具标配）

## 风险与缓解

| # | 风险 | 缓解 |
|---|---|---|
| R1 | imgui 月更 vs vendored pin | 版本进 .7z 文件名；不追月更，按需升级 |
| R2 | 扩展与 imgui 版本强耦合 | 扩展 .7z 与 imgui .7z 同 commit 锁定；manifest sidecar 记三体版本 |
| R3 | 平台矩阵 | P0-P1 只验 Linux x64；macOS 顺手；Windows 归档（crash 同惯例） |
| R4 | SHARED/ABI | IMGUI_API 重定义经 imgui_global.hpp 强制包含顺序；build-shared 树验证（T2 验收项） |
| R5 | imgui_demo.cpp 是否编入 | 编入（体积小，ShowDemoWindow 可用；IMGUI_DISABLE_DEMO_WINDOWS 可关） |
| R6 | 覆盖率分母膨胀 | **上游 5 cpp 同样进全口径**（.gcda 落 cxxkit/imgui/ 下，\$BUILD_DIR/3rdparty/* 过滤不命中）→ coverage.sh 排除清单追加 5 个 unit 名（决策 8）；cxxkit 自有 context/fake_backend.cpp 推水位 ≥80%（C12） |
| R7 | headless 测试盲区 | 三层测试：context-only 单测（主力，可断言）→ fake backend 协议时序 → 真实 backend 编译期+手动 smoke（P1 起诚实标注） |

---

## Task 0（人工/controller）：下载与打包 imgui v1.92.9b

前置：无。产出：`3rdparty/imgui-v1.92.9b.7z`。

- [ ] 0.1 下载上游 release tag v1.92.9b（github.com/ocornut/imgui/releases）到 /tmp
- [ ] 0.2 剔除 `.git/`、`misc/`（绑定示例非必需）；保留 `imgui_fonts/`（默认字体 ProggyClean）
- [ ] 0.3 `7z a 3rdparty/imgui-v1.92.9b.7z imgui-v1.92.9b/`（顶层目录进包，与现有 21 包同构）
- [ ] 0.4 记录 SHA256 进本计划或 wrap 注释（防篡改/可复现）；顺带记录 imgui_fonts/ 内字体各自许可到 wrap 注释/decisions D29（字体与 imgui 本体 MIT **不同许可体系**，见上游 docs/FONTS.md）
- [ ] 0.5 验证：包内含 7 个核心文件（imgui.h + 5 cpp + imconfig.h + LICENSE.txt）
- [ ] 0.6 **backends/ 目录完整保留**（P1 要用 imgui_impl_sdl3.cpp/imgui_impl_opengl3.cpp；体积代价 ~2MB 源码，可接受）

## Task 1：FindWrapImGui（extract-only wrap）

前置：Task 0。参考：`cmake/wrap/FindWrapLibyuv.cmake`（结构）——**去掉编译/安装静态库部分**。

- [ ] 1.1 新建 `cmake/wrap/FindWrapImGui.cmake`：
  - target 守卫 `if(TARGET CxxKitWrapImGui::WrapImGui) return()`
  - `cxxkit_stamp_file_info` 防重复解包（C6）
  - 解包 `3rdparty/imgui-v1.92.9b.7z` 到 build 树 wrap 缓存目录（现有 wrap 同款路径约定——读 libyuv wrap 确认）
  - 定义 `CxxKitWrapImGui::WrapImGui` INTERFACE IMPORTED：
  - 定义 `CxxKitWrapImGui_INSTALL_DIR`（extract-only 无构建产物，但 `cxxkit_install_public_wrap_headers` 强制消费此变量——未设置则 WARNING + 头静默不装，Momus F2）
  - 头暂存布局：解包后将 imgui.h/imconfig.h/imgui_internal.h/imstb_textedit.h/imstb_truetype.h/imstb_rectpack.h + LICENSE.txt 拷贝到 `${CxxKitWrapImGui_INSTALL_DIR}/include/cxxkit/3rdparty/imgui/`
  - 定义 `CxxKitWrapImGui::WrapImGui` INTERFACE IMPORTED：
    - `INTERFACE_INCLUDE_DIRECTORIES` = `${CxxKitWrapImGui_INSTALL_DIR}/include`（构建/安装双侧 D6 路径一致：`<cxxkit/3rdparty/imgui/imgui.h>`）
    - 暴露变量 `CXXKIT_WRAP_IMGUI_SOURCES`（绝对路径列表）= imgui.cpp/imgui_draw.cpp/imgui_tables.cpp/imgui_widgets.cpp/imgui_demo.cpp（解包目录，非暂存目录）
  - wrap **不产任何 .a/.so**、不做 install(TARGETS)（头安装由消费侧 `cxxkit_install_public_wrap_headers` 完成——它从 INSTALL_DIR/include 拷贝）
- [ ] 1.2 验证：configure 时 wrap 被发现、解包、stamp 生效（二次 configure 不重解包——时间戳断言）
- [ ] 1.3 Self-review：变量/target 名与 Task 2 brief 引用一致

**验收**：configure 干净；二次 configure 秒级（stamp 命中）。

## Task 2：cxxkit/imgui 骨架（context + backend 抽象 + CMake 门控）

前置：Task 1。参考：`cxxkit/media/media_global.hpp`（导出宏）、`cxxkit/kernel/signals.hpp`（头风格）、cxxkit/crash/CMakeLists.txt（门控注册）。

- [ ] 2.1 `cxxkit/imgui/imgui_global.hpp`：
  - 箱式横幅 2026 + `CXXKIT_IMGUI_API`（`CXXKIT_BUILDING_IMGUI_LIB`/`CXXKIT_BUILD_SHARED_IMGUI` 切换，C10 惯例照抄 media_global）
  - `#define IMGUI_API CXXKIT_IMGUI_API` 重定义锚点 + 末尾直接 `#include <cxxkit/3rdparty/imgui/imgui.h>`——把"先于 imgui.h"顺序纪律从注释升级为头文件结构保证（Momus F3：消费方只 include imgui_global.hpp 即可）
- [ ] 2.2 `cxxkit/imgui/backend.hpp`（header-only 接口）：
  - `PlatformBackend`：`virtual bool init(void *native_window)` / `virtual void new_frame()` / `virtual void shutdown()` + 虚析构
  - `RendererBackend`：`virtual bool init()` / `virtual void render(ImDrawData *draw_data)` / `virtual void shutdown()` + 虚析构
  - 头注释：宿主注入模式说明（cxxkit 永不开窗/不建 GL context）
- [ ] 2.3 `cxxkit/imgui/context.hpp/.cpp`（ImGuiHost）：
  - 构造注入 `(PlatformBackend&, RendererBackend&)`；`CXXKIT_DISABLE_COPY_MOVE`
  - `bool init()`：CreateContext → IO 基本配置 → 两 backend init → 状态保存
  - `void begin_frame()`：platform.new_frame() → `ImGui::NewFrame()`
  - `void end_frame()`：`ImGui::Render()` → renderer.render(`ImGui::GetDrawData()`)
  - `void shutdown()`：两 backend shutdown → `ImGui::DestroyContext()`；幂等
  - 析构兜底：未 shutdown 自动调（RAII）
  - 实现细节自裁：pimpl（DEFINE_DPTR）或直接成员——择简
- [ ] 2.4 `cxxkit/imgui/CMakeLists.txt`：
  - `cxxkit_add_library(cxxkit_imgui ...)`——**不传类型关键字**（跟随 CXXKIT_BUILD_SHARED_LIBS，numerics R20 先例）
  - SOURCES = context.cpp + `${CXXKIT_WRAP_IMGUI_SOURCES}`（上游 5 cpp 直编进 target——决策 1/7 核心）
  - `cxxkit_find_package(ImGui PROVIDED_TARGETS CxxKitWrapImGui::WrapImGui)`（对照 media 的 WrapLibyuv 用法）
  - 链接 `PUBLIC_LIBRARIES cxxkit::base cxxkit::tools CxxKitWrapImGui::WrapImGui`（helper 合法参数名 PUBLIC_LIBRARIES，media 先例原样——非"PUBLIC"措辞）
  - `cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImGui::WrapImGui|imgui)`（D6 头命名空间 `<cxxkit/3rdparty/imgui/...>`）
  - `COMPILE_DEFINITIONS IMGUI_API=CXXKIT_IMGUI_API`（PRIVATE）——上游 5 TU 不包含 imgui_global.hpp，编译定义是 IMGUI_API 到达它们的**唯一通道**；helper 注入的 `CXXKIT_BUILDING_IMGUI_LIB` 使 CXXKIT_IMGUI_API=export 态（Momus F3）
  - 头文件进 target 源列表（D15 GLOB CONFIGURE_DEPENDS）
- [ ] 2.5 根 `CMakeLists.txt`：`cxxkit_option(CXXKIT_ENABLE_LIB_IMGUI ...)` + 门控 add_subdirectory（照抄 crash 块位置）
- [ ] 2.6 验证：
  - `cmake -S . -B build -DCXXKIT_ENABLE_LIB_IMGUI=ON && cmake --build build --parallel` 0 error
  - OFF 构建 0 影响
  - build-shared 树（若启用）验证 R4：`nm -DC` 抽查 `libcxxkit_imgui.so` 中 `ImGui::CreateContext` 等符号**存在且可见**（非仅 link 成功——Momus F3 加固）
  - clang-format 新文件（禁碰 CMakeLists）

**验收**：ON 0 error / OFF 零影响 / 包含顺序实证（预处理或编译失败反证）。

## Task 3：fake backend + tst_imgui

前置：Task 2。测试策略三层之 1+2 层。

- [ ] 3.1 `cxxkit/imgui/fake_backend.hpp/.cpp`：
  - `FakePlatformBackend`：init 记录 window 指针、new_frame 计数、shutdown 标志 + 检查器（new_frame_count 等）
  - `FakeRendererBackend`：init/render(记录 draw_data 指针+顶点/索引快照)/shutdown 标志 + 检查器
  - 纯状态机，零平台/渲染依赖
- [ ] 3.2 `tests/tst_imgui.cpp`：
  - 生命周期全链（init→3×(begin→end)→shutdown）零崩溃；重复 shutdown 幂等
  - 帧计数断言：new_frame_count==3；render 调用 3 次
  - **DrawData 内容断言**：帧内 `ImGui::Text("hello")` → fake 快照顶点数 > 0（context 真实工作证明）
  - RAII：未 shutdown 析构（作用域 unique_ptr）→ 无泄漏（ASAN 兜底）
  - 双 host 串行隔离（imgui 全局单例——禁止嵌套/并行 host）
  - `ImGui::ShowDemoWindow()` 冒烟（R5：demo 编入）
- [ ] 3.3 `tests/CMakeLists.txt`：IMGUI 门控内 `cxxkit_add_test`（照抄 crash 测试条件注册样式）
- [ ] 3.4 验证：
  - ON：主 build + build-asan 双树 tst_imgui 全绿
  - OFF：ctest 总数不含 tst_imgui
  - 覆盖率：coverage.sh 排除清单已含 5 个上游 unit（决策 8 落地），summary.txt 不含 imgui.cpp/imgui_draw.cpp 等条目；cxxkit 自有新 .cpp（context/fake_backend）gcda 出现且 ≥80%（C12）——build-cov 树重配 IMGUI=ON 验证
  - clang-format

**验收**：双树绿 / OFF 不注册 / 顶点断言过 / 覆盖率水位达标。

## Task 4：exp_imgui + 文档同步

前置：Task 3。

- [ ] 4.1 `examples/exp_imgui.cpp`（40-70 行）：
  - 横幅 2026 + `// exp_imgui: headless imgui context walkthrough — real UI code, fake backend, zero GPU.`
  - Fake 双 backend 驱动 ImGuiHost 3 帧；每帧搭 UI（Text/SliderFloat/Button）——注释注明"与真实渲染后端下写法完全一致"
  - 打印帧号/widget 计数/最终 DrawData 顶点-索引统计（确定性）
  - 尾部提示：真实窗口渲染 = P1 SDL3 backend
- [ ] 4.2 `examples/CMakeLists.txt`：IMGUI 门控内注册 `cxxkit_exp_imgui`（链 cxxkit::imgui + 基线）
- [ ] 4.3 `README.md`：Examples 表 +1 行（exp_imgui | imgui | headless context walkthrough…）；Sublibraries 表 imgui 行从 N/A 更新
- [ ] 4.4 pkg-config：若 .pc 模板体系覆盖（对照 crash 先例），补 `-lcxxkit_imgui`；无模板则记录 skip 理由
- [ ] 4.5 验证：
  - ON：build 0 error + 手跑 3 次逐字节一致
  - OFF：examples 零影响
  - ctest 71/71 不回归（ON 时 72/72）
  - clang-format

**验收**：exp_imgui 确定性输出 / README 同步 / 矩阵 17/17 收官。

---

## 提交拆分（controller 收尾，用户确认后执行）

| 提交 | 内容 |
|---|---|
| ① | Task 0+1：imgui-v1.92.9b.7z + FindWrapImGui |
| ② | Task 2：cxxkit/imgui 骨架（wrapper + 门控）|
| ③ | Task 3：fake backend + tst_imgui |
| ④ | Task 4：exp_imgui + README/.pc 同步 |
| ⑤ | docs：status.md B1 解除记录 + decisions.md D29 + 本计划文档 |

## P1 预告（本计划不含，P0 验收后另立计划）

T5 SDL3 vendored（完整 wrap）· T6 cxxkit::imgui_sdl3 backend（imgui_impl_sdl3 + imgui_impl_opengl3，CI 编译期+本机手动 smoke；注意 1.92 起 ImTextureID 默认 `ImU64` 非 `void*`——fake 快照若加 texture-id 字段须用原类型）· T7 implot · T8 ImGuizmo · T9 exp_imgui 窗口化升级 + smoke checklist 文档。

## P2 候选（需求触发）

implot3d（T2 tier）· imgui_markdown（单头 Zlib）· ImGuiFileDialog · imnodes · 其它 backend（vulkan/dx12）· node editor。
