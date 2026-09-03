# abseil/webrtc 第二批移植实施计划（P1：9 件）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 从 `.refinfo`（abseil 20220623.2 + libwebrtc rtc_base）移植 9 件功能点到 CxxKit，每件 C++11 降级 + cxxkit 命名规范 + 配套 gtest。

**Architecture:** 按落点子库分 4 个提交批次（numerics×5 件统计/序列号族 → numerics byte_order → containers fixed_array → text str_split+crc32）。每件 TDD（先写测试→FAIL→移植→PASS→暂存）。fixed_array 因 abseil 依赖链重，采用**简化重实现**而非逐行移植（对齐 08-25 flat_hash_map 先例）。byte_order 弃 webrtc 平台宏版、直接 C++11 干净实现。crc32 沿用 rtc_base 的表驱动结构。

**Tech Stack:** C++11 库代码 / vendored gtest 1.12.1 / CMake（cxxkit_add_test helper）/ clang-format 23.1.0（pixi）

**Spec:** 交互式裁定记录（2026-09-02 会话）——9 件清单 + Cleanup 不移植（scope_guard 逐字节等价：invoke/cancel/工厂/nodiscard 全同构）+ node_hash/btree/Cord/int128/absl::hash/flags 不做。源码位置 `.refinfo/`。

**审核状态:** Momus 审核（bg_b5c3a2f9）结论 APPROVE-WITH-FIXES，F1–F5 已全部修订进本版。

## 消费方依据（为何是这 9 件）

| 件 | 消费信号 |
|---|---|
| running_statistics / percentile_filter / exp_filter | media 媒体质量统计刚需 |
| sequence_number_util + unwrapper | OpenCTK media `rtp_headers.hpp` 已有 transportSequenceNumber 字段（真实下游） |
| byte_order | network 子库现成消费方 |
| fixed_array | inlined_vector 的定长语义兄弟，webrtc 全家使用 |
| str_split | 现有 `string_split(source, char)` 仅单字符，by_any_of+skip_empty 是真实增量 |
| crc32 | 表驱动校验通用件 |

## Global Constraints

- **C4/C16**：库代码 C++11（禁 `_t` 别名模板/`if constexpr`/泛型 lambda/数字分隔符）；`std::multiset` C++11 直接用
- **类型名实况（Momus F1 实证）**：智能指针包装是 `cxxkit::Optional<T>`（Pascal 别名，`tools/optional.hpp` L40），**不是** `cxxkit::optional`；nullopt 在 `cxxkit::utils::nullopt`（L48）；字符串视图是 `cxxkit::StringView`（`text/string_view.hpp` L39），**不是** `string_view`——三个名字全文照此拼写
- **C16⑥**：全部公开标识符迁 snake_case（`AddSample`→`add_sample`、`GetPercentileValue`→`get_percentile_value`）；**构造器名 = 类名**（PascalCase，Momus F1b）；POD 聚合字段纯 snake；成员 mPascalCase；类型别名按 std-镜像例外仅限容器/trait——`rtp_timestamp_unwrapper` 域别名保持 snake（用户裁定对齐 flat_hash_map 先例）
- **头结构**：`#pragma once` + 箱式许可横幅（2026 模板）+ doxygen `@file/@brief` + `CXXKIT_BEGIN_NAMESPACE`
- **测试**：vendored gtest，注册走 `cxxkit_add_test(cxxkit_tst_<name> SOURCES tst_<name>.cpp LIBRARIES ...)` 四段式（照 tests/CMakeLists.txt 现有块）；测试文件 `tests/tst_<name>.cpp`
- **每件验证**：`cmake --build build --parallel` 0 error + `ctest --test-dir build --output-on-failure` 全绿 + clang-format dry-run（`find -print0 | xargs -0 ... 2>&1` 形态）0 违规
- **提交**：`git -c user.name=chengxuewen -c user.email=1398831004@qq.com`；本轮约定：**执行后不自动提交**，攒批请示
- **禁止**：头文件直接 `#include <optional>`（用 `cxxkit/tools/optional.hpp`）；引入 abseil 头；`RTC_*`/`absl::` 残留

---

### Task 1: running_statistics（`numerics/running_statistics.hpp`）

**Files:**
- Create: `cxxkit/numerics/running_statistics.hpp`（header-only，~180 行）
- Create: `tests/tst_running_statistics.cpp`
- Modify: `tests/CMakeLists.txt`（注册 `cxxkit_tst_running_statistics`）

