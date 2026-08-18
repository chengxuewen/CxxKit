# Phase 1: cxxkit 仓库骨架 + 宏体系迁移 + 纯头子库 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立 cxxkit 仓库骨架（CMake + vendored 3rdparty wrap 体系 + helpers），完成宏体系 OCTK→CXXKIT / 命名空间 octk→cxxkit 迁移，落地 base 与全部纯头子库（containers/functional/numerics/io/patterns），达到"改名清零 + C++11 编译 + 子库测试跑绿"验收门禁。

**Architecture:** abseil 式（目录=子库=CMake target），扁平 `cxxkit::` 命名空间，C++11 起步，vendored 3rdparty + stamp + NO_DEFAULT_PATH 沿用 octk 体系。本阶段产出 `cxxkit::base`、`cxxkit::containers`、`cxxkit::functional`、`cxxkit::numerics`、`cxxkit::io`、`cxxkit::patterns` 六个可独立构建的 target。

**Tech Stack:** CMake 3.15...3.31、C++11、GoogleTest 1.12.1（vendored）、vendored 三方库（fmt/json/spdlog/yaml-cpp/jwt-cpp/CLI11/lite 系列/concurrentqueue/readerwriterqueue）。

**上游设计文档:** `docs/superpowers/specs/2026-08-18-cxxkit-refactor-design.md`（§2-§5、§9 审核记录）

---

## 文件结构总览

```
cxxkit/                              (新建)
├── CMakeLists.txt                   (顶层：options、helpers include、add_subdirectory)
├── .cmake.conf                      (策略 3.15...3.31，从 OpenCTK 迁移)
├── 3rdparty/                        (vendored 压缩包，从 OpenCTK 拷贝 core+network 所需 14 个)
├── cmake/                           (helpers：cxxkit_option、add_subdirectory、compiler flags、
│                                    find_package、install、test、fetch_3rdparty、pkg-config)
├── cxxkit/
│   ├── base/                        (← source/global/ 6 头 + macros 体系)
│   │   ├── compiler.hpp  global.hpp  macros.hpp  preprocessor.hpp  processor.hpp  system.hpp  types.hpp
│   │   └── detail/
│   ├── containers/                  (← source/containers/ 7 头，纯头)
│   ├── functional/                  (← source/functional/ 3 头，纯头)
│   ├── numerics/                    (← source/numerics/ 6 头，纯头)
│   ├── io/                          (← source/io/ file_wrapper，1 cpp)
│   ├── patterns/                    (← source/patterns/ singleton.hpp，纯头)
│   └── core_config.hpp              (configure 生成：CXXKIT_VERSION_*、CXXKIT_BUILD_* 宏)
├── src/                             (编译型子库 cpp：src/io/file_wrapper.cpp)
├── tests/                           (gtest：tst_*.cpp 按子库分目录)
└── examples/                        (exp_core_version.cpp 迁移)
```

**迁移规则（贯穿所有任务）：**
- 转发壳消除：`include/xxx.hpp`（一行 `#include "../source/xxx.hpp"`）直接删除；`source/<sub>/xxx.hpp` 提升为 `cxxkit/<sub>/xxx.hpp`
- 宏替换：`OCTK_` → `CXXKIT_`（仅替换宏名，不替换 `OCTK_NAMESPACE` 的值）
- 命名空间：`octk` → `cxxkit`；`OCTK_BEGIN_NAMESPACE` → `CXXKIT_BEGIN_NAMESPACE`（宏值从 `octk` 改为 `cxxkit`）
- include 路径：`<openctk/core/xxx.hpp>` → `"cxxkit/xxx.hpp"`（子库头）；`<openctk/core/xxx.hpp>` 在子库间用 `"cxxkit/base/macros.hpp"` 风格
- C++11：不得使用 C++14+ 语法（structured binding/if constexpr/auto 返回类型推导等）；gtest 测试 target 例外（C++14）

---

