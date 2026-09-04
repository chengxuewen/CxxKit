# cxxkit/imgui P2a 实施计划（4 件扩展：implot3d / imgui_markdown / ImGuiFileDialog / imnodes）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 `cxxkit/imgui` 补齐 T1 级扩展四件——implot3d / imgui_markdown / ImGuiFileDialog / imnodes，全部 extract-only wrap + 独立子 target，与 P1 T7/T8 完全同构（第三次重复的流水线）。

**Architecture:** 与 P1 T7+T8 字节级同构：`3rdparty/<name>-<ver>.7z`（extract-only 打包）→ `cmake/wrap/FindWrap<Name>.cmake`（头暂存 INSTALL_DIR + SOURCES 变量）→ `cxxkit/imgui/<ext>/CMakeLists.txt`（cxxkit_add_library + 双 include root + install(TARGETS EXPORT)）+ 父 CMakeLists `add_subdirectory`。**wrap_header 安装调用必须住进创建子目录**（PIT-34）。单开关 `CXXKIT_ENABLE_LIB_IMGUI`（决策 3）。

**Tech Stack:** C++11 库门禁（四件均兼容——见各 Task 兼容注记）/ CMake vendored+stamp / clang-format 23.1.0（pixi）。

**Spec:** 2026-09-04 生态全景调研（imgui 官方 Wiki + awesome-dear-imgui + 22 仓库 GitHub API 活跃度实证）→ T1 级四件入选；T2 级 9 件列入观察名单（见文末）。用户裁定批准 P2a 范围。

**审核状态:** Momus 审核（bg_16205649）结论 APPROVE-WITH-FIXES——F1（INTERFACE 链接被 helper 静默吞掉：须显式 target_link_libraries(INTERFACE) + wrap_header 住子目录）/ F2（IGFD:: 命名空间）/ F3（imnodes 无版本宏回退）/ F4（markdown 冒烟顺序：imgui.h 在前）/ F5（coverage 5 新 unit 排除核对）/ F6（保留 dirent+stb）/ F7（imnodes_internal.h 暂存）已全部修订进本版。上游实证补强：implot3d 实为 0.5 WIP（SOURCES 含 implot3d_meshes.cpp）；IFD v0.6.8 + IGFD_IMGUI_SUPPORTED_VERSION 1.92.3 兼容正向证据。

## 上游实证锚点（2026-09-04 GitHub API）

| 扩展 | 仓库 | 版本策略 | stars/pushed | 许可 | 形态 |
|---|---|---|---|---|---|
| implot3d | brenocq/implot3d | 最新 master（v0.2+？grep IMPLOT3D_VERSION）| 1255★ / 2026-08 | MIT | implot3d.cpp/implot_items.cpp/3d/…（读仓库实况） |
| imgui_markdown | juliettef/imgui_markdown | master（官方 wiki 活跃维护）| 1338★ / 2026-07 | **Zlib** | **单头** imgui_markdown.h |
| ImGuiFileDialog | aiekick/ImGuiFileDialog | 最新 release tag（v0.6.x？）| 1512★ / 2026-04 | MIT | ImGuiFileDialog.cpp/.h + ImGuiFileDialogConfig.h |
| imnodes | Nelarius/imnodes | 最新 master（v0.5+？grep IMNODES_VERSION）| 2485★ / 2026-05 | MIT | imnodes.cpp/imnodes_internal.h + 头 |

**版本纪律**：与 imgui v1.92.9b 的兼容性以"编译过=实证"为准（P1 T7 implot 先例——1.1 WIP 对 1.92 字体系统兼容实证）。四件中 markdown/filedialog/imnodes 对 imgui API 面依赖小，风险最低；implot3d 与 implot 同生态大概率已适配（brenocq 2026-08 仍活跃）。

---

## Task 10: implot3d vendored + cxxkit_imgui_plot3d

前置：无（P1 已闭环）。参考：`cmake/wrap/FindWrapImplot.cmake` + `cxxkit/imgui/plot/CMakeLists.txt`（直接模板）。

- [ ] 10.1 下载 implot3d master（--depth 1 或 tarball），剔除 `.git/`；`grep IMPLOT3D_VERSION implot3d.h` 记版本号 → 包名 `implot3d-<ver>.7z`（CMake tar 7zip）；拷入 `3rdparty/`；SHA256 记录
- [ ] 10.2 `cmake/wrap/FindWrapImplot3d.cmake`（照抄 FindWrapImplot，名字全替换）：头暂存 implot3d.h/implot3d_internal.h（以仓库实况为准——可能有额外的 3d 相关头）→ `${INSTALL_DIR}/include/cxxkit/3rdparty/implot3d/`；`CXXKIT_WRAP_IMPLOT3D_SOURCES` = 全部上游 .cpp（implot3d.cpp/implot_items.cpp/其它——grep 仓库 *.cpp 实况；demo 不编）
- [ ] 10.3 `cxxkit/imgui/plot3d/CMakeLists.txt`：`cxxkit_imgui_plot3d` target（照抄 plot——SOURCES/PUBLIC_LIBRARIES cxxkit::imgui/双 include root `$<BUILD_INTERFACE:...>`（imgui extract root + implot3d source root）/EXPORT_NAME imgui_plot3d/install(TARGETS EXPORT)/wrap_header 安装调用住本目录（PIT-34））
- [ ] 10.4 父 `cxxkit/imgui/CMakeLists.txt`：`add_subdirectory(plot3d)` 插在 plot 与 gizmo 之间（顺序无实质，保持字母序可读）
- [ ] 10.5 验证：ON 构建 0 error；`nm -C libcxxkit_imgui_plot3d.a | grep ImPlot3D::CreateContext`；stamp 二次命中；ctest 72/72；clang-format（.cmake 不动）