**Interfaces:**
- Consumes: `cxxkit::Optional`（tools/optional.hpp）、`cxxkit::utils::nullopt`
- Produces: `template <typename T> class RunningStatistics`
  - `void add_sample(T)` / `void remove_sample(T)` / `void merge_statistics(const RunningStatistics&)`
  - `int64_t size() const`
  - `Optional<T> get_min/get_max() const`；`Optional<double> get_sum/get_mean/get_variance/get_standard_deviation() const`
  - `namespace detail { infinity_or_max<T>() / minus_infinity_or_min<T>() }`（自 math_utils.h 提取的 2 个 constexpr SFINAE 模板）
  - 成员 `mSize/mMin/mMax/mMean/mCumul/mSum`
- 源：`.refinfo/libwebrtc/rtc_base/numerics/running_statistics.h`（171 行，Welford 单遍算法）+ math_utils.h 两函数

- [ ] **Step 1: 写失败测试** `tests/tst_running_statistics.cpp`：

```cpp
#include <cxxkit/numerics/running_statistics.hpp>
#include <gtest/gtest.h>
#include <cmath>

TEST(RunningStatisticsTest, EmptyReturnsNullopt)
{
    cxxkit::RunningStatistics<int> stats;
    EXPECT_FALSE(stats.get_min().has_value());
    EXPECT_FALSE(stats.get_mean().has_value());
    EXPECT_EQ(0, stats.size());
}

TEST(RunningStatisticsTest, MeanVarianceIncremental)
{
    cxxkit::RunningStatistics<double> stats;
    const double data[] = {1.0, 2.0, 3.0, 4.0};
    for (double v : data)
        stats.add_sample(v);
    EXPECT_NEAR(2.5, *stats.get_mean(), 1e-9);
    EXPECT_NEAR(1.25, *stats.get_variance(), 1e-9); // 总体方差
}
```

完整测试集：空态 nullopt × 6 getter、增量 vs 直接公式对拍、remove_sample 恢复性、
merge_statistics 两批合并 == 合并批直接统计、int/double 双型实例化。

- [ ] **Step 2: 跑测试确认 FAIL**（头不存在 → 编译失败即 RED）
- [ ] **Step 3: 移植实现**：源 1:1 逻辑；命名 `add_sample/remove_sample/merge_statistics/size/get_min/get_max/get_sum/get_mean/get_variance/get_standard_deviation`；`std::optional/std::nullopt` → `cxxkit::Optional/cxxkit::utils::nullopt`；`RTC_DCHECK_GT` → `CXXKIT_DCHECK_GT`；math_utils 两 constexpr 进 `detail`
- [ ] **Step 4: PASS + 构建 0 error + clang-format 干净**
- [ ] **Step 5: 暂存不提交**

---

### Task 2: sequence_number_util + unwrapper（`numerics/sequence_number_util.hpp` + `sequence_number_unwrapper.hpp`）

**Files:**
- Create: `cxxkit/numerics/sequence_number_util.hpp`（~90 行 header-only）
- Create: `cxxkit/numerics/sequence_number_unwrapper.hpp`（~85 行 header-only）
- Create: `tests/tst_sequence_number.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces:
  - `template <typename T, T M = 0> T forward_diff(T prev, T cur)`（回绕前向差）
  - `template <typename T, T M = 0> T reverse_diff(T prev, T cur)`
  - `template <typename T, T M = 0> bool ahead_of(T a, T b)` / `bool ahead_or_at(T a, T b)`
  - `template <typename T, T M = 0> T min_diff(T a, T b)`
  - `template <typename T, T M = 0> class SeqNumUnwrapper`：`int64_t unwrap(T)` / `int64_t peek_unwrap(T) const` / `void reset()`
  - snake 别名：`rtp_timestamp_unwrapper = SeqNumUnwrapper<uint32_t>`、`rtp_sequence_number_unwrapper = SeqNumUnwrapper<uint16_t>`（域别名 snake，用户裁定）
  - 内部 `detail::unwrapper_delta`（源 Delta 静态函数）
- 源：`sequence_number_util.h`（84 行，AheadOrAt 的 M 奇偶分支保留）+ `sequence_number_unwrapper.h`（81 行，PeekUnwrap 保留）

- [ ] **Step 1: 失败测试**：uint8 小模数全遍历对拍（`forward_diff(a,b)+forward_diff(b,a)==M`）、ahead_of 回绕边界（254→255→0→1）、unwrapper 连续 unwrap 跨回绕单调递增、peek 不改状态、reset 后重新锚定、uint16/uint32 双实例化
- [ ] **Step 2: RED 确认**
- [ ] **Step 3: 移植**：`std::optional` 成员 → `cxxkit::Optional`
- [ ] **Step 4: PASS + 全绿**
- [ ] **Step 5: 暂存**

---

### Task 3: percentile_filter（`numerics/percentile_filter.hpp`）

**Files:**
- Create: `cxxkit/numerics/percentile_filter.hpp`（~130 行 header-only）
- Create: `tests/tst_percentile_filter.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `template <typename T> class PercentileFilter`
  - `explicit PercentileFilter(float percentile)`（0.0-1.0，构造 `CXXKIT_DCHECK` 域检查）
  - `void insert(const T&)` / `bool erase(const T&)` / `T get_percentile_value() const` / `void reset()`
  - 成员 `mPercentile/mSet/mPercentileIt/mPercentileIndex`（`std::multiset` C++11 直接用）
