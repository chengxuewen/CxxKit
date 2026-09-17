# 信号槽 × 事件循环 × 调度：vs Qt 6 + 主流生态差距分析与路线图（hyperplan 修订版）

- 日期：2026-09-17（初版）/ 2026-09-17（hyperplan 修订）
- 状态：规划（未排期；各波启动需另行确认；OQ1/OQ2/OQ3 为波前用户裁定门）
- 来源：团队模式三路调研（qt-signals / qt-loop / eco-researcher），报告原文在
  `/tmp/opencode/cxxkit-gap-analysis/{qt-signals,qt-loop,eco}.md`（临时目录，不随库分发；
  本文档自包含所有决策所需内容）。cxxkit 侧基线 = D45 后工作树（e7e1437）。
- 参照：Qt 6.8-6.11 官方文档 + qtbase 6.8 源码；boost::signals2 / sigc++ 3.6 / libuv 1.49 /
  asio 1.32 / Chromium scheduler / WebRTC / glib / seastar。严重度按远控主机场景
  （常驻守护、多连接会话、低延迟交互、优雅退出）评估。
- 证据可信度：三份报告全部带 Qt 文档/源码出处 + cxxkit file:line；lead 已抽查关键
  file:line（组合器、Observer、ScopedBlock、signal_wrapper、exclude flags 0 消费、
  uv 嵌套 FATAL 文案）全部属实。行号以 D45 工作树为准，后续波次执行前应复核
  （本修订版新增/变更的行号已按当前工作树核对，见下方修订记录第 8 条）。

---

## 修订记录（hyperplan，2026-09-17）

经 4 人对抗性交叉评审三轮（skeptic / validator / architect / creative）。存活见解全部
并入，被击毙项不保留。逐条修订如下（成员名见对抗谱系尾注）：

| # | 修订 | 来源 | 落点 |
|---|------|------|------|
| 1 | G1 排队词汇永居 kernel 层头文件：connect_queued 家族原地生长（loop 参数变体/重载/返回 Connection），**signals.hpp 零 EventLoop 知识**——撤销原"kQueued 进 signals API"方向 | H1 (architect-A1) | G1 / W3 |
| 2 | queued 连接 liveness 用 **weak loop token**（capture weak -> lock 前 expired 检查 -> dead 静默 skip），住 kernel 侧包装，与 tracked 槽静默跳过哲学一致 | H2 (architect-A2) | G1 / W3 |
| 3 | G3 Object<->signals **组合**（ObjectPrivate 内嵌 observer 成员 + add_connection 转发），禁继承（D33 非侵入裁定） | H3 (architect-A3) | G3 / W3 |
| 4 | G3 必须 **eager 释放**（observer disconnect_all / loop-owned connection registry），lazy emission-skip = 死槽慢性泄漏 | H4 (creative-C5 + architect-A3) | G3 / W3 |
| 5 | G4 future 族续体执行上下文 = **站点显式参数，两命名上下文封顶**（loop 亲和 or thread_pool），拒通用 executor 框架 | H5 (architect-A4) | G4 / W6 |
| 6 | G7 采用 **stop_and_wait() 方法形态**（queue 级 bounded drain，Task 类型不动，dtor 保持 PIT-33 drop-pending），per-Task flag 出局 | H6 (architect-A7 + validator-V5) | G7 / W5 |
| 7 | G2 call_and_wait **不可按原计划 standalone W2 交付**——归宿 OQ1 用户裁定（存留底线：同环 fatal + 阻塞反模式警告） | H7 (creative-C4 + validator-V1 + skeptic-S3) | G2 / OQ1 |
| 8 | G1 裁定前置 W3 + **4 现有消费者迁移入计划**：tst_event_loop.cpp:192/216/234 + exp_qt_embed.cpp:73（已按当前工作树核对） | H8 (skeptic-S2) | G1 / W3 |
| 9 | W3 新增 **receiver-liveness purge 机制设计项**——queued closure 无 purge 路径（实证：event_loop_p.hpp:178 mPostQueue、event_loop.cpp:379 purge_pending 只覆盖 mEventQueue），receiver 死于 drain 前 = UAF | H9 (validator-V7 + creative-C2) | G1 / W3 |
| 10 | D4 timer collapse 契约须 **asio 审计先行/同波**，否则标注 "uv-only; asio pending audit" | H10 (validator-V4) | D4 / W1 |
| 11 | G6 prepare/check **处死**：三后端漂移（uv=poll 迭代/shell=process_events/qt=无 poll 边界）；复活条件 = shell 级锚点 + has_poll_hooks() | D1 (skeptic-S4 + architect-A5) | G6 / 不做 / W1 |
| 12 | W5 波解散：G6 死、G5 并入 W1、G7 独立组件级 | D2 (skeptic-S6 + validator-V5) | 波次表 |
| 13 | D2 升级：**四值全死**（kExcludeUserInputEvents/kExcludeSocketNotifiers/kX11ExcludeTimers/kDialogExec），0.x 窗口直接删无 alias（处置见 OQ2） | D3 (architect-A6) | D2 / W1 |
| 14 | D1 扩为：drain-till-empty 契约 + 再投递放大警示 + 交叉引用 process_events(flags, maximum_ms) | D4 (creative-C7 + architect-A5) | D1 / W1 |
| 15 | 明确不做清单补重开触发条件（D43 史实：strand/blockSignals/set_combiner 反复重提） | D5 (skeptic-S7) | 不做 |
| 16 | G3 重定价组件级：五设计轴 + loop-owned registry 倾向 + 与 G1 形状裁定合并 | D6 (validator-V6 + architect-A3) | G3 / W2 |
| 17 | G4 修正：真缺口 = **无 .then() 续体链**（删"get 阻塞语义"）；future.hpp 空壳删除 + README 同步 | D7 (skeptic-S1 + architect) | G4 / D8 / W1 |
| 18 | G1+G3 **连接形状裁定一次钉死**，消灭 W2/W3 两次设计返工 | D8 (architect-A1/A3 + validator-V6) | W2 |

