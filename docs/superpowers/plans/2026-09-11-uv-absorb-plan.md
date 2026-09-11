# uv 库解散计划：引擎并入 kernel（默认后端）+ IO 组件迁 network（D38）

日期：2026-09-11 ｜ 依据：QtCore 模式分析（2026-09-11 会话，用户终裁）
前置：D37（loop 词汇族）、D32（模块开关体系）、D30/D31（事件循环子系统）

## 一、动因与模式对齐

**QtCore 实证**（本机 Qt 环境 `ldd libQt6Core.so`）：QtCore 直接链接 glib/ICU/pcre2/zlib——**Core 库允许携带三方引擎，但对外 API 面零引擎词汇**（glib 字样不出现在任何 Qt 公开头）。cxxkit 现状"kernel 无引擎、uv 独立 opt-in"造成开箱体验断裂（裸 EventLoop 无法运行）。本计划按 Qt 模式重排：**引擎并入 kernel 作默认后端（不暴露），IO 组件归位 network**。

**撤销的历史论断**："kernel 零三方依赖"不再立法——kernel 允许携带三方引擎（uv），但保持：不暴露引擎词汇、引擎可换（qt 桥仍 opt-in）、开关可控。

## 二、终版结构

```
cxxkit/kernel/
├── uv/                                  # 新增（引擎私有区，自 cxxkit/uv/ 迁入）
│   └── detail/uv_event_dispatcher.{hpp,cpp} + detail/uv_event_dispatcher_p.hpp
│       # PRIVATE 化：不安装、不进 doxygen、导出宏改 CXXKIT_KERNEL_API
├── default_dispatcher.{hpp,cpp}         # 新增：make_default_dispatcher()（kernel 唯一引擎入口）
└── （其余不动）

cxxkit/network/                          # 吞入 IO 组件（语义归位）
├── tcp_socket.{hpp,cpp} + detail/tcp_socket_p.hpp   # 自 uv 迁入
├── tcp_server.{hpp,cpp} + detail/tcp_server_p.hpp   # 同上
└── http.*                               # 既有

cxxkit/uv/                               # 整库消亡（dispatcher_factory.{hpp,cpp} / uv_global.hpp 删除）
```

## 三、开关与 API

- **`CXXKIT_ENABLE_LIB_UV` → `CXXKIT_ENABLE_LOOP_BACKEND_UV`**（用户裁定）：
  - `ENABLE` 动词族保留（全库 17+ 开关一致）；`LIB` 删除（不再指子库）；`LOOP_BACKEND` 对齐 D37 loop 词汇族
  - **默认值 OFF → ON**（QtCore 模式：glib 在 Qt 默认开；开箱即跑）
  - DEPENDS 收敛为 `BASE AND TOOLS`（kernel 变条件内含）
- **`make_default_dispatcher()`**（kernel，`default_dispatcher.hpp`，`CXXKIT_KERNEL_API`）：
  - `CXXKIT_ENABLE_LOOP_BACKEND_UV=ON` → 内部 `new UvEventDispatcher()`
  - OFF → `CXXKIT_CHECK` fatal："no default loop backend — enable CXXKIT_ENABLE_LOOP_BACKEND_UV or inject a dispatcher"
- **`make_uv_dispatcher()` 删除**（uv 字样从全部公开头清零；含 ELT/Application 头文件的 doc 示例同步换词）
- **公开头安装面**：kernel/uv/detail/ 不安装（私有引擎）；network 的 tcp 头照常安装；`<cxxkit/3rdparty/libuv/...>` vendored 头安装面**不变**（uv 现行 install 块归 kernel target 执行）
- **EventLoop 默认 ctor**（新增便利）：`EventLoop(Object *parent = nullptr)` 重载 → 内部调 make_default_dispatcher()——`cxxkit::EventLoop loop;` 开箱即跑（Qt QEventLoop 对位）

## 四、迁移清单

