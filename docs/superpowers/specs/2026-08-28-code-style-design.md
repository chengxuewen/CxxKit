# CxxKit 代码风格统一设计（2026-08-28 · v2 修订版）

> **决策路径**: 团队对抗分析（hyperplan 三轮）→ 头脑风暴逐项裁定（9 问）→ v1 spec → 4 维度团队审核（correctness/completeness/feasibility/coherence）→ 本 v2。
> **目标状态**: 用户裁定 **A — 全库统一，无豁免层**（std-镜像类名/trait structs/POD 聚合字段为制度化正条目，枚举 4 离群族保留豁免，见 §5）。
> **v2 变更摘要**: 修正 3 事实错误（类名一致性/迁移量/signals 谱系）、补 POD-struct 规则行、补 make_ 工厂规则、β 改依赖序原子符号批、门禁正则换 -P 版、C16⑥ 移植改名条款、枚举重复/静态成员/模板 Args 规则补行。

## 1. 命名规范矩阵（唯一标准）

| 实体 | 风格 | 示例 | 说明 |
|------|------|------|------|
| 类/类型（业务类）| PascalCase | `TaskQueueThread`, `I420Buffer` | **例外**：std-镜像容器类保持小写（`flat_hash_map`, `flat_hash_set`——镜像 std API 形状）；trait/metafunction structs 保持 snake（`is_pointer`, `is_weak_ptr` 等仿 std type_traits 惯例）|
| **成员变量（封装类实例成员）** | **mPascalCase** | `mSize`, `mRefCount` | 仅限 class/含私有态 struct |
| **成员变量（POD/聚合 struct 公共字段）** | **snake_case 无前后缀** | `HdrMetadata::luminance_max`, `http::Parameter::name` | 实测：hdr_metadata/http/numerics 结果结构/thread_pool_p 全是纯 snake——制度化保留，聚合字段禁 m 前缀 |
| **静态数据成员** | kPascalCase | `SharedMemory::kInvalidHandle` | static ≠ 实例成员，不适用 m 前缀 |
| **函数（成员/自由）** | **snake_case** | `post_task()`, `to_string()` | 含虚函数族（§4β 原子批）|
| **局部/参数变量** | **snake_case** | `pending_task` | 随 β 顺带 |
| 枚举值 | kPascalCase | `kVideoRotation_0`, `kLowest` | **不强制**重复外层类型名（`kLowest` 优于 `kPriorityLowest`）；存量重复形式（`kVideoRotation_0`）本次不迁，followup |
| 常量 | kPascalCase | `kInvalidHandle` | 含 static constexpr（`LogLevelNum` → `kLogLevelNum` 顺带修正）|
| 模板参数 | 单大写或 std 缩写 | `T`, `P0`, `Args`, `Ts` | `Args` 20+ 处为 std 惯例，合规不迁 |
| 宏 | UPPER_SNAKE | `CXXKIT_CHECK` | 不动 |
| 命名空间 | 扁平小写 | `cxxkit::` / `detail::` / `utils::` | 不动（D2）；**新自由函数一律扁平 `cxxkit::`**，`utils::` 仅存量不扩散 |

## 2. 函数语义前缀规则

| 前缀形态 | 语义 | 示例 |
|---------|------|------|
| 无前缀名词 | 属性 getter（含单位取值）| `size()`, `us()`, `millis()`（getter 形态）|
| `is_/has_/should_/can_` | 谓词 | `is_empty()`, `has_value()`, `should_drop_frame()` |
| `set_` | 写入 | `set_size(10)` |
| `to_` | **拥有型转换**（已有值 → 新类型对象）| `to_string()`, `to_i420()` |
| `as_` | **非拥有视图** | `as_array_view()`, `as_bytes()` |
| **`make_`** | **工厂构造**（从散参数组合新建）| `make_status_or<T>()`, `make_i420_buffer(w, h)` |
| `create_`（静态成员工厂）| 类绑定构造语义 | `I420Buffer::create(w, h)` |
| `_out` 后缀 | 输出参数（白名单场景）| `copy_to(dst_out)` |