researcher（迟到独立复核）实证 2 处事实错误（G7 引用/dtor 空壳、D1 drain-till-empty 半假契约）+ 3 处表述修正（Qt 引文粘贴要求、~85% 降级、webrtc 三档出处），全部折叠。
对抗谱系：skeptic 4/7、validator 6/7、architect 6/7+1 变形、creative 5/7；
完全击杀 6 项（G2-delete、G3-doc-only、不做清单膨胀、bookkeeping-symptom、
std::future strawman、time-sliced-drain-as-new-mechanism）。

---

## 〇、波前用户裁定门（启动相关波次前必须回答）

三问已于 2026-09-17 逐项审核中全部裁定（12/12 通过），门均已开。

### OQ1（门：W4 之前）—— G2 call_and_wait 归宿三选一 ✅ 已裁定（2026-09-17 定稿审核，12/12 通过）：**(b) kernel 层**

- (a) `thread/` 层 15 行即刻 helper（无超时/loop 死 fatal，std::promise 直译）
- (b) **（推荐，D24 首位）** `kernel/` 层 v1-封顶原语（无超时/loop 死 fatal，mutex+condvar+
  结果盒 40-60 行，std 原语 kernel 合法，复用 connect_queued 排队通道）
- (c) 并入 G4 then() 落地后的利基（远期；前提 G4 需求触发）
- **存留底线**（任何选择均须）：同环 fatal + 阻塞反模式警告进 conventions 或头注释。
- 推荐理由：call_and_wait 语义主场是 EventLoop（kernel）；(a) 与 (b) 是同物换目录；
  (c) 依赖 G4 时点未知，远控快照痛点在当下。

### OQ2（门：W1-D2 条目之前）—— D2 死枚举处置深度 ✅ 已裁定（2026-09-17 定稿审核，12/12 通过）：**(a) 全删 4 值无 alias**

- (a) **（推荐）** 全删 4 值无 alias —— 0.x 窗口 + 全库零消费已核实
- (b) 全部标注 "accepted but ignored"（保 Qt 枚举对齐面）

### OQ3（门：任何 G4 工作之前）—— G4 spike vs 维持 W4 门 ✅ 已裁定（2026-09-17 定稿审核，12/12 通过）：**维持门 + HC5 预埋**

- **裁定**：维持 W4 需求触发门 + HC5 预埋——排名轴与启动轴正交；W2 裁定文档强制包含 G4 续体上下文设计小节。

---

## 一、总判

1. **信号槽域：定位准确，勿大动。** D43/D43.5 收敛后与 Qt 语义差异 = "机制不同、结论相同"。
2. **事件循环域：能力面对齐度高（绝大多数 Qt 循环/调度能力有对应物或已文档化的有意差异），** 真差距集中在契约文字（行为已等价但未文档化），逐项 enumerated 于 §二。
3. **调度域：最大结构性缺口是 future 族。** `thread/future.hpp` 空壳（30 行）→ 修订后直接删除；
   D42 TLS 编排回调嵌套痛点已实证需求。
