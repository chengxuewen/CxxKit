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

### 2026-08-25 abseil/webrtc 移植（6 项功能）

- [x] **Task 1: flat_hash_map / flat_hash_set (Swiss Table)**（`f97d0a3`）：从 abseil-cpp 20220623.2 移植，简化实现（C++11，无 SIMD，无自定义 allocator）。3 新文件 921 行，30 tests。
- [x] **Task 2: ArrayView 扩展**（`36e8071`）：添加 `at()` 边界检查、`MakeArrayView`/`MakeConstArrayView` 工厂函数。+44 行，7 tests。
- [x] **Task 3: StatusOr<T>**（`1d04a4d`）：新增 `tools/status_or.hpp`，持有 Status 或 T 的期望值类型。192 行，9 tests。
- [x] **Task 4: Mutex / RWLock RAII 守卫**（`56d46ee`）：添加 `MutexLock`、`RWLock`、`ReadLock`、`WriteLock`（pthread 实现）。+61 行，6 tests。
- [x] **Task 5: PendingTaskSafetyFlag**（`39d4ba2`）：新增 `thread/pending_task_safety_flag.hpp`，防异步回调 use-after-free。94 行，7 tests。
- [x] **Task 6: Barrier (P1)**（`6099332`）：新增 `thread/barrier.hpp`，N 线程同步屏障（C++11 实现）。116 行，3 tests。
- [x] **文档补全**：许可横幅 + doxygen 注释（`afbacb6`）
- 验证：55/55 tests pass，coverage 80.0%（29 files 全口径）

### 2026-08-25 media 子库移植（第 16 子库）

- [x] **Task 1: libyuv vendored**（`bca08da`）：复用 OpenCTK FindWrapLibyuv + libyuv.7z（BSD-3, SVN 1916），CxxKit 命名空间适配
- [x] **Task 2: media 骨架**（`9eb9925`）：media_global、VideoType（CalcBufferSize inline 单归属 M6）、VideoRotation、ColorSpace（PrimaryID 访问器 API）、HdrMetadata
- [x] **Task 3: VideoFrameBuffer 接口家族**（`c72d5e0`）：header-only，test double（NativeI420Buffer 不依赖 Task 4，B1）；GetDataY/StrideY webrtc 命名规范（M2）
- [x] **Task 4: I420Buffer**（`b5e2ef9`）：SharedRefPtr 重构（M3）、libyuv 后端（I420Copy/Rotate/Scale）
- [x] **Task 5: VideoFrame 裁剪版**（`afc98d6`）：砍 RTP 字段，Builder + UpdateRect
- [x] **Task 6: VideoFrameBufferPool**（`0b9b383`）：I420-only（M4），list + HasOneRef 回池
- [x] **Task 7: webrtc_libyuv 转换层**（`ea4a101`）：CalcBufferSize 复用 video_types（M6）、I420Psnr/Ssim 命名（M2）
- [x] **Task 8: FramerateController**（`d9aa89e`）：OpenCTK ShouldDropFrame API
- [x] **Task 9: FrameGenerator + Capturer**（`77a6d49`）：M5 自包含裁剪（无 VideoTrackSource/broadcaster），同步 GenerateOneFrame（无线程，ponytail 注释）
- 验证：**63/63 tests pass**（+8 media 套件）；tst_mutex 偶发 flaky 单跑过
- [x] **覆盖率 80.3% 门禁达标**（`a98798a`/`ee3c643`）：扩展 color_space/webrtc_libyuv 测试；修复 hdr_metadata.cpp 空壳（3 个构造声明未定义 → link error，移植缺陷）——color_space.cpp 28.8%→85.6%，全口径 39 files 80.3%（2764/3440）
- 备注：libyuv 是唯一新增第三方依赖；media 链接 cxxkit::base/tools/thread + WrapLibyuv

- [x] **media 安装功能修复**（`cf9e1e9`）：media CMakeLists 补 install(TARGETS)+install(DIRECTORY)+install_public_wrap_headers(Libyuv)；CxxKitPkgConfigHelpers 补 -lyuv 映射；**cxxkitConfig 导出 stub 链接缺陷修复**——libyuv/spdlog/fmt 空 stub 无链接库导致消费方缺 -lyuv/-lspdlog/-lfmt（C11 消费验证暴露的既有缺陷），stub 补 INTERFACE_LINK_LIBRARIES 指向安装树 .a
- 验证：/tmp 消费方 find_package(cxxkit COMPONENTS media) + I420Buffer + ScaleVideoFrameBuffer 构建/链接/运行全通；link.txt 含 yuv+spdlog+fmt；**find_dependency(fmt/spdlog) 与 stub 双轨并存，stub 负责真链接**
- 备注：消费方需 C++14（function2 vendored 头依赖 C++14 语法，thread 引入）

