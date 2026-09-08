# CxxKit 决策记录

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

## D8: 内部 include 统一尖括号 <cxxkit/...>（2026-08-18）

用户指出迁移脚本用引号 `#include "cxxkit/..."` 与三方头 `<cxxkit/3rdparty/...>` 风格不一致。已全局统一为尖括号（192 文件）：严格依赖 include path 解析，杜绝当前目录同名文件歧义，与 octk 原风格及三方头命名空间一致。教训：库内头与三方头的 include 风格必须一致。

## D9: 私有头模式（_p.hpp → <sub>/detail/）

沿用 octk：私有实现头 `xxx_p.hpp` 放 `cxxkit/<sub>/detail/`，cpp 用 `<cxxkit/<sub>/detail/xxx_p.hpp>` 引用。**2026-08-19 用户决策：detail/ 随公共头一起安装**（原"不安装"约定已反转）。OpenCTK 的 include/detail 转发壳层已消除（真实实现直接放 detail/）。

## D10: 原项目残留问题（迁移时发现）

- network_config.hpp 死引用（OpenCTK network 无法编译，已删）
- tst_platform_thread POSIX 链接失败（OpenCTK 原问题）
- 36 个注释测试引用的头不存在（crypto_random/file_utils/task_queue_for_test/sleep/task_event/task_thread）
- curl 默认依赖 ssh2/nghttp2/brotli/zstd 未 vendored（已禁用精简）

## D11: vcpkg 式三方依赖处理（2026-08-18）