4. **（新增）** G6 从"建议实施"降为"处死 + 墓志铭"；G5 并入文档波；G7 独立立案。

---

## 二、真差距 Top-7（按 场景影响 x 成本 排序）

### G1. 跨线程排队连接词汇 —— 信号域唯一高严重度差距

- **Qt**：每条连接可携带投递策略（Auto/Direct/Queued/BlockingQueued/Unique/SingleShot）；
  queued 路径参数经 QMetaType 深拷贝。
- **cxxkit 现状**：全库无 ConnectionType 词汇（grep 零命中）；所有连接同步直调；
  跨线程只有 `kernel/connect_queued.hpp` 自由函数（bind 拷贝 + loop->post）。
- **严重度：高**；**成本：组件级**。
- **修订后建议（v1 形状一次钉死，H1/H2/H8/H9）**：
  - **词汇永居 kernel 层**：`connect_queued` 家族在 `kernel/connect_queued.hpp` 原地生长
    （loop 参数变体/重载/返回 Connection），**signals.hpp 保持 EventLoop 零知识**。
  - **weak loop token**（H2）：queued 连接 liveness 用 weak 环 token（capture weak ->
    投递前 lock 检查 -> loop 亡则静默 skip），与 tracked 槽哲学一致。
  - **4 现有消费者迁移入本项**（H8）：tst_event_loop.cpp:192/216/234 + exp_qt_embed.cpp:73。
  - **receiver-liveness purge 机制（新增预算，H9）**：queued closure 无 purge 路径
    （mPostQueue std::function 无清除接口；event_loop.cpp:379 purge_pending 只覆盖
    mEventQueue）—— receiver 亡于 drain 前 = UAF。仿 D35 mWatching 先例 + SafetyFlag；
    与 G3 loop-owned registry 合并设计。
  - 语义红线：保留"emit 时拷贝一次"契约。Auto/BlockingQueued 不进 v1。
- **成功判据**：connect_queued 家族 v1 落地 + signals.hpp 零 EventLoop 依赖 +
  weak token death 用例 + purge death 用例（ASAN 无 UAF）+ 4 消费者回归 +
  uv/asio 双树 ctest + ASAN + check.sh。

### G2. 同步跨线程查询原语（BlockingQueued 语义）

- **Qt**：BlockingQueuedConnection（QSemaphore 握手，参数不拷；同线程死锁仅运行时告警）。
- **cxxkit 现状**：无。
- **严重度：中**；**成本：行数级（~40-60 行 + 测试）**。
- **修订后建议（H7）**：`call_and_wait(loop, fn) -> R` 自由函数——**归宿三选一，见 OQ1**；
  同环 fatal + 阻塞反模式警告为存留底线。不再作为 W2 独立交付。
- **成功判据**：按 OQ1 裁定交付；round-trip 用例 + 同环 fatal death 用例 + 双树 + ASAN。

### G3. Object <-> signals 打通（context-object 自动断连）

- **Qt**：任何 QObject 可作 context，销毁即自动断连。
- **cxxkit 现状**：机制已有（Observer / tracked weak_ptr），但 Object 树未集成。
- **严重度：中**；**成本：组件级**（D6 重定价）。
- **修订后建议（H3/H4/D6/D8）**：
  - **组合非继承**（H3）：ObjectPrivate 内嵌 observer 成员 + add_connection 转发。
  - **eager 释放**（H4）：observer disconnect_all / loop-owned registry；lazy skip 禁止。
  - **五设计轴**（W2 一次性钉死）：锁变体 / 析构插入点（vs destroying->purge->cascade）/
    move_to_loop 过期环 / 锁模型张力 / 过期环 pending delivery。
  - **形状倾向**：loop-owned connection registry（D35 mWatching 先例）。
  - **与 G1 形状裁定合并**（D8），消灭 W2/W3 两次设计返工。
  - **fallback 路径**（2026-09-17 定稿裁定）：若五轴裁定不成立，G3 可独立降级
    （Object 集成不进 v1，connect_queued 家族照发）——W2 文档必须写明此路径。
- **成功判据**：ObjectPrivate 组合（非继承 grep）+ eager disconnect_all 用例 +
  receiver death 零派发 + 析构计数归零 + 4 消费者回归 + 双树 + ASAN。

