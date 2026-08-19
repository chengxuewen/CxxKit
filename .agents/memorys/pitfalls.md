# CxxKit 踩坑记录

> 问题 + 原因 + 解法 + 验证 + 禁止。格式：`## PIT-{n}: 标题 (日期)`。


## PIT-1: 三方头路径不得擅自简化 (2026-08-18)
- **症状**: 实现 Phase 1 时把三方 include 简化为原始路径（`fmt/...`、`tl/...`），与设计文档 §5.4 不一致
- **根因**: 实现时未对照设计文档验证三方头命名空间路径
- **解法**: 恢复为 `<cxxkit/3rdparty/<lib>/...>`（D6 决策），构建/安装双侧一致
- **验证**: `grep -rn '#include <fmt/' cxxkit/` 应为空；`grep -rn '#include <cxxkit/3rdparty/' cxxkit/` 应有结果
- **禁止**: 设计文档明确写过的机制，实现时不得擅自简化（AGENTS.md ANTI-PATTERNS 已引用）

## PIT-2: INTERFACE 库 target_sources 不能加源目录内头 (2026-08-19)
- **症状**: `target_sources(cxxkit_base INTERFACE $<BUILD_INTERFACE:${_cxxkit_headers}>)` 报 "INTERFACE_SOURCES property contains path ... prefixed in the source directory"；绝对路径版报 "Cannot find source file"
- **根因**: CMake 禁止 INTERFACE_SOURCES 引用源目录内路径（该属性面向安装消费者）
- **解法**: header-only 库用 `add_library(xxx INTERFACE ${headers})` 直接带源参数（仅供 IDE 显示，不编译）
- **验证**: `cmake -S . -B build` 通过；`build/.cmake/api/v1/reply/codemodel-v2-*.json` 对应 target jsonFile 的 sources 含 .hpp

## PIT-3: install(DIRECTORY) EXCLUDE 不匹配顶层子目录 (2026-08-19)
- **症状**: `PATTERN "*/detail/*" EXCLUDE` 后 detail/*.hpp 仍被安装
- **根因**: pattern 转 REGEX 为 `/[^/]*\/detail\/[^/]*$`，要求路径以 /detail/ 开头；但 install 相对路径是 `detail/xxx`（无前导斜杠）
- **解法**: 顶层 detail 用 `PATTERN "detail" EXCLUDE`（连目录一起排除）
- **⚠ 2026-08-19 已反转**: 用户决策 detail/ 随公共头安装（D9），EXCLUDE 已移除；PIT-3 保留作 pattern 匹配机制参考
- **验证**: `find build/install/include -type d -name detail` 应为 4（kernel/thread/tools/network）
- **验证**: `find build/install/include -type d -name detail` 应为空

## PIT-4: file(GLOB) 相对路径在 target 源列表被拒 (2026-08-19)
- **症状**: GLOB 相对路径（`*.hpp`）用于 INTERFACE_SOURCES 报 prefixed；用于 add_library 报找不到文件
- **根因**: file(GLOB) 相对当前目录展开，CMake 要求 target 源路径可解析
- **解法**: GLOB 必须 `${CMAKE_CURRENT_SOURCE_DIR}` 前缀（返回绝对路径），加 CONFIGURE_DEPENDS 保证新头自动进大纲
- **验证**: configure 通过 + codemodel 含头

## PIT-5: text ↔ tools 循环依赖（底层子库误用上层编译符号）(2026-08-19)
- **症状**: 链接报 `undefined reference to cxxkit_assert`（text 的 .cpp 用 CXXKIT_ASSERT 宏，符号在 libcxxkit_tools.a）
- **根因**: tools 依赖 text（公共头 include），text 又用 tools 的编译符号 → 依赖环；AGENTS.md 依赖方向 text 在 tools 之下
- **解法**: 底层子库廉价参数校验用 `<cassert>` 的 assert()（tools/AGENTS.md 断言选型本就推荐）；memory 保留 fatal 语义（CXXKIT_ASSERT_X）并显式链接 tools（无环：tools 不依赖 memory）
- **验证**: `cmake --build build` 链接通过；`grep -c CXXKIT_ASSERT cxxkit/text/*.cpp` 应为 0

## PIT-6: 违反 C6 执行 rm -rf build (2026-08-19)
- **症状**: 为"干净验证"执行 `rm -rf build`，用户立即中止——3rdparty stamp 缓存全清导致 5+ 分钟三方库重建
- **根因**: AI 把"验证干净配置"优先于项目硬禁令 C6；rm -rf build 无豁免场景
- **解法**: 只清 CMake 生成物 `rm -rf build/CMakeCache.txt build/CMakeFiles build/Testing`；三方库需重建时才删对应 `build/3rdparty/<lib>-<buildtype>`
- **验证**: `ls build/3rdparty/ | wc -l` 应为 21（stamp 缓存未清）
- **禁止**: 任何"图省事/求干净"的 rm -rf build（C6 硬禁令，用户已明确纠正）
