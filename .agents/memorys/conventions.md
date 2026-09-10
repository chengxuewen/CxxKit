# CxxKit 开发约定

## C1：构建系统统一用 CMake

所有模块通过 CMakeLists.txt 接入，编译配置集中在 `.cmake.conf`。
禁止引入 Cargo/Meson/Bazel 等第二套构建体系。

## C2：模块化目录结构

按功能域组织模块（高内聚低耦合），单一模块目录内源码 + 头文件 + 测试共存。
参照 OpenCTK 原有模块拆分，避免巨型单体。

## C3：格式化交给 clang-format

不手写风格，提交前 `clang-format -i <file>`。仓库根 `.clang-format` 为唯一权威。

## C4：C++11 起步，按子库可升级

库代码最低 C++11（用户明确要求，2026-08-18 变更，原为 C++17 起步）。
RAII 资源管理，`constexpr`/`auto` 可用，避免裸 `new/delete`。
注意：**测试 target 用 C++14**（gtest 1.12.1 要求），库 target 保持 11。
检查：`grep -rnE "auto\s+\w+\s*=|if constexpr" cxxkit/ --include="*.hpp"` 应为空（C++14 语法禁用）

## C5：检查命令

```bash
cmake --build build          # 构建
cmake --build build --target install   # 安装（默认装到 build/install/）
ctest --test-dir build --output-on-failure   # 测试
clang-format --dry-run --Werror <files>      # 格式
```

## C6：禁止 `rm -rf build`（保护 3rdparty 缓存）

**`build/3rdparty/` 是 vendored 三方库的 stamp 缓存**，全清会导致三方库重新解压+构建（5+ 分钟）。
需要干净配置时：

```bash
# 只清 CMake 生成物，保留 3rdparty：
rm -rf build/CMakeCache.txt build/CMakeFiles build/Testing build/*.ninja build/cmake_install.cmake
cmake -S . -B build    # 重新配置（3rdparty 走 stamp，秒级）
```

仅当三方库自身需要重建时才删对应目录：`rm -rf build/3rdparty/<lib>-<buildtype>`
（或按需删单个 wrap 目录）。

## C7：品牌名 CxxKit，代码标识符一律小写 cxxkit

品牌显示层（README 标题、`project(CxxKit)`、`CXXKIT_PRODUCT_NAME`、`CxxKit*Helpers` 模块名）用大写；
代码标识层（头目录 `cxxkit/`、namespace `cxxkit::`、CMake target `cxxkit::<sub>`、
include 路径 `<cxxkit/...>`、包名 `cxxkitConfig.cmake`/pkg-config）一律小写——
Linux 大小写敏感 + abseil 式「目录=子库=target」三位一体，只保留一种拼写，避免同名异写。
检查：`test -d cxxkit && grep -rn "#include <CxxKit" cxxkit/ tests/ examples/ | wc -l` 应为 0

## C8：子库目录 = 头 + 源 + CMakeLists 聚合（abseil 式）

编译实现与公共头同放 `cxxkit/<sub>/`（私有头在 `detail/`），**无 src/ 目录**（D14）。
新增子库：头 + 同名 .cpp + CMakeLists 三件套放同一目录，头必须进 target 源列表：
```cmake
file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_xxx ${_cxxkit_headers} xxx.cpp)   # header-only 用 add_library(cxxkit_xxx INTERFACE ${_cxxkit_headers})
```
安装由 `install(DIRECTORY ... FILES_MATCHING "*.hpp")` 控制——.cpp 自动不装；detail/ 私有头随公共头安装（D9，2026-08-19 用户决策）。
检查：`test -d src` 应不存在；`find build/install/include -type d -name detail` 应为 4（kernel/thread/tools/network）
检查：`test -d src` 应不存在；`find build/install/include -type d -name detail` 应为空

## C9：vcpkg 集成方式（2026-08-19，用户多轮对齐 QExt/OpenCTK）