### G4. F-P-C future 族 —— 全库最大结构性缺口

- **cxxkit 现状**：`thread/future.hpp` 空壳（30 行，修订后删除）；异步编排仅回调式。
- **严重度：中-高**；**成本：项目级**。
- **修订后建议（H5/D7）**：
  - 真缺口 = **无 .then() 续体链**（删"get 阻塞语义"措辞）。
  - 续体上下文 = **站点显式参数**，两命名上下文封顶（loop 亲和 / thread_pool）（H5）。
  - **future.hpp 空壳 = 删除**（D7/R3，落 W1-D8）。
  - **W6 门保留**（OQ3 已裁定维持）：需求触发启动；且 **HC5 预埋**（定稿裁定）——
    W2 裁定文档强制包含 G4 续体上下文设计小节，提前完成最难设计题，缩短未来启动流血窗口。

### G5. timer 契约化（行为已等价，缺契约 + asio 审计）

- **修订后**：并入 W1。start_timer 契约 3 句 + asio 超期审计先行/同波（H10）。
- **成功判据**：契约段落盘 + asio 审计结论明确 + D4 文字一致。

### G6. prepare/check 迭代钩子 → **处死**（D1）

- 三后端漂移：uv=poll 迭代 / shell=process_events / qt=无 poll 边界。
- 复活条件：shell 级锚点 + has_poll_hooks() 设计成立。
- 落 W1-D9 墓志铭 + §四 不做 栅栏。

### G7. Task shutdown 行为注解 → 独立组件级（D2）

- **修订后建议（H6）**：
  - **stop_and_wait() 方法形态**：queue 级 bounded drain + deadline。
  - **destroy()/dtor 保持 PIT-33 drop-pending**（机制：destroy() -> mQuit=true -> join -> process_tasks 退出后 :308 clear；dtor 本体为空，不得改——现有依赖此语义的消费者/测试必须枚举迁移）。
  - **ThreadPool 明示 out of scope**。
- **成功判据**：PIT-33 破坏清单交付 + bounded-drain 用例 + Task 类型零改动（grep 零命中）+
  dtor 语义不变（PIT-33 测试全绿）+ 双树 + ASAN。

---

## 三、文档速修包（W1，半天~一天）

| # | 项 | 位置 | 动作 |
|---|----|------|------|
| D1 | process_events 排空边界 | event_loop.hpp process_events 段 | **两句契约**：(a) **task queue = snapshot 语义**（take_post_queue 一次 swap，同轮新 post 等下轮）；(b) **Event queue = drain-until-sentinel**（pop 派发循环，同轮新 post_event 并入本轮；再投递放大路径警示）。交叉引用 process_events(flags, maximum_ms) 逃生口。**引文纪律**：拟写 processEvents "queued-before-call" 契约句前，须从 qtbase 官方文档粘贴原文到本任务注释，树中仅 vendored Qt 头无 qtbase 源码（防转述漂移）。 |
| D2 | exclude flags 死枚举 | event_loop.hpp:46-54 ProcessFlag | 四值处置见 **OQ2**（删 or 标注） |
| D3 | 嵌套环分裂契约 | event_loop.hpp exec 段 | qt 允许 / uv fatal |
| D4 | timer 契约 3 句 | event_loop.hpp start_timer | + **asio 审计先行**（H10），否则 uv-only 标注。**引文纪律**：拟写 Coarse ±5% / overrun-collapse / qtimer.html 相关契约句前，须从官方文档粘贴原文到本任务注释，树中仅 vendored qtimer.h:182（仅 ≥2s Coarse 启发式），无 qtbase 完整源（防转述漂移）。 |
| D5 | track 静默失效 | signals.hpp 头注释 | "dead -> skip, not throw" |
| D6 | context-less connect 约定 | conventions.md + AGENTS.md | lambda 捕 this 必须经 tracked/observer/ScopedConnection |
| D7 | connect_unique（可选） | signals.hpp | PMF/函数指针限定去重 |
| D8 | **（新增）future.hpp 空壳删除** | thread/future.hpp + README | 删除 + thread 组件表删 future 行 |
| D9 | **（新增）G6 墓志铭** | decisions.md | 处死理由 + 复活条件 |
| D10 | **（新增）G2 存留底线** | conventions.md 或头注释 | 同环 fatal + 阻塞反模式警告 |