**验收**：构建/符号/stamp/回归四绿。

## Task 11: imgui_markdown vendored + cxxkit_imgui_markdown

前置：Task 10 无依赖（可并行）。参考：同上模板。

- [ ] 11.1 下载 juliettef/imgui_markdown master，剔除 `.git/`；包名 `imgui_markdown-<commitdate 或 version>.7z`（头内无版本宏则用 commit date）；拷入 + SHA256
- [ ] 11.2 `FindWrapImguiMarkdown.cmake`：**单头** imgui_markdown.h → `${INSTALL_DIR}/include/cxxkit/3rdparty/imgui_markdown/`；`CXXKIT_WRAP_IMGUI_MARKDOWN_SOURCES` = **空**（header-only！target 无上游 cpp——与 sdl3/plot/gizmo 的 compiled 形态不同，用 INTERFACE 处理或仍建空 compiled target？→ **裁定：`cxxkit_imgui_markdown` 用 INTERFACE target**（cxxkit_add_library 无 SOURCES 的 INTERFACE 形态——读 helper 签名确认支持；对照 cxxkit_profiling 先例）
- [ ] 11.3 `cxxkit/imgui/markdown/CMakeLists.txt`：INTERFACE target。**F1（Momus）：helper 对 INTERFACE 形态静默忽略 `PUBLIC_LIBRARIES` 参数**——链接必须显式：`target_link_libraries(cxxkit_imgui_markdown INTERFACE cxxkit::imgui CxxKitWrapImguiMarkdown::WrapImguiMarkdown)`（cxxkit_profiling/CMakeLists.txt 39 行先例）。**并补** `cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImguiMarkdown::WrapImguiMarkdown|imgui_markdown)` 住本目录（PIT-34 同款——否则安装树无 `<cxxkit/3rdparty/imgui_markdown/...>` 可 include）。install(TARGETS EXPORT)/install(DIRECTORY hpp) 照 cxxkit_profiling 写法
- [ ] 11.4 父接线 `add_subdirectory(markdown)`
- [ ] 11.5 验证：ON 构建 0 error（无编译单元）；**编译冒烟**：临时 TU `#include <imgui.h>` **在前**、`#include <cxxkit/3rdparty/imgui_markdown/imgui_markdown.h>` 在后（**F4（Momus）：markdown 头不自带 imgui include——90-98 行是注释示例，真实 include 仅 <stdint.h>，先 include imgui.h 是上游契约**），-I 两个暂存/源 root，编译过即验收；stamp；ctest 回归

**验收**：头暂存 + 冒烟 TU 编译过 + 回归绿。

## Task 12: ImGuiFileDialog vendored + cxxkit_imgui_file_dialog

前置：可并行。

- [ ] 12.1 下载 aiekick/ImGuiFileDialog 最新 release tag（**Momus 实证：最新 v0.6.8，头内 IGFD_VERSION "v0.6.9 WIP"；IGFD_IMGUI_SUPPORTED_VERSION "1.92.3" ≤ vendored 1.92.9b——兼容性正向证据**），剔除 `.git/`——保留主对 ImGuiFileDialog.cpp/.h + ImGuiFileDialogConfig.h；**F6（Momus）：包内保留 `dirent/` 与 `stb/` 子目录**（Linux 默认 config 走系统 dirent、stb 仅 USE_THUMBNAILS 需要——不参与编译，但删除会断未来 Windows/缩略图编译面；SOURCES/staged 头仍按最小集）；包名 `imguifiledialog-v0.6.8.7z`；SHA256
- [ ] 12.2 `FindWrapImGuiFileDialog.cmake`：头暂存 ImGuiFileDialog.h + ImGuiFileDialogConfig.h → `include/cxxkit/3rdparty/imgui_file_dialog/`；SOURCES = ImGuiFileDialog.cpp
- [ ] 12.3 `cxxkit/imgui/file_dialog/CMakeLists.txt`：`cxxkit_imgui_file_dialog` compiled target（照抄 gizmo 模板）；**注意 ImGuiFileDialogConfig.h 是用户可配头**——暂存进 D6 路径让消费方可 include 配置
- [ ] 12.4 父接线 `add_subdirectory(file_dialog)`
- [ ] 12.5 验证：同 Task 10 四绿。**F2（Momus）：头内命名空间实证为 `IGFD::`**——nm grep 用 `IGFD::`（勿用 ImGuiFileDialog::/FileDialog::，会假阴性）

