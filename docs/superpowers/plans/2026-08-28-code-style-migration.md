# CxxKit 代码风格统一实现计划（Spec v2 执行）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 CxxKit 全库统一到 spec v2 命名矩阵（mPascal 成员 / snake 函数与局部 / kPascal 枚举常量），完成横幅/D8/文档等机械修复。

**Architecture:** 4 批次串行（α 机械修复 → β 依赖序原子符号批 → γ 成员迁移 → δ 文档门禁）。β 核心是 python 符号重命名脚本（全词边界精确替换），依赖序 + 原子符号批（声明+全库调用点同批）保证每批编译通过。虚接口由 `override` 关键字编译器兜底。

**Tech Stack:** CMake/Ninja、python3 re.sub 脚本、clang-format、gtest 63 套件、doxygen。

**Spec:** `docs/superpowers/specs/2026-08-28-code-style-design.md`（v2 修订版——执行者必须同时读 spec）

## Global Constraints

- 禁 `rm -rf build`（只清 `build/CMakeCache.txt build/CMakeFiles`）；build/ 用 Ninja
- 库代码 C++11（禁 C++14 语法：`grep -rnE "std::[a-z_]+_t<|if constexpr" cxxkit/` 空）
- 脚本批量替换必须 python 全词边界（禁 sed 全局正则——edit-safety 规则）
- **commit 点由用户裁定**——每批次完成即报告等待，未明示不 commit；用户确认后用 `git -c user.name=chengxuewen -c user.email=1398831004@qq.com`
- 每批次门禁：clang-format 改动文件 0 违规 → `cmake --build build --parallel` 0 error → `ctest --test-dir build` 63/63 → Docs 零致命 warning → 批次专属 grep 门禁
- 迁移量基线（correctness 实测）：camel 292 符号 + Pascal 多词 440（脚本需过滤类名/构造噪声）≈ 700 函数符号；`xxx_` 成员 74 unique/809 出现
- 例外保留（禁碰）：std-镜像容器类名（flat_hash_map/set 小写）、trait structs（is_pointer 等）、POD 聚合 struct 字段（纯 snake）、枚举 4 离群族（base64 DO_*/logging 严重级/safe_conversions TYPE_*/once_flag State）与类型名重复存量（kVideoRotation_0）、宏名（UPPER_SNAKE）、命名空间

---

### Task 1: 批次 α——机械修复

**Files:**
- Modify: 17 横幅缺失文件（10 `*_global.hpp` + crash_handler.hpp/.cpp + detail/crash_handler_p.hpp + event_loop_thread.cpp + race_checker.cpp + media_global.hpp + video_rotation.hpp + video_types.hpp）
- Modify: `cxxkit/text/string_builder.hpp/.cpp`（双横幅）
- Modify: `cxxkit/media/color_space.{hpp,cpp}`, `hdr_metadata.{hpp,cpp}`（OpenCTK→CxxKit 行改名）
- Modify: media 8 个 .cpp + examples 3 个 .cpp（引号→尖括号 include）
- Modify: `cxxkit/thread/context_checker.hpp:58`, `thread_pool.hpp:243-244`, `time/elapsed_timer.hpp:135,165`（尾随空格）
- Modify: `cxxkit/memory/ref_counted_object.hpp:95,98,105,110`（`ref_count_`→`mRefCount`，FinalRefCountedObject only）
- Modify: `cxxkit/tools/logging.hpp:54`（`LogLevelNum`→`kLogLevelNum`）
- Modify: tests/ ~35 无横幅文件（箱式横幅统一）

**Interfaces:**
- Consumes: spec §4α 清单
- Produces: 干净基线树（β 从此起跑）；`FinalRefCountedObject` 成员名 `mRefCount`（β 的 SharedRefPtr 调用点已兼容——成员名不影响调用方）

- [ ] **Step 1: 横幅插入**。从 `cxxkit/base/global.hpp:1-24` 复制箱式横幅模板，插入 17 文件头部（#pragma once 之前）。string_builder 双横幅：CxxKit 箱 + 下方保留 WebRTC BSD 全文（复刻 `cxxkit/media/i420_buffer.hpp:1-11` 结构）。4 个 OpenCTK 文件只改 Library 行单词。tests 35 文件同模板。
- [ ] **Step 2: D8 include 修复**。逐文件将 `#include "own.hpp"` → `#include <cxxkit/media/own.hpp>`（media 8 文件）；examples 3 文件同理（`exp_logging.cpp:4-6` 三行）。验证：`grep -rn '#include "' cxxkit/ examples/ | wc -l` = 0
- [ ] **Step 3: 杂项**。5 处尾随空格 sed 单行删除（`sed -i 's/[[:space:]]*$//'`，仅 3 文件 5 行，逐行验证）；ref_counted_object 4 处 rename（edit 工具，同文件内有 mRefCount 上下文防误改）；`LogLevelNum` → `kLogLevelNum`（logging.hpp 1 处 + grep 全库引用同步）
- [ ] **Step 4: 门禁**。clang-format 改动文件；`cmake --build build --parallel` 0 error；`ctest --test-dir build` 63/63；横幅普查 `for f in $(find cxxkit -name '*.hpp' -o -name '*.cpp'); do head -15 "$f" | grep -q 'Library: CxxKit' || echo "$f"; done` → 期望 0 输出
- [ ] **Step 5: 报告等待 commit 裁定**