- 源：`percentile_filter.h`（124 行）

- [ ] **Step 1: 失败测试**：单元素 P0.0/P0.5/P1.0 精确值、递增序列 P0.5 中位数、erase 存在/不存在返回值、边界（0.0=min/1.0=max）、reset 语义
- [ ] **Step 2: RED**
- [ ] **Step 3: 移植**：erase 返回 bool（存在与否）保留；空集行为按源（DCHECK）
- [ ] **Step 4: PASS + 全绿**
- [ ] **Step 5: 暂存**

---

### Task 4: exp_filter（`numerics/exp_filter.hpp` + `.cpp`）+ numerics STATIC 化

**Files:**
- Create: `cxxkit/numerics/exp_filter.hpp`（~50 行）
- Create: `cxxkit/numerics/exp_filter.cpp`（~45 行）
- Create: `cxxkit/numerics/numerics_global.hpp`（编译子库导出宏，C10）
- Modify: `cxxkit/numerics/CMakeLists.txt`（INTERFACE → STATIC）
- Create: `tests/tst_exp_filter.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `class CXXKIT_NUMERICS_API ExpFilter`
  - `explicit ExpFilter(float alpha, float max = kValueUndefined)`
  - `void reset(float alpha)` / `float apply(float exp, float sample)` / `float filtered() const` / `void update_base(float alpha)`
  - `static constexpr float kValueUndefined = -1.0f;`（源为 .cc 内 `static const float` 定义，constexpr 化是 C++11 合法改进）
  - 成员 `mAlpha/mFiltered/mMax`（注意源 `max_` 是 const 成员 → 拷贝赋值删除，保持）
- 源：`exp_filter.h`（48 行）+ `exp_filter.cc`（43 行）

- [ ] **Step 1: 失败测试**：首次 apply 直通（filtered==sample）、exp=1/3 步进收敛、alpha=1 直通不记忆、max 上限截断、reset 清态、update_base 动态改 alpha
- [ ] **Step 2: RED 确认**
- [ ] **Step 3: 移植实现**
- [ ] **Step 4: numerics STATIC 化 CMake 五步（Momus F4）**：
  1. `cxxkit_add_library(cxxkit_numerics INTERFACE ...)` → 显式 `STATIC` + SOURCES 列 exp_filter.cpp（C10：现有编译子库显式 STATIC 保现状，不跟 CXXKIT_BUILD_SHARED_LIBS）
  2. `target_link_libraries` 的 `INTERFACE` 关键字 → `PUBLIC`；**补链 `WrapOptional`**（running_statistics 的 optional.hpp include 路径来自该 wrap target——参照 `cxxkit/tools/CMakeLists.txt` L29-35 先例含 `cxxkit_install_public_wrap_headers`）
  3. 新建 `numerics_global.hpp`（`CXXKIT_NUMERICS_API` 导出宏；helper 自动注入 `CXXKIT_BUILDING_NUMERICS_LIB`）
  4. exp_filter 类加 `CXXKIT_NUMERICS_API` 标注
  5. 重跑 configure 核对 `cxxkit_generate_pkg_config` 输出（STATIC 后 .pc 需含 `-lcxxkit-numerics`）
- [ ] **Step 5: PASS + 全量重构建 + 全测 + build-shared 重验（共享构建暴露静态掩盖的缺链，C10）**
- [ ] **Step 6: 暂存**

---

### Task 5: byte_order（`numerics/byte_order.hpp`）

**Files:**
- Create: `cxxkit/numerics/byte_order.hpp`（~220 行 header-only）
- Create: `tests/tst_byte_order.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces:
  - 模板/内联函数族 `load_be16/32/64(const void*)`、`load_le16/32/64`、`store_be16/32/64(void*, v)`、`store_le16/32/64`
  - `set8/get8(void*, offset, v)` 单字节存取
  - **不导出** webrtc 的 htobe16/htonl 宏族（平台分支地狱全砍）；命名裁定：`SetBE16/GetBE16` → `store_be16/load_be16`（load/store 内存语义比 set/get 精确，用户裁定）