## Task 1: 仓库骨架（顶层 CMake + .cmake.conf + 目录结构）

**Files:**
- Create: `CMakeLists.txt`
- Create: `.cmake.conf`
- Create: `cxxkit/`、`src/`、`tests/`、`examples/`、`cmake/` 目录
- Reference: `/Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/.cmake.conf`

- [ ] **Step 1: 创建 `.cmake.conf`**

从 `/Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/.cmake.conf` 拷贝，替换全部 `OpenCTK`/`OCTK` 为 `CXXKIT`/`cxxkit`：

```bash
cp /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/.cmake.conf .cmake.conf
# 内容替换：OCTK_ → CXXKIT_（变量名）、OpenCTK → cxxkit（注释/文字）
```

保留关键策略：
```cmake
set(CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION 3.15)
set(CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION 3.31)
set(CMAKE_POLICY_VERSION_MINIMUM 3.15)
```

- [ ] **Step 2: 创建顶层 `CMakeLists.txt`（骨架，helpers 后续任务填实）**

```cmake
cmake_minimum_required(VERSION 3.15...3.31)
include(.cmake.conf)

project(cxxkit VERSION 0.1.0 LANGUAGES CXX C)

set(CXXKIT_VERSION ${PROJECT_VERSION})
set(CXXKIT_COPYRIGHT "Copyright (C) 2025~Present ChengXueWen.")
set(CXXKIT_LICENSE "MIT License")
set(CXXKIT_PRODUCT_NAME "cxxkit")

# Pre-calculate dev_build feature
if(NOT DEFINED CXXKIT_FEATURE_DEV_BUILD)
    if(NOT DEFINED INPUT_CXXKIT_FEATURE_DEV_BUILD)
        set(CXXKIT_FEATURE_DEV_BUILD ON)
    else()
        set(CXXKIT_FEATURE_DEV_BUILD ${INPUT_CXXKIT_FEATURE_DEV_BUILD})
    endif()
endif()
message(STATUS "Project feature dev build ${CXXKIT_FEATURE_DEV_BUILD}")

# C++ standard (default 11)
if(NOT DEFINED INPUT_CXXKIT_FEATURE_CXX_STANDARD)
    set(CXXKIT_FEATURE_CXX_STANDARD 11)
else()
    set(CXXKIT_FEATURE_CXX_STANDARD ${INPUT_CXXKIT_FEATURE_CXX_STANDARD})
endif()
message(STATUS "C++ standard: ${CXXKIT_FEATURE_CXX_STANDARD}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

# Helpers (Task 2 提供)
include(CxxKitOptionHelpers)
include(CxxKitSubdirectoryHelpers)
include(CxxKitCompilerHelpers)
include(CxxKitFindPackageHelpers)
include(CxxKitFetchHelpers)
include(CxxKitInstallHelpers)
include(CxxKitTestHelpers)
include(CxxKitConfigureHelpers)
include(CxxKitLibraryHelpers)

# Options (Task 3 提供 octk_option 后启用)
octk_option_dummy: # placeholder — Task 3 替换为真实 octk_option 块
```

- [ ] **Step 3: 创建目录结构**

```bash
mkdir -p cxxkit/base cxxkit/containers cxxkit/functional cxxkit/numerics cxxkit/io cxxkit/patterns
mkdir -p cmake src/io tests/examples
```

- [ ] **Step 4: 提交**

```bash
git add .cmake.conf CMakeLists.txt
git commit -m "chore: bootstrap cxxkit cmake skeleton"
```

---

## Task 2: 移植 cmake helpers（精简版）

