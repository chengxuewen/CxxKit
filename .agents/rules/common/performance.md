# Performance Optimization

## Context Window Management

Avoid last 20% of context window for:
- Large-scale refactoring
- Feature implementation spanning multiple files
- Debugging complex interactions

Lower context sensitivity tasks:
- Single-file edits
- Independent utility creation
- Documentation updates
- Simple bug fixes

## Model Tier Selection

按任务复杂度选择模型层级（详见 `.opencode/agent-model-tiers.md`）：
- 架构设计 / 复杂推理 → `premium-max`
- 日常编码 / 规划 / 审查 → `premium`
- 搜索 / 探索 / 简单改动 → `fast`
- 多模态 / 图片分析 → `vision`

## Build Troubleshooting

If build fails:
1. Use **build-error-resolver** agent
2. Analyze error messages
3. Fix incrementally
4. Verify after each fix

## 可执行检查

```bash
# 验证：无大文件（>800 行）
find src/ \( -name "*.cpp" -o -name "*.h" \) -exec wc -l {} + | awk '$1>800{print}'
```
