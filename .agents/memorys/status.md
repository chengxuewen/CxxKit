# CxxKit 项目状态

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
- [x] OpenCTK 残留问题归档：network_config.hpp 死引用、tst_platform_thread POSIX 链接、"36 个注释测试"（2026-08-20 评估：CxxKit 侧 41 个测试文件零注释；36 个是 OpenCTK 原仓未迁移文件，对应功能 json/xml/socket/http_server 不存在，无补全工作；inlined_vector 缺 absl test helper 明确不迁移）
- [ ] media/imgui 续建路径（设计 B1，已延后）
- [x] **2026-08-19 结构重构**：src/<sub>/ 合并入 cxxkit/<sub>/（49 cpp git mv，D14）；CMake 大小写统一（export/Config/.pc/doxygen GLOB，C7 落地）；text↔tools 循环依赖解除（PIT-5）；detail 私有头不再安装（PIT-3）；头文件进 target 源列表（D15，IDE 大纲可见）
- [x] **2026-08-20 target-helper 统一（C10）**：14 子库 CMakeLists 迁移至 `cxxkit_add_library`（显式 STATIC 保现状 R20）+ examples 迁移至 `cxxkit_add_executable`（R22）+ `cxxkit_add_test`（<name>_check target）——注册全走 helper，`grep -rnE "^(add_library|add_executable)" cxxkit/ examples/` 为空


## 已落地子库（14 个 target）

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
| `cxxkit::crash` | 编译 | crash handler/minidump（breakpad）+ 栈回溯（backward-cpp），opt-in `CXXKIT_ENABLE_LIB_CRASH`，vcpkg 导出 .7z 缓存（D17） |
| `cxxkit::profiling` | header-only | profiling.hpp — Tracy 后端包装（CXXKIT_PROFILE_SCOPE，opt-in，CXXKIT_ENABLE_LIB_TRACY） |

## 测试状态

- **33 个 gtest 套件 + crash 套件共 34 个全部通过（357 用例）**：crash 套件 `cxxkit_tst_crash`（4 用例，`CXXKIT_ENABLE_LIB_CRASH=ON` 时启用）——2026-08-19 实测 `ctest` 34/34 通过（91.5s）
- OpenCTK 65 个测试文件中仅 30 个实际启用（36 个注释掉：inlined_vector/crypto_random/file_utils/task_queue 等引用不存在的头）
- **不迁移**：tst_inlined_vector（absl test_instance_tracker 未 vendored）、tst_file_wrapper（io 子库已删）
- cxxkit_tst_context_checker 偶发 SEGFAULT，单独重跑即过（与 semaphore 时序测试同类，非回归）

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

1. ~~覆盖率补全~~ **已完成**（61.4% → 80.0%，11 个 0% 文件已补测：tst_once_flag/tst_task_queue_factory/tst_race_checker/tst_fake_clock/tst_id_registry/tst_shared_buffer/tst_string_encode/tst_base64/tst_metrics/tst_random + string_utils/random 扩展。29-file 全口径 80% 门禁达标。后续优化可补未覆盖分支（ascii 63%、metrics 65%、platform 71%）到更高水位。
2. media/imgui 续建路径（设计 B1，已延后）
3. clang-tidy warn-only 门禁→首跑后基线干净收紧为零告警（需 GitHub 侧 push 触发 CI；本机无 LLVM 工具链）
4. Windows/arm64 验证（项目声明非首要）
5. 剩余 36 个 OpenCTK 注释测试（已评估：对应功能不存在，N/A——见 PIT-24 前记录）

### 2026-08-19 崩溃库落地（D17 + vcpkg 基础设施）

- [x] **第 15 子库 `cxxkit::crash` 完成**（commit 47af11a 链，482be7b..47af11a 13 个提交）
- [x] **vcpkg 通用基础设施**：`cmake/InstallVcpkg.cmake`（`cxxkit_vcpkg_install_package`，QExt/OpenCTK 移植，默认自动拉取 + NO_FALLBACK 严格模式）；`cmake/wrap/FindWrapBreakpad.cmake` + `cmake/wrap/FindWrapBackward.cmake`（每库一 wrap，单包 .7z 缓存 + find_package + PIC/relocatability 门禁）
- [x] **测试 33→34 套件**：新增 `cxxkit_tst_crash`（4 用例：install 守卫/手动 minidump/崩溃产 dmp/栈打印 opt-in）——`ctest` 34/34 通过（91.5s）
- [x] **cxxkit_option 修复**：恢复 OpenCTK 动态类型切换（CACHE BOOL 无 FORCE 普通态 + 强制态 STRING 标志）——所有选项 GUI 可编辑
- 平台：Linux x64 全链路验证通过（vcpkg 自举→4 包构建→导出 .7z→解包→门禁→编译→测试）；macOS/Windows 归档待对应平台产出

### 2026-08-20 测试质量加固（D19：sanitizer/coverage/边界测试）