**Files:**
- Create: `cmake/CxxKitOptionHelpers.cmake`（← OpenCTKOptionHelpers.cmake，129 行）
- Create: `cmake/CxxKitSubdirectoryHelpers.cmake`（← OpenCTKSubdirectoryHelpers.cmake，39 行）
- Create: `cmake/CxxKitCompilerHelpers.cmake`（← OpenCTKCompilerHelpers.cmake，56 行）
- Create: `cmake/CxxKitFetchHelpers.cmake`（← OpenCTKCMakeHelpers.cmake 的 octk_fetch_3rdparty/stamp/reset_dir 部分）
- Create: `cmake/CxxKitFindPackageHelpers.cmake`（← OpenCTKFindPackageHelpers.cmake 精简）
- Create: `cmake/CxxKitInstallHelpers.cmake`（← OpenCTKInstallHelpers.cmake 精简）
- Create: `cmake/CxxKitTestHelpers.cmake`（← OpenCTKTestHelpers.cmake 精简）
- Create: `cmake/CxxKitConfigureHelpers.cmake`（← OpenCTKConfigureHelpers.cmake 精简：configure_definition/configure_library_begin/end）
- Create: `cmake/CxxKitLibraryHelpers.cmake`（← OpenCTKLibraryHelpers.cmake 精简：add_library 核心路径）

- [ ] **Step 1: 拷贝源文件到临时目录**

```bash
mkdir -p /tmp/cxxkit-helpers
cp /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKOptionHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKSubdirectoryHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKCompilerHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKCMakeHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKFindPackageHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKInstallHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKTestHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKConfigureHelpers.cmake \
   /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake/OpenCTKLibraryHelpers.cmake \
   /tmp/cxxkit-helpers/
```

- [ ] **Step 2: 逐个精简并改名**

每个文件执行：
```bash
# 1. 复制为 CxxKit 命名
cp /tmp/cxxkit-helpers/OpenCTKOptionHelpers.cmake cmake/CxxKitOptionHelpers.cmake
# 2. 全局替换函数名/变量名：octk_ → cxxkit_，OCTK_ → CXXKIT_，OpenCTK → CxxKit
# 3. 删除本设计明确"不移植"的依赖调用：
#    - 删除 octk_parse_all_arguments 依赖（内联为 cmake_parse_arguments 直用）——若保留则把 parse 宏也移植
#    - 删除对 OpenCTKScopeFinalizerHelpers / OpenCTKPublicWalkLibsHelpers / OpenCTKSyncIncludeHelpers 的 include 依赖
#    - 删除 ModuleDescription.json 相关
```

具体裁切范围：
- **CxxKitOptionHelpers**：完整保留 `cxxkit_option` 函数（DEPENDS/EMIT_IF/SET/OR_CONDITION/INPUT_ 注入全保留）；内联 `cxxkit_parse_all_arguments`、`cxxkit_evaluate_expression`（从 OpenCTKCMakeHelpers 拷入）
- **CxxKitSubdirectoryHelpers**：完整保留 `cxxkit_add_subdirectory`
- **CxxKitCompilerHelpers**：保留 `cxxkit_replace_compiler_option`、`cxxkit_set_compiler_warnings`
- **CxxKitFetchHelpers**：只保留 `cxxkit_fetch_3rdparty`、`cxxkit_stamp_file_info`、`cxxkit_make_stamp_file`、`cxxkit_reset_dir`（从 OpenCTKCMakeHelpers 提取，不依赖其他 octk 函数）
- **CxxKitFindPackageHelpers**：保留 `cxxkit_find_package` 宏（wrap target 查找 + PROVIDED_TARGETS）；删除 global scope 提升等非必要逻辑
- **CxxKitInstallHelpers**：保留 `cxxkit_install`（install 包装 + EXPORT 传递）；删除 `octk_internal_install_versioned_link`（本阶段无共享库）；保留 `cxxkit_configure_process_path`
- **CxxKitTestHelpers**：保留 `cxxkit_add_test`（SOURCES/INCLUDE_DIRECTORIES/LIBRARIES/OUTPUT_DIRECTORY）；删除 ENVIRONMENT/TIMEOUT 以外的复杂 finalizer
- **CxxKitConfigureHelpers**：保留 `cxxkit_configure_definition`、`cxxkit_configure_library_begin/end`、`cxxkit_configure_compile_test*`（从 OpenCTKConfigureHelpers 提取）
- **CxxKitLibraryHelpers**：保留 `cxxkit_add_library`（name/SOURCES/LIBRARIES/PUBLIC_LIBRARIES/INCLUDE_DIRECTORIES/EXCEPTIONS/PRECOMPILED_HEADER）；删除 module 解析、depends 文件生成、win32 rc、PCH 外部逻辑（PCH 用原生 target_precompile_headers）

