# CxxKit 实施计划：按需模块开关（core 12 库 opt-in 化）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 12 个无条件 add_subdirectory 的核心子库全部改走 `cxxkit_add_subdirectory` + `CXXKIT_ENABLE_LIB_<SUB>` 开关（默认 ON 保 backward compat），使父项目 add_subdirectory 引入时可按需裁剪；依赖缺省走 cxxkit_option DEPENDS 钳制+警告（octk/QExt 同款，已实现零新基建）。

**Architecture:** 顶层 CMakeLists 声明区批量加 option（DEPENDS 表达依赖）+ 无条件 add_subdirectory 块整体替换为 cxxkit_add_subdirectory 条件化；cxxkitConfig 的 find_package(COMPONENTS) 对账不动（组件不存在时 cxxkit_<sub>_FOUND=false 天然成立）；tests/examples 的子库引用块加开关守卫（引用未启用库的测试不注册）。

**Tech Stack:** CMake only——cxxkit_option（INPUT_ 注入/DEPENDS 钳制/OR_CONDITION 已实现）+ cxxkit_add_subdirectory 已就位，零新基建。

**Spec:**
- 调研依据：本会话 octk/QExt 机制分析（2026-09-08，本机 OpenCTK/QExt 源码实据——六问结论 + 方案 A 变体裁定）
- 约束基线：conventions C1/C4/C7/C8/C10、D1-D16（一期）、bundle I1-I7 继续有效
- 一期/二期实现：只消费不可改（除本计划授权的 CMakeLists 修改）

## Global Constraints（继承）

- C6：禁 rm -rf build——验证用独立 build 目录或增量重配
- C10：target 注册全走 helper；helper 参数列表内禁注释行
- 每任务收口：configure 0 error + ctest 全绿（默认全 ON 形态不回归）+ 提交（-c user.name=chengxuewen -c user.email=1398831004@qq.com）
- CMake 变更后立即 configure 验证（edit-safety）；CMakeLists 禁 clang-format
- **验收三态**：①全 ON（默认，不回归）②裁剪中间态（关几个库，其消费者连带钳制）③最小集（只 base 等 header-only + 单库）

## 依赖图（2026-09-08 实测，钳制依据）

```
base(0) → containers(base) → units(base,containers,numerics)
base → functional(base) / numerics(base) / patterns(base) / time(base)
base+time → tools(base,time)
base+memory(tools) → kernel(base,memory,tools)
全家 → thread(base,functional,kernel,memory,time,tools,units)   ← 扇出最大
base+numerics+tools → text
base+thread+tools → media
base+text+thread+memory+tools → network
base+network+patterns+tools → crash
base+tools → imgui / qt(+kernel)
profiling(0)
```

**钳制方向**：开关上游依赖未开 → 本库强制 OFF + WARNING（cxxkit_option DEPENDS 已实现，不自动传播）。

## 任务分解

### Task M0：开关声明区 + 条件化（主体改动）

**Files:**
- Modify: `CMakeLists.txt`（声明区 L181-198 批量加 12 option；L242-255 无条件块改条件化）
- Modify: `cmake/CxxKitConfigureHelpers.cmake`（仅当 cxxkit_option 的 DEPENDS 语法有缺口时——预计零改动）

**Interfaces:**
- Produces（声明区形态，依赖按上图 DEPENDS 表达）:
```cmake
cxxkit_option(CXXKIT_ENABLE_LIB_CONTAINERS "..." ON DEPENDS "CXXKIT_ENABLE_LIB_BASE" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_THREAD "..." ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_FUNCTIONAL AND CXXKIT_ENABLE_LIB_KERNEL AND CXXKIT_ENABLE_LIB_MEMORY AND CXXKIT_ENABLE_LIB_TIME AND CXXKIT_ENABLE_LIB_TOOLS AND CXXKIT_ENABLE_LIB_UNITS"
    OR_CONDITION CXXKIT_BUILD_ALL)
# ... 12 个（base/profiling 无依赖）
```
- 条件化形态：
```cmake
cxxkit_add_subdirectory(cxxkit/containers CXXKIT_ENABLE_LIB_CONTAINERS)
# base 保持无条件 add_subdirectory（core 的 core——所有开关的 DEPENDS 锚点）？
# → 裁定（R-M0-1 终版，Momus F7）：**base 与 profiling 均不配开关、保持无条件**——
#   base 是依赖图根（关它=全关，option 纯噪音）；profiling 零依赖零受益者。
#   12 开关 = 14 core − base − profiling。
```
- **裁定（R-M0-2）**：默认全 ON；`CXXKIT_BUILD_ALL` 已有总闸语义（OR_CONDITION）沿用——不开 BUILD_ALL 时逐库裁剪，开时全开。
- **tests/examples 守卫**（Momus F5 修订）：现有守卫 tests 仅 4 个（CRASH/IMGUI/UV/QT）、examples 仅 5 个——**media 全裸**（tests 8 块 + exp_media）。本任务补：tests **12 块**（11 核心 + media；network 无测试）、examples **12 块**（11 核心 + media）。