**W1 成功判据**：
- SV1：D1-D10 全部落盘；doc-only diff 验证纯注释（C13）+ build 0 error + ctest 全绿。
- SV2：asio timer 超期审计结论明确（等价->钉 / 不等价->标注），D4 文字一致。
- SV3：future.hpp 删除后 `grep -rn "thread/future" cxxkit/ tests/ examples/` = 0。
- SV4：若 OQ2=(a) 删枚举后 grep 零命中 + ctest 全绿。
- SV5：clang-format 干净 + check.sh 全绿。

---

## 四、明确不做（防重开栅栏，修订后每条补重开触发条件——D5）

- **Qt orphaned-connection 回收协议**替换 COW 快照
  - 重开：扇出实测触达表拷贝瓶颈（>10 万级）。
- BlockingQueued 做成 ConnectionType
  - 重开：OQ1 交付后显式函数形态被证明不足。
- 通用 event compression
  - 重开：widget/UI 层出现。
- invokeMethod Auto 语义
  - 重开：消费方论证 post 无法覆盖。
- Qt::TimerId 强类型 / remainingTime / aboutToBlock / awake
  - 重开：任一接口出现消费方。
- strand / SequenceManager / seastar shard/fair_queue
  - 重开：非亲和调度需求实证。
- deconstruct / QSignalSpy / 信号元信息
  - 重开：调试消费方出现。
- blockSignals / 共享 connection_block / at_front / 运行时 set_combiner
  - 重开：任一 API 出现消费方。
- 嵌套环 uv 侧实现
  - 重开：uv 上游提供嵌套迭代 API。
- **G6 prepare/check（新增，D1）**：语义不可移植
  - 重开：shell 级锚点 + has_poll_hooks() 设计成立。
- 任务书勘误："Qt6 return-value connections" 不成立。永久封档。

---

## 五、cxxkit 反超项

组合器家族（SignalR + slot_call_iterator）；connect_once / connect_scoped；连接级
ConnectionBlocker（Qt 仅对象级）；三轨生命周期追踪；move_to_loop 整树迁移 + 队列随迁；
事件优先级稳定插入 + 对象级 timer；SignalUnsafe ST 零锁变体。

---

## 六、建议路线（波次；修订后）

### 波次总表

| 波 | 内容 | 量级 | 前置 |
|----|------|------|------|
| **W1** | 文档速修包 D1-D10 + G5 契约 + asio 审计 + future.hpp 空壳删除 + G6 墓志铭 | 半天~一天 | 无（D2 需 OQ2） |
| **W2** | G1+G3 连接形状联合裁定（设计先行，Momus 审核，**零代码**） | 1 天 | 无（可与 W1 并行） |
| **W3** | G1 connect_queued 家族 v1 + G3 Object<->signals + purge 机制 + 4 消费者迁移 | 组件级 | **W2 裁定完成** |
| **W4** | G2 call_and_wait（OQ1 已裁定：kernel 层 v1-封顶原语） | 行数级 | OQ1 ✅（门已开） |
| **W5** | G7 stop_and_wait() bounded drain | 小组件级 | **需要优雅退出的真实组件出现时**（定稿裁定） |
| **W6** | G4 future 族（then/when_all/finally + 错误传播） | 项目级 | **需求触发 + W2 预埋就绪（OQ3 门）** |

原 W5 波已解散（D2）：G6 死、G5 并入 W1、G7 独立为 W5。
原 W2 G2 独立交付取消（H7），G3 并入 W3；原 W2 D7 并入 W1。

### W2 判据（G1+G3 形状联合裁定，D8）

- SV1：设计文档覆盖 connect_queued 家族形状 + weak token + Object registry + eager 释放 + purge 草图。
- SV2：Momus APPROVE（或 APPROVE-WITH-FIXES 全修订闭环）。
- SV3：W3 边界明确（Auto 是否进 v1、purge 落点）。
- SV4：与现有 connect_queued.hpp 契约一致。

### W3 判据（G1 v1 + G3）

- SV1：连接形状落地；signals.hpp 零新增 EventLoop 依赖（grep 零命中）。
- SV2：weak token：loop 亡后 queued emit 静默 skip（death 用例）。
- SV3：receiver-liveness purge：receiver 亡后 queued closure 零派发（ASAN 零 UAF）。
- SV4：G3 组合落地：非继承 grep 零命中 + eager disconnect_all 用例。
- SV5：4 现有消费者迁移并运行。
- SV6：uv + asio 双树 ctest + ASAN + clang-format + C4 + check.sh。
- SV7：覆盖率 >= 80% 门禁不倒退。

