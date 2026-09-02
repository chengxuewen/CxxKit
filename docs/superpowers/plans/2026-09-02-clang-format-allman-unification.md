# Clang-Format 全库大括号统一（Allman）实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让 `.clang-format`（`BreakBeforeBraces: Allman`）从纸面配置变成全库事实——安装工具、保护风险文件、分批格式化、门禁落地。

**Architecture:** clang-format 经 pixi 工具链供给（本机无系统 clang-format）→ 风险文件（冻结特性表/宏中枢）加 `// clang-format off` 护栏 → 按 abseil 式子库分 6 批格式化，每批独立「格式化→构建→测试→提交」闭环 → check.sh 格式门禁从「缺失即跳过」变为「缺失即 WARN、存在即硬门禁」。

**Tech Stack:** pixi (conda-forge `clang-format`) + CMake/Ninja + ctest (63 套件) + check.sh

**Spec:** `docs/superpowers/specs/2026-08-28-code-style-design.md`（D26 风格规范 v2；本计划是其大括号/格式维度的补充落地）+ `.clang-format`（唯一权威，C3）

## 体检结果（2026-09-02 全库扫描，10 项门禁）

| 检查项 | 结果 | 处置 |
|---|---|---|
| 裸 `namespace cxxkit {`（绕过 BEGIN 宏，丢失 MSVC C4251 抑制） | **24 文件**（media 20 + flat_hash 三件套 + video_types Allman 变体） | **前置任务 N0 机械迁移** |
| `}  // namespace cxxkit` 注释闭合（同样绕过 WARNING_POP） | 与上同源，24+ 文件 | 同 N0 一并处理 |
| D8 引号 include | 0 ✅ | — |
| octk 残留 | 1（注释内引用 WEBOCTK 文档名，非代码） | 免改，上游文档引用 |
| include 守卫 | `#pragma once` 全覆盖 ✅ | — |
| C++14 语法入库 | 0 ✅ | — |
| 尾随下划线成员 | 0 ✅ | — |
| 许可横幅 | 0 缺失 ✅ | — |
| 双重前缀 | 0 ✅ | — |
| 尾随空格 | **0 ✅**（c155111 已清） | — |
| 挂行 `{`（K&R） | **193 处**（类型/namespace 56、控制语句/函数签名 46、初始化列表等 91） | 本计划 F1–F5 主目标 |
| 已是独立行 `{`（Allman） | 2840 处 | 不动 |

**N0 的实质风险**：`CXXKIT_BEGIN/END_NAMESPACE` 不是简单开闭——宏内包 `CXXKIT_WARNING_PUSH/DISABLE_MSVC(4251)/POP`（DLL 接口告警抑制）。裸 namespace 在 Linux 无感，但未来 MSVC shared 构建会在这些文件喷 C4251。

**历史存量尾随空格**：c155111 已清零，本计划 format 阶段只需维持。

**为什么不曾在批次 α–δ 里顺带做：** 全库 format 是纯格式大 diff，会淹没语义改动（违反 C13 逐文件可审原则）、一次性打断 blame。故独立成计划、独立提交序列。