### 2026-08-28 CXX 标准机制落地（D25）

- [x] **OpenCTK 式 C++ 标准选择机制**（`97bf4d9`，团队模式分析 OpenCTK/QExt 后设计）：三级优先链 `INPUT_CXXKIT_FEATURE_CXX_STANDARD`（父项目注入 FORCE）> `CMAKE_CXX_STANDARD`（FORCE）> 默认 11（CACHE STRING + STRINGS 下拉 {11,14,17,20,23,26}，GUI 可编辑）；非法值 FATAL_ERROR；阶梯变量 `CXXKIT_CXX_STANDARD_11..26`（0/1 向上）供 CMake 侧按标准条件启用；STATUS 打印 `Using C++: <std> (<source>)`
- [x] **变量名保留 `CXXKIT_FEATURE_CXX_STANDARD`**（用户裁定）——ConfigureHelpers/LibraryHelpers/ExecutableHelpers/core_config defines 零改动（重命名为 CXXKIT_CMAKE_CXX_STANDARD 式会静默打断 core_config 生成）
- [x] **测试 target 跟随主标准**（原硬编码 11）；Library/ExecutableHelpers 已有 CXX_EXTENSIONS OFF（团队报告误报，实际已存在）
- 已知语义：INPUT_ 经 -D 传入后残留 CMakeCache 持续 FORCE（GUI 编辑被覆盖，OpenCTK 同款，清 CMakeCache 解除）
- 验证：default=11 / -DINPUT_=14 / 非法 13→FATAL 三态 + 构建 + 63/63 tests；决策记录 decisions.md D25（`30592d5`）

### 2026-08-28 遗留改动验证落地（`908ad8f`）

- [x] **type_traits C++17 class 形式导入补全**：C++17 分支原只导入 `_v` 变量模板，`cxxkit::traits::is_void<T>`/`is_same<T,U>`（class 形式）编译失败——补 `using std::is_void; using std::is_same;`（其余 trait 均已双形式导入，这两个是漏网）。A/B stash 验证：修复前 static_assert 失败，修复后通过
- [x] **vendored gtest include 优先级**：TestHelpers 用 BEFORE PRIVATE 前置 vendored gtest 头目录（-I 优先于 -isystem，防 pixi gtest 1.17 头 + vendored 1.12 库 MakeAndRegisterTestInfo ABI 不匹配）
- 验证：全量构建 0 error + 63/63 ctest 全绿（95.65s）

### 2026-08-28 代码风格全库统一（D26，α/γ/β 三批次）

- [x] **批次 α**（`6dbcdc4`）：横幅 82 文件（cxxkit 23 + tests 37 + examples 3 + 5 年份修正；string_builder 双横幅保 WebRTC BSD；4 OpenCTK 改名）+ D8 引号 include 清零 + 5 尾随空格 + ref_counted_object 混用点修复 + LogLevelNum→kLogLevelNum。3 成员团队复查 8/8 PASS 补漏后收口
- [x] **批次 γ**（`54c9161`）：74 trailing-underscore 成员符号 → mPascalCase（29 文件 736 处），POD 聚合字段制度性保留纯 snake；迭代扫漏 3 轮到 0
- [x] **批次 β**（`c020811`+`f0e34d9`+`e8436fa`）：661 函数符号 → snake_case（camel 291 + Pascal 多词 346 + 残余 4；约 5900 调用点，210+ 文件）。22 类型别名甄别剔除；UpdateRect::Union 关键字例外；libyuv/gtest/gmock 上游符号豁免；SafeGt 族宏双参数化重构
- [x] **批次 δ**：coding-style.md 命名段整段替换 + AGENTS.md UNIQUE STYLES 更新（含守卫断言修正）+ C16⑥ 移植改名条款 + check.sh 双 gate（snake/mPascal，spec §4δ 修正版正则）+ D26 决策记录
- 验证：每批次构建 0 error + ctest 63/63 + clang-format 0 违规；双 gate 实跑 0/0
- 教训（D26 详录）：类型别名四重普查先行；宏 ## 拼接名扫描盲区；上游 C API/gtest 符号永久豁免；误伤整批回退优于就地补

### 2026-09-02 clang-format 全库统一（D27，pixi 工具链 + Allman 落地）

