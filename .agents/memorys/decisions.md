# cxxkit 决策记录

> 架构/技术决策 + 理由 + 参考。格式：`## D{N}: 标题`。

（暂无条目 —— 首个架构决策在此追加）

## D1: abseil 式子库组织（2026-08-18）

cxxkit 采用目录 = 子库 = CMake target 三位一体，参考 boost 按需取用理念但粒度取 abseil（单一仓库内细 target，不做独立包分发）。子库划分依据 OpenCTK source/ 13 个天然子目录。参考：abseil 组织方式。

## D2: 扁平命名空间 cxxkit::

所有子库共享 `cxxkit::` 顶级命名空间（abseil 的 absl:: 风格），不按子库嵌套；内部实现用 `cxxkit::detail::`。宏体系：`OCTK_*` → `CXXKIT_*`。

## D3: C++11 起步（用户明确要求，覆盖原 C++17 约定）

库代码最低 C++11；测试 target 用 C++14（gtest 1.12.1 要求）。OpenCTK 默认即 C++11，降级面极小（全 core 仅 2 文件用 std C++17 类型、2 文件用结构化绑定）。compile-test 宏体系保留（CXXKIT_BUILD_CXX_STANDARD_*）。

## D4: vendored 3rdparty 沿用（拒绝 FetchContent）

用户明确要求保留 octk 方式：vendored 压缩包 + stamp 防重复构建 + `find_package NO_DEFAULT_PATH` 强制 vendored。理由：删 build 目录后重新 configure 不报错（FetchContent 会挂）；杜绝与系统库冲突。三方头用**自定义命名空间路径** `<cxxkit/3rdparty/<lib>/...>`（构建树 + 安装树双侧拷贝一致），静态库随包分发 + 安装后 stub target 链接。

## D5: media/imgui 留原仓库（续建路径待定）

迁移范围仅 core + network；media（WebRTC 系）与 imgui 留在 OpenCTK。审核发现：core 迁出后 media（13 文件用 octk::、112 文件引用 core 头）无法编译——过渡期 OpenCTK 冻结 media/imgui 构建，续建路径（find_package(cxxkit) + 改名 / 旧副本 / 停构建）延后决策（设计 B1）。

## D6: 三方库机制决策（用户纠正，2026-08-18）

实现 Phase 1 时曾把三方 include 简化为原始路径（`fmt/...`、`tl/...`），用户指出应借鉴 octk 的自定义路径方式。已恢复：`#include <cxxkit/3rdparty/fmt/format.h>` 等，构建/安装双侧一致。教训：设计文档 §5.4 明确写过的机制不得在实现时擅自简化。

## D7: cmake helpers 移植清单

移植（精简）：cxxkit_option/add_subdirectory/compiler flags/fetch/stamp/find_package/install(含 public_wrap_headers)/configure(core_config 生成)/library/test。不移植：Qt-style 模块解析（ModuleHelpers/GlobalState）、PublicWalkLibs、SyncInclude、Android/Framework/SeparateDebugInfo、FFmpeg/Doxygen/Python 安装辅助。pkg-config 生成待 Phase 2。