- [ ] **Step 3: 验证 helpers 无残留 octk 引用**

```bash
grep -rn "octk\|OCTK_\|OpenCTK" cmake/ || echo "CLEAN"
# 期望输出 CLEAN
```

- [ ] **Step 4: 提交**

```bash
git add cmake/
git commit -m "chore: port cxxkit cmake helpers from OpenCTK"
```

---

## Task 3: 顶层 options + 3rdparty 目录迁移

**Files:**
- Modify: `CMakeLists.txt`（补 options 块）
- Create: `3rdparty/`（vendored 压缩包）

- [ ] **Step 1: 顶层 options 块（替换 Task 1 的 placeholder）**

```cmake
octk_option_dummy 删除，替换为：

cxxkit_option(CXXKIT_BUILD_ALL "Enable this to build all artifacts" OFF)
cxxkit_option(CXXKIT_BUILD_SHARED_LIBS "Enable this to build as dynamically" OFF
    SET BUILD_SHARED_LIBS)
cxxkit_option(CXXKIT_BUILD_USE_PCH "Enable this to build use precompiled header files" ON
    DEPENDS BUILD_SHARED_LIBS)
cxxkit_option(CXXKIT_BUILD_COMPILER_WARNING "Enable compiler warnings" OFF)
cxxkit_option(CXXKIT_BUILD_WARNINGS_ARE_ERRORS "Enable warnings as errors" ON)
cxxkit_option(CXXKIT_BUILD_BENCHMARKS "Enable benchmarks" OFF)
cxxkit_option(CXXKIT_BUILD_DOCS "Enable documentation" OFF)
cxxkit_option(CXXKIT_BUILD_LIBS "Enable libs" ON OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_APPS "Enable apps" ON OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_TESTS "Enable tests" ON OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_EXAMPLES "Enable examples" ON OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_INSTALL "Enable installer" ${CXXKIT_WILL_INSTALL_VALUE}
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_NETWORK "Enable network lib" OFF
    OR_CONDITION CXXKIT_BUILD_ALL)

# 子库
add_subdirectory(cxxkit/base)
add_subdirectory(cxxkit/containers)
add_subdirectory(cxxkit/functional)
add_subdirectory(cxxkit/numerics)
add_subdirectory(cxxkit/io)
add_subdirectory(cxxkit/patterns)
cxxkit_add_subdirectory(src/network CXXKIT_ENABLE_LIB_NETWORK)  # Phase 2
cxxkit_add_subdirectory(tests CXXKIT_BUILD_TESTS)
cxxkit_add_subdirectory(examples CXXKIT_BUILD_EXAMPLES)
```

- [ ] **Step 2: 拷贝 core+network 所需 vendored 包**

core 需要（14 个，与 FindWrap 对应）：
```bash
cd /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/3rdparty
cp fmt-12.1.0.zip json-3.12.0.7z CLI11-2.6.2.tar.gz jwt-cpp-v0.7.2.7z \
   spdlog-1.15.3.tar.gz yaml-cpp-0.9.0.zip any-lite-master.zip variant-1.4.0.tar.gz \
   expected-1.2.0.tar.gz optional-1.1.0.tar.gz function2-4.2.5.tar.gz \
   filesystem-1.5.14.tar.gz string-view-lite-1.8.0.tar.gz concurrentqueue-1.0.4.tar.gz \
   readerwriterqueue-1.0.7.tar.gz googletest-release-1.12.1.tar.gz \
   benchmark-1.8.4.tar.gz \
   /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/3rdparty/
```

