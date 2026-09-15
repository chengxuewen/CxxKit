# Signal/Slot 修复 + 强化 Plan (D43)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**一句话**：我们家的信号槽（`cxxkit/kernel/signals.hpp`，palacaze/sigslot v1.2.3 的移植）有 **4 个真 bug、2 个安全漏洞级别的语义缺口、一批缺的常用 API，外加测试/例子覆盖严重偏科**（晴天用例 17 个，雨天用例 0 个）。这个计划把它们分三波修掉。

**调研来源**（团队模式 4 路并行，2026-09-15）：
- Qt 6 官方文档 + Woboq 内部机制博客
- Boost.Signals2 / libsigc++ 官方文档 + rationale
- eventpp / palacaze 上游 / nano-signal-slot 生态对比（确认血统 = palacaze v1.2.3，上游同样没修这些 bug）
- 本地逐行审计（findings F1-F16，file:line 证据）
- 本地测试/例子覆盖盘点（对标 eventpp 70+ tests / palacaze 40+ / S2 200+，见覆盖盘点章节）

**评审记录**（两路独立评审团队，均 APPROVE-WITH-FIXES，已全部折入本版）：
- oracle-reviewer：逐条对照源码验证断言——B1/B2 确认（B2 比预想更糟：mCleaner 是引用，真实 UAF）、B3 重定靶（MT 版 cow 已有快照语义，仅 SignalUnsafe 需修）、B4 驳回（全程 seq_cst）、keep-alive 驳回（四个 tracked 槽位早已实现 S2 协议）、Task 8 钦定 SignalBaseR/SignalR 新主模板（否决改既有签名）
- plan-critic：10 条 findings（R1-R10）——RED 双变体加固、RED/GREEN 分开提交、门禁措辞硬化、SignalUnsafe 重定靶（与 oracle 独立得出同结论）、repo 内 MT 测试先例确认（tst_task_queue/tst_thread_pool barrier/spin）
---

## 大局观：一张图看懂

```
现状                    问题                          修完之后
┌──────────────┐      ┌──────────────────────┐      ┌──────────────┐
│ signals.hpp  │      │ P0 四个真 bug:        │      │ 修完 P0:      │
│ (1899 行)    │─────▶│  B1 disconnect(obj)  │      │  能编译能跑   │
│ 能用但带病    │      │     编译错误(死重载)  │      │  不撒谎       │
└──────────────┘      │  B2 move 后连接错位   │      └──────┬───────┘
                      │  B3 emit 中变更=UB    │             │
                      │  B4 blocked() 有 race │             ▼
                      └──────────────────────┘      ┌──────────────┐
                      ┌──────────────────────┐      │ 修完 P1:      │
                      │ P1 两个安全缺口:      │─────▶│  线程安全     │
                      │  对象中途死亡(无保活)  │      │  名副其实     │
                      │  "支持递归"零测试背书  │      └──────┬───────┘
                      └──────────────────────┘             │
                      ┌──────────────────────┐             ▼
                      │ P2 缺的 API:          │      ┌──────────────┐
                      │  combiner(返回值聚合) │─────▶│  修完 P2:      │
                      │  num_slots/empty     │      │  对标 S2,     │
                      │  RAII blocker 等     │      │  性能仍快10倍  │
                      └──────────────────────┘      └──────────────┘
```

**说人话版**：
- **P0 = 止血**。B1 是用户调 `sig.disconnect(obj)` 直接编译不过（SFINAE 参数写反了，上次 de900da 修了 12 处同款，漏了这里）；B2 是 signal move 走后连接还认旧地址（打空枪+僵尸槽位）；B3 是 emit 正在跑的时候有人 connect/disconnect = 未定义行为（参照对象是 S2 的精确契约：拷贝快照 + 逐槽检查）；B4 是 `blocked()` 读和 `block()` 写打架。
- **P1 = 兑现承诺**。头文件声称 thread safety 是旗舰特性，但 a) 槽位对象可能在回调执行**中途**被别的线程删掉（S2 的解法：执行前把 weak_ptr 提升成临时 shared_ptr 保活——我们没有）；b) 「递归 emission 支持」这句注释底下**零条测试**。
- **P2 = 补课**。combiner（槽位返回值聚合）是 S2/sigc 的招牌，也是我们最大的 API 缺口；`num_slots()`/`empty()` 是十行的事；RAII blocker、一次性连接、连接去重都是小甜点。性能不用修——我们 emission ~10ns，比 Boost.Signals2（92ns）快一个数量级，比 Qt moc 派更快。

---

## 各波对比（为什么这么分）