**边界裁定**：`I420Buffer::Copy(buf)` → `to_i420(buf)` 语义（输入是同族对象 = 转换/复制，非 make_）；`Millis(T value)` 静态工厂 → `millis(value)`，与未来 getter `millis()` 构成**预期内重载对**（webrtc `ms()` 同构），非命名冲突。

**out 参数白名单**：① 缓冲重用 `copy_to(dst&)`；② optional 产出 `bool try_parse(buf, T& out)`。其余多返回值用 `StatusOr<T>`/struct/pair。（prospective 规则，实测存量零违例）

**函数归属制**：静态工厂/构造变体/需内部访问 → 静态成员；只走公共接口 → 自由函数（扁平 `cxxkit::`）；可多态扩展 → 自由函数。

## 3. 依据摘要

- 2020+ 新立项 C++ 项目一致选 snake（std 委员会 format/ranges/print 零 Pascal 新增；Boost.JSON/urls/redis；SerenityOS）。**SerenityOS/Ladybird 是 PascalCase 类型 + snake_case 方法组合的大规模先例**（`String::is_empty()`, `Widget::set_visible()` 数千类）。
- m/snake 分工：`m*` = 对象状态（this 显式标记），snake = 计算流水；POD 聚合字段 snake 与 webrtc/C++ 集合初始化惯例一致。
- 外部消费方 = 0（实测），API 重命名历史最低价窗口。

## 4. 迁移策略（4 批次）

### 批次 α：机械修复（~35 文件，0.5 天）
横幅（17 插入 + string_builder 双横幅 + 4 OpenCTK 改名 + tests 35 统一箱式）、D8 引号 include（media 8 + examples ~5 行）、尾随空格 5 处、`ref_counted_object.hpp` 4 处 `ref_count_→mRefCount`（归属 α，γ 清单去重）、coding-style.md/AGENTS.md 文本替换（δ 最终文本）、`LogLevelNum→kLogLevelNum`。

### 批次 β：函数 + 局部变量 snake 迁移（**依赖序原子符号批**，核心批次，2-3 天）
**迁移量（correctness 实测修正）**：lowercase-camel 374 + PascalCase 多词 ~459（剔除构造噪声后实方法 ~300+）≈ **700 符号**；调用点 cxxkit 1328 + tests/examples 1057 ≈ **2400+**。

**批次规则（feasibility 裁定）**：
- **依赖序**：base → containers/functional/numerics → units/memory → text → time → thread → tools → kernel → media → network（kernel 先于 media：media 用 RefCountedObject）
- **原子符号批**：每批 = 符号声明 + **全库所有调用点**（含未处理子库与 tests）同批替换 → 调用点永无旧名残留
- **虚接口族**：`addRef/Release/HasOneRef/callSlot/hasCallable` 等跨 memory+media+tests 耦合族，单原子批处理（compiler 兜底：`override` 关键字使漏改 = 编译错，无静默 vtable 风险——feasibility+correctness 双验证）
- **宏生成名**：`dFunc`（CXXKIT_DECLARE_PRIVATE 生成，39 使用点）改宏体一次全生效；`getPointerHelper` 同理（macros.hpp 单点）
- **脚本契约**：python `re.sub(r'\b'+name+r'\b')` 全词精确替换（禁 sed 全局正则）；覆盖 lib + tests + examples；含 doxygen 注释内符号名同步（G8）
- **特判**：signals 虚族 `callSlot→call_slot`（8 处 override）+ `hasCallable`（3 处）+ `toWeakPtr`；functional `CallFunPtr/CallVoidPtr→call_fun_ptr/call_void_ptr`