network 需要（Phase 2 迁入）：cpr-1.9.9.tar.gz curl-8.16.0.tar.xz zlib-1.3.1.tar.gz

- [ ] **Step 3: 移植 FindWrap 文件（core 用 14 个）**

```bash
mkdir -p cmake/wrap
cd /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/cmake
for f in FindWrapFmt FindWrapJson FindWrapCLI11 FindWrapJwtcpp FindWrapSpdlog \
         FindWrapYamlCpp FindWrapAnyLite FindWrapVariant FindWrapExpected \
         FindWrapOptional FindWrapFunction2 FindWrapFilesystem FindWrapStringViewLite \
         FindWrapConcurrentQueue FindWrapReaderWriterQueue FindWrapGTest FindWrapBenchmark; do
  cp ${f}.cmake /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cmake/wrap/${f}.cmake
done
# 每个文件：OpenCTKWrap → CXXKitWrap、OpenCTK → CXXKIT、OCTK_ → CXXKIT_、octk_ → cxxkit_
```

- [ ] **Step 4: 验证**

```bash
ls 3rdparty/ | wc -l   # 期望 17（14 core + gtest + benchmark + 其余）
cmake -S . -B build -DINPUT_CXXKIT_FEATURE_DEV_BUILD=ON 2>&1 | tail -20
# 期望：无 FATAL_ERROR；显示 "Project feature dev build ON"
```

- [ ] **Step 5: 提交**

```bash
git add 3rdparty/ cmake/wrap/ CMakeLists.txt
git commit -m "chore: add vendored 3rdparty and wrap find modules"
```

---

## Task 4: 宏体系迁移（base 子库）— 核心改名任务

**Files:**
- Create: `cxxkit/base/compiler.hpp` `global.hpp` `macros.hpp` `preprocessor.hpp` `processor.hpp` `system.hpp` `types.hpp`（← source/global/ 同名文件）
- Create: `cxxkit/core_config.hpp`（configure 生成模板）
- Delete: 转发壳（OpenCTK 的 include/ 不迁移，仅 source/ 内容提升）

- [ ] **Step 1: 拷贝 source/global/ 文件并做系统性替换**

```bash
cd /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/src/libs/core
cp source/global/*.hpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cxxkit/base/

# 在每个文件中执行替换：
# 1. OCTK_ → CXXKIT_          （宏名：OCTK_BEGIN_NAMESPACE → CXXKIT_BEGIN_NAMESPACE）
# 2. octk    → cxxkit          （命名空间值：#define OCTK_NAMESPACE octk → #define CXXKIT_NAMESPACE cxxkit）
# 3. openctk → cxxkit          （include 路径）
# 4. OpenCTK → cxxkit          （注释/文字，非必须但统一）
```

- [ ] **Step 2: 验证 macros.hpp 核心宏**

`cxxkit/base/macros.hpp` 必须包含：
```cpp
#define CXXKIT_NAMESPACE               cxxkit
#define CXXKIT_PREPEND_NAMESPACE(name) ::CXXKIT_NAMESPACE::name
#define CXXKIT_USE_NAMESPACE           using namespace ::CXXKIT_NAMESPACE;
#define CXXKIT_BEGIN_NAMESPACE ... namespace CXXKIT_NAMESPACE { ...
#define CXXKIT_END_NAMESPACE   ... }  // namespace CXXKIT_NAMESPACE
```

- [ ] **Step 3: 创建 base 子库 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitBase)

add_library(cxxkit_base INTERFACE)
add_library(cxxkit::base ALIAS cxxkit_base)

target_include_directories(cxxkit_base INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>   # cxxkit/ 在根下
    $<INSTALL_INTERFACE:include>)

# base 是纯头子库（header-only）：INTERFACE target，无源文件
target_compile_features(cxxkit_base INTERFACE cxx_std_11)