| 波 | 内容 | diff 规模 | 风险 | 为什么放这波 |
|---|---|---|---|---|
| **P0 止血** | B1-B4 修复 + 每个配 RED→GREEN 测试 | 小（每个 ≤15 行） | 低 | 全是既有行为的正确性问题，用户撞上就是坑 |
| **P1 安全** | keep-alive 提升 + 多线程/递归测试 | 中（slot_tracked 改造） | 中 | P0 修完才有干净的底座；旗舰特性不能裸奔 |
| **P2 API** | 小件先行，combiner 大件收尾 | 小件~10 行；combiner 大 | 小件低/combiner 中 | combiner 动模板签名，单独 Task 隔离 |

**反超项（不修，保持）**：GroupId 有序分组（Qt/sigc 都没有）、connect_extended（Qt/sigc 缺）、非侵入 weak_ptr tracking（sigc 侵入式已被历史证伪）、性能。

## 测试/例子覆盖现状（对标主流，全面盘点）

### 现有 17 个用例的分布（tst_signals.cpp）

| 已覆盖 | 用例数 | 说明 |
|---|---|---|
| 基础收发/参数传递/成员函数槽 | 3 | happy-path ✔
| 顺序/group 排序 | 2 | happy-path ✔
| disconnect 各形态/scoped/析构清理 | 5 | happy-path ✔
| block（单连接 RAII + 信号级） | 3 | happy-path ✔
| weak_ptr 追踪（对象已死再 emit） | 1 | **唯一的雨天用例**，但只测 emit 前已死，未测执行中途死
| extended slot 自断 | 1 | happy-path ✔
| SignalUnsafe 冒烟 | 1 | 仅 smoke

### 对标主流项目的测试维度（差距矩阵）

| 维度 | eventpp | palacaze | S2 | cxxkit 现状 | 结论 |
|---|---|---|---|---|---|
| emission 期间 connect/disconnect | ✔ | ✔ | ✔（精确契约） | **无** | B3 修复时补（Task 3） |
| slot 执行中途自断/删信号 | ✔ | ✔ | ✔ | **无** | B3 修复时补（Task 3） |
| 对象执行中途死亡（keep-alive） | n/a | n/a | ✔ | **无**（UAF） | Task 5 |
| 递归 emission | ✔ | ✔ | ✔ | **零测试** | Task 6 |
| 多线程并发 connect/disconnect/emit | ✔ | n/a | ✔（专门的 thread-safety 测试） | **零 MT 测试** | Task 6 |
| move 语义 | ✔ | ✔ | ✔ | **无** | Task 0 B2 复现即测 |
| 组合器/返回值 | n/a | n/a | ✔ | **无此 API** | Task 8 |
| 覆盖率（signals.cpp 本体） | — | — | — | kernel 无 signal 定向覆盖报告 | Task 9 加 coverage 查验 |

### 例子覆盖

- `exp_kernel.cpp`：signals-only walkthrough 已有（connect/emit/disconnect/scoped/group），**补两节**：a) tracked 槽位生命周期演示；b) 组合器用法（Task 8 落地后）
- 缺一个**跨线程演示**：signal 在 worker 线程 emit、主线程收——与 kernel EventLoop 组合的最小例子。并入 exp_kernel 而非新文件（一个子库一个例子的项目惯例）

结论：**17 用例全部晴天，与「thread safety 是旗舰特性」的定位严重不符**。补全后目标 ~40 用例（P0 复现 4 + B3 契约 3 + keep-alive 2 + MT/递归 6 + P2 API 8 ≈ +23）。
---

## Global Constraints（与 D42 计划同款）

- C18: commit messages + code comments English only; plan doc + AI dialogue Chinese.
- C4: C++11 lib code——**combiner 的 slot_call_iterator 不得用 C++14/17 语法**（这是本计划最大的 C4 风险点，模板迭代器最容易顺手写 `auto` 返回）。
- D26 naming: snake_case / mPascalCase / kPascalCase。
- D8: angle-bracket includes。
- 每任务门禁：`cmake --build build --parallel` 0 errors → `ctest --test-dir build -R signal` green → `pixi run clang-format`（禁碰 CMakeLists）。
- 收官全量门禁：主树全量 ctest 全绿 + ASAN 定向 signal 套件零诊断（signal 有 MT 用例，ASAN 是照妖镜）。
- PIT-45: 复合 CXXKIT_CHECK 条件加括号。NEVER `rm -rf build*`。
- 提交身份：`git -c user.name=chengxuewen -c user.email=1398831004@qq.com`。

---

### Task 0: 热身——基线 + 复现测试（RED）

