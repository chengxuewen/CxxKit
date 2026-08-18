# cxxkit 重构设计：OpenCTK core+network 迁移（abseil 方式）

日期：2026-08-18
状态：已确认（交互式讨论完成）

## 1. 目标

将 OpenCTK 的 `core`（56K 行）与 `network`（2.4K 行）模块重构迁移到 cxxkit 项目，采用 abseil 式组织：目录 = 子库 = CMake target 三位一体，按需取用，命名空间统一为扁平 `cxxkit::`。

- `media`（58K 行，WebRTC 系）与 `imgui`（4.5K 行）**留在 OpenCTK 原仓库**继续演进，不迁移。
- 参考 boost 的"按需取用"理念，但组织粒度采用 abseil 方式（单一仓库内细 target），不做 boost 式独立包分发。

## 2. 范围

| 项 | 决策 |
|---|---|
| 迁移模块 | core（56K 行）+ network（2.4K 行） |
| 不迁移 | media（WebRTC）、imgui（GUI）、tools/rcc |
| 代码 | ~58K 行源码 + 65 个 gtest 测试 + 3 个示例 |
| 仓库关系 | cxxkit 独立新仓库；media/imgui 留 OpenCTK，**过渡期冻结构建，续建路径待定**（审核 B1：media 13 文件用 octk::、112 文件引用 core 头，core 迁出后无法编译；决策延后） |

## 3. 目录结构

```
cxxkit/
├── CMakeLists.txt
├── .cmake.conf                  # 策略版本 3.15...3.31
├── 3rdparty/                    # vendored 三方库压缩包（沿用 OpenCTK）
├── cxxkit/                      # 唯一 include root（public 头，子库目录）
│   ├── base/                    # 宏、断言、类型基础（← source/global）
│   ├── containers/              # header-only: vector/inlined_vector/flat_set/concurrent_queue
│   ├── functional/              # header-only: signals/invocable/unique_function
│   ├── numerics/                # header-only: safe_compare/safe_conversions/divide_round
│   ├── text/                    # string/base64/ascii/bit_buffer/string_builder
│   ├── thread/                  # thread_pool/task_queue/event_loop/future/semaphore
│   ├── time/                    # date_time/clock/elapsed_timer/ntp_time/fake_clock
│   ├── units/                   # data_size/frequency/timestamp/time_delta
│   ├── memory/                  # aligned_malloc/shared_memory/zero_memory
│   ├── tools/                   # logging/metrics/random/error/status/filesystem/json/yaml
│   └── network/                 # http/ssl_*
├── src/                         # 各子库实现 cpp（同名子目录，仅编译型子库有）
├── tests/                       # gtest，按子库分目录
├── examples/
└── cmake/                       # cxxkit CMake helpers（精简自 OpenCTK）
```

### 子库划分依据

- 现有 `source/` 下 13 个子目录即为天然边界：`tools`（38 内部头）、`thread`（18）、`text`（13）、`memory`（12）、`kernel`（8）、`global`（7）、`units`（7）、`containers`（7）、`numerics`（6）、`functional`（3）、`time`（2）、`io`（1）、`patterns`（1）。
- `containers`/`functional`/`numerics`/`patterns` 为纯 header-only（无 cpp），零成本独立。
- `base` 合并 `global` + `kernel` 中的基础设施（宏、断言、类型基础），具体归属在实现期按依赖图确定。

## 4. 代码迁移规则

### 4.1 转发壳消除

现状：`include/*.hpp` 116 个文件全部是转发壳（一行 `#include "../source/xxx.hpp"`），真实实现在 `source/` 下（128 个内部头）。

迁移：删除转发壳层，public 头直接承载实现，`cxxkit/` 为唯一 include root。原 `source/` 下内部头按子库归位到 `cxxkit/<sub>/detail/` 或实现 cpp 旁。

### 4.2 头文件路径

- 统一 `#include "cxxkit/<sub>/<name>.hpp"`，如 `cxxkit/text/string.hpp`。
- 禁止 `../` 相对路径跨子库引用；跨子库依赖必须通过 `target_link_libraries` 声明 + 标准 include 路径。

### 4.3 命名空间

- 扁平 `cxxkit::`（abseil 风格：字符串、容器、时间都在 `absl` 下，不按子库嵌套）。
- 内部实现用 `cxxkit::detail::`。
- 宏体系：`OCTK_*` → `CXXKIT_*`；`OCTK_NAMESPACE octk` → `CXXKIT_NAMESPACE cxxkit`；`OCTK_BEGIN_NAMESPACE/END_NAMESPACE` 保留为 `CXXKIT_BEGIN_NAMESPACE/END_NAMESPACE`。

### 4.4 C++ 标准