- [x] **pixi 工具链供 clang-format**：pixi.toml 加 `clang-format = ">=17,<24"`（conda-forge 独立包，pixi.lock 锁 23.1.0，~35MB 不拖 LLVM 工具链）；本机无系统 clang-format 的历史至此终结
- [x] **裸 namespace 宏迁移（N0）**：24 文件 72 处（media 20 + flat_hash 三件套 + video_types Allman 变体）`namespace cxxkit {` → `CXXKIT_BEGIN_NAMESPACE`；根因：宏内含 MSVC C4251 警告抑制对，裸 namespace 绕过之。containers 三件套顺带补 `#include <cxxkit/base/global.hpp>`（宏不可见曾致 145 error）
- [x] **clang-format 调用纪律（实测三坑）**：不递归目录（必须 find -print0 | xargs -0）；违规走 stderr（2>/dev/null 会出假 0）；基线 2076 行违例 → F1–F5 五批归零
- [x] **护栏**：macros.hpp 全宏区 clang-format off（覆盖 `#\s*define` 缩进变体，on 置 #endif 后）；compiler.hpp diff=0 免护栏；Ctrl 枚举表格接受碾平（用户裁定，abseil 同款）
- [x] **F1–F5 分批格式化**：header-only 六库 → text/tools → memory/units/time → thread/kernel → network/media/crash+tests+examples；每批 build 0 error + ctest 63/63
- [x] **单行函数裁定（交互确认两轮）**：`AllowShortFunctionsOnASingleLine` 先 All 后**回退 InlineOnly**（用户终裁）——类内内联单行（has_one_ref 样式），类外/cpp 定义一律多行（UpdateRect::is_empty 样式）；F6 补充批次展开 83 处 cpp 类外单行
- [x] **cpr 上游 API 还原（β 遗留 bug）**：http.cpp 37 处调用点（SetUrl/Get/IsIncludingSubdomains 等）被 β 批次误迁移成 snake——vendored cpr 1.9.9 是 PascalCase API；`.o` 缓存掩盖至今，format 触碰时间戳暴露；已全部还原 + 构建 0 error
- [x] **check.sh 门禁硬化**：clang-format 缺失时 WARN（不再静默跳过）+ pixi 路径兜底；grep 链 pipefail 零匹配兜底（`|| true` 包裹）；2/8 白名单补 WEBOCTK 注释
- 验证：全库 dry-run 0 违规（幂等复验）+ 构建 0 error + ctest 63/63；check.sh 1-5/8 绿（6/8 build-shared 需清缓存重配 Ninja——Makefiles 历史残留；7/8/8/8 asan/cov 树待增量重验）
- 教训：clang-format 每版本格式化结果有差异（lock 锁版本对冲）；类外定义单行不受 AllowShortFunctionsOnASingleLine 控制的认知被实测推翻（All 下保持）；.o 缓存会掩盖编译错误——大改动后应 touch 全量重编一次

### 2026-09-02 abseil/webrtc 第二批移植 P1（D28，9 件，SDD 流水线）

- [x] **numerics STATIC 化**（`cxxkit_numerics` INTERFACE→compiled）：exp_filter 为首个 .cpp；numerics_global.hpp 导出宏（CXXKIT_NUMERICS_API）；WrapOptional 链 + install_public_wrap_headers（tools 先例）；pkg .pc 含 -lcxxkit_numerics；build-shared 67/67 验证
- [x] **9 件移植**：running_statistics（Welford）/ sequence_number_util+unwrapper（回绕比较+展开，M 奇偶分支保留）/ percentile_filter（multiset 流式 P 分位）/ exp_filter（RTC 码控滤波）/ byte_order（load_be16/store_le32 族，平台宏版弃用）/ fixed_array（abseil 简化重实现，n=0 合法）/ str_split（by_any_of/by_string + skip_empty/allow_empty 标签分发）/ crc32（zlib 兼容语义，表驱动）
- [x] **TDD + SDD 流水线**：每件独立实现者子代理 + Momus 独立 review（8/8 首轮 APPROVED）；controller 裁定 2 起（Task 4 numerics 不传显式 STATIC——brief 与 C10 矛盾时从参考模式；Task 8 测试计数 70 正确——controller 误把 crash 条件块计入）
- [x] **测试 63→70**（+7 套件）：tst_running_statistics/tst_sequence_number/tst_percentile_filter/tst_exp_filter/tst_byte_order/tst_fixed_array/tst_str_split/tst_crc32
- [x] **实现者 token 腐化 3 起**（Task 4/5/6 写坏文件）——自愈预案固化：小块写+grep 自验+编译器裁判+损坏即删重写禁增量修补；clang-format 禁碰 CMakeLists（Task 7 新坑）
- [x] **文档同步**：README 子库表（numerics compiled + 新组件）、fixed_array 对齐限制文档、str_split 空 delimiter 文档修正（abseil per-byte 语义不移植的明示）、D28 决策记录
- 验证：每件 build 0 error + 定向 ctest + 全量 ctest + clang-format 0 违规；deferred minors 17 条中 2 条必修已修（fixed_array 对齐文档/str_split 文档），其余 no-action 备案
- P2 备忘：moving_max_counter/moving_average/event_rate_counter/node_hash_* 需求触发再取