**Files:**
- Read: `cxxkit/kernel/signals.hpp`（重点 :1345-1363 move、:1681-1720 disconnect、emit 路径）
- Modify: `tests/tst_signals.cpp`（注意：实际文件名带 s，17 用例，见覆盖盘点章节）

- [ ] **Step 0.1**: 基线——`git status` clean；build 0 err；`ctest --test-dir build -R signals | tail -1` 记录用例数（预期 17）。
- [ ] **Step 0.2**: 写 B1 复现测试（RED）：`Signal<int> s; Foo f; s.connect([](int){}); s.disconnect(f);`——预期**编译失败**即为 RED 证据（SFINAE 死重载）。临时用 static_assert 探针确认后注释掉编译断言，留运行时测试骨架（oracle：:1695 一处参数序即根因）。
- [ ] **Step 0.3**: 写 B2 复现测试（RED，双变体）：① 构造 signal → connect → move 构造新 signal → 旧 Connection::disconnect() → 断言新 signal slot_count 为 0（当前实现下不为 0 = RED）；② **oracle F5 加固**：源 signal 先析构 → 再对旧 Connection::disconnect()——当前直接 UAF，修复后安全 no-op。
- [ ] **Step 0.4**: 写 B3 复现测试（RED，仅 SignalUnsafe）：slot A 执行中 disconnect 自己 + connect 新槽位——期望契约：新槽位本轮不执行、下轮执行（S2 契约）。**oracle F1 修订：Signal（MT 版）不加此用例**——cow 机制已提供快照语义（slots_reference 持引用计数，emit 期间 mSlots 变更走 cow 路径），现契约已生效，禁止加 per-emit 拷贝（会毁掉 ~10ns emission 性能）。
- [ ] **Step 0.5**: 提交 RED 测试（允许暂红；plan-critic R4：RED 与 GREEN 分开提交，每步可回退）。
- [ ] **Step 0.6**: 覆盖率基线：`bash scripts/coverage.sh build-cov | grep signals` 记录 signals.cpp 当前行覆盖（供收官对比）。

### Task 1: B1 修复——disconnect(obj) 死重载（GREEN）

**Files:**
- Modify: `cxxkit/kernel/signals.hpp` :1693-1701

- [ ] **Step 1.1**: 修复 SFINAE 参数序：`is_callable<ext_arg_list, Obj>` → `!is_callable<Obj, arg_list>::value && !is_callable<Obj, ext_arg_list>::value`（对齐 de900da 的 12 处修法；oracle F4 确认仅 :1695 一处）。
- [ ] **Step 1.2**: Step 0.2 的测试转 GREEN；`ctest -R signals` 全绿。
- [ ] **Step 1.3**: commit `fix(kernel): disconnect(object) overload was unreachable (SFINAE arg order)`。

### Task 2: B2 修复——move 后连接重路由（GREEN，方案唯一）

**Files:**
- Modify: `cxxkit/kernel/signals.hpp` move ctor/assignment :1345-1363 + detail 槽位族（mCleaner 字段）

- [ ] **Step 2.1**: **oracle F3 定案（唯一解，无二选一）**：mCleaner 当前是 `SignalBase&` 引用（不可重绑），move 后悬垂 = 真实 UAF。修法：**引用改指针 + move 时在锁内遍历槽位改写指向新 signal**。null-husk 方案否决（源 signal 析构后 husk 也没了，悬垂依旧）。
- [ ] **Step 2.2**: Step 0.3 双变体转 GREEN（含 destroy-then-disconnect UAF 变体）；补 move-assign 变体用例。
- [ ] **Step 2.3**: commit `fix(kernel): re-route slot cleaners on signal move (dangling-reference UAF)`。

### Task 3: B3 修复——SignalUnsafe emission 期变更契约化（GREEN，重定靶）

**Files:**
- Modify: `cxxkit/kernel/signals.hpp` SignalUnsafe 的 emit 路径（NullMutex 分支）

- [ ] **Step 3.1**: **oracle F1 重定靶**：Signal（MT 版）不加任何 per-emit 拷贝——cow 已给快照语义。只需给 SignalUnsafe 的 emit 循环加锁内快照（NullMutex 下拷贝开销可忽略，单线程无竞态问题但 vector 扩容失效问题真实存在）。
- [ ] **Step 3.2**: 契约写入 doxygen（两变体统一措辞）：emission 中 connect = 下轮生效；disconnect = 已过的跑完、未到的按实现快照决定。**注明 MT 版由 cow 保证、ST 版由 emit 内快照保证**。
- [ ] **Step 3.3**: Step 0.4 转 GREEN + ASAN 定向跑该用例零诊断；commit `fix(kernel): SignalUnsafe emission-time mutation contract (snapshot in emit loop)`。