**已知风险文件（不许盲格式化）：**
- `cxxkit/base/compiler.hpp` — 编译器特性表，AGENTS.md 明令"deprecated, do not update this list"；clang-format 重排宏参数 = 变相改表
- `cxxkit/base/macros.hpp` — 1177 行宏中枢，`##` 拼接与续行 `\` 对格式化敏感
- `cxxkit/numerics/safe_compare*.hpp` / `cxxkit/tools/checks.hpp` — `CXXKIT_SAFECMP_MAKE_FUN` / `CHECK_OP` 宏族，参数表换行有破坏先例（PIT-20 同类）
- abseil 移植件（`containers/detail/raw_hash_set.hpp` 等）— 可格式化，但 diff 噪声大，单独成批便于审

## Global Constraints

- C3：格式化唯一权威 `.clang-format`，禁止手调风格；禁改 `.clang-format` 本身
- C6：禁 `rm -rf build`；只清 `build/CMakeCache.txt build/CMakeFiles`（本计划不需要清）
- C4：库代码 C++11；format 不改语义，若 format 后 C++14 门禁 grep 报错说明 format 引入了换行歧义——回退该文件排查
- 提交身份：`git -c user.name=chengxuewen -c user.email=1398831004@qq.com`

**2026-09-02 交互式确认四项裁定（clang-format 风格语义，执行时不得偏离）：**
1. `AllowShortFunctionsOnASingleLine: All`——**Task 2 执行时顺带改配置**（原 InlineOnly）。单行自由函数（raw_hash_set is_empty 家族、numerics 内联位函数）不膨胀。这是唯一一处 `.clang-format` 修改，属「首次执行前定稿」，不违反 C3（C3 禁的是执行期手调）
2. 接受连续赋值/枚举值手工对齐被碾平（`AlignConsecutiveAssignments: false` 维持）；个别珍视表格（如 raw_hash_set 的 Ctrl 枚举）用 `clang-format off` 护栏
3. `FixNamespaceComments: true` 保留——N0 后主命名空间闭合是宏不受影响，仅嵌套 detail/utils 字面闭合被规范化（~144 处，预期内）
4. `.clang-format` 死配置（IndentBraces 在 Allman 下无效、ObjC/Java 条目）不动，D27 记一句
- 每批提交信息前缀 `style(format):`，一批一提交，禁止跨批混提交
- 验证以实跑为准：构建 0 error + `ctest --test-dir build` 63/63 + 尾随空格 grep 0；禁止声称通过而不跑
- format 引入编译错误的处置：先 `git checkout -- <file>` 回退该文件，再插护栏重跑；禁止连环改

---

### Task N0（新增，前置）: 裸 namespace → CXXKIT_BEGIN/END_NAMESPACE 宏迁移

**Files:**
- Modify: `cxxkit/media/*.hpp/*.cpp`（20 文件）、`cxxkit/containers/flat_hash_map.hpp`、`cxxkit/containers/flat_hash_set.hpp`、`cxxkit/containers/detail/raw_hash_set.hpp`、`cxxkit/media/video_types.hpp`（Allman 变体 `namespace cxxkit\n{`）

**Interfaces:**
- Produces: `grep -rn '^namespace cxxkit\s*{' cxxkit/` 归零；`CXXKIT_BEGIN_NAMESPACE` 使用数 164 → 188

**为什么前置**：N0 是语义迁移（宏替换），必须在 F 批次纯格式化之前落——否则 format 先把大括号搬位，N0 的 diff 会混入格式噪声，两件事互相污染。且 N0 有独立验证价值（MSVC C4251 抑制恢复）。

- [ ] **Step 1: 精确串替换（python，非 sed——edit-safety 规则）**

```python
# 三种形态，逐文件验证计数后替换：
#   形态 A（22 文件）: "namespace cxxkit {" → "CXXKIT_BEGIN_NAMESPACE"
#   形态 B（video_types）: "namespace cxxkit\n{" → "CXXKIT_BEGIN_NAMESPACE"
#   形态 C（闭合）: "}  // namespace cxxkit" → "CXXKIT_END_NAMESPACE"
#   注意：raw_hash_set.hpp 内部有嵌套 "}  // namespace detail" 闭合（属于内层
#   detail 命名空间），不动——只替换主命名空间边界
```

- [ ] **Step 2: 计数对账**

```bash
grep -rn '^namespace cxxkit\s*{' cxxkit/ --include='*.hpp' --include='*.cpp' | wc -l   # 期望 0
grep -rc 'CXXKIT_BEGIN_NAMESPACE' cxxkit/media cxxkit/containers | grep -v ':0' | awk -F: '{s+=$2} END {print s}'  # 期望 = 替换数
```

- [ ] **Step 3: 构建 + 全量测试**

```bash
cmake --build build --parallel 2>&1 | grep -c error        # 期望 0
ctest --test-dir build --output-on-failure 2>&1 | tail -2  # 期望 63/63
```

- [ ] **Step 4: Commit**

```bash
git add cxxkit/media cxxkit/containers
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(namespace): migrate bare namespace to CXXKIT_BEGIN/END macros (24 files)

Ported files (media from OpenCTK, flat_hash from abseil) kept literal
'namespace cxxkit { ... }' and bypassed the warning push/pop pair inside
the macros — future MSVC shared builds would emit C4251 there. Video_types
Allman variant included. Nested detail namespace closes untouched.
Build 0 errors, ctest 63/63."
```

---

### Task 1: pixi 工具链供给 clang-format —— ✅ 已执行（2026-09-02，未提交，随本计划首个提交落盘）

**Files:**
- Modify: `pixi.toml`（dependencies 段加 clang-format + 注释）
- Modify: `pixi.lock`（pixi install 自动锁定 23.1.0）

**实际状态**：pixi.toml 已加 `clang-format = ">=17,<24"`（附注释：独立轻量包、C++11 项目用 17+ 即可），pixi install 已完成，`pixi run clang-format --version` = **23.1.0**。本任务剩余动作只有提交（与 N0 或首个 F 批次同车）。

**实测确认**：
- conda-forge 独立包（clangdev feedstock 拆出），单二进制 ~35MB，不拖 LLVM 工具链
- 现有 `.clang-format`（中文注释 + BraceWrapping 全键 + Allman）在 23.1.0 下解析正常

---

### Task 1.5（实测新增）: clang-format 调用纪律（全计划通用约束）

**实测踩坑三条，后续所有 Task 的命令必须遵守：**

1. **不递归目录**：`clang-format --dry-run cxxkit/` 静默返回 0 违规（不报错、不处理）
2. **违规报 stderr**：`2>/dev/null` 会吞掉全部违规（假 0 的根因）
3. **必须统一用此形态**：

```bash
find cxxkit tests examples \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format ... 2>&1
```

**实测基线（此命令形态）**：全库 **1650 行违规**，首批样例即 `CXXKIT_END_NAMESPACE` 宏行（N0 迁移后还会新增一批——宏展开的 120 列对齐 vs 源码缩进差异，属预期，F 批次统一消化）。

- [ ] **Step 1: 把上述命令形态写入 check.sh 第 1 步**（与 Task 9 门禁硬化合流）

- [ ] **Step 2: 每次 format 批次前后都用同一命令跑 dry-run 对账**（前：预期违规数；后：预期 0）

---

### Task 2: 配置定稿（唯一一处 .clang-format 修改）+ 风险文件护栏（clang-format off/on）

**Step 0（配置定稿）**: `AllowShortFunctionsOnASingleLine: InlineOnly` → `All`（交互式裁定 2026-09-02）。
改完在探针上验证：`pixi run clang-format --style=file:$PWD/.clang-format /tmp/opencode/probe.cpp` 单行自由函数保持单行。

**Files:**
- Modify: `cxxkit/base/compiler.hpp`（特性表段前后插护栏）
- Modify: `cxxkit/base/macros.hpp`（仅当本任务干跑显示重排时）
- Modify: `cxxkit/tools/checks.hpp`、`cxxkit/numerics/safe_compare.hpp`（同上，按干跑结果定）

**Interfaces:**
- Consumes: Task 1 的 `pixi run clang-format`
- Produces: 护栏后的风险文件在 `--dry-run --Werror` 下 diff 为空（后续批次 format 不再触碰它们）

- [ ] **Step 1: 干跑观测风险文件的实际重排**

```bash
for f in cxxkit/base/compiler.hpp cxxkit/base/macros.hpp cxxkit/tools/checks.hpp cxxkit/numerics/safe_compare.hpp; do
  echo "=== $f ==="
  pixi run clang-format --dry-run "$f" 2>&1 | grep -c "warning\|error" || true
  pixi run clang-format "$f" | diff "$f" - | head -20
done
```

记录每个文件的 diff 是否触碰宏表/`##` 拼接行（判定标准：diff 行包含 `#define` 或 `\` 续行 → 需护栏；纯空格/独立大括号 → 免护栏）。

- [ ] **Step 2: 给需护栏的文件插护栏**

护栏模式（以 compiler.hpp 特性表为例，锚定实际表格起止行）：

```cpp
// clang-format off
#define CXXKIT_CC_FEATURE(...) <特性表原文，一字不动>
// clang-format on
```

注意：`clang-format off/on` 必须各占独立一行，包住**完整**的宏表/宏族定义（含所有续行）。

- [ ] **Step 3: 验证护栏生效**

```bash
for f in cxxkit/base/compiler.hpp cxxkit/base/macros.hpp; do
  pixi run clang-format --dry-run --Werror "$f" && echo "$f CLEAN"
done
```

Expected: 每个文件输出 `... CLEAN`（无 warning）

- [ ] **Step 4: 编译 + 测试**

```bash
cmake --build build --parallel 2>&1 | grep -c error
ctest --test-dir build --output-on-failure 2>&1 | tail -2
```

Expected: `0` + `100% tests passed ... 63`

- [ ] **Step 5: Commit**

```bash
git add cxxkit/base/compiler.hpp cxxkit/base/macros.hpp   # 按实际护栏文件
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): guard frozen feature table & macro hub from clang-format

compiler.hpp feature table is do-not-edit (AGENTS.md); macros.hpp uses
##-concatenation sensitive to reflow. clang-format off/on brackets keep
them byte-stable while the rest of the repo formats."
```

---

### Task 3: 基线确认（格式化前快照）

**Files:** 无改动（纯验证）

**Interfaces:**
- Produces: 全绿基线记录——后续任何批次失败时，区分「format 引入」vs「本已存在」

- [ ] **Step 1: 记录基线（命令形态遵 Task 1.5 纪律）**

```bash
cmake --build build --parallel 2>&1 | grep -c error
ctest --test-dir build --output-on-failure 2>&1 | tail -2
find cxxkit tests examples \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format --dry-run 2>&1 | grep -c warning:   # 2026-09-02 实测 1650
grep -rE ' +$' cxxkit/ --include="*.hpp" --include="*.cpp" | wc -l   # c155111 后应为 0
```

Expected: `0` + `100% tests passed ... 63` + 记录 dry-run 违规总数（写入会话笔记，供 F 批次前后对账）

- [ ] **Step 2: 确认工作树干净**

```bash
git status --short
```

Expected: 仅 `.codegraph/`（已 ignore）等工具缓存 + 本计划的 pixi.toml/pixi.lock 待提交对（Task 1 遗留），无其它源码改动

---

---

### Task 4: 批次 F1 —— header-only 小库（base/containers/functional/numerics/patterns/profiling）

**Files:**
- Modify: `cxxkit/base/**.{hpp,cpp}`、`cxxkit/containers/**`、`cxxkit/functional/**`、`cxxkit/numerics/**`、`cxxkit/patterns/**`、`cxxkit/profiling/**`
- Test: 无新测试（回归靠 63 套件）

**Interfaces:**
- Consumes: Task 2 护栏（base 内风险文件已 byte-stable）
- Produces: 这 6 个子库全部 Allman + 归一化空格

- [ ] **Step 1: 格式化**

```bash
find cxxkit/base cxxkit/containers cxxkit/functional cxxkit/numerics cxxkit/patterns cxxkit/profiling \
  \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format -i
```

- [ ] **Step 2: C++11 门禁 + 尾随空格复核**

```bash
grep -rnE "auto\s+\w+\s*=|if constexpr" cxxkit/ --include="*.hpp" | wc -l   # 期望 0
grep -rE ' +$' cxxkit/base cxxkit/containers cxxkit/functional cxxkit/numerics cxxkit/patterns cxxkit/profiling | wc -l   # 期望 0
```

- [ ] **Step 3: 构建 + 全量测试**

```bash
cmake --build build --parallel 2>&1 | grep -c error        # 期望 0
ctest --test-dir build --output-on-failure 2>&1 | tail -2  # 期望 63/63
```

失败处置：`git checkout -- <出错文件>` → 插护栏 → 重跑本步。禁止连环改。

- [ ] **Step 4: Commit**

```bash
git add cxxkit/base cxxkit/containers cxxkit/functional cxxkit/numerics cxxkit/patterns cxxkit/profiling
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): batch F1 — base/containers/functional/numerics/patterns/profiling

clang-format (Allman) across 6 header-only sublibs. Braces-only count:
~193 hanging '{' repo-wide begin to flip here. Build 0 errors, ctest
63/63, trailing-ws 0."
```

---

### Task 5: 批次 F2 —— text + tools（编译子库第一组，abseil 移植件重灾区）

**Files:**
- Modify: `cxxkit/text/**`、`cxxkit/tools/**`

**Interfaces:**
- Consumes: Task 1 工具、Task 3 基线
- Produces: text/tools 全 Allman

- [ ] **Step 1: 格式化**

```bash
find cxxkit/text cxxkit/tools \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format -i
```

- [ ] **Step 2: 构建 + 全量测试**（同 Task 4 Step 3，命令与期望一致）

- [ ] **Step 3: Commit**

```bash
git add cxxkit/text cxxkit/tools
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): batch F2 — text/tools

ascii.cpp (100 hanging braces) and thread_pool-adjacent tool headers
normalized. Build 0 errors, ctest 63/63."
```

---

### Task 6: 批次 F3 —— memory + units + time

**Files:**
- Modify: `cxxkit/memory/**`、`cxxkit/units/**`、`cxxkit/time/**`

- [ ] **Step 1: 格式化**

```bash
find cxxkit/memory cxxkit/units cxxkit/time \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format -i
```

- [ ] **Step 2: 构建 + 全量测试**（同 Task 4 Step 3）

- [ ] **Step 3: Commit**

```bash
git add cxxkit/memory cxxkit/units cxxkit/time
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): batch F3 — memory/units/time

Build 0 errors, ctest 63/63."
```

---

### Task 7: 批次 F4 —— thread + kernel（宏密集 + 最大文件 signals.hpp 181 处挂括号）

**Files:**
- Modify: `cxxkit/thread/**`、`cxxkit/kernel/**`

- [ ] **Step 1: 格式化**

```bash
find cxxkit/thread cxxkit/kernel \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format -i
```

- [ ] **Step 2: 抽查信号宏存活**

```bash
grep -c "CXXKIT_DEFINE" cxxkit/kernel/signals.hpp
```

Expected: 与 format 前 `grep -c` 数字一致（format 不删宏，只动位置）

- [ ] **Step 3: 构建 + 全量测试**（同 Task 4 Step 3；thread/kernel 时序套件如偶发 fail，单跑重验后放行——非回归判据见 AGENTS.md NOTES）

- [ ] **Step 4: Commit**

```bash
git add cxxkit/thread cxxkit/kernel
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): batch F4 — thread/kernel

signals.hpp (largest file, 181 hanging braces) flips to Allman. Build 0
errors, ctest 63/63."
```

---

### Task 8: 批次 F5 —— network + media + crash + tests + examples（收尾）

**Files:**
- Modify: `cxxkit/network/**`、`cxxkit/media/**`、`cxxkit/crash/**`、`tests/*.cpp`、`examples/**`

**Interfaces:**
- Consumes: 全部前置批次；video_types.hpp 已手工 Allman（c155111），本批归一其余

- [ ] **Step 1: 格式化**

```bash
find cxxkit/network cxxkit/media cxxkit/crash tests examples \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format -i
```

- [ ] **Step 2: 全库终检（终检以 clang-format dry-run 为准，grep 只是旁证）**

```bash
find cxxkit tests examples \( -name '*.hpp' -o -name '*.cpp' \) -print0 | xargs -0 pixi run clang-format --dry-run 2>&1 | grep -c warning:   # 期望 0
grep -rE ' +$' cxxkit/ tests/ examples/ --include="*.hpp" --include="*.cpp" | wc -l                  # 期望 0
cmake --build build --parallel 2>&1 | grep -c error                                                   # 期望 0
ctest --test-dir build --output-on-failure 2>&1 | tail -2                                             # 期望 63/63
```

- [ ] **Step 3: Commit**

```bash
git add cxxkit/network cxxkit/media cxxkit/crash tests examples
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): batch F5 — network/media/crash + tests + examples

Repo-wide Allman unification complete: hanging '{' 193 → 0, trailing-ws
→ 0. Build 0 errors, ctest 63/63."
```

---

### Task 9: 门禁硬化 + 记忆/文档收口

**Files:**
- Modify: `scripts/check.sh`（第 1 步：缺失时 WARN 而非静默；存在时保持硬门禁）
- Modify: `.agents/memorys/decisions.md`（新增 D27）
- Modify: `.agents/memorys/status.md`（补一段 2026-09-02 格式统一记录）
- Modify: `.agents/rules/common/coding-style.md`（命名段后补一句：大括号 Allman 由 clang-format 强制，非可选风格）
- Create: `.git-blame-ignore-revs`（列出 F1–F5 五个提交哈希，GitHub/Gitee 侧 blame 免疫）

**Interfaces:**
- Consumes: F1–F5 的提交哈希
- Produces: check.sh 格式门禁状态可见；后续 `git blame --ignore-rev-file .git-blame-ignore-revs` 不被格式噪声污染

- [ ] **Step 1: check.sh 第 1 步改造**

把现有：

```bash
if command -v clang-format >/dev/null; then
```

改为：

```bash
if command -v clang-format >/dev/null; then
    # 硬门禁（原有逻辑不动）
elif command -v pixi >/dev/null && pixi run clang-format --version >/dev/null 2>&1; then
    # pixi 供给路径
else
    echo "[WARN] clang-format not found — format gate SKIPPED (install via: bash scripts/pixi-init.sh)"
fi
```

（保持脚本既有结构，最小 diff；pixi 路径内用 `pixi run clang-format --dry-run --Werror` 复用同一检查。）

- [ ] **Step 2: 验证 check.sh 全跑**

```bash
bash scripts/check.sh 2>&1 | tail -8
```

Expected: 8 步全绿；第 1 步在有 clang-format 的环境为硬检查

- [ ] **Step 3: 写 D27 决策 + status.md 记录**

`decisions.md` 追加（heredoc 方式，edit-safety 规则）：

```markdown
## D27: 全库 clang-format 统一（Allman 落地）(2026-09-02)
- **决策**: `.clang-format` 的 BreakBeforeBraces: Allman 首次全库强制执行；
  风险文件（compiler.hpp 特性表 / macros.hpp 宏中枢）用 clang-format off 护栏
- **理由**: 双风格共存（193 挂行 vs 2840 独立行）不可持续；配置从未被
  执行等于没有配置
- **护栏**: 特性表禁改（AGENTS.md）、## 拼接宏防重排（PIT-20 先例）
- **blame**: .git-blame-ignore-revs 收录 F1–F5
```

status.md 用 `cat >> ... <<'EOF'` 追加批次记录（含 193/2840 数据与五批提交哈希）。

- [ ] **Step 4: 验证护栏文件未被动过**

```bash
git log --oneline -1 -- cxxkit/base/compiler.hpp
git diff HEAD~6..HEAD -- cxxkit/base/compiler.hpp | wc -l   # 期望仅护栏两行
```

- [ ] **Step 5: Commit**

```bash
git add scripts/check.sh .agents/memorys/decisions.md .agents/memorys/status.md .agents/rules/common/coding-style.md .git-blame-ignore-revs
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "style(format): enforce gate + record D27 + blame-ignore-revs

check.sh now WARNs (instead of silently skipping) when clang-format is
absent and uses pixi-run as fallback; D27 records the Allman unification
decision; .git-blame-ignore-revs lists F1–F5."
```

---

## Self-Review

1. **Spec 覆盖**：spec（D26 规范 v2）管命名/横幅/include——已在 α–δ 落地；本计划补的是 spec 未覆盖的**格式维度**（大括号/空格/换行），与 spec 无冲突，D27 记录补充决策 ✅
2. **占位符扫描**：无 TBD/“适当处理”；所有步骤有具体命令与期望输出 ✅
3. **类型/命令一致性**：`pixi run clang-format` 形态在 Task 1 定义、后续一致；护栏文件清单在 Task 2 按干跑结果收敛（计划显式写了判定标准，不猜） ✅
4. **风险处置闭环**：每批「失败 → checkout 该文件 → 护栏 → 重跑」路径明确，符合 C13/禁连环改 ✅