- **直接移植** InstallVcpkg.cmake 为通用基础设施 `cxxkit_vcpkg_install_package()`，不发明脚本层（QExt/OpenCTK 无 export 脚本——一切在 CMake 函数内完成）
- **默认 OpenCTK 式自动拉取**：.7z 缓存缺失 → `cxxkit_vcpkg_install()` 自举 vcpkg（**必须全量 clone，禁 --depth 1**——builtin-baseline 版本锁定需要 port 历史 checkout）→ `vcpkg install` → `export --raw` → `cmake -E tar cvf --format=7zip` 打包 .7z；`NO_FALLBACK` 严格模式（FATAL 附导出命令）
- **每库一个 wrap 文件**（`FindWrap<Name>.cmake` → `CXXKitWrap*::Wrap*` target，QExt FindWrapBreakpad 同款：helper 直接传真 TARGET 建空 INTERFACE IMPORTED target + 显式 INTERFACE include 目录 + NOT_IMPORT 自 import）
- **缓存优先 = 同源保证**：`INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR` 注入目录（CACHE PATH）天然含 QExt/父项目导出的 .7z（breakpad-x64-linux.7z = 2024-02-16），FindWrap 优先解包用之
- **未传入目录不打包（QExt 语义，2026-08-19 人工修正）**：无 INPUT 注入时 `CXXKIT_3RDPARTY_PACKAGES_DIR` 为空 → `cxxkit_vcpkg_install_package` 的 pack 步骤（`if(NOT "X$dir" STREQUAL "X")`）跳过，**不在源码树产出 .7z**（仅留在 build 树 export 目录 / 解包缓存）；显式传入才打包到指定目录
- **vcpkg 产物 gitignore**：`vcpkg/` `vcpkg-tools/`（仓库根自举克隆目录）；vendored .7z 归档与父项目一致**可入库**（json/jwt .7z 已跟踪）——由 `CXXKIT_3RDPARTY_PACKAGES_DIR` 控制是否产出
- **vcpkg manifest 版本锁定**：`version>=` + 顶层 `builtin-baseline`（vcpkg.json 里 `version<` 是非法字段）；custom triplet 必须声明 `VCPKG_CMAKE_SYSTEM_NAME`（未知后缀默认按 Windows 处理）
- **GUI 可编辑性**：cxxkit_option 普通态 `CACHE BOOL` 无 FORCE + `_option_string_type_if_cache_<var>` 标志跟踪强制态（OpenCTK 原版机制，曾移植丢失）
- 检查：`grep -rn "CrashDeps\|export_crash_deps" cmake/ cxxkit/` 应为空；`find cmake/wrap -name "FindWrap*" | wc -l`（当前 23 个）

## C10: target 注册统一走 cxxkit_add_* helper（2026-08-20）

