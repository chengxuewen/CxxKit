# cxxkit 项目状态

## 项目定位

cxxkit 是 OpenCTK（an open cpp toolkit）的成功重构版本 —— 精简、模块化。
跨平台 C++ 开发工具库，提供算法、数据结构、功能组件、脚本语言与实用框架。
采用 **abseil 式组织**：目录 = 子库 = CMake target 三位一体，按需取用（参考 boost 理念但粒度取 abseil）。

## 技术栈

- 语言：C++（**C++11 起步**，兼容至 C++23，按子库可升级），C
- 构建：CMake（3.15...3.31），`.cmake.conf` 承载编译配置
- 格式化：clang-format（`.clang-format`）
- 静态分析：clang-tidy / cppcheck
- 测试：GoogleTest 1.12.1（vendored）+ ctest
- 许可：MIT（含少量第三方开源代码，商用需替换/移除）

## 当前阶段（2026-08-18）

- [x] **Phase 1 完成**（合并至 main，commit `014e0ed`）：仓库骨架 + 8 子库 + 测试 + 安装
- [ ] Phase 2：剩余编译子库（thread/time/units/kernel）+ network + 剩余 ~58 测试
- [ ] 接入 CI（clang-format / clang-tidy / cmake build / ctest）

## 已落地子库（8 个 target）

| 子库 | 类型 | 内容 |
|---|---|---|
| `cxxkit::base` | header-only | 宏体系、类型、编译器检测、core_config.hpp（configure 生成）、3rdparty 命名空间中枢 |
| `cxxkit::containers` | header-only | vector/array_view/inlined_vector/flat_set/concurrent_queue/vector_map |
| `cxxkit::functional` | header-only | function_view/invocable/unique_function |
| `cxxkit::numerics` | header-only | bits/divide_round/numeric/safe_compare/safe_conversions/safe_minmax |
| `cxxkit::io` | 编译 | file_wrapper |
| `cxxkit::patterns` | header-only | singleton |
| `cxxkit::text` | 编译 | string/ascii/string_utils/format/string_view |
| `cxxkit::tools` | 编译 | logging/random/assert/checks/buffer/enum_flags/filesystem/optional/expected/...（Phase 1 只编 3 个 cpp） |

## 测试状态

- 6 个 gtest 套件通过（69 用例，与 OpenCTK 基线对等）：tst_array_view/checks/divide_round/enum_flags/function_view/file_wrapper
- **延后**：tst_inlined_vector（依赖 absl test_instance_tracker，OpenCTK 本身也未 vendored，原项目同样无法构建）
- 待迁移：~58 个测试（Phase 2）

## 三方库体系

- 17 个 vendored 压缩包 + 17 个 FindWrap 模块（`cmake/wrap/`），stamp 防重复构建
- **三方头命名空间**：`#include <cxxkit/3rdparty/<lib>/...>`（自定义路径，避免与系统库冲突——octk 方式）
- 构建树：`build/include/cxxkit/3rdparty/`；安装树：`<prefix>/include/cxxkit/3rdparty/`
- 静态库随包分发（`<prefix>/lib/`），安装后生成 stub target 链接（M3 方案）
- 9 个 stub：Fmt/StringViewLite/Optional/Expected/ConcurrentQueue/ReaderWriterQueue/Function2/Filesystem/Spdlog

## 安装体系

- `find_package(cxxkit COMPONENTS base text tools ...)` 消费验证通过
- 安装开关：dev build 默认不装；`-DCMAKE_INSTALL_PREFIX=xxx` 触发（`CXXKIT_BUILD_INSTALL`）
- `cxxkitConfig.cmake` + `cxxkitTargets.cmake`（EXPORT_NAME 去前缀，导出为 `cxxkit::<name>`）

## 待办

1. Phase 2 计划编写与执行（thread/time/units/kernel + network + 剩余测试 + benchmark 确认）
2. `.agents` 约定同步（conventions.md C4 已改为 C++11）
3. media/imgui 续建路径（设计 B1，已延后）
4. 接入 CI