### 批次 γ：成员变量 `xxx_` → mPascal（**74 unique / 809 出现**，0.5 天）
- 范围（correctness 实测）：media 10 文件 + tools（shared_buffer 42/metrics 24/strong_alias 18/fake_clock/ntp_time）+ containers/raw_hash_set（93 出现）+ **memory/ref_count.hpp（RefCounter::ref_count_ 等，v1 漏）**
- POD 聚合 struct 字段**禁碰**（纯 snake 制度化）
- 构造初始化列表同步：`: mWidth(width), mStrideY(stride_y)`；getter `int stride_y() const { return mStrideY; }`
- 与 β 批间安全性已验证：`\bstride_y_\b` 全词边界使成员改名不触碰 `stride_y(` 调用
- 门禁正则（G9）：`grep -rnE '^\s+.*\s[a-z][a-z0-9]*_\s*(;|=)' cxxkit/ --include='*.hpp' --include='*.cpp'` → 期望 0

### 批次 δ：文档与门禁（0.5 天）
- **coding-style.md 命名段最终文本（整段替换）**：

```markdown
## Naming Conventions
- Types/Classes: `PascalCase`（std-镜像容器与 trait structs 保持 snake，见 spec §1）
- Functions/Methods: `snake_case`
- Member variables: `mPascalCase`（封装类）；POD/聚合 struct 公共字段 `snake_case`
- Static members & constants & enum values: `kPascalCase`
- Local/parameter variables: `snake_case`
- Namespaces: `lowercase`（新自由函数一律扁平 cxxkit::）
- Macros: `UPPER_SNAKE_CASE`
- 语义前缀：getter 无前缀 / is_has_should_can_ 谓词 / set_ 写入 / to_ 拥有转换 / as_ 视图 / make_ 工厂
```

- AGENTS.md：UNIQUE STYLES 更新 + 清除失效头守卫描述
- **snake 函数 gate（feasibility 修正版）**：`grep -rnP '\b(?!m[A-Z])[a-z]+[A-Z][a-zA-Z0-9]*\s*\(' cxxkit --include='*.hpp' --include='*.cpp' | grep -vP ':\d+:\s*(//|\*|/\*)'` → 期望 0（排除 m 前缀成员与注释行）
- conventions.md **C16⑥**：移植代码所有公开标识符同 PR 迁移到本项目命名规范，不留豁免层
- memorys/decisions.md **D26**：本次风格决策全链记录

### 每批次验证门禁
1. `clang-format --dry-run --Werror` 全部改动文件
2. `cmake --build build --parallel` 0 error
3. `ctest --test-dir build` 63/63
4. 批次专属 grep 门禁（见上）
5. **Docs target 重跑零致命 warning**（doxygen 注释符号同步，G8）
6. commit 点由用户裁定（项目规则：未明示不 commit）

### 回滚
每批次独立可回退（`git checkout -- <files>`）；β 原子符号批失败回退整批；α/γ/δ 文件集不相交。

## 5. 豁免清零（余量）

| 原豁免项 | v2 处置 |
|---------|---------|
| media/tools/containers/memory 的 `xxx_` 成员 | γ 全迁（74 unique）|
| webrtc/abseil/信号库 Pascal/camel 函数 | β 全迁（~700 符号，含 signals 虚族）|
| **std-镜像容器类名**（flat_hash_map/set）| **永久保留小写**（API 形状镜像，制度化例外）|
| **trait/metafunction structs**（is_pointer 等）| **永久保留 snake**（std type_traits 惯例）|
| **POD 聚合 struct 字段** | **永久 snake**（§1 规则行，非豁免而是正条目）|
| 枚举 4 离群族 + 类型名重复存量（kVideoRotation_0）| 保留豁免 + followup（logging 改名 = API break）|
| tests/ 自有符号 | 随 β/γ 同文件顺带迁（G5），横幅 α 统一 |

## 6. 明确不做 / Followups

- 混用 gate 用 clang-tidy（仅未来出现新混用时）；正向 'webrtc' 横幅 gate 永久否决（36% 误报）
- color_space/hdr_metadata 出处确认；`<libyuv.h>` vs `<cxxkit/3rdparty/libyuv.h>` D6 语义——独立 followup
- 枚举类型名重复存量迁移——followup
- C16⑥ 已否决「移植代码豁免命名规范」