安装每个 vendored 三方库**自有的 Config.cmake + pkgconfig + bin/**（fmt/spdlog/cpr/CURL/MbedTLS），cxxkitConfig 用 `find_dependency` 真找（递归链天然完整：spdlog→fmt、cpr→CURL），stub 仅兜底 header-only lite 系。.pc 的 Requires 含三方（fmt/spdlog/libcurl/mbedtls），cpr（无 .pc）进 Libs。已知限制：vendored .pc 绝对 prefix 不可移动（vcpkg 同）。

## D12: 安装体系（octk 式，2026-08-18）

默认 `CMAKE_INSTALL_PREFIX` = `<build>/install`（构建安装测试），`cmake --build build --target install` 始终可用；显式 `-DCMAKE_INSTALL_PREFIX` / `INPUT_CXXKIT_FEATURE_INSTALL_PREFIX` 覆盖。自定义 target：`BuildAll`（全量重建）、`BuildInstall`（构建+安装）、`Docs`（doxygen）。FOLDER 归类：libs→cxxkit/libs、tests→cxxkit/tests、examples→cxxkit/examples。**禁止 `rm -rf build`**（C6：3rdparty stamp 缓存）。

## D13: 文档方案（2026-08-18）

三层：① API 参考=doxygen（CXXKIT_BUILD_DOCS + Docs target，输出 build/doc/html，排除 detail/_p）；② 使用指南=README（子库表+快速开始+CMake/pkg-config 消费）+ examples（3 个）；③ 内部设计=docs/README.md 索引 + superpowers specs/plans + .agents memorys。doxygen 的 INPUT 需空格分隔 + 绝对路径（configure_file 分号坑）。

## D14: 源码聚合到子库目录（2026-08-19）

用户指出 src/<sub>/ 与 cxxkit/<sub>/ 分离不便，与 abseil 式组织（目录=子库=target，头源同目录）不符。已 `git mv` 49 个 .cpp 从 src/<sub>/ 到 cxxkit/<sub>/，删除 src/。子库 CMakeLists 源路径改相对路径。安装靠 install(DIRECTORY ... FILES_MATCHING "*.hpp") 过滤——.cpp 自动不装，detail/ 头随公共头安装（D9 反转）。

## D15: 头文件进 target 源列表（2026-08-19）

用户反馈 CMake 项目大纲看不到 .hpp。13 个子库统一：`file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)`；编译型 add_library 引用，header-only 用 `add_library(xxx INTERFACE ${_cxxkit_headers})`（源参数仅供 IDE 显示，不编译）。教训：INTERFACE 库禁用 target_sources INTERFACE 加源目录内头（PIT-2）；GLOB 必须绝对路径（PIT-4）。

## D16: profiling 子库（Tracy 后端，opt-in）（2026-08-19）

新增第 14 个子库 `cxxkit::profiling`（header-only INTERFACE，仿 absl/profiling 功能命名，不绑定后端）。Tracy v0.13.1 vendored（FindWrapTracy，官方 CMake 构建 TracyClient 静态库，C++17 编译库——PIT-8；TracyConfig 随包分发走 D11 vcpkg 式）。`CXXKIT_ENABLE_LIB_TRACY=ON` 时注入 `CXXKIT_PROFILING_ENABLED`+`TRACY_ENABLE` INTERFACE 宏并链接 `Tracy::TracyClient`（LINK_ONLY——官方 include/tracy 路径由 cxxkitConfig.cmake 清空，头统一走 `<cxxkit/3rdparty/tracy/tracy/Tracy.hpp>` 命名空间，D6）。**不挂 tools**（tools 禁三方依赖），未来 breakpad 独立建 `cxxkit::crash`（folly/SerenityOS 分离模式）。

## D17: crash 子库设计定案（2026-08-19）

第 15 子库 `cxxkit::crash`（breakpad + backward-cpp，opt-in `CXXKIT_ENABLE_LIB_CRASH`，纯 C++11 无 Qt 无网络上传 v1）。依赖走 **QExt/OpenCTK 式 vcpkg 导出 .7z 缓存**：`cmake/InstallVcpkg.cmake` 移植（`cxxkit_vcpkg_install_package`，**默认 OpenCTK 式自动拉取**——无 .7z 时 clone vcpkg + install + export + repack；`NO_FALLBACK` 严格模式）；`scripts/export_crash_deps.sh` 产出合并归档 `crash-deps-<triplet>.7z`（manifest 锁定 breakpad 2024-02-16 与 QExt 同源 + -fPIC overlay triplet + .version sidecar + license 断言）。FindWrapCrashDeps 单解包双 target + 三重门禁（版本/relocatability/PIC 冒烟），归档缺失自动跑导出脚本。**二进制级互斥契约**：crash ON ⇒ QExt::Breakpad OFF（install() 原子守卫拒二次安装）。backward-cpp 不注册自身 SignalHandling，仅 opt-in 回调内 `load_from(崩溃线程 ucontext)` 打栈（Linux；macOS 无 ucontext 跳过）。公共头零三方类型泄漏（pimpl 隔离，回调签名自有）。测试用独立 fixture + fork/exec（禁 gtest death test）。

## D18: crash 实施期裁定汇总（2026-08-19 用户多轮对齐）

- **R11（OpenCTK 式自动拉取）**：install_vcpkg 缓存缺失默认自动自举 vcpkg 构建导出；`NO_FALLBACK` 保留严格模式（评审 R-H3 版本漂移担忧由"缓存优先 + FindWrap 门禁"兜底）。已实测：vcpkg 自举 → 4 包构建 → 导出 .7z → 解包 → 门禁全通
- **R12（弃 Singleton<T,true> 模板）**：该模板从未被实例化——C++11 atomic copy-init、缺 CXXKIT_ASSERT include、private 析构 vs unique_ptr 三处编译错误。CrashHandler 改用 C++11 函数局部静态（线程安全、进程生命周期）+ public 析构。**patterns 子库的模板缺陷待修**
- **R13（每库一 wrap）**：删 FindWrapCrashDeps 聚合模块，回归 QExt 一对一惯例（FindWrapBreakpad/FindWrapBackward）；删 export 脚本层（纯 CMake 函数完成一切）；砍 .so 安装/POST_BUILD 复制（backward 无 libdw 降级为地址级栈，测试用 LD_LIBRARY_PATH 注入）
- crash 回调签名暴露面：`CrashCallback` 自有类型（bool(*)(const char*, void*, bool)），平台类型只存在于 detail/（pimpl 隔离，验证通过）
- v1 明确不做上传（用户决策）：不依赖 cxxkit::network，无 cpr/curl/mbedtls 构建代价
- 关联：D17（架构）、D16（profiling 先例）；全部裁定记录于 docs/superpowers/plans/2026-08-19-cxxkit-crash-sublib.md + SDD 台账

## D19: 测试质量底层（2026-08-20）

sanitizer（ASAN/LSAN/UBSan）与 coverage 用**独立 build 目录**（build-asan / build-cov，守 C6）。**sanitizer 暴露真实 bug 的价值论证成立**：F1 平台线程泄漏、F2 ElapsedTimer 有符号溢出、F5 error.cpp FNV 溢出、context_checker use-after-free 均由 sanitizer 抓出并最小修复；F4 __forced_unwind 判定为良性（glibc，不改）。R31：coverage 门禁默认仅报表不阻断（环境 flaky 防御），`CXXKIT_COVERAGE_GATE=ON` 才强校验 ≥80%；R32：安全测试在 ASAN 下最有价值，无 ASAN 降级为正常断言；R33：flaky context_checker 用 ASAN + systematic-debugging 抓根因。R30：3rdparty 缓存复用优先。覆盖基线 ~41%（行加权 1761/4313；per-file 均值 ~63%）。详见 docs/superpowers/plans/2026-08-20-cxxkit-test-quality.md + SDD 台账

## D20: TaskQueueThread 空队列短轮询换长睡（2026-08-24，PIT-26）
- **决策**：popNextTask 空队列/大 sleepTime（>1000us）返回 1ms 短轮询，替代默认 PlusInfinity 睡满 wait 1s 上限
- **理由**：谓词 wait_until 不缩短 deadline——notify 到达时 delayed 未到期则继续睡旧 deadline，3ms 任务迟到 1s（隔离 15/15 必失败）；1ms 轮询把通知/截止错位窗口缩到 1ms
- **代价**：空队列 1000 次/s 唤醒（CPU 微开销，非忙等）；delayed 精确 sleepTime（<1000us）保持精确等待
- **参考**：std::condition_variable 谓词 wait 惯用法 + 短轮询

## D21: 覆盖率口径全量化（2026-08-24）
- **决策**：覆盖率只认 scripts/coverage.sh 的全库 .cpp 加权口径（29 files 含 0%）；80.5% 历史数字（19-file 子集）作废
- **理由**：gcda 收集范围随构建树演进而变，子集口径漏 0% 文件造成虚高；全口径暴露真实短板（11 个 0% 文件 ~850 行）
- **影响**：80% 门禁需先补 0% 文件测试

## D22: sanitizer 顶层传播保持（2026-08-24）
- **决策**：CXXKIT_BUILD_SANITIZERS/COVERAGE 继续顶层 add_compile_options/add_link_options，不改 target_* 方式
- **理由**：sanitizer 需全链接二进制裁一致（vendored 3rdparty 也带）；coverage 顶层虽让 3rdparty 带 --coverage（略慢）但 coverage.sh 已只统计库 .cpp；收益/风险比不值得改
- **参考**：用户确认保持不变（2026-08-24）

## D23: abseil/webrtc 移植候选决策（2026-08-25 用户交互式逐项讨论）
- **决策**：18 个候选逐项讨论——6 项移植（flat_hash_map/set、ArrayView 扩展、StatusOr、Mutex RAII、PendingTaskSafetyFlag、Barrier），11 项排除（scoped_refptr/SwapQueue/WeakPtr/absl::Mutex/Notification/BlockingCounter/SequenceChecker/FixedArray/Cleanup/AlwaysValidPointer/StrCat），1 项捆绑（Hash 框架随 flat_hash_map 后续增强）
- **理由**：排除项均有 std 替代（shared_ptr/promise-future/atomic+cv/lock_guard）或与现有功能重叠（ContextChecker/scope_guard/InlinedVector/moodycamel 队列/fmtlib）；保留项填补真实缺口且移植成本低（header-only 或小配对）
- **参考**：docs/portability-analysis.md（8 轮 question 交互式逐项确认）

## D24: 决策讨论选项带推荐标注（2026-08-25，用户偏好）
- **决策**: question 工具列选项时，有推荐项则第一项标注"(推荐)"；用户对照推荐决策，不确定项会问"你的推荐？"
- **理由**: 用户 3 次（FixedArray/Hash/AlwaysValidPointer）要求先给推荐——平行选项无导向性，交互效率低
- **参考**: 2026-08-25 移植候选交互式讨论（18 项逐项）

## D25: CXX 标准机制（2026-08-28，commit 97bf4d9）
- **决策**: OpenCTK 式三级优先链 `INPUT_CXXKIT_FEATURE_CXX_STANDARD`（父项目注入，FORCE）> `CMAKE_CXX_STANDARD`（FORCE）> 默认 11（CACHE STRING，GUI 可编辑 + STRINGS 下拉 {11,14,17,20,23,26}）。非法值 FATAL_ERROR（精确匹配校验）。阶梯变量 `CXXKIT_CXX_STANDARD_11..26`（0/1，`>=` 向上）供 CMake 侧按标准条件启用逻辑。
- **理由**: 用户要求 GUI 可配 + 有效性校验 + 子项目外部传入；OpenCTK 机制完整（QExt 无 INPUT_ 通道且无校验，QExt 阶梯仅向上 ON 不便 if() 双向使用）。**变量名保留 `CXXKIT_FEATURE_CXX_STANDARD` 不重命名**（CXXKIT_CMAKE_CXX_STANDARD 式重命名会静默打断 ConfigureHelpers core_config defines 生成——消费方零改动是硬约束）。测试 target 跟随主标准（原硬编码 11）。
- **已知语义**: INPUT_ 经 -D 传入后残留在 CMakeCache，重配置持续 FORCE 生效（GUI 编辑被覆盖）——OpenCTK 同款；清除需 rm build/CMakeCache.txt。
- **验证**: default=11 / -DINPUT_=14 / 非法 13→FATAL 三态 + 63/63 tests。

## D26: 代码风格全库统一（2026-08-28，批次 α/γ/β 三连提交）
- **决策**: 用户裁定路线 C 变体——全库统一无豁免层：函数/局部 `snake_case`（生态调研：2020+ 新库一致选 snake；SerenityOS 是 Pascal类+snake方法大规模先例）；封装类成员 `mPascalCase`（89% 存量多数）；POD 聚合字段纯 snake；枚举值/常量/static `kPascalCase`。语义前缀闭环：getter 无前缀 / is_has_should_can_ / set_ / to_ 拥有转换 / as_ 视图 / make_ 工厂 / _out 白名单出参。getter 属性式（size 非 get_size）。
- **执行**: α 机械修复（横幅 82 文件 + D8 + ref_counted_object 混用点）；γ 成员 74 符号 mPascal 化（迭代扫漏到 0）；β 函数 661 符号 snake 化（camel 291 + Pascal 多词 346 + 残余 4 + 宏双参数化 SafeGt 族 + CHECK_OP；22 类型别名甄别剔除；UpdateRect::Union 关键字例外保留；libyuv 上游 C API 豁免）。
- **方法论教训**: ① 全量符号替换必须先做类型别名普查（using/typedef/class/struct/#define 四扫），否则 Value/SharedPtr 型灾难；② 宏拼接名（Safe##Gt）扫描期不可见——调用点替换前必须 grep 宏体 ## 拼接；③ 上游 C API（libyuv::I420Copy）与 gtest/gmock 符号（testing::Test/IsEmpty/Invoke）设永久豁免；④ 文件级补丁不如全库扫描——color_space.cpp 构造列表漏改教训；⑤ 大批量替换遇误伤时 git checkout 整批回退重做受控子批，优于就地打补丁。
- **验证**: 每批构建 0 error + ctest 63/63 + clang-format 0 违规；snake gate 与成员 gate 已入 check.sh（spec §4δ 修正版正则）。

## D27: clang-format 全库统一（Allman 落地）（2026-09-02）
- **决策**: `.clang-format` 的 BreakBeforeBraces: Allman 首次全库强制执行（此前配置从未被执行，挂行 { 193 处 vs 独立行 2840 处双风格共存）；工具经 pixi 供给（conda-forge 独立包，pixi.lock 锁 23.1.0）；风险文件 macros.hpp（宏中枢 ## 拼接敏感）用 clang-format off 护栏，compiler.hpp 实测 diff=0 免护栏
- **交互裁定四项**: ① AllowShortFunctionsOnASingleLine: InlineOnly（类内内联单行/类外多行，用户终裁，先 All 后回退）；② 连续赋值手工对齐接受碾平（abseil 同款，Ctrl 枚举表格不护栏）；③ FixNamespaceComments: true 保留（N0 后主闭合是宏不受影响）；④ 配置死条目（IndentBraces 在 Allman 下无效）不清理
- **前置 N0**: 裸 `namespace cxxkit {` 24 文件先迁 CXXKIT_BEGIN/END_NAMESPACE（宏内含 MSVC C4251 抑制对，语义修复先于纯格式化，防 diff 互相污染）
- **blame 免疫**: 批次提交哈希录入 .git-blame-ignore-revs
- **验证**: 全库 clang-format --dry-run 0 违规（幂等）+ 构建 0 error + ctest 63/63

## D28: abseil/webrtc 第二批移植 P1（2026-09-02，9 件，SDD 子代理流水线）
- **决策**: 从 .refinfo（abseil 20220623.2 + libwebrtc rtc_base）移植 9 件：running_statistics / sequence_number_util+unwrapper / percentile_filter / exp_filter（numerics，STATIC 化）/ byte_order（numerics）/ fixed_array（containers）/ str_split 精简版 + crc32（text）。
- **四项裁定**: ① byte_order 弃 webrtc 平台宏版，C++11 干净实现，命名 load_be16/store_le32（load/store 内存语义，非 SetBE16/GetBE16）；② fixed_array 简化重实现（单 ::operator new 分配，无 allocator/EBO/增长；n=0 合法 data()==nullptr；对齐限于 max_align_t 已文档化）；③ str_split 非 owning（@warning 悬垂警告；空 delimiter 不匹配 → 整体单 token，abseil per-byte 语义不移植）；④ crc32 zlib 兼容语义（内部 init/final XOR，增量链式 crc32(d2,l2,crc32(d1,l1))==crc32(both)）。
- **Cleanup 不移植**: scope_guard 与 absl::Cleanup API 同构（invoke/cancel/工厂/nodiscard 逐项一致），零功能增量。
- **不移植清单**: node_hash_map/set（无指针稳定消费方）、btree/Cord/int128/absl::hash/flags（成本/重复建设）、moving_* 家族（等需求，P2 候选）。
- **执行方式**: SDD 子代理流水线（每 Task 独立实现者 + Momus 独立 review，controller 裁定），8/8 Task 首轮 APPROVED；实现者 token 腐化事故 3 起（自愈：小块写+grep 自验+编译器裁判纪律）。
- **消费方依据**: media/rtp_headers（sequence_number）、network（byte_order）、媒体质量统计（statistics/percentile/exp_filter）。
- **验证**: 每件 TDD（RED→GREEN）+ 全量 ctest（63→70，+7 新套件）+ clang-format 0 违规 + build-shared 67/67（numerics STATIC 化后）+ pkg-config .pc 含 -lcxxkit_numerics。
- **P2 候选备忘**: moving_max_counter/moving_average/event_rate_counter/event_based_exponential_moving_average/node_hash_*——需求触发再取。

## D29: cxxkit/imgui 子库落地 P0（2026-09-03，B1 半边解除）
- **决策**: 集成 Dear ImGui v1.92.9b（stable，不用 docking），opt-in `CXXKIT_ENABLE_LIB_IMGUI`。八项裁定：① wrap 走 **extract-only** 新形态（imgui few-files 无构建系统，上游 5 cpp 直编进 cxxkit_imgui target，IMGUI_API 重定义为 CXXKIT_IMGUI_API——imgui.h 官方 `#ifndef IMGUI_API` 空默认守卫支持；行业先例 imgui_bundle/vcpkg port 全是直编派）；② P0 用 fake backend 保无 GPU CI 全绿，真实平台层 P1 用 **SDL3**（dummy driver 可无头运行、渲染中立、imgui_impl_sdl3/sdlrenderer3/sdlgpu3 官方内置；有构建系统走完整 wrap——与 extract-only 组成双轨规则）；③ 单父开关全建扩展子 target；④ 扩展梯队 P1: implot+ImGuizmo，P2: implot3d/imgui_markdown/FileDialog/imnodes；⑤ Platform+Renderer 双接口 + ImGuiHost（宿主注入已有原生句柄，cxxkit 永不开窗）；⑥ C++11 零提升（imgui core 官方 C++11 兼容）；⑦ 上游 5 unit 从 coverage 全口径排除（决策 8——extract-only 使 .gcda 落 cxxkit/imgui/ 下，路径过滤失效；libyuv 先例是 wrap 目录独立构建天然被排除）；⑧ demo windows 编入（R5，可宏关）。
- **Momus 审核三缺口**（APPROVE-WITH-FIXES 全修订）：F1 覆盖率分母（上游 ~4.5 万行直编会砸穿 80% 门禁→24.83% 实证，排除后 80.78%）；F2 `cxxkit_install_public_wrap_headers` 强制消费 `_INSTALL_DIR`（extract-only 也须头暂存 `include/cxxkit/3rdparty/imgui/` 布局，否则头静默不装）；F3 IMGUI_API 到达上游 5 TU 的唯一通道是 target COMPILE_DEFINITIONS（它们不包含 imgui_global.hpp）。
- **imgui 1.92 headless 三坑**（实证）：NewFrame 断言 font atlas 已构建（legacy 路径需 GetTexDataAsRGBA32）；首帧 ImDrawData::Valid=false、窗口类内容第 2 帧起才有顶点；demo 窗口需连续 2 帧调用才出顶点（Active 延迟）。
- **验证**: 主 72/72 / asan 72/72 零诊断 / cov 80.8%（43 files）/ exp_imgui 3 次逐字节一致 / OFF 树零影响 / 二次 configure stamp 命中 / nm 符号断言过。提交 `ab8f906`..`5b76925` 5 个。
- **P1 备忘**: SDL3 vendored（完整 wrap）→ imgui_sdl3 backend → implot → ImGuizmo → exp_imgui 窗口化 + smoke checklist。P2: implot3d/imgui_markdown（单头 Zlib）/FileDialog/imnodes/ColorTextEdit（停滞按需）。

## D30: 事件循环子系统一期落地（2026-09-04~08，kernel 抽象 + uv/qt 双引擎）

**架构**（spec docs/superpowers/specs/2026-09-04-event-loop-design.md + bundle D1-D16 + I1-I7 两级不变量）：
- AbstractEventDispatcher 5 纯虚（process_events/wake_up/interrupt/start_timer/stop_timer）；register_socket_notifier 留 §9 二期（D2 裁定：预留虚函数=死 API 债，二期新声明带默认空实现+@since）
- EventLoop 壳：ctor 注入 dispatcher（null CHECK）+ post 队列住壳（mutex+swap 排空，S10/I4）+ exit→wake_up 强耦合（M4/D7 三检查点）+ maximumTime 壳合成（**deadline timer 必须 repeat=true**——one-shot 在门铃先耗尽的轮次后 epoll 无超时源永久阻塞，`de4ce96`）+ repeat=false 壳包装（先 stop 再 fn，M3）
- connect_queued（header-only）：返回 signals::Connection（M1）；std::bind 实现（M2 C++11）；type_identity 非推导上下文单源推导（A3 R2）
- cxxkit::uv（第 18 子库）：UvEventDispatcher——uv_async 空体门铃+壳排空（R-B2-2）/uv_timer 恒 repeat=ms（R-B2-4）/线程断言恒开（R-B2-5）/嵌套 fatal（R-B2-7 真触发面=timer 回调）；vendored libuv 1.49.2（FindWrapLibuv libyuv 形态+PIC+Config 克隆 pthread;dl;rt）
- cxxkit::qt（第 19 子库）：QtEventDispatcher——QPointer 门铃+桥 QTimer 嵌入约束（R-C1-5，宿主侧周期调壳 process_events）/mBellPending 自合并（R-C1-6）/阻塞形态诚实降级（R-C1-8）；M9 target C++17；M10 cxxkitConfig 独立 Qt 分支（无 NO_DEFAULT_PATH）

**关键教训**：
- kernel 曾是死预处理代码 8 个月（CXXKIT_FEATURE_ENABLE_KERNEL 无人定义）——A2 R1 激活
- DISABLE_COPY_MOVE 抑制隐式默认 ctor——抽象基类必须显式 `= default`（A2 R2）
- maximumTime 一次性 interrupt timer 的 deadline 失守链（A2 预言→B2 实证→de4ce96 修复：repeat=true）
- check.sh naming gate 正则把 `->method()` 链误报为 camel 函数——负向回顾排除成员访问
- check.sh 5/8+6/8 硬编码 -G Ninja 与树实际生成器冲突——去耦（沿用缓存生成器）
- PIT-38/39（Qt signals 宏纪律/conda libstdc++ 显式链接）

## D31: 事件循环子系统二期落地（2026-09-08，SocketNotifier + TcpSocket/TcpServer + timer 精化）

**架构**（plan docs/superpowers/plans/2026-09-08-event-loop-phase2-plan.md，T0→T3 SDD 流水线 + Momus review，T4 全链验收）：
- **register_socket_notifier 家族声明于 AbstractEventDispatcher**（F6 三处有意偏离 spec §9 字面，源码注释 + 本条对账）：① 基类默认实现 = `CXXKIT_CHECK(false, E1 文案)` 而非「默认空实现」——fail-loud 优于静默 no-op（fd 永不就绪类 bug 静默化更难查），源码级兼容不变（不 override 仍编译，调用才 fatal）；② 回调签名 `std::function<void(SocketEventMask)>` 带 fired mask（对齐 curl event_bitmap 回灌），非字面 `void()`；③ std::function 而非 signals（轻量，signals 版可薄封装其上）。独立 RAII SocketNotifier 类 deferred（TcpSocket 直消费 dispatcher API 无独立消费者，YAGNI，F7）
- **uv 侧**：uv_poll level-triggered 常驻注册（vs curl edge 重注册制——声明式披露于 porting-scan A5 对照表）；同 fd 重 register = 幂等更新兴趣（uv_poll_start 语义，memcached 每转换重挂前提）；poll 活跃期关 fd = 调用者 UB（libuv 契约，先 unregister 后 close）
- **TcpSocket**：memcached drive_machine 直译（kIdle/kConnecting/kConnected/kClosing/kClosed 显式状态机 + 每转换重挂兴趣）；读缓冲 beast flat_buffer 形状（单块连续 vector+游标），不做 prepare/commit 泛化；构造显式传 EventLoop（asio 式无虚基类，三期模板化留门）；accept 侧 `TcpServer::adopt_uv_tcp`（F10，detail/tcp_socket_p.hpp 内部构造）
- **loop() API 边界裁定**：EventLoop 暴露 `dispatcher()` accessor（F3）——TcpSocket 经 loop 拿注册入口，EventLoopPrivate 私有不可达
- **I5 on_connection 契约**：on_connection 回调内销毁 server 触发 fatal（0d71d27）——所有权语义：回调期间 server 必须存活
- **TcpSocket use-after-free 坑（F8-①）**：on_data 回调里 close 摧毁 mOnData 本体——修复 = 回调入口局部拷贝 std::function 再调用（PIT-40）

**timer 精化小件包**：0ms 直投已落（T0）；aggregate connect / coarse timer deferred（与 queued 异步语义冲突/需求触发）；ratas wheel 不移植（uv handle 模型已够，A1 留档）

**F6 对账记录（T4 收口）**：grep 全库 "F6" —— abstract_event_dispatcher.hpp 1 处集中标注（①②③全录）+ D31 本条 + task-T1-report.md；三处偏离均与 vendored libuv 实源核对过（T1 review：uv_poll_start 换集幂等/invalidate_fd 在飞防护）

**验证（T4 全链）**：主树 UV+QT=ON 78/78（套件 75→78：+kernel_event_loop_ext/tcp_socket/tcp_server）；shared UV=ON 78/78（T1 kernel 头 include tools/checks.hpp 链接闭包天然成立，无断链）；asan UV=ON 78/78 零新增诊断（仅 2 条既有良性 runtime error：nonstd string_view null 参数+glibc __forced_unwind，B2 先例）；coverage 全口径 80.7%（50 files，3429/4248）——tcp_server 80.00%、tcp_socket 83.26%（<85% 目标：uv 侧 uv_poll 分支/错误路径未全触发，用例覆盖主状态机路径；后续按需补）+ uv_event_dispatcher 87.96%；check.sh 8/8 ALL PASSED（naming gate 补字符串字面量排除——kIdle( 式 log 消息误报）；rc 矩阵 exp_event_loop=0 / exp_tcp_echo=0 / exp_qt_embed=0（无链接摩擦）