### 2026-09-03 examples 一一对应补全（16/17 库全覆盖，imgui N/A）

- [x] **三批次 SDD 并行流水线**（P0 六代理 / P1 六代理 / P2 四代理，16 example）——提交 `33615e5` / `eefadf8` / `c69262e`
- [x] **矩阵终态**：base/containers/functional/kernel( signals-only)/media/memory/numerics/patterns/profiling/text/thread/time/tools(logging)/units 14 个默认路径可编译可跑；crash(CRASH=ON)/network(NETWORK=ON) 门控注册，验证层级诚实标注；imgui 空目录唯一 N/A
- [x] **controller 补刀**：exp_profiling 注册移出 if(TRACY) 块——cxxkit_profiling 本就是无条件空操作 INTERFACE target，旧二进制（8/19 TRACY=ON 产物）掩盖了默认路径 target 缺失
- [x] **README**：Examples 索引表（16 行：example/子库/亮点）+ 确定性声明（无真实外网请求/无故意崩溃）
- [x] **质量沉淀**：PIT-32（多线程工作项 push_back 共享 vector = 堆损坏，ASAN 实证，槽位预分配解法）/ PIT-33（TaskQueueThread::destroy() 丢弃 pending 非 drain，哨兵信号量模式）
- 验证：全量 build 0 error + ctest 71/71 + clang-format 0 违规 + 14 example 手跑 rc=0 + 确定性复验（thread 3 次逐字节/containers/media 3 次/kernel 2 次）
- 备忘：exp_crash/exp_network_version 的运行时验证分别等 CRASH=ON / NETWORK=ON 树（crash 树需 vcpkg breakpad 构建 10-25 分钟）

### 2026-09-03 cxxkit/imgui P0 落地（D29，B1 imgui 半边解除）

- [x] **调研→裁定→SDD 流水线**：团队调研（2 子代理交付 + 2 由 lead 补位实证 GitHub API）+ 8 项交互裁定 + Momus 计划审核（APPROVE-WITH-FIXES F1-F6 全修订）+ 4 Task SDD（T3 socket 断连由 controller 收尾验证）
- [x] **五提交**：`ab8f906`(vendored v1.92.9b + extract-only FindWrapImGui) / `15f8fc0`(ImGuiHost+双接口骨架) / `4db1fa7`(fake backend+7 用例+coverage 决策 8) / `732e056`(exp_imgui+README) / `5b76925`(计划文档)
- [x] **新 wrap 形态 extract-only**：无构建无 .a/.so，头暂存 INSTALL_DIR 供 install_public_wrap_headers，SOURCES 变量供 target 直编上游 5 cpp；`IMGUI_API=CXXKIT_IMGUI_API` 双通道（头收编 include + COMPILE_DEFINITIONS）
- [x] **测试**：tst_imgui 7 用例（生命周期/帧计数/DrawData 顶点>0/shutdown 幂等/RAII/demo 冒烟/顺序钉子）——imgui 1.92 headless 三坑实证并文档化
- [x] **examples 17/17 收官**：imgui 从唯一 N/A 转正（headless 形态，P1 升级窗口化）
- 验证：主 72/72 / asan 72/72 零诊断 / cov 80.8%（43 files，上游 5 unit 排除）/ exp_imgui 确定性 / OFF 零影响
- 备注：coverage.sh 新增 unit 名排除清单（basename 须用 $unit 变量——曾误用 basename 忘剥 .gcda 后缀）；SDL3 采纳为 P1 平台层主选（决策 2b）

### 2026-09-04 cxxkit/imgui P1 收官（D29 续，SDL3 + 扩展 + 窗口化）

