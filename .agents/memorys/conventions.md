# cxxkit 开发约定

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