### Task 2: β 前置——重命名脚本 + 符号清单构建

**Files:**
- Create: `scripts/rename_symbols.py`（β 核心工具）
- Create: `/tmp/rename_map.json`（中间产物，不入库）

**Interfaces:**
- Produces: `rename_symbols.py` 契约——输入 JSON `{"old_name": "new_name", ...}`；对 lib+tests+examples 全部 .hpp/.cpp 逐文件 `re.sub(r'\b'+old+r'\b', new, content)`；输出每文件替换计数；`--check` 模式仅报告不写
- Produces: camel→snake 转换函数 `camel_to_snake('postTask')='post_task'`；`StrideY`→`stride_y`（Pascal 多词同函数）

- [ ] **Step 1: 写脚本**。功能：① 读 rename_map.json；② 遍历 `cxxkit/**/*.hpp,*.cpp, tests/*.cpp, examples/*.cpp`；③ 全词替换；④ `--dry-run` 统计模式；⑤ 替换后打印 per-file counts。附 `camel_to_snake()`：`re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', s).lower()`。
- [ ] **Step 2: 符号清单构建（半自动）**。运行符号收集（Task 2 实测命令），输出候选清单；**人工/LLM 复核过滤三类噪声**：类名与构造（ArrayView/InlinedVector/SharedBuffer 等——出现在 `\b([A-Z][a-z]+[A-Z]\w*)\(` 形态的多为构造调用，非方法）、宏（UPPER_SNAKE 不在 camel 正则内天然排除）、std 符号（to_string 已 snake 天然排除）。Pascal 方法白名单判定法：符号在 hpp 中以 `返回类型 Name(` 声明形态出现即方法。
- [ ] **Step 3: dry-run 验证**。`python3 scripts/rename_symbols.py --dry-run < map.json` 检查替换计数与预期文件数吻合；抽查 3 个符号的命中行（含 tests 调用点）。
- [ ] **Step 4: 脚本自测**。对 `scripts/rename_symbols.py` 写 10 行自测（构造临时文件+map 跑一遍断言输出）——放 `scripts/test_rename_symbols.py`，`python3 scripts/test_rename_symbols.py` 通过。

### Task 3: 批次 β——依赖序原子符号批（核心）

**Files:**
- Modify: 全库 ~110 含 camel/Pascal 函数符号文件（cxxkit 205 + tests 69 + examples 4 中实际命中者）
- 特别原子批：`memory/ref_count.hpp + ref_counted_object.hpp + media/video_frame_buffer.hpp + video_frame_buffer_pool.* + tests/tst_media_video_frame_buffer.cpp`（addRef/Release/HasOneRef/SharedRefPtr 调用族）

**Interfaces:**
- Consumes: Task 2 脚本 + map
- Produces: 全库函数 snake 化中间态（成员仍 `xxx_` 后缀——γ 处理；`\bstride_y_\b` 与 `stride_y(` 全词边界不冲突已验证）

- [ ] **Step 1: 按依赖序分子批执行**。顺序：base → containers/functional/numerics → units/memory（**addRef/Release/HasOneRef 原子批在此**：声明+全部 override+media/tests 调用点一个 map 同批）→ text → time（`Seconds/Millis/Micros/Abs/...` 工厂+取值族）→ thread → tools → kernel（**signals 特判**：`callSlot→call_slot` 8 虚 override + `hasCallable` + `toWeakPtr`，单原子批）→ media（`StrideY/GetDataY/SetBlack/Create/CalcBufferSize/...`）→ network（如启用；默认 OFF 跳过）。每子批：生成该批 map（该子库声明符号 + `to_`/Pascal 例外表）→ dry-run 计数核对 → 实跑 → **立即** `cmake --build build --parallel` 0 error → `ctest --test-dir build -R <该子库相关套件>` 绿 → 下一批。
- [ ] **Step 2: 宏生成名单点改**。`cxxkit/base/macros.hpp`：`CXXKIT_DECLARE_PRIVATE` 宏体内 `dFunc` → `d_func`（snake），39 使用点因宏展开自动跟随——编译验证；`getPointerHelper` 同批改 `get_pointer_helper`。
- [ ] **Step 3: 局部变量顺带**。局部 camel 变量（threadData/pendingTask 等 ~6:4 中 camel 部分）手工或同脚本 map 迁 snake——作用域窄，逐子库编译兜底。
- [ ] **Step 4: 全量门禁**。构建 0 error；`ctest --test-dir build` **63/63**；snake gate：`grep -rnP '\b(?!m[A-Z])[a-z]+[A-Z][a-zA-Z0-9]*\s*\(' cxxkit --include='*.hpp' --include='*.cpp' | grep -vP ':\d+:\s*(//|\*|/\*)' | wc -l` → 期望 0（若残留：判定类名构造噪声误报 vs 真漏网，漏网补 map 重跑）
- [ ] **Step 5: 报告等待 commit 裁定**