### W4 判据（G2，按 OQ1）

- round-trip 用例 + 同环 fatal + 无超时契约文档化 + 阻塞警告 + ~40-60 行。
- 双树 ctest + ASAN。

### W5 判据（G7）

- SV1：PIT-33 哨兵消费者破坏清单交付。
- SV2：Task 类型零改动（grep kSkipOnShutdown/kBlockOnShutdown = 0）。
- SV3：dtor 仍 drop-pending（PIT-33 测试全绿）。
- SV4：stop_and_wait 用例（限时 + 超时）。
- SV5：双树 + ASAN + check.sh。

### W6 判据（G4）

- SV1：then/finally/when_all/when_any + 错误传播。
- SV2：两命名上下文（loop / thread_pool），零 executor 框架。
- SV3：README 组件表恢复 future 行 + .pc 同步。
- SV4：需求触发（有真实消费方）才启动。

---

## 七、关键 file:line 索引（执行时先复核）

- signals.hpp：组合器 1519/1541、SignalR 2654、SignalBase 1582、ConnectionBlocker 694（当前 ~717）、
  Connection weak_ptr 806、Observer 858、tracked 槽 1220、connect_extended 1187/1264、
  connect_once 1943、ScopedBlock 2673、signal-wrapper 622/2623、operator() 直调 1653
- **connect_queued.hpp:69** — 家族生发地（H1），返回 signals::Connection；G1 形状裁定后
  家族在此生长（变体/重载/weak token 载体）
- event_loop.hpp：ProcessFlag :46-54（kAllEvents / kExcludeUserInputEvents:49 /
  kExcludeSocketNotifiers:50 / kX11ExcludeTimers:52 / kDialogExec:54——四值死枚举 D2+OQ2）、
  process_events :79/:89、exec :125、start_timer :110
- **event_loop.cpp:379** purge_pending（Event 队列清除；**mPostQueue 无 purge 路径——H9 gap**）
- event_loop_p.hpp:178 mPostQueue（std::deque<std::function<void()>>，裸闭包无清除接口）
- uv_event_dispatcher.cpp:238-244（嵌套 FATAL）、:309（uv_timer interval 直传）
- thread/task_queue.hpp:236 post_delayed_task、thread_pool.hpp:86-93 Priority 五档（ThreadPool 五档 kLowest/kLow/kNormal/kHigh/kHighest——上游 webrtc TaskQueueFactory 三档的超集，cxxkit 自有；task_queue.hpp 无 Priority 枚举）
- thread/future.hpp（**30 行空壳，修订后直接删除**——G4 立项证据 + D7/R3）
- task_queue_thread.cpp destroy() -> join -> process_tasks() worker 退出清空 pending（:308）；dtor 本体为空（:121-123）——G7 现状证据 + PIT-33
- **测试消费者迁移钉**：tst_event_loop.cpp:31（include）/ :185/192/211/216/228/234；
  exp_qt_embed.cpp:29（include）/ :70/73

---

## 八、风险与消解

| 风险 | 消解 | 关联任务 |
|------|------|---------|
| R1: queued closure UAF | W3 预算 receiver-liveness token（H9，SafetyFlag 先例） | W2 草图 + W3 实现 |
| R2: G7 flag 形态破 PIT-33 消费者 | stop_and_wait() 方法形态 + 破坏测试枚举清单（D-2/H6） | W5 |
| R3: future.hpp 空壳 = README 谎言 | 删壳 + README/pc 同步（D-7） | W1-D8 |
| R4: G6 复活时残废形状 | 墓志铭写不可移植 + 复活条件（D-1） | W1-D9 + §四 |
| R5: W3 连接 API 返工 | G1+G3 形状合并裁定前置（D-8） | W2 -> W3 |

---

> **定稿记录（2026-09-17）**：修订版经逐项审核 12 项全部通过。三项定稿微修已写回：① HC5 预埋（W2 文档强制含 G4 续体上下文小节，OQ3）② G3 fallback 路径显式化（五轴裁定不成立时 G3 可独立降级，W2 文档写明）③ W5 门措辞改为"需要优雅退出的真实组件出现时"。三扇 OQ 门全部裁定开门：OQ1=(b) kernel 层、OQ2=(a) 全删、OQ3=维持门+预埋。

**每波执行沿用既定纪律**：SDD 流水线 + TDD + Momus 审核 + 全门禁
（uv/asio 双树 ctest + ASAN + clang-format + C4 + check.sh）。