| # | 项 | 明细 |
|---|---|---|
| M1 | git mv ×9（Momus F-1 计数修正） | uv_event_dispatcher.{hpp,cpp}+detail/uv_event_dispatcher_p.hpp → kernel/uv/detail/；tcp_socket.{hpp,cpp}+detail/tcp_socket_p.hpp → network/；tcp_server.{hpp,cpp}+detail/tcp_server_p.hpp → network/（detail 两 p.hpp 必须随迁，勿按"对=hpp+cpp"漏掉） |
| M2 | 删除 ×3 | dispatcher_factory.{hpp,cpp}、uv_global.hpp（CXXKIT_UV_API 消亡） |
| M3 | 新增 ×2 | kernel/default_dispatcher.{hpp,cpp} + EventLoop 默认 ctor 重载（无二义已验证：nullptr→Object\* 标准转换压过 unique_ptr 用户转换） |
| M4 | 导出宏 | 引擎类 CXXKIT_UV_API → CXXKIT_KERNEL_API；TCP 组件 → CXXKIT_NETWORK_API |
| M5 | include+comment 换词 | 全库 `#include <cxxkit/uv/...>`（14 处：tests 10 + examples 4，计数已核实）→ `<cxxkit/kernel/default_dispatcher.hpp>` / `<cxxkit/network/tcp_*.hpp>`；**make_uv_dispatcher 文档注释站点同步换词**（Momus F-3）：cxxkit/thread/event_loop_thread.hpp L45/L61 + cxxkit/qt/qt_event_dispatcher.hpp L118 + tests/tst_event_loop_thread.cpp L44/L81 |
| M6 | CMake | kernel：条件 +WrapLibuv(PUBLIC)+条件源列表 + make_default 源 + **install 块加 `PATTERN "uv/detail" EXCLUDE`**（Momus F-1：现行递归 install 会把私有引擎头装进安装树，无门禁可抓）+ libuv install 块条件 `if(LOOP_BACKEND_UV OR CXXKIT_ENABLE_LIB_NETWORK)`（Momus F-2）；network：+WrapLibuv(PUBLIC)（无条件，tcp 直用 uv_tcp_t）+ kernel 依赖；根 CMakeLists：开关改名/默认 ON/DEPENDS 收敛 BASE AND TOOLS + network DEPENDS 行补 `AND CXXKIT_ENABLE_LIB_KERNEL` + :393 VENDORED_FIND_DEPS libuv 同条件化 + `cxxkit_add_subdirectory(cxxkit/uv)` 删除 |
| M7 | pkg/Config | cxxkit-uv.pc 消亡；kernel.pc 条件含 -luv（PkgConfigHelpers L73 映射已备）；network.pc 含 -luv；CxxKitConfig：uv stub/find_dependency 条件 `LOOP_BACKEND_UV OR NETWORK` |
| M8 | 测试 | tst_uv_event_dispatcher：include 改 kernel 路径（build 树直含源码树，detail/ 可达）+ **守卫改 LOOP_BACKEND_UV**（Momus F-2 漏项）；tst_tcp_* 三件（socket/server/faults，计数已核实）守卫改 `NETWORK AND LOOP_BACKEND_UV`（Momus F-2：NETWORK=ON+backend=OFF 时编译过但运行即 fatal）；新增默认 ctor/默认工厂用例（OFF fatal 路径若可行） |
| M9 | 示例 | exp_event_loop：`make_uv_dispatcher` → `EventLoop loop;`（开箱形态，rc=0 契约不变）；exp_tcp_echo：include 换 network + factory 换 default |
| M10 | 文档/记忆 | README 子库表（uv 行删除；network 行 +tcp；kernel 行 +default loop backend）；.agents/memorys/decisions.md 追加 D38；status.md 同步；AGENTS.md 若有 uv 残留清理 |

## 五、验收门禁

1. `grep -rn "cxxkit/uv\|make_uv_dispatcher\|CXXKIT_UV_API\|CXXKIT_ENABLE_LIB_UV" cxxkit/ tests/ examples/` → 清零
2. 主树（LOOP_BACKEND_UV=ON）构建 0 error + ctest 全绿（83 套件；tst_tcp 守卫改 network 后计数不变）
3. ASAN 全量零诊断（正跑法）
4. **三态验证**：默认 ON 全绿 / `-DCXXKIT_ENABLE_LOOP_BACKEND_UV=OFF` 构建+测试绿（backend=OFF 时 tcp 测试随守卫裁剪；另验 NETWORK=ON+backend=OFF 组合安装树含 libuv） / shared 构建 0 error
5. 消费方 `/tmp` 验证：`find_package(cxxkit COMPONENTS kernel)` + `EventLoop loop;` 开箱编译运行（ON 态）
6. clang-format 全触碰文件干净；C18 英文

## 六、风险与备案

- **install 面搬家风险**：libuv 的 install 块（头/lib/cmake/pc 四段）从 uv CMakeLists 挂到 kernel target——install 树布局不变，消费方零感知
- **network 依赖变重**：network 新增 libuv 链（http.pc 传递面变化）——备案为 network 语义归位的合理代价（TCP 本就是 network 组件）
- **内核测试对 uv 的隐式依赖**：tst_kernel_event_loop* 若经 dispatcher_factory 拿引擎需改 default 工厂（执行时 grep 定死）
- **开关改名迁移摩擦**：INPUT_CXXKIT_ENABLE_LIB_UV 老缓存残留——configure 时 WARNING 提示新名（ octk 式，可选）
- **qt 桥不动**：`cxxkit/qt` + `make_qt_dispatcher` 独立 opt-in 保持（Qt 宿主嵌入场景优先选 qt 引擎，非默认）