- [x] **CXXKIT_BUILD_SANITIZERS**（ASAN/LSAN/UBSan）：独立 `build-asan/` 目录；LSAN 全绿用 `LSAN_OPTIONS=suppressions=scripts/lsan.supp`
- [x] **CXXKIT_BUILD_COVERAGE** + `coverage` target：独立 `build-cov/`，产出 `build-cov/coverage/summary.txt`；库 .cpp 覆盖 ~41% → 61.9%（补充 6 套件后曾报 80.5% = **19-file 子集口径**；2026-08-24 全量 29 库 .cpp gcda 口径修正为 **61.4%（1705/2775）**——含 11 个 0% 文件，80% 门禁须以全口径为准）
- [x] **tst_boundary**：19 安全/边界/越界用例，普通 + ASAN 双环境全过 0 sanitizer 诊断
- [x] **真实 bug 修复（sanitizer 暴露）**：F1 平台线程泄漏（恢复 deref + lsan.supp 抑制设计性单例）、F2 ElapsedTimer 有符号溢出、F3 测试 atomic 未初始化、F4 __forced_unwind 良性（glibc，不改）、F5 error.cpp FNV 溢出 → 无符号；另修 context_checker use-after-free（40/100 SEGFAULT → 100/100 稳定）
- [x] **check.sh 7 步**：format → C++14 门禁 → namespace → build → 主 ctest → sanitizer(4/7, build-asan) → coverage(7/7, build-cov 若存在)
- 验证：主 build 34/34（task_queue_thread 偶发 flaky，单跑过）；build-asan + lsan.supp 34/34 零诊断；crash=OFF（保守态）

### 2026-08-24 doxygen 完善 + flaky 根治 + 文档/CI

- [x] **doxygen 环境**：本机无 doxygen/sudo → 下载官方 1.9.8 tarball 到 `~/tools/`（免 sudo，兼容旧 glibc）；`Docs` target 首次真实运行成功（958 页 HTML / 21MB）
- [x] **doxygen warning 清零**：30+ 历史遗留警告全修（status.hpp @param ×6、type_info @list、singleton/type_list 模板特化 @cond、thread_pool/task_queue 参数、ascii/string_utils link、elapsed_timer enum、README/Doxyfile）；残留仅无害 fmt 递归别名
- [x] **注释补全**：9 个并行 doxygen 代理全灭（RPM 限流）→ 遗留损坏编辑 + 部分合法注释；逐文件 diff 纯注释审核后保留 ~25 文件注释（base/numerics/containers/functional/patterns/text/thread/kernel），还原 5 个破坏性编辑（PIT-25）
- [x] **TaskQueueThread flaky 根治（PIT-26）**：空队列 sleepTime=PlusInfinity → 睡满 1s wait 上限 + notify 无法缩短 deadline → 延迟任务迟到 1s（隔离 15/15 必现）。改 1ms 短轮询 → 15/15 绿 + 全 40/40 + shared/ASAN 绿
- [x] **semaphore flaky 修复**：MultiRelease/MultiAcquireRelease 固定 sleep_for(1ms) → startup 信号量 barrier（5 轮全量绿）
- [x] **构建/CI/文档**：BuildAll/BuildInstall 顶层判断（PROJECT_SOURCE_DIR==CMAKE_SOURCE_DIR，octk）；clang-tidy warn-only 门禁 + .clang-tidy；CONTRIBUTING/SECURITY/CHANGELOG/platform-support 落盘 + docs 索引
- [x] **测试 40 套件全绿**：main 40/40、shared 40/40、ASAN 40/40（零诊断）、coverage 61.4%（29 文件全口径，见待办 1）
- 验证：覆盖率门禁须按 29 库 .cpp 全口径（含 0% 文件），80.5% 历史数字是 19-file 子集

- [x] **PIT-28 stringCompare 逻辑修复**（`3f21081`）：实现把 `stringCaseCmp` 折叠逻辑写反——相等字节也 return false（调用方为零，低风险）。已修 + 断言更新
- [x] **覆盖率 80% 门禁达标**（`993d305`）：10 个新测试套件（once_flag/task_queue_factory/race_checker/fake_clock/id_registry/shared_buffer/string_encode/base64/metrics/random）+ string_utils/random 扩展。全量 50 套件 50/50 全绿，build-cov 29-file 口径 80.0%（2220/2775）
- [x] **CI coverage 门禁**（`9bca39f`）：Linux job 加 build-cov coverage 步骤（warn-only，独立目录），report 29-file 全口径 aggregate——填补 CI 无覆盖率监控的缺口
- [x] **edit-safety sed 规则**（`7abf176`）：sed -i 全局替换前 grep 验证匹配数量——tst_string_utils utils:: 替换误伤的教训
- [x] **测试 50 套件全绿**：main 50/50、shared 40+/40+、ASAN 50/50（零诊断），coverage 80.0% 门禁达标
