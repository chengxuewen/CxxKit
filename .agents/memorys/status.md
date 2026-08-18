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

- [x] **Phase 1 完成**（commit `014e0ed`）：仓库骨架 + 8 子库 + 测试 + 安装
- [x] **Phase 2 完成**（commit `dae9c45`）：13 子库 + 33 测试（353 用例）+ network 消费验证
- [x] **基础设施完成**：CI（.github/workflows + scripts/check.sh）、pkg-config（13 个 .pc）、doxygen（Docs target）、examples（3 个）、BuildAll/BuildInstall target、FOLDER 归类、vcpkg 式三方依赖、默认 build/install 安装
- [ ] OpenCTK 残留问题归档：network_config.hpp 死引用、tst_platform_thread POSIX 链接、36 个注释测试（含 inlined_vector absl 依赖）
- [ ] media/imgui 续建路径（设计 B1，已延后）

## 已落地子库（13 个 target）

| 子库 | 类型 | 内容 |
|---|---|---|
| `cxxkit::base` | header-only | 宏体系、类型、编译器检测、core_config.hpp（configure 生成）、3rdparty 命名空间中枢 |
| `cxxkit::containers` | header-only | vector/array_view/inlined_vector/flat_set/concurrent_queue/vector_map |
| `cxxkit::functional` | header-only | function_view/invocable/unique_function |
| `cxxkit::numerics` | header-only | bits/divide_round/numeric/safe_compare/safe_conversions/safe_minmax |
| ~~`cxxkit::io`~~ | ~~编译~~ | ~~file_wrapper（WebRTC 版权，已删，改用 tools/filesystem）~~ |
| `cxxkit::patterns` | header-only | singleton |
| `cxxkit::text` | 编译 | string/ascii/string_utils/format/string_view/base64/bit_buffer/string_builder/string_encode/string_to_number |
| `cxxkit::tools` | 编译 | logging/random/assert/clock/status/error/once_flag/id_registry/shared_buffer/metrics/ntp_time/fake_clock + 头（checks/buffer/enum_flags/filesystem/optional/expected/variant/...） |
| `cxxkit::memory` | 编译 | aligned_malloc/shared_memory/zero_memory + 智能指针（shared/unique/ref_count） |
| `cxxkit::units` | 编译 | data_size/data_rate/frequency/time_delta/timestamp/unit_base |
| `cxxkit::time` | 编译 | date_time/elapsed_timer |
| `cxxkit::kernel` | 编译 | object/event/event_loop/signals/application（5 cpp） |
| `cxxkit::thread` | 编译 | thread_pool/task_queue/event_loop_thread/future/semaphore/...（14 cpp） |
| `cxxkit::network` | 编译 | http（cpr 后端，vendored cpr/curl/mbedtls） |

## 测试状态

- **33 个 gtest 套件全部通过（353 用例，与 OpenCTK 启用基线对等）**
- OpenCTK 65 个测试文件中仅 30 个实际启用（36 个注释掉：inlined_vector/crypto_random/file_utils/task_queue 等引用不存在的头）
- **不迁移**：tst_inlined_vector（absl test_instance_tracker 未 vendored）、tst_file_wrapper（io 子库已删）

## 三方库体系

- 21 个 vendored 压缩包 + 21 个 FindWrap 模块（`cmake/wrap/`），stamp 防重复构建
- **三方头命名空间**：`#include <cxxkit/3rdparty/<lib>/...>`（自定义路径，避免与系统库冲突——octk 方式）
- 构建树：`build/include/cxxkit/3rdparty/`；安装树：`<prefix>/include/cxxkit/3rdparty/`
- 静态库随包分发（`<prefix>/lib/`），安装后生成 stub target 链接（M3 方案）
- 15 个 stub：Fmt/StringViewLite/Optional/Expected/ConcurrentQueue/ReaderWriterQueue/Function2/Filesystem/Spdlog/Variant/Benchmark/Libcpr/Libcurl/MbedTLS（ZLIB 已删——curl 禁 zlib）
- **vcpkg 式三方依赖**：安装三方自有 Config.cmake（find_dependency 递归链 spdlog→fmt、cpr→CURL）+ 三方 .pc + bin/；stub 仅兜底 header-only lite 系
- curl 精简后端：禁 ssh2/nghttp2/brotli/zstd/ldap/zlib；macOS 需 CoreFoundation/SystemConfiguration/Security 框架（Config stub 附加）
- 已知限制：vendored .pc 绝对 prefix，安装树不可移动（vcpkg 同）

## 安装体系

- `find_package(cxxkit COMPONENTS base text tools network ...)` 消费验证通过
- **默认装到 `build/install/`**（octk 式构建安装测试）：`cmake --build build --target BuildInstall`
- 显式 `-DCMAKE_INSTALL_PREFIX=xxx` 或 `-DINPUT_CXXKIT_FEATURE_INSTALL_PREFIX=xxx` 装到指定位置
- 自定义 target：`BuildAll`（全量重建）、`BuildInstall`（构建+安装）、`Docs`（doxygen）
- `cxxkitConfig.cmake` + `cxxkitTargets.cmake`（EXPORT_NAME 去前缀，导出为 `cxxkit::<name>`）
- pkg-config：13 个 `.pc` 文件装到 `lib/pkgconfig/`，Requires 含三方链（fmt/spdlog/libcurl/mbedtls）

## 待办

1. media/imgui 续建路径（设计 B1，已延后）
2. 剩余 36 个 OpenCTK 注释测试是否补全（需先补依赖头）
3. clang-tidy 静态分析接入（CI 目前只有 format/build/test）