- 源：`rtc_base/byte_order.h`（212 行）——**仅作语义参照，直接写 C++11 干净实现**（移位组合，不逐行移植，不依赖 htons/htonl）

- [ ] **Step 1: 失败测试**：已知字节序列 {0x12,0x34,0x56,0x78} 的 BE/LE load 对拍固定值、store/load 往返、set8/get8、跨偏移读写、64 位大值
- [ ] **Step 2: RED**
- [ ] **Step 3: 实现**
- [ ] **Step 4: PASS + 全绿**
- [ ] **Step 5: 暂存**

---

### Task 6: fixed_array（`containers/fixed_array.hpp`）

**Files:**
- Create: `cxxkit/containers/fixed_array.hpp`（~350 行 header-only，简化重实现）
- Create: `tests/tst_fixed_array.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `template <typename T> class FixedArray`
  - `explicit FixedArray(size_type n)` / `FixedArray(size_type n, const T& value)` / `FixedArray(std::initializer_list<T>)`
  - `T& operator[](size_type)` / `const T&` 版 / `size() / empty() / data() / fill()`
  - 迭代器 `begin/end/cbegin/cend`（`T*` 裸指针）
  - 拷贝/移动语义完整（placement-new 手写，参照 `cxxkit/containers/vector.hpp` 先例）
  - **不做**：自定义 allocator、EBO compression、非常量容量增长
  - **裁定**：n=0 合法（对齐 abseil——`empty()` 可用；**不设** CXXKIT_CHECK(n>0)，webrtc 调用方 `FixedArray<T>(n)` 的 n==0 场景不炸）。单 allocation（头 + n 元素一次分配）
- 源：abseil `fixed_array.h`（529 行）——API 参照；**禁复制 abseil 体**（依赖链过重，简化重实现）

- [ ] **Step 1: 失败测试**：构造/析构计数、fill、operator[] 边界、initializer_list、移动语义（指针转移 + 源空态）、const 正确性、range-for、n=0 空数组、n=1 最小值
- [ ] **Step 2: RED**
- [ ] **Step 3: 实现**
- [ ] **Step 4: PASS + 全绿**
- [ ] **Step 5: 暂存**

---

### Task 7: str_split 精简版（`text/str_split.hpp`）

**Files:**
- Create: `cxxkit/text/str_split.hpp`（~250 行 header-only）
- Create: `tests/tst_str_split.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces:
  - `std::vector<cxxkit::StringView> str_split(StringView text, char delim)`（快速路径重载）
  - `class by_any_of`（abseil 原名 ByAnyChar，snake 化；256 项 bool 表驱动；`find(text, pos)` 返回 token 边界）
  - `class by_string`（多字符分隔符；内部 `StringView::find`）
  - 空结构体 predicate：`skip_empty / allow_empty`（标签分发重载决议，零运行时开销，不用 std::function）
  - 泛型入口 `template <typename Delim, typename Pred> std::vector<StringView> str_split(text, delim, pred)`
  - **非 owning**：doxygen `@warning` 明示结果元素引用源缓冲区，源亡则悬垂
- 源：abseil `str_split.h`（547 行）——裁剪移植

- [ ] **Step 1: 失败测试**：单 char、by_any_of（"a,b;c d" 按 ",; " 分）、by_string（"--a--b--" → ["","a","b",""]）、skip_empty 过滤、allow_empty 保留、空串输入、全分隔符串、相邻分隔符
- [ ] **Step 2: RED**
- [ ] **Step 3: 实现**
- [ ] **Step 4: PASS + 全绿**
- [ ] **Step 5: 暂存**