### Task 4: 批次 γ——成员 `xxx_` → mPascal

**Files:**
- Modify: media 10 文件（`stride_y_→mStrideY` 等）+ tools（shared_buffer.hpp 42 处/metrics.cpp 24/strong_alias.hpp 18/fake_clock/ntp_time）+ containers/detail/raw_hash_set.hpp（93 处）+ memory/ref_count.hpp（RefCounter::ref_count_ / RefCountedBase::ref_count_）

**Interfaces:**
- Consumes: β 后中间态（函数已 snake）
- Produces: 全库实例成员 mPascal 化；getter 形如 `int stride_y() const { return mStrideY; }`

- [ ] **Step 1: 生成成员 map**。`grep -rnE '\b[a-z][a-z0-9]*_\b' cxxkit --include='*.hpp' --include='*.cpp'` 收集全部 trailing-underscore 标识符，**剔除 POD 聚合 struct 字段**（hdr_metadata/http::Parameter/numerics 结果结构——spec §1 禁碰清单）与函数参数 shadow（`stride_y(stride_y_)` 构造列表中 `stride_y` 参数不带 `_` 天然无碰撞）→ camel_to_snake 逆变换生成 `mXxx` 目标名（`stride_y_` → `mStrideY`：去尾 `_`，snake→Pascal 加 m）。
- [ ] **Step 2: 同脚本机制跑 γ**。rename_symbols.py 复用（`--files` 参数限定 media/tools/containers/memory 四子库）；raw_hash_set 内部成员（`pos_/mask_/ctrl_/...`）同批迁 mPascal（abseil 移植件豁免已被用户 A 裁定取消）。构造初始化列表自动跟随全词替换（`stride_y_(stride_y)` → `mStrideY(stride_y)`）。
- [ ] **Step 3: 门禁**。构建 0 error；ctest 63/63；成员 gate：`grep -rnE '^\s+.*\s[a-z][a-z0-9]*_\s*(;|=)' cxxkit/ --include='*.hpp' --include='*.cpp'` → 期望仅剩 POD struct 字段（白名单核对输出行数 = hdr_metadata/http/numerics 聚合字段计数）
- [ ] **Step 4: 报告等待 commit 裁定**

### Task 5: 批次 δ——文档与门禁落地

**Files:**
- Modify: `.agents/rules/cpp/coding-style.md`（命名段整段替换为 spec §4δ 最终文本）
- Modify: `AGENTS.md`（UNIQUE STYLES 更新 + 删除失效头守卫描述行 + CONVENTIONS 补命名矩阵引用）
- Modify: `.agents/memorys/conventions.md`（C16 追加 ⑥：移植标识符同 PR 迁规范）
- Modify: `.agents/memorys/decisions.md`（D26 记录）
- Modify: `scripts/check.sh`（追加 snake gate + 成员 gate 两行，插在 namespace 检查后）

**Interfaces:**
- Produces: check.sh 新门禁两条（后续 CI 复用）；D26 决策记录

- [ ] **Step 1: 文档替换**。coding-style.md 按 spec §4δ "最终文本"块整段替换命名段；AGENTS.md 更新（含删除 "头部守卫不统一" 失效行——全部 pragma once 已实测）；conventions.md C16⑥；decisions.md D26 全链记录。
- [ ] **Step 2: check.sh 追加门禁**。在 namespace 检查步骤后追加 snake 函数 gate 与 mPascal 成员 gate（命令从 spec §4δ 拷贝），本地跑 `bash scripts/check.sh` 确认新步骤绿。
- [ ] **Step 3: Docs 验证**。`cmake --build build --target Docs` 零致命 warning（doxygen 注释内符号已随 β 同步）。
- [ ] **Step 4: 全量终验**。`bash scripts/check.sh` 全绿（7 步 + 新 2 gate）。
- [ ] **Step 5: 报告等待 commit 裁定；更新 memorys/status.md**

## Self-Review 结论

- **Spec 覆盖**：§1 矩阵→T3/T4 迁移+T5 文档；§2 前缀规则→T3 map 生成规则；§4 四批次→T1-T5 一一对应；§5 例外→Global Constraints 禁碰清单；§6 followups→不入本计划 ✓
- **占位符扫描**：无 TBD/TODO；T2 Step2 "半自动"已给出白名单判定法 ✓
- **类型/名一致性**：`rename_symbols.py` 契约 T2 定义 T3/T4 消费；`mStrideY`/`stride_y()` 示例跨任务一致 ✓
