# Development Constraints

## 平台约束

- 跨平台：macOS / Linux（Windows 非首要目标，按需验证）
- 编译器：Clang / GCC，遵循 C++17 起步（C++20/23 按模块选择）
- 构建：仅 CMake（3.15...3.31），配置集中在 `.cmake.conf`

## Git Commit Rules

### 提交前检查

- [ ] `clang-format --dry-run --Werror <changed-files>` 通过
- [ ] `cmake --build build` 构建通过
- [ ] `ctest --test-dir build --output-on-failure` 测试通过
- [ ] 无硬编码密钥/端口/路径（用配置或常量）
- [ ] `git status` 干净，无无关文件混入
