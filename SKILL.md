# Skills

本项目通过 **superpowers 插件**、**OMO 编排体系** 和 **项目专属技能** 三层提供 AI 辅助能力。

## 关系说明

- **Rules** (`.agents/rules/`) — 定义标准、约定和检查清单，告诉 AI *做什么*
- **Skills** (`.agents/skills/`) — 提供深入、可操作的参考材料，告诉 AI *怎么做*
- **Memorys** (`.agents/memorys/`) — 项目状态、决策记录、坑位档案，告诉 AI *项目走到哪了*

Rules 中通过 `See skill: <name>` 引用 Skills，形成 "规则约束 → 技能实现" 的层级。

> 📖 完整工作流见 [AGENTS.md](./AGENTS.md)。

## Superpowers 技能

通过 `.opencode/opencode.json` 加载，来自 `superpowers@git+https://github.com/obra/superpowers.git`。

| 技能 | 用途 | 说明 |
|------|------|------|
| `brainstorming` | 需求探讨与设计方案 | 交互式讨论 → 设计文档 |
| `writing-plans` | 编写实施计划 | 设计确认后生成分步计划 |
| `executing-plans` | 执行实施计划 | 任务逐条执行 + 门禁验证 |
| `subagent-driven-development` | 子代理驱动开发 | 每任务独立 subagent |
| `test-driven-development` | TDD 工作流 | 先红后绿 |
| `systematic-debugging` | 系统性调试 | 假设驱动排查 |
| `verification-before-completion` | 完成前验证 | 证据先于断言 |
| `requesting-code-review` / `receiving-code-review` | 代码审查 | 提交前/接收反馈 |
| `finishing-a-development-branch` | 分支收尾 | merge/PR 决策 |
| `using-git-worktrees` | Git Worktree 隔离 | 大规模改动隔离 |
| `dispatching-parallel-agents` | 并行代理调度 | 独立任务并行 |
| `writing-skills` | 编写新技能 | — |

## Ponytail 技能

通过 `@dietrichgebert/ponytail` 加载 — 强制最简实现（YAGNI、stdlib 优先、最短 diff）。

| 技能 | 用途 |
|------|------|
| `ponytail` | 编码时最简方案（lazy mode） |
| `ponytail-review` | 仅查过度工程的代码审查 |
| `ponytail-audit` | 全仓库过度工程审计 |
| `ponytail-debt` | `ponytail:` 注释债务台账 |

## 项目专属技能

位于 `.agents/skills/`，覆盖 cxxkit 开发的元约束、审计与回顾流程：

| 技能 | 文件 | 内容 |
|------|------|------|
| `think-before-act` | `think-before-act/SKILL.md` | **元约束**：先调研→列方案→用户审批→执行，任何非平凡操作前触发 |
| `skill-router` | `skill-router/SKILL.md` | 分析用户意图，推荐最合适的技能组合与执行顺序 |
| `ecosystem-scan` | `ecosystem-scan/SKILL.md` | 审计 .agents/ 体系并扫描社区生态寻找可引入的技能/规则/MCP，双层（Quick/Full）+ 质量评分 + 安全门禁 |
| `lesson-review` | `lesson-review/SKILL.md` | 批量会话回顾：系统性提取经验教训，写入项目记忆 |
| `doc-audit` | `doc-audit/SKILL.md` | 文档架构审计：并行检查架构/设计/决策/参考之间的自洽性、完整性和缺口，交互式确认每项发现 |
| `book-to-skill` | `book-to-skill/SKILL.md` | 将书籍/文档转换为 agent 技能，提取框架、原则、技术、反模式 |

## 使用方式

技能由 AI 代理根据任务上下文自动激活。无需手动调用。