### Task 4: B4 —— 文档化（REFUTED，无代码改动）

**Files:**
- Modify: `cxxkit/kernel/signals.hpp` block()/blocked() doxygen 注释

- [ ] **Step 4.1**: **oracle F2 裁决：B4 误报**——mBlock 全程默认 seq_cst 原子操作，无 race。原「memory_order 修复」取消，只补 doxygen：说明 blocked() 的可见性语义（seq_cst 全序）与 block 窗口语义（block 后已开始的 emission 仍会跑完当前快照）。
- [ ] **Step 4.2**: commit `docs(kernel): signal block/blocked visibility semantics`。

### Task 5: P1-a keep-alive —— 降级为回归钉（REFUTED-实现已存在）

**Files:**
- Modify: `tests/tst_signals.cpp`（新增用例）

- [ ] **Step 5.1**: **oracle F6 裁决：keep-alive 早已实现**——四个 tracked 槽位（slot_tracked/slot_pmf_tracked 及 extended 变体）的 operator() 内已有 weak_ptr::lock() 提升局部 shared_ptr 的 S2 协议原文级代码。原「实现 keep-alive」任务取消。
- [ ] **Step 5.2**: 改为回归钉用例：线程 A emit 长槽位（sleep 门控），线程 B 中途 reset 目标 shared_ptr——断言槽位跑完 + ASAN 零诊断（钉住既有不变量，防未来回退）。进 `tests/tst_signal_mt.cpp`。
- [ ] **Step 5.3**: commit `test(kernel): pin tracked-slot keep-alive invariant (MT)`。

### Task 6: P1-b 递归/多线程测试补齐

**Files:**
- Create: `tests/tst_signal_mt.cpp`（Task 5.2 已建则复用）
- Modify: `tests/CMakeLists.txt`（注册新套件，走 cxxkit_add_test）

- [ ] **Step 6.1**: **oracle F7 澄清：递归声明今天就是真的**——emit 不持锁调槽位（cow 引用放锁外），两变体都支持递归。原「探路」任务改为钉子用例：SignalUnsafe 直接递归 + Signal 递归（背靠背 connect(emit)）——断言递归深度计数。
- [ ] **Step 6.2**: MT 压测：4 线程并发 connect/disconnect/emit × N 轮（ASAN 下跑；仿 tst_task_queue/tst_thread_pool 既有 barrier/spin 先例，plan-critic R6 已核实在repo 有先例）。
- [ ] **Step 6.3**: 仿 S2 thread-safety 测试维度补齐三件：① emit 进行中另一线程 disconnect（cow 契约的 MT 钉）② Connection 跨线程 disconnect vs 并发 emit ③ move 后旧 Connection 跨线程操作（B2 修复的 MT 回归钉）。三者全部进 `tst_signal_mt.cpp`，ASAN 下零诊断是门禁。
- [ ] **Step 6.4**: 全量 ctest（新套件 +1）；commit `test(kernel): MT stress + recursion coverage for signals`。

### Task 7: P2 小件——num_slots/empty + RAII blocker + SingleShot

**Files:**
- Modify: `cxxkit/kernel/signals.hpp`

- [ ] **Step 7.1**: `size_t num_slots() const`（锁内计数，注释线性复杂度，S2 同款理由）+ `bool empty() const`。
- [ ] **Step 7.2**: `ScopedBlock`（RAII，构造 block 析构 unblock，可叠加计数版留后——先布尔版，5 行）。
- [ ] **Step 7.3**: SingleShot：`connect_once(callable, gid=0)`——内部 connect_extended 槽位首行自 disconnect。
- [ ] **Step 7.4**: 每件配测试；commit `feat(kernel): signal queries + RAII block + one-shot connect`。

### Task 8: P2 大件——返回值 combiner（独立评审点，方案已定）

**Files:**
- Modify: `cxxkit/kernel/signals.hpp`（新增 SignalBaseR 主模板 + slot_call_iterator + SignalR 别名）
- Modify: `docs/`（README kernel 行提及）