---

### Task 8: crc32（`text/crc32.hpp` + `text/crc32.cpp`）

**Files:**（IEEE 802.3，多项式 0xEDB88320）
- Create: `cxxkit/text/crc32.hpp`（~40 行声明 + doxygen）
- Create: `cxxkit/text/crc32.cpp`（~60 行）
- Create: `tests/tst_crc32.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `uint32_t crc32(const void* data, size_t len, uint32_t crc = 0)`——**接口对齐 zlib 语义，内部自处理 init/final XOR**，返回值可直接对拍 zlib；可选 `StringView` 重载
- 256 项 static 表，函数内首次调用初始化（C++11 magic static 线程安全）
- 源：`rtc_base/crc32.cc`（50 行）——**事实修正（Momus F3）：源本就是表驱动**（LoadCrc32Table 函数内 static 表），沿用其结构，非逐位计算

- [ ] **Step 1: 写失败测试**："123456789" → 0xCBF43926（IEEE 802.3 标准检验向量）、空输入 → 0、增量计算（`crc32(data2, len2, crc32(data1, len1))` == 一次算）
- [ ] **Step 2: 跑测试确认 FAIL**
- [ ] **Step 3: 实现**：256 项表驱动（函数内 static，首调生成；表生成 `c=i; 8×(c&1 ? 0xEDB88320^(c>>1) : c>>1)`）
- [ ] **Step 4: PASS + 全绿**
- [ ] **Step 5: 暂存**

---

### Task 9: 收口批次（文档 + 覆盖率 + 请示提交）

**Files:**
- Modify: `tests/CMakeLists.txt`（8 个新测试注册齐全性核对）
- Modify: `AGENTS.md`（STRUCTURE 子库表 numerics header-only→compiled；WHERE TO LOOK/CODE MAP 如涉及）
- Modify: `README.md`（子库表 numerics 类型修正 + 内容列补新组件）
- Modify: `.agents/memorys/status.md`、`.agents/memorys/decisions.md`
- 无库代码改动

- [ ] **Step 1: 8 新套件 + 63 旧套件 = 71 套件全绿确认**
- [ ] **Step 2: 文档同步（Momus F5）**：AGENTS.md + README.md 子库表 + decisions.md **D28**（四项裁定：byte_order load/store 命名、fixed_array 简化重实现、str_split 非 owning、crc32 zlib 语义；Cleanup 不移植与 scope_guard 等价的证据链）+ status.md 段落
- [ ] **Step 3: 覆盖率抽查**（build-cov 存在则 `bash scripts/coverage.sh build-cov`，新文件应 ≥80%）
- [ ] **Step 4: P2 候选备忘写入 status 草稿**（moving_*/event_rate_counter/node_hash_* 需求触发）
- [ ] **Step 5: 9 件已暂存清单打印，请示提交拆分**

---

## Self-Review

1. **Spec 覆盖**：9 件全有 Task（cleanup 不移植已记 spec）；byte_order load/store 命名、fixed_array 简化重实现 + n=0 合法、str_split 非 owning 警告、crc32 zlib 语义——四项裁定落点明确 ✅
2. **占位符扫描**：初版起草噪声已全部清理（Momus F2 清单逐条销账：crtc32/惯存/涫存/死断言/畸形 checkbox/重复步骤/0xCBF43126 伪向量）✅
3. **类型一致性**：`cxxkit::Optional`/`cxxkit::utils::nullopt`/`cxxkit::StringView` 三个真名全文统一（F1）；构造器名 = 类名（F1b）；snake 命名链一致（forward_diff/ahead_of/store_be16）✅
4. **事实核对（F3）**：crc32 源本就表驱动（计划已修正表述）；abseil 分隔器原名 ByAnyChar（已注明 snake 化）；PeekUnwrap/kValueUndefined 等 API 均与源实证一致 ✅
5. **CMake/文档闭环（F4/F5）**：numerics STATIC 五步（含 numerics_global.hpp 导出宏 + WrapOptional 链 + pkg-config 核对 + build-shared 重验）+ Task 9 文档三件套 + D28 ✅
6. **依赖排序**：Task 1-3 header-only 无依赖；Task 4 STATIC 化不影响 1-3 头文件；5-8 相互独立；Task 9 收口 ✅
