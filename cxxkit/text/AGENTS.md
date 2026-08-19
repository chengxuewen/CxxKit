# cxxkit/text AGENTS.md

## OVERVIEW

文本子库：字符串操作、格式化、编码转换、ASCII 工具。依赖 string-view-lite + fmt（vendored）。

## WHERE TO LOOK

| 任务 | 文件 | 备注 |
|------|------|------|
| 字符串基础操作 | `string.hpp` / `string.cpp` | 自定义 String 类 |
| 轻量字符串视图 | `string_view.hpp` | nonstd string_view_lite 包装 |
| 字符串拼接/拆分 | `string_utils.hpp` / `.cpp` | split/join/trim 等 |
| 流式拼接 | `string_builder.hpp` / `.cpp` | 链式 append |
| 格式化输出 | `format.hpp` | fmt 包装 + utils::fmt 别名 |
| 编解码 | `string_encode.hpp` / `.cpp` | URL/HTML 等 |
| 字符串转数字 | `string_to_number.hpp` | 纯头文件 |
| ASCII 判定/转换 | `ascii.hpp` / `ascii.cpp` | **最大文件**（785 行），行为契约严格 |
| Base64 编解码 | `base64.hpp` / `.cpp` | |
| 位缓冲区 | `bit_buffer.hpp` / `.cpp` | 位级读写 |
| 构建入口 | `cxxkit/text/CMakeLists.txt` | TEXT_SOURCES 汇总 7 cpp |

## CONVENTIONS

- **StringView ≠ std::string_view**：`cxxkit/text/string_view.hpp` 是 nonstd::string_view_lite 包装。用它，不要直接用 `std::string_view`（C++11 兼容）。
- **utils::fmt 别名**：`format.hpp` 提供 `namespace utils { namespace fmt = ::fmt; }`。库内格式化统一走 `utils::fmt::format()`，不直接 `::fmt::format()`。
- **ASCII 0x 契约**：`ascii.hpp` 的十六进制输入**不带 "0x" 前缀**。传 `"0xff"` 行为未定义。这是设计契约，不是 bug，**不要修**。
- **ascii.cpp 是热点**：785 行实现，改动前先通读完整文件。查表法逻辑密集，改一处可能波及多处。
- **string_view 头部守卫**：`string_view.hpp` 用 `#ifndef _CXXKIT_STRING_VIEW_HPP`（旧风格），不统一，新文件照 `#pragma once`。

## ANTI-PATTERNS

- **禁用 std::string_view**：库代码一律用 `cxxkit/text/string_view.hpp` 的 nonstd 视图。C++11 下 std::string_view 不存在。
- **禁改 ascii 十六进制行为**：`grep -rn "0x" cxxkit/text/ascii.hpp` 检查——输入语义不含 "0x" 前缀是核心契约。
- **禁直接 #include `<fmt/...>`**：三方头走 `<cxxkit/3rdparty/fmt/...>` 路径（D6）。
- **禁在 text 子库引入 C++14 语法**：`string_to_number.hpp` 等纯头文件被整个项目消费，必须 C++11 兼容。