- [x] **T5+T6**（`c3a9053`）：SDL3 release-3.4.16 **完整 wrap**（libyuv 形态——静态库 20MB 外部构建 10 分钟，stamp 幂等；X11/wayland 自动降级 headless）+ `cxxkit::imgui_sdl3`（Sdl3PlatformBackend：init 存 SDL_Window* + SDL_GL_GetCurrentContext→InitForOpenGL、new_frame 排空 PollEvent+quit_requested()；Sdl3RendererBackend：OpenGL3 Init/Render/Shutdown——宿主拥有 SDL 生命周期）+ imgui 主库 .pc 补齐（T4 遗留闭环）
- [x] **T7+T8**（`e2abbcf`）：implot v1.1 WIP + ImGuizmo v1.92.5 extract-only wrap + `cxxkit_imgui_plot`/`cxxkit_imgui_gizmo` target——implot demo 不编入库（与 imgui 自带 demo 反向裁定）；ImGuizmo 只取主件对（GraphEditor/imfilebrowser YAGNI）；**PIT-34**（IMPORTED target 只能在创建目录作用域内修改——wrap_header 安装调用必须住进子目录，--trace 干扰项教训）；T7 socket 断连仅留 7z，controller 照 gizmo 先例重建三件
- [x] **T9**（`a91c245`）：exp_imgui 窗口化（SDL 生命周期由 example 作为宿主演示——cxxkit-never-opens-windows 契约的消费层示范）+ `docs/imgui-smoke.md` 人工运行门禁（有显示环境 + GL 3.0+；与 crash/network 验证分层同款）+ headless rc=1 失败路径即无显示验证证据
- 验证：主 72/72 / 全链编译 0 error（SDL3 静态链接零系统 GL 依赖——imgui 自带 loader 头）/ nm 符号抽查（ImPlot::CreateContext/13 处 backend 符号）/ stamp 全命中 / clang-format 干净
- target 拓扑：`cxxkit::imgui` + `imgui_sdl3` + `imgui_plot` + `imgui_gizmo`，单开关 `CXXKIT_ENABLE_LIB_IMGUI`
- P2 备忘：implot3d / imgui_markdown（单头 Zlib）/ ImGuiFileDialog / imnodes / ColorTextEdit（停滞按需）/ vulkan-dx12 backend / imgui_sdl3 安装树消费验证（CxxKitConfig stub 链，media cf9e1e9 同款）

### 2026-09-04 cxxkit/imgui P2b examples/imgui 全家桶（8 例目录化）

- [x] **三提交**：`bc8ed1d`(计划) / `edb4ac0`(目录化+8 例) / `f0a5220`(smoke 矩阵+README) / `822d79e`(联调修复)
- [x] **目录化**：examples/imgui/ + 共享 sdl_host.hpp（example 内部件——cxxkit-never-opens-windows 契约在消费层示范）；headless 例独立（CI rc=0 唯一 imgui 可跑面）
- [x] **联调抓盲 4 起**（controller 修复）：①5 扩展 target PUBLIC 缺暂存 D6 include root（P2a 埋雷消费引爆，`822d79e`）②gizmo/file_dialog 例 include 顺序违上游契约（imgui.h 必须在前）③file_dialog API 名想当然（FileBrowser 不存在→实证 Display/Close）④markdown 例 NSDMI 需 C++14→目标级 CXX_STANDARD 14（D25 口子）
- [x] **知识沉淀**：PIT-35（stash 不保护 untracked——多代理树红线）/ PIT-36（markdown NSDMI C++14）/ smoke 清单 8 例视觉验证矩阵
- 验证：全链 0 error / rc 矩阵 headless=0 + 7 窗口例=1 / ctest 72/72 / format 干净
- 教训：error_count grep 假绿（二次增量构建归零）——构建退出码为准

### 2026-09-08 事件循环子系统一期落地（D30，kernel+uv+qt 三子库 SDD 流水线）

- [x] **架构**：AbstractEventDispatcher 5 纯虚 + EventLoop 壳（post/exit→wake_up 耦合/maximumTime 合成/repeat 壳包装）+ connect_queued（header-only 返 Connection）+ uv/qt 双 opt-in 引擎（libuv 1.49.2 vendored/Qt6 系统包）
- [x] **hyperplan 对抗收敛**：4 人审核（30 findings）+ 4 人对抗三轮 → D1-D16 终局裁定 + I1-I7 两级不变量（契约级零豁免）
- [x] **测试**：75 套件全绿（kernel 15 + uv 8 + qt 7 + kernel_event 4 新增）；asan 74/74 零诊断；coverage 80.5%（48-file，uv 88.46%+factory 100%）
- [x] **check.sh 8/8 ALL PASSED**：naming gate 修误报（->method() 链）/5+6/8 去 -G Ninja 硬编码耦/NETWORK=ON 增量重链
- [x] **examples 19/19**：exp_event_loop（CI rc=0）+ exp_qt_embed（本机 rc=0，spec §5.2 桥形态）
- [x] **S5 顺手修**：crash.pc Breakpad 双分支误映射 TracyClient（`4ba89f9`）
- 裁定/教训全录：decisions.md D30 + pitfalls.md PIT-38/39