- 库注册统一 `cxxkit_add_library`（签名见 cmake/CxxKitLibraryHelpers.cmake）：类型 STATIC/SHARED/INTERFACE（现有子库显式 STATIC 保现状 R20，新库默认跟随 CXXKIT_BUILD_SHARED_LIBS）；HEADERS 默认自动 GLOB *.hpp+detail/*.hpp（D15）；自动 alias `cxxkit::<sub>`（剥 cxxkit_ 前缀）、include 三元组、cxx_std_11、CXX_STANDARD、GLOBAL_COMPILE_DEFINITIONS；参数 EXPORT_NAME/FOLDER/PRECOMPILED_HEADER/EXCEPTIONS/NO_ALIAS
- 可执行注册统一 `cxxkit_add_executable`（cmake/CxxKitExecutableHelpers.cmake）：SOURCES/INCLUDE_DIRECTORIES/LIBRARIES/EXCEPTIONS/FOLDER/WIN32/MACOSX_BUNDLE
- 测试注册 `cxxkit_add_test`：FOLDER 自动推导（tests → CxxKit/tests/<sub>）+ `<name>_check` target（ctest -V -R 单测）
- 检查：`grep -rnE "^(add_library|add_executable)" cxxkit/ examples/` 应为空（注册全走函数）
- 参考 OpenCTK octk_* 骨架，剔除 Octk 生态（framework/plugin/Android/WASM/RC/依赖扫描，YAGNI）
- **CxxKit vs octk 封装边界（2026-08-20 裁定）**：cxxkit_add_library 并入 COMPILE_DEFINITIONS + INSTALL_RPATH（纯参数透传）；pkg-config（cxxkit_generate_pkg_config，D11/vcpkg 定制需 DESCRIPTION + 三方链映射）与 configure_begin/end（配对语义）**保持调用外**——不照搬 octk 1550 行单函数全封装，KISS ~140 行
- 注意：helper 参数列表内禁注释行（CMake 解析会当额外参数段报错）；多值参数表不要重复新增（会双行残留）
- **C4 补充（2026-08-20）**：测试 target 也已 C++11（gtest 1.12.1 最低即 C++11，非 C++14——旧注释误导）。测试代码含 C++14 语法时用 `#if CXXKIT_CC_CPP14_OR_GREATER` 适配（如泛型 lambda），全项目统一 cxx_std_11
- **C4 测试质量补充（2026-08-20）**：sanitizer/coverage 用**独立 build 目录**（build-asan/build-cov，守 C6）；`lsan.supp` 抑制设计性单例泄露；覆盖基线 ~41%（行加权；per-file 均值 ~63%）。检查：`grep -n "CXXKIT_BUILD_SANITIZERS\|CXXKIT_BUILD_COVERAGE" cxxkit/ cmake/ scripts/check.sh` 非空
- **C10 补充（2026-08-20）**：每个编译子库独立 `CXXKIT_<SUB>_API` 导出宏（`<sub>_global.hpp`，octk 式）——不再共用 CXXKIT_CORE_API（共享构建下会错位导出）。helper 对编译型 target 注入 `CXXKIT_BUILDING_<SUB>_LIB` + 共享时 `CXXKIT_BUILD_SHARED_<SUB>`。子库 CMakeLists **不传 STATIC/SHARED**（跟随 `CXXKIT_BUILD_SHARED_LIBS`，octk 式）。共享构建（build-shared）会暴露静态掩盖的缺失依赖——如 tools→cxxkit::time。
- **C10 补充（2026-08-20）**：子库**不传 STATIC/SHARED**（跟随 `CXXKIT_BUILD_SHARED_LIBS`，octk 式）；共享构建（build-shared）会暴露静态掩盖的**缺失链接依赖**与**循环依赖**（text↔tools 双向 → 用 header-only 内联打断：tools 内联 basename 免链 text → text→tools→time 单向）。各库若用 vendored fmt 需自行 `cxxkit_find_package(Fmt)` + 链 WrapFmt。验证：静态 + 共享双 34/34。
- **C10 补充（2026-08-20）**：共享库 ELF soname 版本化（octk/Qt 模式）——helper 对编译 target 设 `VERSION=${PROJECT_VERSION}` + `SOVERSION=${PROJECT_VERSION_MAJOR}`（产物 `libcxxkit_<sub>.so.<VER>` + soname `.<MAJOR>`）。major 升=ABI 断，minor/patch 原位升级。
- **C11（2026-08-20）**：安装树消费验证——`cxxkitConfig.cmake` 的 vendored find_dependency 必须跟随启用的子库（network OFF 时不得 find cpr/CURL/MbedTLS；base=fmt;spdlog）。共享库 3 链（.so → .so.MAJOR → .so.VER）+ SONAME 使消费方 find_package+链接+运行正常。验证用 `/tmp` 最小消费方 find_package(cxxkit COMPONENTS text tools time units)。

- **C12（2026-08-24）**：覆盖率门禁一律按**全库 .cpp 全口径**（`scripts/coverage.sh` 汇总所有 `*.cpp.gcda`，含 0% 文件）。历史 80.5% 是 19-file 子集口径（漏 0% 文件）——**声称覆盖率必须报告 total files 数**，80% 门禁以 29 文件加权为准。检查：`bash scripts/coverage.sh build-cov | grep "Line-weighted"` 报告总数应含 assert.cpp 等 0% 文件
- **C13（2026-08-24，PIT-25）**：注释/文档-only 批量更改纪律：逐文件 diff 验证纯注释（`grep 非注释行 = 0`）+ 提交前三重验证（全量编译 0 error + ctest 全绿 + doxygen 重跑致命 warning 清零）；**禁并行代理写公共头**（RPM 限流 + edit 吞行/删宏/改 target）；**禁 `git add -A`** 对含代理改动树（逐文件 add）
- **C14（2026-08-24，PIT-26）**：condition_variable wait 用固定长 deadline 时**不可指望 notify 缩短它**——谓词版 `wait_until(lock, deadline, pred)` 不改 deadline。空队列/未知间隔等待用**短轮询（1ms）+ 谓词重查**，不睡长上限。检查：task_queue_thread.cpp popNextTask 空队列分支 `sleepTime.us() > 1000 → Millis(1)`
- **C15（2026-08-24）**：BuildAll/BuildInstall 等 convenience target 必须包 `if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)`（octk 惯例，PROJECT_IS_TOP_LEVEL 需 3.21+ 不用）——被父项目 add_subdirectory 时跳过防同名冲突

- **C16（2026-08-25）**：外部库移植纪律——从 abseil/webrtc 移植代码时：① 库代码保持 C++11（降级 auto 返回/if constexpr/泛型 lambda/_t/_v）；② 统一 `cxxkit::` 命名空间 + `#pragma once`；③ 简化实现（禁 SIMD/自定义 allocator/异常安全等重依赖）；④ 适配现有 API（如 StatusOr 用 `isOk()` 而非 abseil `ok()`）；⑤ 每个移植功能配套 gtest 测试 + 许可横幅 + doxygen 注释。检查：新头 `grep -c "pragma once"` 应为 1

- **C16⑥（2026-08-28）**：移植代码中的所有公开标识符（函数名/成员变量/枚举值）必须在同 PR 内迁移到本项目命名规范（spec §1 矩阵 + §2 前缀规则），不留豁免层。上游 C API 限定调用（如 libyuv::I420Copy）保持原名。

- **C18（2026-09-10）**：提交消息与代码注释**必须使用英文**；计划文档与 AI 对话交互使用中文。检查：`git log --format="%s" -20 | grep -P "[\x{4e00}-\x{9fff}]" | wc -l` 应为 0；新代码注释抽查无中文。
