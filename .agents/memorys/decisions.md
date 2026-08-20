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