- **C++11 起步**（用户明确要求，替代原" C++17 起步"约定；需同步更新 `.agents/memorys/conventions.md`）。
- 编译宏体系保留：`CXXKIT_BUILD_CXX_STANDARD_11/14/17/20/23/26`。
- 依赖 lite 系列（variant-lite/optional-lite/expected-lite/any-lite/string-view-lite/function2）填补 C++11 缺口——这些已在 vendored 3rdparty 中。

## 5. 构建体系

### 5.1 顶层 feature（沿用 octk 模式）

| Feature | 默认 | 说明 |
|---|---|---|
| `CXXKIT_FEATURE_DEV_BUILD` | ON | 开发模式（默认 debug 构建、不安装） |
| `CXXKIT_FEATURE_NO_PREFIX` | ON | 不装系统前缀 |
| `CXXKIT_FEATURE_INSTALL_PREFIX` | 派生 | 显式安装前缀 |
| `CXXKIT_FEATURE_CXX_STANDARD` | 11 | 编译标准 |
| `CXXKIT_BUILD_TESTS` / `CXXKIT_BUILD_EXAMPLES` | 按构建 | 控制子目录 |

支持 `INPUT_CXXKIT_FEATURE_*` 命令行注入（octk 兼容方式）。

### 5.2 模块级 feature

子库 CMakeLists 内 if/else 选择后端或特性，如 network：

- `CXXKIT_NETWORK_USE_BOOST_BACKEND` → boost.asio/beast
- `CXXKIT_NETWORK_HTTP_USE_LIBCPR_VCPKG` → vcpkg cpr
- 默认 → vendored cpr

### 5.3 CMake target

- 每个子库一个 target：`cxxkit::base`、`cxxkit::text`、`cxxkit::thread`、`cxxkit::tools`、`cxxkit::network` 等。
- 依赖显式：`target_link_libraries(cxxkit::text PUBLIC cxxkit::base)`。
- 构建顺序（依赖自底向上）：

```
base → {containers, functional, numerics}   # 纯头，零依赖
     → {units, memory} → text → time
     → thread → tools → network
```

### 5.4 三方库（完整沿用 octk wrap 体系）

用户明确要求保留 octk 的健壮性，不替换为 FetchContent：

| 机制 | 决策 | 理由 |
|---|---|---|
| 获取 | vendored 压缩包 + `octk_fetch_3rdparty` 解压 + stamp 防重复构建 | 删 build 目录后重新 configure 不报错（FetchContent 会挂） |
| 绑定 | `find_package(... PATHS <vendored install dir> NO_DEFAULT_PATH REQUIRED)` | 强制 vendored 版本，杜绝与系统库冲突 |
| 构建 | 每个三方库独立 `cmake configure + build + install` 到 `build/3rdparty/<lib>-<buildtype>/install/` | 隔离、可重复 |
| 集成 | `INTERFACE IMPORTED` wrap target（`CXXKITWrapFmt::WrapFmt` 等） | 消费方 `target_link_libraries` 直连 |
| 头文件安装 | 拷贝到 `<prefix>/include/cxxkit/3rdparty/<shortname>/`，include 接口改写 | 统一命名空间 |
| 消费方配置 | 安装时生成 INTERFACE IMPORTED stub target | 安装后 find_package 最简可用 |
| 静态化 | 三方一律静态编译，符号打入 cxxkit 子库（无独立 .so 分发） | 单库分发、无 rpath 问题 |
| 命名 | `octk_*` → `cxxkit_*`；`openctk/3rdparty/` → `cxxkit/3rdparty/` | 品牌统一 |

### 5.5 安装体系

- **标准导出 + 安装开关**（octk 式 `WILL_INSTALL` 逻辑）：
  - 默认（dev build 且未指定 prefix）→ 不生成 install 规则，开发态使用；
  - 显式 `-DCMAKE_INSTALL_PREFIX=xxx` 或 `-DINPUT_CXXKIT_FEATURE_INSTALL_PREFIX=xxx` → 生成安装规则。
- 安装实现：
  - `install(TARGETS ... EXPORT cxxkitTargets)` + `install(EXPORT cxxkitTargets ...)`；
  - `cxxkitConfig.cmake`（`find_dependency` 处理 3rdparty）；
  - 支持 `find_package(cxxkit COMPONENTS text network)` 按组件消费。
- 不做：OpenCTK 的 Qt-style ModuleDependencies 递归解析、ModuleDescription.json、Doxygen/FFmpeg/Python/Vcpkg 安装辅助。
### 5.6 cmake helpers 移植清单（分析自 OpenCTK cmake/ 体系）

**移植（高价值）**：

