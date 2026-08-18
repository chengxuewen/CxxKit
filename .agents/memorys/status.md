# cxxkit 项目状态

## 项目定位

cxxkit 是 OpenCTK（an open cpp toolkit）的成功重构版本 —— 精简、模块化。
跨平台 C++ 开发工具库，提供算法、数据结构、功能组件、脚本语言与实用框架。

## 技术栈

- 语言：C++（C++17/20/23），C
- 构建：CMake（3.15...3.31），`.cmake.conf` 承载编译配置
- 格式化：clang-format（`.clang-format`）
- 静态分析：clang-tidy / cppcheck
- 测试：ctest + sanitizers
- 许可：MIT（含少量第三方开源代码，商用需替换/移除）

## 当前阶段

- [ ] 骨架：CMakeLists.txt（30KB）与构建配置已就位
- [ ] 源码：仓库当前无 .cpp/.h 源文件（重构早期，源码待模块化接入）
- [ ] 配置：opencode 配置已从 MediaServo 移植并适配 C++ 技术栈

## 模块规划

（待定 —— 按 OpenCTK 模块拆分：算法 / 数据结构 / 功能组件 / 脚本语言 / 框架）

## 待办

1. 模块化接入 C++ 源码，建立 `src/` 或按模块目录结构
2. 补全 CMake 目标与测试（ctest）
3. 接入 CI（clang-format / clang-tidy / cmake build / ctest）
