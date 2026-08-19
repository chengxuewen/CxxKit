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
