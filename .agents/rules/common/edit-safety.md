# Code Edit Safety

> **Target audience**: AI agents editing cxxkit source code.
> **Violation of these rules causes token waste from repeated fix cycles.**

## Tool Selection

| Change Size | Tool | Reason |
|-------------|------|--------|
| Rewrite entire function/file | `write` | Guarantees brace balance, no stale lines |
| ≤20 line single-location edit | `edit` | Minimal diff, safe for small changes |
| Structural pattern replacement | `ast_grep_replace` | Syntax-aware, preserves matching |
| Complex multi-file refactor | Delegate to subagent | Isolated context, verify independently |

## Forbidden Patterns

| Anti-Pattern | Why |
|--------------|-----|
| `sed` for code modification | Quote escaping errors, regex silent failures |
| Multiple sequential `edit` calls without re-reading | Line numbers drift, stale hash IDs |
| Deleting a line by replacing with empty content and assuming brace count is still correct | May leave unbalanced braces |
| Appending `}` to "fix" an unclosed delimiter without counting braces first | Masks root cause, may create double-close |

## Verify Immediately

After EVERY code change (edit, write, or ast_grep_replace):

```
C++:     cmake --build build           (编译验证)
Format:  clang-format --dry-run --Werror <file>
JSON:    python3 -m json.tool <file>
Shell:   bash -n <script>
```

If verification fails, STOP. Do NOT apply another edit on top. Instead:
1. `git diff` to see what changed
2. If the change is wrong, `git checkout -- <file>` to revert
3. Re-apply the fix correctly

## Brace Safety Checklist

Before marking any multi-line edit complete, verify:
- [ ] Every `{` has a matching `}` at the same indent level
- [ ] Every `(` has a matching `)`
- [ ] Every `[` has a matching `]`
- [ ] No duplicate function definitions or closing braces
- [ ] `cmake --build build` passes

## When to Delegate

Delegate to a `deep` category subagent when:
- The change touches 3+ files
- The change requires understanding cross-module dependencies
- You've failed the same edit 2+ times

The subagent gets a clean context, reads the files fresh, and applies all changes atomically.

## Architectural Decision Gate (NON-NEGOTIABLE)

Before implementing ANY architectural change (protocol, data flow, API contract, module boundary):
- **ALWAYS ask the user first** using the `question` tool with explicit options
- **NEVER fall back to an alternative architecture** without user approval
- **NEVER silently switch** from the agreed architecture even if it seems "easier"
- **NEVER implement a workaround** that changes the system's design without explicit user consent

If the agreed approach fails, report the failure and ask: "方案 X 失败，原因是 Y。建议改用 Z，是否同意？"

## Test Execution Constraint (NON-NEGOTIABLE)

After claiming tests are written or features are working:
- **ALWAYS run the tests** against the live build. Writing test files without executing them is a violation.
- **ALWAYS report actual test output** — pass/fail counts, error messages. Never claim "tests pass" without evidence.
- If tests fail, fix them in the same turn. Do not defer to "later".

## Verification Honesty (NON-NEGOTIABLE)

- **NEVER claim a feature works based on a partial test.**
- **ALWAYS report exactly what was tested and what was NOT tested.**
- **NEVER present a component test as end-to-end proof.** Each layer must be verified independently.
- **If you cannot verify at the target layer, say so explicitly.** Do not imply success.

## User Confirmation Before Edit (NON-NEGOTIABLE)

- **NEVER start editing files without explicit user approval.** Describing a plan ≠ approval to execute.
- **When user asks 'what can be done' or 'is it possible to...', they are asking a question, not giving an instruction to edit.** Answer the question. Do NOT edit files.
- **Before editing, present the plan AND use the `question` tool to confirm.** Wait for affirmative response before touching files.
- **Silence / '继续' / timeout ≠ approval.** Only explicit 'yes' / 'do it' / '执行' counts.

## edit 工具使用纪律

### 多行替换后验证行唯一性

对多行替换后若替换内容含重复模式（相同行），必须 grep 验证唯一性：

```bash
grep -c "重复模式" <file>    # 期望 1；>1 = edit 重复插入
```

### 同区域连续 edit 前 grep 现状

对同一文件同一函数/区域做连续 edit 时，每次 edit 前先 `grep -c "<锚点行内容>" <file>` 确认唯一性。

### 批量替换脚本逐块写盘或前置验证

多块替换脚本（assert → replace → write 模式）**每块 replace 后立即写盘**，或**所有 assert 前置验证后再统一替换**；禁止"全部替换后末尾一次写盘"（任一 assert 失败 → 全盘丢失）。

### 大块 markdown 追加用 heredoc

对 `.agents/memorys/*.md` 等大块内容追加，优先用 `cat >> file <<'EOF'` heredoc，不用 edit 工具 JSON（长内容含引号/反引号易解析失败）。

### 批量 edit 遇 hash mismatch → 完整 re-read 再重试

批量 edit 报 "hash mismatch" 后，**先完整 re-read 目标文件再重试**；禁止直接用错误输出中部分 tags 拼接第二次调用。

### CMake 批量修改后立即 configure 验证

批量 sed/edit 多个 CMakeLists 后，**先 `cmake -S . -B build` 验证再继续**；configure 报错按顺序修第一个错误（后续多为级联）。本会话 3 次 configure 失败（INTERFACE_SOURCES / EXCLUDE pattern / GLOB 路径）均因批量修改未逐项验证，且错误链从第一个失败点级联扩散。

### 单行 replace + 多行 lines = 插入而非替换（高频教训）

edit 工具 `replace` 带 `pos`（单行）且 `lines` 为多行时，**只在目标行后插入，不会删除目标行**——连续使用会累积重复行（本会话 5+ 次：CMakeLists 选项块、examples 块、CxxKitConfig 模板、profiling CMakeLists、profiling.hpp 宏定义）。**正确姿势**：替换多行块用 `pos`+`end` 范围；插入用 `append`。每次 edit 后 `grep -c` 验证无重复。

### 大面积模板替换禁全局正则——用括号深度匹配器（PIT-20 教训）

正则（`[^<>]*` 等）无法处理嵌套尖括号与多模板参数（如 `std::enable_if_t<is_callable_v<A,B>, Connection>`），全局替换会拆错、丢 `::type`、制造语法破坏（signals.hpp 大改教训）。**正确姿势**：
1. 用**括号深度匹配器**（扫描 `<`/`>` 计数对齐闭合）逐段替换结构性模板
2. **每一步替换后立即编译验证**（grep error），不积累未验证变更
3. 替换含逗号/嵌套实参的模板用显式逐块 `replace`（如 python 的 `s.replace(old, new)` 精确串），非全局 regex
验证：`cmake --build build --target <affected>` 无 error。

### 严格 C++11 门禁——CI 强制无 C++14 语法混入库代码（PIT-22 教训）

base 曾 `target_compile_features(cxxkit_base INTERFACE cxx_std_14)` 传播给全库，掩盖了分散各文件的 C++14 依赖。**正确姿势**：库 target 统一 `cxx_std_11`（helper 已设），CI 用脚本检查 C++14 语法残留：
```bash
# check.sh 应含（无输出=通过）：
grep -rnE "std::[a-z_]+_t<|if constexpr|\[\][^)]*\(auto|0b[01]'[, ]" cxxkit/ --include="*.hpp" --include="*.cpp" || true
```
（`_v` 变量模板、`_t` 别名、泛型 lambda、数字分隔符全禁）