### 2026-09-08 事件循环子系统二期落地（D31，SocketNotifier + TcpSocket/TcpServer，T0-T4 SDD 流水线）

- [x] **架构**：register/unregister_socket_notifier 虚函数家族声明于 AbstractEventDispatcher（kernel，F6 三处有意偏离 spec §9 字面——CHECK(false) 默认/回调带 fired mask/std::function，对账完成）；UvEventDispatcher uv_poll 实现（level-triggered 常驻注册 + 同 fd 幂等更新兴趣）；TcpSocket（memcached 状态机 kIdle→kClosed 直译 + beast flat_buffer 形状读缓冲 + F8-① 回调局部拷贝纪律）；TcpServer（uv_listen + accept 产出 adopt_uv_tcp 包 TcpSocket，I5 on_connection 契约）
- [x] **五提交**：T0 热身 0ms 直投 / T1 notifier 三侧 `01e45fd` / T2 TcpSocket `0d43bab`→`01e45fd` 链 / T3 TcpServer+exp_tcp_echo `63ebb19`/`d4bbd45`/`0d71d27` / 文档见 task-T*-report
- [x] **测试 75→78 套件**：+tst_kernel_event_loop_ext/+tst_tcp_socket/+tst_tcp_server；主树 UV+QT=ON 78/78、shared UV=ON 78/78（kernel→tools 链接闭包成立）、asan UV=ON 78/78 零新增诊断（2 条既有良性 runtime error 维持 B2 先例）
- [x] **coverage**：全口径 80.7%（50 files 3429/4248）；uv_event_dispatcher 87.96% / tcp_server 80.00% / tcp_socket 83.26%（uv 错误分支未全触发，<85% 目标留补测备忘）
- [x] **check.sh 8/8 ALL PASSED**：naming gate 新增字符串字面量排除（"kIdle (" 式 log 消息误报 3 处）
- [x] **rc 矩阵**：exp_event_loop=0 / exp_tcp_echo=0（tcp echo roundtrip ok）/ exp_qt_embed=0（queued 42 + 3 ticks）
- 裁定/教训全录：decisions.md D31（F6 对账）+ pitfalls.md PIT-40（uv 回调销毁成员 std::function = UAF）
- 备忘：tcp_socket/tcp_server 覆盖率补测到 85%+（错误分支）；UdpSocket/PipeStream 需求触发；三期 TLS 走 mbedTLS ssl_set_bio 路（A4 已备）

### 2026-09-08 事件循环二期落地（D31 续，SocketNotifier + TcpSocket/TcpServer，T0-T4 SDD 流水线）

- [x] **架构**：register/unregister_socket_notifier 虚函数家族声明于 AbstractEventDispatcher（kernel，F6 三处有意偏离 spec §9 字面——CHECK(false) 默认/回调带 fired mask/std::function，对账完成）；UvEventDispatcher uv_poll 实现（level-triggered 常驻注册 + 同 fd 幂等更新）；TcpSocket（memcached 状态机 kIdle→kClosed 直译 + beast flat_buffer 形状读缓冲 + F8-① 回调局部拷贝纪律）；TcpServer（uv_listen + accept 产出 adopt_uv_tcp 包 TcpSocket，I5 on_connection 契约）
- [x] **五提交**：T0 热身 0ms 直投 0ms→post 直投绕引擎 / T1 notifier 三侧 `01e45fd` / T2 TcpSocket `0d43bab`→`b6075b0` 链 / T3 TcpServer+exp_tcp_echo `63ebb19`/`d4bbd45`/`0d71d27` / 文档见 task-T*-report
- [x] **测试 75→78 套件**：+tst_kernel_event_loop_ext/+tst_tcp_socket/+tst_tcp_server；主树 UV+QT=ON 78/78、shared UV=ON 78/78、asan 零新增诊断
- [x] **coverage**：全口径 80.7%（50 files）；uv_event_dispatcher 87.96% / tcp_server 85.71%（FU1 故障注入后）/ tcp_socket 83.33%（CHECK/FATAL 消息分支占比高，封顶备案）
- [x] **check.sh 8/8**：naming gate 字符串字面量排除（"kIdle (" 式 log 消息误报）
- [x] **rc 矩阵**：exp_event_loop=0 / exp_tcp_echo=0 / exp_qt_embed=0
- 裁定/教训全录：decisions.md D31（F6 对账）+ pitfalls.md PIT-40（uv 回调内 close 摧毁执行中成员 std::function = UAF，局部拷贝纪律）
- 备忘：三期 TLS 走 mbedTLS ssl_set_bio + WANT_* 重挂 poll 兴趣（A4 已备）