# configure 生成 core_config.hpp
cxxkit_configure_library_end(CxxKitBase)
```

- [ ] **Step 4: 验证编译（临时 smoke test）**

```bash
cat > /tmp/smoke.cpp << 'EOF'
#include "cxxkit/base/macros.hpp"
#include "cxxkit/base/types.hpp"
CXXKIT_BEGIN_NAMESPACE
void smoke() { (void)0; }
CXXKIT_END_NAMESPACE
int main() { return 0; }
EOF
c++ -std=c++11 -I. /tmp/smoke.cpp -o /tmp/smoke && /tmp/smoke && echo "SMOKE PASS"
```

- [ ] **Step 5: 提交**

```bash
git add cxxkit/base/ CMakeLists.txt
git commit -m "feat: migrate base sublibrary with cxxkit namespace"
```

---

## Task 5: 纯头子库迁移（containers/functional/numerics/io/patterns）

**Files:**
- Create: `cxxkit/containers/`（7 头）`cxxkit/functional/`（3 头）`cxxkit/numerics/`（6 头）`cxxkit/io/file_wrapper.hpp` `cxxkit/patterns/singleton.hpp`
- Create: `src/io/file_wrapper.cpp`
- Create: 各子库 CMakeLists.txt

- [ ] **Step 1: 拷贝 + 替换（与 Task 4 相同规则）**

```bash
cd /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/src/libs/core
# containers（纯头 7 个）
cp source/containers/*.hpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cxxkit/containers/
# functional（纯头 3 个）
cp source/functional/*.hpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cxxkit/functional/
# numerics（纯头 6 个）
cp source/numerics/*.hpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cxxkit/numerics/
# patterns（纯头 1 个）
cp source/patterns/*.hpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cxxkit/patterns/
# io（1 cpp + 1 hpp）
cp source/io/file_wrapper.hpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/cxxkit/io/
cp source/io/file_wrapper.cpp /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/src/io/

# 每个文件替换（同 Task 4 Step 1）：
# OCTK_ → CXXKIT_、octk → cxxkit、openctk → cxxkit、#include "../" → 新子库路径
```

- [ ] **Step 2: 修正子库间 include 路径**

```bash
# 凡 #include "../xxx/yyy.hpp" 或 #include "xxx/yyy.hpp" 引用其他子库：
# 改为 #include "cxxkit/<sub>/yyy.hpp"
# 例：containers/vector.hpp 引用 base → #include "cxxkit/base/macros.hpp"
grep -rn '#include "\.\./' cxxkit/ | head -20   # 逐一修正，期望清零
```

- [ ] **Step 3: 创建子库 CMakeLists（containers 示例，其余同构）**

```cmake
# cxxkit/containers/CMakeLists.txt
cxxkit_configure_library_begin(CxxKitContainers)
add_library(cxxkit_containers INTERFACE)
add_library(cxxkit::containers ALIAS cxxkit_containers)
target_include_directories(cxxkit_containers INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_containers INTERFACE cxxkit::base)
target_compile_features(cxxkit_containers INTERFACE cxx_std_11)
cxxkit_configure_library_end(CxxKitContainers)
```

functional/numerics/patterns 同构（link cxxkit::base）。
io 是编译型子库：
```cmake
# cxxkit/io/CMakeLists.txt
cxxkit_configure_library_begin(CxxKitIo)
add_library(cxxkit_io src/../../src/io/file_wrapper.cpp)
add_library(cxxkit::io ALIAS cxxkit_io)
target_include_directories(cxxkit_io PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_io PUBLIC cxxkit::base)
target_compile_features(cxxkit_io PUBLIC cxx_std_11)
cxxkit_configure_library_end(CxxKitIo)
```

- [ ] **Step 4: 验证全量编译 + 改名清零**

```bash
cmake -S . -B build
cmake --build build --parallel 8
# 期望：构建成功，无错误

grep -rn "octk\|OCTK_" cxxkit/ src/ || echo "NAMESPACE CLEAN"
# 期望输出 NAMESPACE CLEAN（注释中的 octk 文字可容忍，代码符号必须清零）
```

- [ ] **Step 5: 提交**

```bash
git add cxxkit/containers cxxkit/functional cxxkit/numerics cxxkit/io cxxkit/patterns src/io
git commit -m "feat: migrate header-only sublibraries (containers, functional, numerics, io, patterns)"
```

---

## Task 6: 测试迁移（纯头子库对应测试）

**Files:**
- Create: `tests/CMakeLists.txt`
- Create: `tests/containers/tst_*.cpp` `tests/numerics/tst_*.cpp` 等（从 OpenCTK core/tests 对应拷贝）

测试文件归属（从 OpenCTK `src/libs/core/tests/` 按 include 的头文件判断）：
- containers: tst_array_view.cpp tst_inlined_vector.cpp
- numerics: tst_divide_round.cpp
- base: tst_checks.cpp tst_enum_flags.cpp
- functional: tst_function_view.cpp
- io: tst_file_wrapper.cpp

- [ ] **Step 1: 拷贝测试文件并替换 include/命名**

```bash
cd /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/src/libs/core/tests
for f in tst_array_view.cpp tst_inlined_vector.cpp tst_divide_round.cpp \
         tst_checks.cpp tst_enum_flags.cpp tst_function_view.cpp tst_file_wrapper.cpp; do
  cp $f /Users/cxw/Documents/Code/Work/DEVSYS/cxxkit/tests/
done
# 替换：#include <openctk/core/xxx.hpp> → #include "cxxkit/xxx.hpp"（或子库路径）
#       octk:: → cxxkit::、OCTK_ → CXXKIT_
```

- [ ] **Step 2: 创建 tests/CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})
find_package(Threads REQUIRED)

set(CXXKIT_TEST_LINK_LIBRARIES
    cxxkit::base cxxkit::containers cxxkit::functional
    cxxkit::numerics cxxkit::io
    CXXKitWrapGTest::WrapGTest)

# 测试 target 必须 C++14（gtest 1.12.1 要求），库保持 11
function(cxxkit_add_test_target name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${CXXKIT_TEST_LINK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(${name} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

cxxkit_add_test_target(cxxkit_tst_array_view tests/tst_array_view.cpp)
cxxkit_add_test_target(cxxkit_tst_inlined_vector tests/tst_inlined_vector.cpp)
cxxkit_add_test_target(cxxkit_tst_divide_round tests/tst_divide_round.cpp)
cxxkit_add_test_target(cxxkit_tst_checks tests/tst_checks.cpp)
cxxkit_add_test_target(cxxkit_tst_enum_flags tests/tst_enum_flags.cpp)
cxxkit_add_test_target(cxxkit_tst_function_view tests/tst_function_view.cpp)
cxxkit_add_test_target(cxxkit_tst_file_wrapper tests/tst_file_wrapper.cpp)
```

- [ ] **Step 3: 验证测试跑绿**

```bash
cmake -S . -B build -DCXXKIT_BUILD_TESTS=ON
cmake --build build --parallel 8
ctest --test-dir build --output-on-failure
# 期望：7/7 测试通过
```

- [ ] **Step 4: 用例数对等校验（H2 门禁）**

```bash
# 迁移前基线（OpenCTK）：
grep -c "TEST(" /Users/cxw/Documents/Code/Work/DEVSYS/OpenCTK/src/libs/core/tests/tst_{array_view,inlined_vector,divide_round,checks,enum_flags,function_view,file_wrapper}.cpp | awk -F: '{s+=$2} END {print "baseline:", s}'
# 迁移后：
grep -c "TEST(" tests/tst_{array_view,inlined_vector,divide_round,checks,enum_flags,function_view,file_wrapper}.cpp | awk -F: '{s+=$2} END {print "migrated:", s}'
# 期望：两数相等
```

- [ ] **Step 5: 提交**

```bash
git add tests/
git commit -m "test: migrate sublibrary tests with gtest"
```

---

## Task 7: 安装体系最小化 + Phase 1 验收

**Files:**
- Modify: 各子库 CMakeLists.txt（加 install 规则）
- Create: `cmake/CxxKitConfig.cmake.in`

- [ ] **Step 1: 给每个子库加 install 规则（以 base 为例）**

```cmake
# 在 cxxkit/base/CMakeLists.txt 追加：
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_base EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/base/
        DESTINATION include/cxxkit/base
        FILES_MATCHING PATTERN "*.hpp")
endif()
```

- [ ] **Step 2: 创建 Config 模板**

`cmake/CxxKitConfig.cmake.in`：
```cmake
@PACKAGE_INIT@
include(CMakeFindDependencyMacro)
include("${CMAKE_CURRENT_LIST_DIR}/cxxkitTargets.cmake")
check_required_components(cxxkit)
```

- [ ] **Step 3: 验证安装（非默认 prefix 触发安装）**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/tmp/cxxkit-install -DCXXKIT_BUILD_INSTALL=ON
cmake --build build --parallel 8
cmake --install build
# 期望：/tmp/cxxkit-install/include/cxxkit/base/*.hpp 存在
```

- [ ] **Step 4: 消费验证（H2 门禁）**

```bash
mkdir -p /tmp/cxxkit-consumer && cd /tmp/cxxkit-consumer
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.15)
project(consumer CXX)
find_package(cxxkit REQUIRED COMPONENTS base containers)
add_executable(app main.cpp)
target_link_libraries(app cxxkit::base cxxkit::containers)
EOF
cat > main.cpp << 'EOF'
#include "cxxkit/base/macros.hpp"
#include "cxxkit/containers/vector.hpp"
int main() { return 0; }
EOF
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/cxxkit-install
cmake --build build && ./build/app && echo "CONSUMER PASS"
```

- [ ] **Step 5: Phase 1 全量验收（汇总门禁）**

```bash
# 1. 构建通过
cmake -S . -B build && cmake --build build --parallel 8
# 2. 改名清零
grep -rn "octk\|OCTK_" cxxkit/ src/ tests/ | grep -vE "\.git|\.omo" || echo "NAMESPACE CLEAN"
# 3. C++11 编译门禁（库代码）
grep -rnE "auto\s+\w+\s*=|if constexpr|\[\[" cxxkit/ --include="*.hpp" | head || echo "C++11 COMPAT"
# 4. 测试全绿
ctest --test-dir build --output-on-failure
# 5. 消费验证
/tmp/cxxkit-consumer/build/app && echo "CONSUMER PASS"
```

- [ ] **Step 6: 提交**

```bash
git add cmake/CxxKitConfig.cmake.in cxxkit/ CMakeLists.txt
git commit -m "feat: minimal install/export with cxxkitConfig"
```

---

## 验收门禁汇总（H2）

| 门禁 | 命令 | 通过标准 |
|---|---|---|
| 构建 | `cmake --build build` | exit 0 |
| 改名清零 | `grep -rn "octk\|OCTK_" cxxkit/ src/` | 无输出（注释文字可容忍） |
| C++11 | `c++ -std=c++11` smoke + 库 target `cxx_std_11` | 编译通过 |
| 测试 | `ctest --test-dir build` | 全部 PASS |
| 用例对等 | grep 计数对比 OpenCTK 基线 | 数字相等 |
| 消费 | find_package(cxxkit COMPONENTS base) + 链接 | 构建并运行成功 |

## Phase 2 预告（后续计划）

编译型子库（units/memory/text/time/thread/tools）、network（+ cpr/curl/zlib wrap + boost 后端可选）、configure.cmake 编译宏生成（CXXKIT_VERSION_* 等 59 项）、PCH 启用、pkg-config 生成、剩余 ~58 个测试迁移、benchmark 依赖确认（L1）。