- [ ] **Step 8.1**: **oracle F8 钦定（否决两案对比）**：新建 `SignalBaseR<R, Lockable, Combiner, T...>` 主模板 + `SignalR<R, Combiner = optional_last_value<R>, Args...>` 别名——**不动既有 SignalBase/Signal**。理由：改 SignalBase 签名自违兼容红线；C++11 无法在参数包 T... 前给 R 默认参数。R 经槽位返回类型萃取。
- [ ] **Step 8.2**: slot_call_iterator：解引用=执行槽位并缓存结果（lazy），跳过 disconnected/blocked（C++11：迭代器 typedef/value_type 手写，禁 auto 返回）。
- [ ] **Step 8.3**: 默认 combiner = S2 `optional_last_value<R>` 语义；附 `maximum` 示例（调研报告有完整 sketch）。
- [ ] **Step 8.4**: C4 门禁自查（`grep -rnE "auto\s+\w+\s*=|if constexpr" cxxkit/ --include="*.hpp"` 干净——迭代器最容易违规）。
- [ ] **Step 8.5**: 测试：单槽/多槽/零槽(空 optional)/短路 combiner/自定义 combiner。exp_kernel 组合器演示节的依赖在此闭环（Task 9.2 消费）。
- [ ] **Step 8.6**: commit `feat(kernel): return-value combiners via SignalR (S2-style lazy slot iterators)`。

### Task 9: 收官——例子补全 + 文档 + 记忆 + 全量门禁

**Files:**
- Modify: `examples/exp_kernel.cpp`（补两节：a) tracked 槽位生命周期演示；b) 组合器用法）
- Modify: `README.md`（kernel 行：补 combiner/queries/MT 语义）、`.agents/memorys/status.md`、`decisions.md`（D43）、`pitfalls.md`（若实施中新坑）

- [ ] **Step 9.1**: emission-time 契约 + 递归支持边界写进 signals.hpp 头注释。
- [ ] **Step 9.2**: exp_kernel 补三节：a) tracked-lifetime（shared_ptr 槽位对象析构 → emit 安全跳过）；b) 组合器（Task 8 落地后）；c) 跨线程发射（worker 线程 emit → 主环收，用 EventLoopThread + Barrier 同步保证输出确定性，符合 README 确定性声明）：跑通 rc=0。
- [ ] **Step 9.3**: 全量门禁：主树全绿 + ASAN signal 定向零诊断 + clang-format 干净 + C4 grep 干净 + `bash scripts/coverage.sh build-cov | grep signals` 报告 signals.cpp 覆盖提升数字（对标 B 波 network 家族扫盲口径）。
- [ ] **Step 9.4**: 记忆三件套更新 + commit `docs: signal/slot D43 records`。

---

## Deferred（需求触发再取，YAGNI）

- UniqueConnection 去重（Qt 也只对 PMF 支持，lambda 场景我们若做可按槽位标识做得更好——但无消费方呼声）
- `shared_connection_block` 引用计数版（布尔版够用到有嵌套需求为止）
- Queued/AutoConnection（kernel EventLoop + connect_queued 已覆盖该场景，Signal 本体不碰）
- signal-to-signal 链式 helper（`make_slot` 返回发射器即可手写）
- 组内 at_front/`connect_first`（GroupId 分组已够用）

---

## 附录 A：现有用例基线（tst_signals.cpp，17 个）

```
Signal            ConnectEmitReceivesArguments              基础收发
Signal            MemberFunctionSlotObserverPattern          成员函数槽
Signal            MultipleSlotsFireInOrder                   顺序
Signal            DisconnectStopsDelivery                    断连
Signal            DisconnectAllClearsEverySlot               全断
Signal            DisconnectByGroupIdRemovesOnlyThatGroup    分组断连
Signal            DisconnectByPmfAndObject                   pmf+对象断连
Signal            ScopedConnectionDisconnectsAtScopeEnd      RAII 断连
Signal            ConnectionBlockBlocksSingleSlot            单连接 block
Signal            ConnectionBlockerRaII                      连接级 RAII block（已有！）
Signal            SignalLevelBlockSuppressesEmission         信号级 block
Signal            GroupOrderAscending                        组排序
Signal            TrackedWeakPtrSkipsDeadObject              追踪（emit 前已死）
Signal            ExtendedSlotCanSelfDisconnect              自断
SignalUnsafe      SmokeEmitAndScopedDisconnect               冒烟
Signal            DestructorDisconnectsAllConnections        析构清理
Signal            ObserverBaseDisconnectAllOnExplicitCall    observer 断连
```

**覆盖空白**（本计划逐项回填）：emission 期变更（Task 0/3）、执行中途对象死亡（Task 5）、递归（Task 6）、MT 并发（Task 6）、move 语义（Task 0/2）、组合器（Task 8）。**注意**：连接级 RAII blocker 已存在（ConnectionBlockerRaII）——Task 7.2 的 ScopedBlock 是**信号级**的（对标 Qt QSignalBlocker），勿混淆。