- [ ] **Step 1: 声明区 12 option**（DEPENDS 按依赖图逐个写；先 cxxkit_option 源码确认 DEPENDS 接受 AND 表达式——CxxKitOptionHelpers 实查）
- [ ] **Step 2: L242-255 条件化**（containers→units 顺序保持；base 保留无条件）
- [ ] **Step 3: tests/examples 守卫补齐**（11 个核心库块——现有无条件块逐一包 if）
- [ ] **Step 4: 三态验证**：①默认全 ON configure+build+ctest（78 套件不回归）②`-DCXXKIT_ENABLE_LIB_MEDIA=OFF -DCXXKIT_ENABLE_LIB_TEXT=OFF`（钳制链：media OFF→network/crash 自动 OFF+WARNING 出现；text OFF→network OFF）→ build+ctest（套件数下降且无 media/text 测试）③最小 `-DCXXKIT_ENABLE_LIB_<非base全部>=OFF` → configure 过 + 只剩 header-only+base 套件
- [ ] **Step 5: 提交** `feat(cmake): per-sublibrary opt-in switches for the 12 core libraries (octk/qext pattern)`

### Task M1：Config/export 对账 + INPUT 通道验证

**Files:**
- Modify: `cmake/CxxKitConfig.cmake.in`（核对 COMPONENTS 对账——预计零改动，只验证）
- Create: `/tmp/opencode/p2-consumer/` 消费方验证工程（临时，不入库）

**Interfaces:**
- Produces: install 树三态验证记录 + 消费方 find_package(cxxkit COMPONENTS …) 组件缺失行为记录

- [ ] **Step 1: install 三态**：全 ON BuildInstall → /tmp 消费方 COMPONENTS base text 全链过；裁剪态（text OFF）install → 同消费方 COMPONENTS text → 应 cxxkit_text_FOUND=FALSE 且 CMake 错误信息可读；未请求的组件不炸
- [ ] **Step 2: INPUT 通道**：`-DINPUT_CXXKIT_ENABLE_LIB_TEXT=OFF` 等价于直接 -D 开关（cxxkit_option INPUT_ 优先已实现——验证而非新开发）
- [ ] **Step 3: 提交**（如有修）或记录-only `docs: component accounting verified (M1)`

### Task M2：memory/文档收尾

**Files:**
- Modify: `.agents/memorys/`（status 段落 + decisions.md D32 + conventions 若定新约定）
- Modify: `README.md`（Build 选项表加 12 开关说明 + 最小构建示例）
- Modify: spec 或 docs（如 porting-scan 报告无需动）

- [ ] **Step 1: memory/README** → **Step 2: check.sh 全链 exit=0** → **Step 3: 提交** `docs(memory): opt-in switches record (D32)`

## 验收门禁

- 默认全 ON：ctest 78 套件不回归 + check.sh 8/8
- 裁剪态：钳制 WARNING 出现 + 套件数按裁剪下降 + build 0 error
- 最小态：configure/build 过 + header-only 套件绿
- install 三态 + INPUT 通道验证记录

## 明确不做

- feature 系统（方案 C，850 行移植不值）
- domain bundle（方案 B，YAGNI——逐库开关已够，bundle 需二级开关兜底）
- 依赖自动传播（octk/QExt 同款：钳制+警告，不自动 ON）
- cxxkitConfig 组件化重构（现有 COMPONENTS 机制天然兼容）