### 2026-09-08 按需模块开关落地（D32，M0/M1/M2 SDD 流水线）

- [x] **开关体系**：10 实开关（text/containers/functional/numerics/patterns/units/memory/kernel/thread/media，拓扑序声明——cxxkit_option 即时求值 DEPENDS，PIT-41）+ 门控 network/crash/imgui/uv/qt/tracy；base/profiling/tools/time 无条件（tools=符号枢纽 nm 6 库实证、time↔tools 符号级环 Ruling v2——**预案 12 开关追认为 10+2**，解耦债务备案：checks/logging 下沉 base 或 clock 移 time）
- [x] **钳制语义**（octk/QExt 同款）：依赖 OFF 时请求 ON 被 clamp + WARNING，不自动传播；INPUT_ 与 direct 双通道行为等价
- [x] **M1 Qt 连坐修复**（`82e7661`）：cxxkitConfig find_dependency(Qt6) 加 `qt IN_LIST cxxkit_FIND_COMPONENTS` 组件级条件（PIT-42）；NOT_FOUND_MESSAGE 点名缺失组件；F6 VENDORED find_dependency 全部随开关条件化
- [x] **测试/示例守卫**（`01da5f8`）：按开关条件注册，条件链接聚合随子库缺失降级
- [x] **三态验证**：默认 79/79（基线 78→79 FU1 祖先）+ asan 78/78 零诊断 + shared 79/79；裁剪态 TEXT=OFF——clamp WARNING + 套件按裁剪下降 + BuildInstall + 消费方 REQUIRED 缺组件点名；最小态 base-only 消费方 build/run 过；install 三态 + INPUT 通道全记录
- [x] **check.sh 8/8 ALL PASSED**（M2 收口 exit=0：主 79 + shared 79 + asan 78 + coverage 全口径不阻断）
- 已知限制：text→numerics / date_time→text 安装树 include 级传递边（裁剪组合消费对应头需自行启用组件）；INPUT_ 残留 cache 持续 FORCE（D25 同款）

### 2026-09-09 Object 树 + WeakPtr 落地（D33，T1-T3 SDD 流水线）

- [x] **T1 Object 树四件套**（`5c85a1c`）：构造挂父 / set_parent 摘旧挂新+环检测 CXXKIT_CHECK fatal / 析构级联 while(!empty) delete front / ChildEvent 同步派发（派发对象是父，构造期坑不触发——Qt 构造期坑只在接收方未构造完时成立）
- [x] **T2 delete_later + destroying**（`65b52ed`+`970c8b4`）：exec-only 语义（controller 裁定 Option A：仅运行中环可收，未 exec 调用 = fatal）+ destroying() 虚函数（析构期虚派发只到 Object 层，派生 override 不被调用——Qt 同款）+ ~EventLoop 排空防泄漏（L1 排空期再 post 静默丢失 / L2 排空期 delete_later fatal / 父子不可同投——已知限制全备案）
- [x] **T3 WeakPtr/WeakPtrFactory**（`3556684`+`50462c4`）：cxxkit::memory Chromium 形非侵入（shared_ptr<atomic<bool>> 标志块 + weak_ptr），拒绝 QPointer 式 intrusive 轨道（侵入构造改所有派生类 = API 破坏）；契约 Factory 生命周期不晚于 owner
- [x] **测试 79→81 套件**：+tst_object +tst_weak_ptr；PIT-43（exec FIFO 轮次陷阱：exit 与依赖前置效果的投递必须同闭包串联）
- 裁定/教训全录：decisions.md D33 + pitfalls.md PIT-43
- 备忘：Phase 2/3（事件投递 postEvent / 线程亲和 moveToThread）需求触发再取——Object 树当前单线程语义

### 2026-09-10 Phase 2 事件投递落地（D34，T1-T3 SDD 流水线，e32e06e..06f93fc）

