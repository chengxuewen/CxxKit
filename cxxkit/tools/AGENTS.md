# cxxkit/tools — 子库指南

> 生成 2026-08-18 · 只讲本子库内部约定，全局约定见根 AGENTS.md

## OVERVIEW

最大子库（32 文件/6075 行），架构枢纽：15 类符号被跨子库引用（检查/日志/类型萃取/随机/状态），头文件面向全部下游，改动影响面最大。

## WHERE TO LOOK

| 任务 | 文件 |
|------|------|
| 断言 | `checks.hpp` — CHECK/DCHECK 双轨（fatal 流式 + debug-only） |
| 日志 | `logging.hpp` + `detail/logging_p.hpp`（私有实现，含 spdlog） |
| 类型萃取 | `type_traits.hpp`（741 行，6+ 子库依赖） |
| 错误/状态 | `error.hpp` / `status.hpp` / `result.hpp`（各自配 src/*.cpp） |
| 时间 | `clock.hpp` + `fake_clock.hpp` + `ntp_time.hpp` |
| 随机 | `random.hpp` |
| 并发原语 | `once_flag.hpp` |
| ID/缓冲/指标 | `id_registry.hpp` / `shared_buffer.hpp` / `metrics.hpp` |
| 轻量值类型 | `optional.hpp` / `expected.hpp` / `variant.hpp`（lite 包装） |
| 文件系统 | `filesystem.hpp`（ghc/std 别名） |
| 单点小工具 | `assert.hpp` `buffer.hpp` `enum_flags.hpp` `exception.hpp` `iterator.hpp` `limits.hpp` `sanitizer.hpp` `scope_guard.hpp` `source_location.hpp` `strong_alias.hpp` `type_info.hpp` `type_list.hpp` `utility.hpp` |

实现：`src/tools/*.cpp` 12 个（logging/random/assert/clock/status/error/once_flag/id_registry/shared_buffer/metrics/ntp_time/fake_clock）。头与 cpp 同名者才有编译实现，其余纯 header-only。

## CONVENTIONS

- **日志**：对外一律 `CXXKIT_LOGGING_*` 宏，禁止直调 spdlog/fmt 对象。
- **断言选型**：前置条件/预期必真 → `CXXKIT_CHECK`；仅调试期 → `CXXKIT_DCHECK`；廉价参数校验 → `<cassert>`。
- **可选值语义**：可能无值 → `optional`；可分级失败 → `expected`；多态替代 → `variant`。禁裸哨兵值（-1/nullptr）当返回码。
- **三方隔离**：fmt/spdlog 只活在 `detail/logging_p.hpp`，include 走 `<cxxkit/3rdparty/fmt/...>`；fmt 别名驻 `cxxkit::utils::`。
- **命名空间**：公共 API 扁平 `cxxkit::`；私有实现进 `cxxkit::detail::` / `cxxkit::utils::`，不进公共头签名。
- **新增编译型工具**：头 + src/*.cpp + CMakeLists 三件套同步提交，缺一不算完成。

## ANTI-PATTERNS

- 禁在公共头 include spdlog/fmt（暴露三方类型 = 锁死下游接口）；扩展日志能力改 `detail/logging_p.hpp`。
- 禁改 `checks.hpp` 错误路径语义（fatal 流式是契约，吞错/静默降级是 bug）。
- 禁给 tools 引入新三方依赖 — 枢纽扇出 15 类，任何新依赖按乘数放大。
- 禁动 `type_traits.hpp` 既有萃取定义；新增往 `cxxkit::utils::` 放，改前 grep 下游调用。
- 禁在头文件做重分配/锁（hot path 用户是 header-only 内联，成本翻倍）。