| Helper | 来源 | 说明 |
|---|---|---|
| `cxxkit_option()` | OptionHelpers (129 行) | 带 DEPENDS/EMIT_IF/INPUT_ 注入的选项系统；配套顶层 option 集（BUILD_ALL/SHARED_LIBS/USE_PCH/COMPILER_WARNING/WARNINGS_ARE_ERRORS/BENCHMARKS/DOCS/LIBS/APPS/TESTS/EXAMPLES/INSTALL）与 `CXXKIT_ENABLE_LIB_*` 子库开关 |
| `cxxkit_add_subdirectory()` | SubdirectoryHelpers (39 行) | 条件表达式 + 目录存在性检查 |
| 编译标志体系 | FlagHandlingHelpers + CompilerHelpers (~320 行) | `set_compiler_warnings`（GNU/Clang -Wall -Werror、MSVC /W4 /WX）、UTF-8 源码、异常标志、MSVC /Zc:__cplusplus、bigobj、version script、--no-undefined、`replace_compiler_option`（/MD↔/MT） |
| PCH 支持 | PrecompiledHeadersHelpers (81 行) | BUILD_WITH_PCH 条件 + target_precompile_headers + SKIP_PRECOMPILE_HEADERS（core/network 在用，必须带） |
| `cxxkit_add_test()` | TestHelpers (583 行，精简) | 统一测试 target + ctest 注册 + WORKING_DIRECTORY/ENVIRONMENT/TIMEOUT |
| RPATH 处理 | RpathHelpers (205 行) | 安装后动态库查找（@rpath/$ORIGIN） |
| vcpkg 后端 | InstallVcpkg（按需） | network 的 `CXXKIT_NETWORK_HTTP_USE_LIBCPR_VCPKG` 分支需要 |
| pkg-config 生成 | PkgConfigHelpers + PkgConfigLibrary.pc.in (182 行) | 生成 cxxkit.pc，非 CMake 项目可消费 |

**不移植**：AndroidHelpers（平台特定，按需再加）、ModuleHelpers/GlobalStateHelpers（Qt-style 模块解析，设计不做）、PublicWalkLibs/PublicTargetHelpers（静态链接复杂度，abseil 式依赖图不需要）、SyncIncludeHelpers（转发壳已消除）、ScopeFinalizer/SeparateDebugInfo（过度工程）、InstallFFmpegTools/InstallDoxygen/InstallPython（media 相关/工具链）、FrameworkHelpers（Apple framework，按需再加）


## 6. 测试

- GoogleTest（vendored，沿用 OpenCTK 3rdparty 中已有 gtest 源码）。
- 65 个测试按子库归属迁移，每个子库一个 test target。
- `CXXKIT_BUILD_TESTS` 控制，ctest 统一注册。

## 7. 拆分实施顺序

1. 建立 cxxkit 仓库骨架：CMakeLists + .cmake.conf + 3rdparty 迁移 + wrap helpers
2. `base` 先行：宏体系改名（OCTK→CXXKIT）、namespace 切换（octk→cxxkit）
3. 纯头子库：containers / functional / numerics（零 cpp，最快见效）
4. 编译子库依依赖序：units → memory → text → time → thread → tools
5. network（依赖 tools + 三方 boost/cpr）
6. 安装体系 + find_package 消费验证
7. 测试全部迁移并跑绿

## 8. 风险与对策

| 风险 | 对策 |
|---|---|
| 跨子库 `../source/` 引用打乱依赖图 | 依赖图上各子库先行确认边界，违例 include 编译期报错 |
| C++11 下部分现有代码需降级改写 | 降级面很小（OpenCTK 默认即 C++11，全 core 仅 2 文件用 std C++17 类型、2 文件用结构化绑定）；作为验证项而非风险 |
| 三方库静态化体积 | 与 OpenCTK 一致，可接受 |
| 65 个测试迁移工作量大 | 按子库分批迁移，每批跑绿再进下一批 |
| media/imgui 续建路径未定（B1） | 过渡期 OpenCTK 冻结构建，后续单独决策 |

## 9. 审核记录（2026-08-18 momus）

**BLOCKER**：B1 media/imgui 续建路径——已延后（见 §2）。

**HIGH**（实施计划须覆盖）：
- H1 子库映射：`io`（file_wrapper）、`patterns`（singleton）补入目录树；kernel 实为 object 体系（application/event/object + 5 cpp），需依赖扫描定归属
- H2 验收门禁：改名清零（`grep -rn octk` 无命中）、`-std=c++11` 全量编译通过、测试用例数对等（758 TEST + 44 TEST_F）

**MEDIUM**（实施计划须覆盖）：
- M1 测试 target 单独覆盖 C++14（gtest 1.12.1 要求），库 target 保持 11
- M2 boost.asio/beast 后端依赖 vcpkg（3rdparty 无 boost 包），网络拉取与 vendored 健壮性理由不一致——默认 vendored cpr，boost 后端标记可选
- M3 "符号打入子库"与"stub target"二选一：静态库随包分发（stub 引用已安装 .a）
- M4 内部头计数 124 非 128；PCH 降为按需移植（查无使用痕迹）

**LOW**：L1 benchmark 依赖待确认（tst_mutex_benchmark.cpp）；L2 MSVC 验证步骤；L3 `CXXKIT_BUILD_TESTS 按构建`表述修正 + status.md 记忆同步；L4 已并入风险表。
