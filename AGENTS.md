# PROJECT KNOWLEDGE BASE

**Generated:** 2026-08-19
**Commit:** (working tree — 49 cpp 迁移 + CMake 大小写修复未提交)
**Commit:** d0a7a2f
**Branch:** main

## OVERVIEW

CxxKit — 跨平台 C++ 工具库（OpenCTK 重构版）。abseil 式组织：目录 = 子库 = CMake target 三位一体，按需取用。C++11 起步（测试 C++14），扁平 `cxxkit::` 命名空间，vendored 三方依赖。

## STRUCTURE

```
cxxkit/
├── cxxkit/<sub>/    # 13 个子库头文件（base/containers/functional/numerics/patterns/
│                    #   memory/units/time/kernel/thread/text/tools/network）
│   └── <sub>/detail/  # 私有实现头 *_p.hpp（随公共头安装，doxygen 排除）
├── cxxkit/<sub>/    # 13 个子库：头 + 源 .cpp + CMakeLists 聚合（abseil 式，无 src/ 目录）
├── tests/           # 33 个 gtest 套件（353 用例）
├── examples/        # 3 个示例（exp_core_version/logging/network_version）
├── cmake/           # 10 个 CxxKit*Helpers + wrap/FindWrap*.cmake（21 个）
├── 3rdparty/        # 21 个 vendored 三方压缩包（stamp 防重复构建）
├── docs/            # Doxyfile.in + superpowers specs/plans + README 索引
├── .agents/         # rules + memorys + skills
└── SKILL.md         # 技能注册表（superpowers + ponytail + 项目技能）
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| 宏体系/命名空间宏 | `cxxkit/base/macros.hpp` | CXXKIT_BEGIN_NAMESPACE、CXXKIT_DEFINE_DPTR 等 |
| 编译期配置 | `cxxkit/base/core_config.hpp` | **构建生成**（build 树），勿手改 |
| CHECK/DCHECK 断言 | `cxxkit/tools/checks.hpp` | 双轨：fatal 流式 + debug-only |
| 子库 API 导出宏 | `cxxkit/<sub>/..._global.hpp` | CXXKIT_CORE_API/NETWORK_API 等 |
| 私有实现 | `cxxkit/<sub>/detail/*_p.hpp` | pimpl 配对（CXXKIT_DEFINE_DPTR） |
| 测试 | `tests/tst_*.cpp` | gtest + gmock |
| 三方 wrap | `cmake/wrap/FindWrap*.cmake` | 21 个 vendored 包 |
| 项目状态/决策 | `.agents/memorys/` | status/conventions/decisions/pitfalls |

## CODE MAP

核心枢纽（按跨子库引用扇出排序）：

| Symbol/File | Location | 扇出 | Role |
|-------------|----------|------|------|
| `tools/type_traits.hpp` | cxxkit/tools | 15 类 | 类型萃取枢纽，被 6+ 子库依赖 |
| `base/macros.hpp` | cxxkit/base | 13 子库 | 宏中枢（1177 行） |
| `tools/checks.hpp` | cxxkit/tools | 13 次 | 断言体系，几乎所有编译子库依赖 |
| `thread/thread_pool.cpp` | cxxkit/thread | 15 类 | 线程池实现（706 行） |
| `kernel/signals.hpp` | cxxkit/kernel | — | 最大文件（1899 行） |
| `network/http.hpp` | cxxkit/network | cpr | HTTP 封装（614 行） |

## CONVENTIONS

- **C1/D4**：唯一构建系统 CMake；三方依赖 vendored + stamp，**禁 FetchContent**
- **C4/D3**：库代码 C++11（按子库可升级）；测试 target 强制 C++14（gtest 1.12.1）
- **C6**：**禁 `rm -rf build`**——3rdparty stamp 缓存全清 = 5+ 分钟重建；只清 `build/CMakeCache.txt build/CMakeFiles build/Testing`
- **C7**：品牌名 CxxKit（显示层）；代码标识符（头目录/namespace/target/include/包名）一律小写 `cxxkit`
- **C8/D14**：子库目录 = 头 + 源 .cpp + CMakeLists 聚合（abseil 式），无 src/ 目录
- **D15**：头文件进 target 源列表（GLOB CONFIGURE_DEPENDS），IDE 大纲可见
- **D2**：扁平 `cxxkit::` 命名空间；内部实现 `cxxkit::detail::`；宏前缀 `CXXKIT_*`
- **D8**：内部 include 一律**尖括号** `<cxxkit/...>`（禁引号，192 文件统一）
- **D9**：私有头 `xxx_p.hpp` 放 `<sub>/detail/`，随公共头安装（2026-08-19 用户决策）
- **D6**：三方头用 `<cxxkit/3rdparty/<lib>/...>` 命名空间路径（构建/安装双侧一致）
- **D11**：三方依赖 vcpkg 式：安装三方自有 Config.cmake，`find_dependency` 递归链
- **D12**：默认装 `build/install/`；自定义 target `BuildAll`/`BuildInstall`/`Docs`
- **D13**：文档三层：doxygen（Docs target）+ README + docs/ 索引
- **D5**：media/imgui 留在 OpenCTK，不迁移
- 头部守卫**不统一**：base/*.hpp 用 `#ifndef _CXXKIT_X_HPP`（旧），其余 60 头用 `#pragma once`（新）——新头照 `#pragma once`

## ANTI-PATTERNS (THIS PROJECT)

- **禁 `rm -rf build`**（C6）——stamp 缓存即全库缓存
- **禁 C++14 语法入库代码**：`grep -rnE "auto\s+\w+\s*=|if constexpr" cxxkit/ --include="*.hpp"` 应为空
- **禁改 `base/compiler.hpp` 特性表**："deprecated, do not update this list"——编译器特性检测冻结
- **禁改 `text/ascii.hpp` 0x 前缀行为**——十六进制输入不带 "0x" 是契约
- **禁"恢复" `tools/checks.hpp` 注释掉的旧 Logger 版 CHECK**——那是历史，新实现用 `CXXKIT_FATAL()`
- **禁静默简化设计文档写过的机制**（D6 教训）——如三方命名空间路径
- **禁 claim 测试通过而不实跑**——`ctest --test-dir build` 输出为准
- **禁 `octk`/`OCTK_` 残留**：`grep -rn "octk\|OCTK_" cxxkit/ tests/` 清零

## UNIQUE STYLES

- 头文件顶部统一**箱式许可横幅**（`/*** Library: CxxKit ... ***/`）
- Pimpl 惯例：`CXXKIT_DEFINE_DPTR(Class)` → `std::unique_ptr<Class##Private> mDPtr`，配 `detail/xxx_p.hpp`
- 特性宏阶梯：`CXXKIT_CC_FEATURE_*` → `CXXKIT_CXX{11,14,17,20,23}_CONSTEXPR` 空降级链
- 断言双轨：`CXXKIT_CHECK(_OP)`（fatal 流式）+ `CXXKIT_DCHECK` 家族（debug-only）
- 聚合头 `base/global.hpp`：core_config + compiler + system + macros + types
- 导出宏按库：`<sub>_global.hpp` 定义，`CXXKIT_BUILD_SHARED` + `CXXKIT_BUILDING_*_LIB` 切 EXPORT/IMPORT
- 全局聚合头 global.hpp 是唯一 include 中枢

## COMMANDS

```bash
cmake -S . -B build -DCXXKIT_ENABLE_LIB_NETWORK=ON   # configure（默认 Debug，装 build/install）
cmake --build build --parallel                        # 构建
cmake --build build --target BuildInstall             # 构建+安装到 build/install/
cmake --build build --target Docs                     # doxygen（需 -DCXXKIT_BUILD_DOCS=ON）
ctest --test-dir build --output-on-failure            # 测试（33 套件/353 用例）
bash scripts/check.sh                                 # 本地门禁：format+namespace+build+test
clang-format -i <file>                                # 格式化（提交前）
```

## NOTES

- 默认装 `build/install/`，显式 `-DCMAKE_INSTALL_PREFIX` 才装出去
- `CXXKIT_ENABLE_LIB_NETWORK=ON` 触发 cpr/curl/mbedtls vendored 编译——构建慢主因
- vendored .pc 是**绝对 prefix**，安装树不可移动（vcpkg 同）
- core_config.hpp 在 build 树生成，编辑源码头无效
- semaphore 等时序测试偶发失败——单独重跑即过，非回归
- 子库依赖方向：base → {containers,functional,numerics} → {units,memory} → text → time → thread → tools → network（无循环）