- [x] **T1 send_event + filter 链 + DeferredDeleteEvent**（`e32e06e`）：send_event（逆序 filter 链 → receiver->event()，返 is_accepted）/ install/remove_event_filter（头插后装先过滤，Qt 同款）/ event() kDeferredDelete 分支 `delete this; return true;` 前瞻实装
- [x] **T2 post_event + mEventQueue**（`25eb7f3`）：EventEntry{mReceiver,mEvent} 裸指针队列（R1）+ enqueue/purge 静态（F3 形态，绕 CXXKIT_D 无 this 陷阱）+ process_events 双排空整合——Event 段**锁内逐条 pop + 锁外派发**（Momus F2 修订，非快照），post 先 Event 后，两队列都空才落 dispatcher；~EventLoop 排空扩展（剩余 Event delete 不派发）
- [x] **T3 delete_later 迁移 + ~Object purge**（`6c47712`+`06f93fc`）：delete_later 改推 DeferredDeleteEvent 入 Event 队列（enqueue 直推，F1 修订）+ ~Object 步骤序 destroying → purge_pending → 级联 → 摘链（父子同投限制解除）+ send_event 摘 kDeferredDelete 守卫（审查裁定唯一正确解：守卫挡 T2 自己的派发路径必然矛盾，kDeferredDelete 到 send_event 的唯一合法路径就是队列派发本身）+ D34 provenance 修正（06f93fc）
- [x] **D34 决策记录**：R1-R4 全录 + 双守卫纪律（post_event 禁投保留、send_event 死守卫摘除）+ spec §4.2 不变量修正（purge 与 pop 锁内互斥、派发在锁外）+ filter 弱化契约 + 已知限制（跨线程 fatal/无压缩/无多优先级）
- [x] **测试**：tst_object 7→19 用例（send 直达/filter 拦截与 LIFO/post 派发顺序/DeferredDelete 正名/父子同投正名），**81 套件不变全绿**；ASAN 定向零诊断（父子同投 RED 期 UAF 已闭合）
- 裁定/教训全录：decisions.md D34 + pitfalls.md PIT-44（手工 g++ 探针悬崖）
- 备忘：Phase 3 线程亲和（moveToThread + 跨线程投递重审）/ filter 反向清理注册表 / DeferredDeleteEvent friend 收敛——需求触发再取

### 2026-09-10 Phase 3 线程亲和落地（D35，T1-T3 SDD 流水线，6852e10..0152ca0）

- [x] **T1 亲和元数据 + move_to_thread**（`6852e10`）：`ObjectPrivate::mThread` + `thread()` 查询 / `Object(ObjectPrivate*)` 委托目标 ctor 内创建即亲和（exec 内构造绑环，环外 null）/ move_to_thread 静态守卫（is_running fatal ×2、同环 no-op、null target 脱离）/ 子树 BFS 先序迁移 + ThreadChangeEvent 直调 event()（非可过滤，不新建事件类）/ 队列条目随迁（take_events_for 逐 receiver 先排空 source 再入 target，无嵌套锁）
- [x] **T2 跨线程投递路由**（`947e587`）：post_event 从 current() 语义改目标环路由（receiver->thread() 非空投亲和环 + wake_up，null fatal）/ **delete_later 语义演进**（D33 exec-only → 亲和优先 current 回退，三态测试改造保留不回归）/ wake_up 契约升级 thread-safe（FakeDispatcher 补 mutex）/ DeferredDeleteEvent ctor friend class Object（R6，禁直发语言级封锁）
- [x] **T3 filter 反向注册表 + 去重**（`3fb9398`）：mWatching 反向表（filter 亡自动从 watched 摘除，D34 弱化契约解除）/ install 先 remove-then-insert 去重（重装移最新位）/ ~Object 双向自摘（**brief 方向笔误实测挂起修正**——remove_event_filter 以 this 作 receiver 每轮两侧各消一项）
- [x] **Momus 修订全落**：F3（EventLoop ctor mDPtr.reset 后补设 mThread）/ F4（随迁测试移 T2）/ F4d（FakeDispatcher exec 前必投 exit 闭包）/ F5（迁移期禁并发 post 备案）/ F6（event() 直调裁定为准，spec §4.4 措辞 D35 更正）/ F-M1（~Object 双 purge 去重 `0152ca0`）
- [x] **测试**：tst_object 20→29 用例（thread 查询/创建即亲和/静态守卫/子树迁移/队列随迁/跨线程 post/反向清理/去重/亲和 delete_later；3 既有用例补 move_to_thread 改造）；**81 套件全绿**；ASAN 定向零诊断
- [x] **coverage**：全口径 82.2%（50 files 3625/4411，BuildCoverage 重建后实测）；object.cpp 91.07% / event_loop.cpp 92.11% / event.cpp 100%
- [x] **PIT-45**：CXXKIT_CHECK 复合条件 `<<` 流式消息归属歧义——复合条件必须整体括号
- 裁定/演进全录：decisions.md D35（R1-R6 + delete_later 正式演进 + 双向自摘正确方向）
- 已知限制：动态迁移 fatal/定时器不迁/无亲和 post fatal/Application 注释态/kDeferredDelete 守卫用户侧不可达/跨线程 filter 生命周期单线程语义
- 备忘：Phase 4 动态迁移（队列条目失效标记）需求触发再取