**验收**：四绿 + config 头可从 D6 路径 include。

## Task 13: imnodes vendored + cxxkit_imgui_nodes

前置：可并行。

- [ ] 13.1 下载 Nelarius/imnodes master，剔除 `.git/`（上游仓库带 example/——裁到编译最小集：imnodes.cpp/imnodes.h/imnodes_internal.h）；包名 `imnodes-<ver>.7z`（**F3（Momus）：grep IMNODES_VERSION 当前 master 实测无版本宏**——无则按 11.1 同款回退：`imnodes-master-<yyyymmdd>.7z`）；SHA256
- [ ] 13.2 `FindWrapImnodes.cmake`：头暂存 imnodes.h **+ imnodes_internal.h**（**F7（Momus）：implot/implot3d 先例均暂存 internal 头——"wrap 所编=所暂存"对称性，零成本补齐**）→ `include/cxxkit/3rdparty/imnodes/`；SOURCES = imnodes.cpp
- [ ] 13.3 `cxxkit/imgui/nodes/CMakeLists.txt`：`cxxkit_imgui_nodes` compiled target（gizmo 模板）；imnodes 用 `IMGUI_API` 标记导出吗？grep 头确认——若有则接 IMGUI_API 通道，若纯声明则同 gizmo（无通道）
- [ ] 13.4 父接线 `add_subdirectory(nodes)`
- [ ] 13.5 验证：四绿（符号 `ImNodes::` 前缀）

**验收**：四绿。

---

## 收尾（controller，用户确认提交拆分后执行）

- [ ] C.1 README：Sublibraries 表 imgui 行更新（列四个新扩展 target）；Examples 无需动（P2 不配 example——扩展 API 由上游 demo 文档承担，与 T7/T8 同口径）
- [ ] C.2 status.md/decisions.md：D29 追加 P2a 段落（4 件 + 版本锚点 + T2 观察名单）
- [ ] C.3 提交拆分建议：每 Task 一提交（4 个）+ docs 1 个 = 5 提交；或按件两两合并 = 3 提交——派工后按实际 diff 再定
- [ ] C.4 全量终验：主 72/72 / asan 72/72 / cov 80.8% 口径不回归 / format 全净 / OFF 树零影响。**F5（Momus）：对比 build-cov/coverage/summary.txt 行数不含 5 个新上游 unit**（implot3d/implot3d_items/implot3d_meshes/ImGuiFileDialog/imnodes .cpp）——若被计入（路径过滤不命中时）向 scripts/coverage.sh 排除清单补录（决策 8 imgui 5 unit 同款）

## 测试策略说明（诚实分层）

- 扩展四件均为**编译期验证**（vendored 库代码不带 cxxkit 侧测试——上游行为由其自身 demo/测试承担，cxxkit 只做"正确编译+正确链接+头可达"的集成验证）——与 P1 T7/T8 同口径
- 运行期 UI 验证统一走 `docs/imgui-smoke.md` 人工门禁（扩展 demo 窗口在有显示环境人工触发）

## T2 观察名单（本批不做，触发条件备案）

| 项 | 触发条件 |
|---|---|
| imgui-node-editor（4501★）| imnodes 能力不足（blueprint 级需求）时替换/并存 |
| goossens/ImGuiColorTextEdit（fork 活跃）| 文本编辑需求出现（配置编辑器/日志高亮） |
| NetImgui（727★）| 远程控制宿主项目的远程 UI 需求正式立项 |
| imgui_test_engine（626★）| UI 自动化测试专项启动 |
| imgui-filebrowser（831★，C++17）| ImGuiFileDialog 不满足时（需同时解决 C++17 隔离） |
| knobs/toggle/coolbar 杂集 | 真实 UI 配置页需求积累到 3+ 小 widget |
| imgui_tex_inspect | 媒体纹理检查需求 + 上游复活 |
| 安装树 stub 消费验证 | C11 闭环专项（media cf9e1e9 模式扩展到 imgui 全家族） |

## 风险

| # | 风险 | 缓解 |
|---|---|---|
| R1 | implot3d 对 imgui 1.92 字体系统兼容性（brenocq 2026-08 活跃大概率已适配）| 编译过=实证；失败则 pin 上游兼容 commit（不魔改） |
| R2 | ImGuiFileDialog 仓库形态比"主对"复杂（config 头+可能的依赖文件）| 裁剪到编译最小集时以"编译器裁判"逐个试；README 集成节为准 |
| R3 | markdown/filedialog 的 C++ 标准版本 | 四件声明 C++11 兼容（markdown C++03 起；filedialog C++11；imnodes C++11；implot3d C++11）——编译器实测为准 |
| R4 | 四件与 imgui 全局符号交互（同 IMGUI_API 通道）| 与 P1 同款通道/无通道（纯声明），PIT-34 约束住进子目录 |
