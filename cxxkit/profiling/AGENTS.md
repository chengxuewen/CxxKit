# cxxkit/profiling — 子库指南

> 生成 2026-08-19 · 只讲本子库内部约定，全局约定见根 AGENTS.md

## OVERVIEW

性能剖析子库（header-only INTERFACE，Tracy 后端，**opt-in**）：默认空操作零开销，`CXXKIT_ENABLE_LIB_TRACY=ON` 时注入 `TRACY_ENABLE` 并链接 vendored TracyClient。命名遵循主流惯例（absl/profiling、folly/tracing、Hazel Profiling.h）：功能名 `profiling`，不绑定后端名。

## WHERE TO LOOK

| 任务 | 文件 |
|------|------|
| 作用域剖析 | `profiling.hpp` — `CXXKIT_PROFILE_SCOPE(name)`（ZoneScopedS 包装） |
| 帧标记 | `profiling.hpp` — `CXXKIT_PROFILE_FRAME()`（FrameMark 包装） |
| Tracy vendored | `cmake/wrap/FindWrapTracy.cmake` + `3rdparty/tracy-0.13.1.tar.gz` |

## CONVENTIONS

- **opt-in 双态**：公共头必须 `#if defined(CXXKIT_PROFILING_ENABLED)` 条件编译——OFF 时空操作宏（`((void)0)`），ON 时真采集。用户代码无需条件编译。
- **宏命名**：一律 `CXXKIT_PROFILE_*` 前缀，禁直接暴露 Tracy API（ZoneScoped 等）到公共头。
- **Tracy 头路径**：`<cxxkit/3rdparty/tracy/tracy/Tracy.hpp>`（双 tracy——Tracy 内部跨目录相对 include `../client/`、`../common/`，整树映射保结构，PIT-3 同源）。
- **子库无条件建**：profiling 总是存在（INTERFACE 空壳）；Tracy 链接/宏注入仅在 `CXXKIT_ENABLE_LIB_TRACY=ON`。
- **依赖方向**：profiling → {base, Tracy}；下游（如未来 crash 子库）可依赖 profiling，反向禁止。

## ANTI-PATTERNS

- **禁把 Tracy 依赖挂到 tools**——tools 扇出 15 类禁新三方依赖（tools/AGENTS.md），profiling 独立是架构决策（D16）。
- **禁在 OFF 时 include Tracy 头**——profiling.hpp 的 include 必须在 `CXXKIT_PROFILING_ENABLED` 条件内，否则非 TRACY 构建缺头。
- **禁在公共头用 ZoneScoped 原名**——用户只能看到 `CXXKIT_PROFILE_*` 宏（Hazel 式包装，后端可换）。
- **禁改 TRACY_ENABLE 注入方式**——它必须作为 INTERFACE compile definition 传播（消费方 TU 生效），放在静态库 compile def 上不生效。
