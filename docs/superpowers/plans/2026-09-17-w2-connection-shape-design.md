# W2 设计文档：G1+G3 连接形状联合裁定（connect_queued 家族 × Object↔signals 组合）

- 日期：2026-09-17
- 状态：W2 交付物（设计裁定，**零代码**；W3 实现的唯一样板）
- 上游：`docs/superpowers/plans/2026-09-17-signal-loop-gap-plan.md`（修订版 + 定稿记录）——本文件实现其
  W2 节判据 SV1-SV4，并预埋 HC5（G4 续体上下文）小节
- 基线：D45 工作树（e7e1437）。所有 file:line 已按当前树复核
- 纪律约束：C++11；D33 非侵入（组合禁继承）；PIT-40 回调局部拷贝纪律；
  **signals.hpp EventLoop 零知识防火墙**（W3-SV1 grep 判据）；每轴一个答案，禁"都行"

---

## 〇、裁定总览（一屏版）

| # | 轴 | 裁定 |
|---|-----|------|
| 1 | G1 词汇落点 | 家族原地生长于 `kernel/connect_queued.hpp`；signals.hpp 唯一允许 diff = `ObserverBase::add_connection` private→public（零 EventLoop 字样） |
| 2 | HC2 weak loop token | `EventLoopPrivate` 持 `shared_ptr<atomic<bool>>` 令牌，**自然过期**（private 析构即过期，无手工 flip）；emit 侧检查，死环静默 skip |
| 3 | HC9 receiver purge | **(a) 闭包内嵌 liveness token**（SafetyFlag 先例）；弃 (b) loop-owned registry + 队列 purge（mPostQueue 手术不划算） |
| 4 | G3 组合形态 | `ObjectPrivate` 内嵌 `signals::observer mConnections`（**mutex 版**，非 observer_st）+ `Object::add_connection` 公有转发 |
| 5 | G3 eager 释放插入点 | `destroying()` 之后、`purge_pending` 之前（Qt destroyed→disconnect 同构） |
| 6 | move_to_loop 过期环 | **显式环绑定不重定向**——queued 连接钉死 connect 时的 loop 参数，不随 receiver 迁移 |
| 7 | 锁模型张力 | 单向嵌套不变量：signal 锁 → observer 锁，永不反向 |
| 8 | 过期环 pending delivery | `~EventLoop` 排空体已执行闭包（令牌仍活）；emit→死环被令牌拦截 |
| 9 | G4 续体上下文 | 两命名上下文 `on_loop` / `on_pool`，站点显式参数，第三上下文须重开栅栏 |
| 10 | fallback | 五轴任一在 W3 review 不收敛 → G3 整体出局，connect_queued 家族（loop 参数形态）照发 |

---

## 一、G1 connect_queued 家族 v1 形状（HC1/HC2）

### 1.1 现状核对（SV4 对账输入）

`cxxkit/kernel/connect_queued.hpp:69-75` 现有唯一形态：

```cpp
template <typename... Args>
signals::Connection connect_queued(Signal<Args...> &sig,
                                   EventLoop *loop,
                                   typename type_identity<std::function<void(Args...)>>::type fn)
{
    CXXKIT_CHECK(loop != nullptr) << "connect_queued requires a loop";
    return sig.connect([loop, fn](const Args &...args) { loop->post(std::bind(fn, args...)); });
}
```

- **返回 Connection 的变体已存在**：返回类型就是 `signals::Connection`（connect_queued.hpp:69，
  由 `SignalBase::connect` 产出，signals.hpp:746）。"变体返回 Connection" 不是新增项，是现状——
  W3 只需在测试与文档中钉死该契约（`connected()/disconnect()/block()/blocker()` 全可用，
  tst_event_loop.cpp:216 已断言 `connected()`）。
- 现有槽闭包 `[loop, fn]` **裸捕获 loop 指针**——loop 亡后 emit = 对 loop 指针 UAF。这是 HC2 要关的洞。

### 1.2 家族 v1 签名（全量，kernel 层）

```cpp
// kernel/connect_queued.hpp — v1 家族全量（2 个公有重载 + 1 个 kernel 内部工具）

namespace detail
{
/// kernel 内部：liveness 令牌检查（lock → 未过期且未失效 = 活）。
/// 令牌统一形态 std::weak_ptr<std::atomic<bool>>：store(true) = 手工失效（Object 早翻），
/// 控制块析构 = 自然过期（EventLoop 路径，从不手工翻）。
bool token_alive(const std::weak_ptr<std::atomic<bool>> &token) noexcept;
} // namespace detail

/**
 * 形态 1（既有，签名不变，行为升级）：跨线程排队连接。
 * emit 侧：loop 令牌死 → 静默 skip（不再 post）；活 → post（值拷贝语义不变）。
 */
template <typename... Args>
signals::Connection connect_queued(Signal<Args...> &sig,
                                   EventLoop *loop,
                                   typename type_identity<std::function<void(Args...)>>::type fn);

/**
 * 形态 2（新增，G3 落地时的 Object-receiver 形态）：receiver 死亡双重防护——
 * ① sig.connect 槽登记进 receiver 的 observer（~Object eager disconnect_all，见 §三）；
 * ② 投递闭包内嵌 receiver liveness 令牌（receiver 死于 drain 前 → 投递时静默 skip，见 §二）。
 */
template <typename... Args>
signals::Connection connect_queued(Signal<Args...> &sig,
                                   EventLoop *loop,
                                   Object *receiver,
                                   typename type_identity<std::function<void(Args...)>>::type fn);
```

**明确不进 v1**（W3-SV3 边界，与计划 §G1 语义红线一致）：

- **Auto / BlockingQueued / Unique**：不进。Auto 语义 = invokeMethod 栅栏项；BlockingQueued 走
  G2 `call_and_wait`（OQ1 已裁定 kernel 层，W4）；Unique 待消费方。
- **SignalR / SignalUnsafe 重载**：不进。queued 投递是 fire-and-forget，无组合器语义可讲
  （跨环回传返回值 = G2 范畴）；`SignalUnsafe`（NullMutex）先天不能跨线程 emit，与 queued
  前提矛盾。v1 只接受 `Signal<Args...>`（std::mutex 版）。
- **ScopedConnection 返回变体**：不进。`ScopedConnection` 有来自 `Connection` 的 implicit
  转换构造（signals.hpp:819-826），调用方一行自包，无需家族膨胀。
- **重载消歧**：形态 2 的 `Object*` 占据第 3 参与形态 1 的 fn 类型（`std::function`）不冲突，
  模板实参推导无歧义；`type_identity` 非推导上下文延续既有手法（connect_queued.hpp:42-46）。

### 1.3 HC2 weak loop token 机制（设计 + 落点）

**事实**：EventLoop 今天没有任何 weak/self 句柄（grep `enable_shared|shared_from|weak`
在 event_loop.{hpp,cpp} / event_loop_p.hpp 全零命中）。`cxxkit/memory/weak_ptr.hpp:141` 的
`WeakPtrFactory` 是 Chromium 形（`shared_ptr<atomic<bool>>` 标志块，D33-T3），但 kernel 不链
memory 子库——**不引依赖，模式内联**。

**设计**（最小增量，触 2 个文件）：

```cpp
// event_loop_p.hpp（EventLoopPrivate 成员区追加一行）
std::shared_ptr<std::atomic<bool>> mAliveToken;   // 惰性创建于首次 alive_token() 调用

// event_loop.hpp（EventLoop 公有区追加）
/**
 * @brief 返回本环的 liveness 令牌（kernel 内部：connect_queued 家族的死环检查）。
 *
 * 令牌随 EventLoopPrivate 析构自然过期（shared_ptr 控制块消亡），无手工失效点——
 * ~EventLoop 排空体（event_loop.cpp:73 起）执行期间令牌仍活，被排空的闭包照常运行；
 * 排空结束后令牌过期，此后 emit 侧 lock 失败 → 静默 skip。
 */
std::weak_ptr<std::atomic<bool>> alive_token();
```

- **为什么不用 WeakPtrFactory 式手工 flip**：工厂的显式失效服务于"owner 成员死光前提前作废"
  （receiver 场景需要，见 §二）；loop 场景恰恰要**排空期间保持活着**（delete_later 闭包必须
  执行到，D33 排空不变量），自然过期 = 正确语义且少一个失效点。同一令牌形态、两种寿命策略。
- **检查站点 = emit 侧**（H2 原文语义）：槽包装器在**发射线程**上先 `token_alive(loop_token)`
  再 `loop->post(...)`；死环 → 不 post，直接 return（静默）。投递侧（闭包内）不查 loop 令牌——
  能进队的闭包，其队列随环排空（~EventLoop 无条件 drain，event_loop.cpp:73-90）。
- **诚实声明（已知窄窗）**：emit 线程"检查通过 → post"与 `~EventLoop` 并发时存在 TOCTOU 残窗
  （检查后、post 前环析构）。彻底关死需 `shared_ptr<EventLoop>` 所有权改造（API 破坏级，拒）。
  令牌把暴露窗从"环亡后的整个存活期"压缩到指令级窗口——与 webrtc SafetyFlag 同性质的最佳
  努力边界（PIT-40 家族：flag 压缩而非消灭竞态）。文档化为已知限制。
- **死环 = 静默 skip 的哲学依据**：与 tracked 槽一致——`slot_tracked::call_slot`
  （signals.hpp:1235-1242）`ptr.lock()` 失败 → `disconnect + return`，不抛不叫；W1-D5 契约句
  "dead -> skip, not throw"（signals.hpp 头注释待落）同源。connect_queued 的死环 skip 是同一
  句话在 loop 维度的投影。
- **每次 emit 成本**：一次 `weak_ptr::lock`（原子 refcount）+ 一次 acquire load；对比现状
  （裸指针直 post）零额外分配。PIT-40 合规：槽闭包经 signals COW 快照在锁外调用
  （signals.hpp 头契约"emission never invokes slots while holding the signal lock"）。

### 1.4 防火墙核验

- 本节全部新增落在 `connect_queued.hpp` / `event_loop.hpp` / `event_loop_p.hpp`（kernel 层）。
- `signals.hpp` 零触碰 → W3-SV1 `grep -n "EventLoop\|event_loop" cxxkit/kernel/signals.hpp`
  维持零命中（本设计复核：当前即零命中）。

---

## 二、HC9 receiver-liveness purge 设计

### 2.1 问题重述（H9 实证）

`mPostQueue` 是 `std::deque<std::function<void()>>`（event_loop_p.hpp:178），裸闭包无 receiver
身份、无清除接口；`purge_pending`（event_loop.cpp:379，实体 `remove_pending_events`
event_loop_p.hpp:138）只扫 `mEventQueue`。connect_queued 投出的闭包捕获用户 fn（典型再捕获
`this`）——**receiver 死于 drain 前 = 投递时 UAF**。G3 的 eager disconnect 只关"生产者"
（不再有新 emit 产生新 post），关不了"在途消费者"（已入队的闭包）。

### 2.2 两案对比

| 维度 | (a) 闭包内嵌 liveness token（SafetyFlag 先例） | (b) loop-owned registry + 队列 purge（D35 mWatching 先例） |
|---|---|---|
| 机制 | Object 令牌随闭包走，投递时检查，死则 skip | 队列元素带 receiver 标签；~Object 持锁扫队列摘除 |
| mPostQueue 手术 | **零**（deque<function> 原样） | 元素改型 `{Object* receiver, fn}` 或双队列——take_post_queue/drain/~EventLoop 排空全链改 |
| 每 post 成本 | 零（普通 post 不付账；只有 queued 连接付一次 weak 拷贝） | 每条 post 付打标签成本（含全部现有 post 消费者） |
| ~Object 成本 | 一次原子 store | 锁 mPostMutex + O(队列长) 扫描（与派发互斥） |
| 释放时机 | 死闭包在**下一轮 drain** 被跳过并随 pop 释放 | 死闭包在 ~Object 即刻摘除 |
| 正确性边界 | 亲和 receiver（收发同环线程）= **精确无竞**；跨环/离环 receiver = best-effort（与 SafetyFlag 同界） | 摘除与派发互斥下无竞；但与 emit 侧并发 post 仍需令牌或锁序论证 |
| 先例契合 | `thread/pending_task_safety_flag.hpp:46`（webrtc 移植，已 shipped） | `mWatching` 是注册表**摘链**（对象在己手），队列 purge 是**匿名容器内摘别人**——先例其实不同构 |

### 2.3 裁定：(a)，三条理由

1. **热路径零污染**。(b) 的标签成本由全体 post 消费者代付（tcp/udp/timer 快路径全在内），
   为一个组件级需求绑架 kernel 最热队列——不成比例。(a) 成本只落在选用 queued 连接的调用点。
2. **"慢性泄漏"不成立，H4 红线不破**。H4 禁的 lazy 是**信号槽永不释放**（死槽常驻 COW 链）；
   (a) 的死闭包最迟在下一轮 `process_events` 排空时被跳过并随 deque pop 释放——post 队列每轮
   清空（event_loop.cpp:168-178），滞留上界 = 一轮 drain 间隔，非泄漏。生产者侧由 G3 eager
   disconnect_all 关死（§三），token 只管在途瞬态。
3. **同环亲和 = 精确语义**。正常用法（receiver 亲和目标环）：投递在环线程、析构也在环线程
   （D35 亲和纪律），令牌检查与 flip 天然串行，无竞态、无窗口；跨环 receiver 退化为
   SafetyFlag 同款 best-effort，契约一句话讲清。

### 2.4 令牌形态（与 §1.3 同一机制，第二种寿命策略）

```cpp
// object_p.hpp（ObjectPrivate 成员区追加）
std::shared_ptr<std::atomic<bool>> mAliveToken;   // 惰性创建于首次 alive_token() 调用；false=活

// object.hpp（Object 公有区追加）
std::weak_ptr<std::atomic<bool>> alive_token() const;   // kernel 内部：queued 闭包 receiver 检查
```

- **与 loop 令牌的差异 = 手工早翻**：`~Object` **第一行**（A1 timer stop 之前）执行
  `mAliveToken->store(true, release)`。必须在最前：派生类成员在 `~Object` body 运行前已死，
  自然过期（private 析构时）太晚——闭包会在"派生成员已亡、令牌仍活"窗口投递。
  这正是 WeakPtrFactory 显式失效存在的理由（D33-T3 同款 flip 时机考量）。
- **投递侧检查**（住 connect_queued.hpp 的 posted 闭包内）：

```cpp
[receiver_token, fn, bound]( /* emit-time copies */ )
{
    if (!detail::token_alive(receiver_token)) { return; }   // receiver 已亡：静默 skip
    fn(bound...);
}
```

- **emit 侧**同时查 loop 令牌（§1.3）；两令牌职责正交：loop 令牌管"入队前"，receiver 令牌
  管"执行前"。
- **无 G3 也有逃生口**：形态 1（无 receiver 参数）用户可自带
  `PendingTaskSafetyFlag::create()` + `safe_task()`（thread/ 已有）包 fn——文档指引一句，
  与 I7 注释（connect_queued.hpp:59-61）既有建议衔接。
- **(b) 的复活条件**（备案）：若未来出现"死闭包必须即刻释放内存"的长队列场景
  （万级积压 + 内存敏感），再启队列标签化 + purge 钩子设计；当前一轮 drain 上界够短。

---

## 三、G3 Object↔signals 组合（HC3/HC4）——五轴全裁定

### 3.0 组合形态（HC3）

**裁定：`ObjectPrivate` 内嵌 `signals::observer mConnections`（`ObserverBase<std::mutex>`，
signals.hpp:899 的 mutex 特化——锁变体轴见 3.1）+ `Object::add_connection` 公有转发。**

```cpp
// object_p.hpp
signals::observer mConnections;   // 内嵌成员（非继承——D33 非侵入红线）

// object.hpp
/** @brief 将一条连接登记到本对象的 observer：~Object 时 eager disconnect_all。 */
void add_connection(signals::Connection conn);   // → d_func()->mConnections.add_connection(conn)
```

- **继承路径处死**（D33/计划 H3）：`Object : observer` = 改所有派生类的基类列表语义 +
  公开面污染，非侵入裁定不容重开。
- **signals.hpp 唯一 diff**：`ObserverBase::add_connection`（signals.hpp:881，现 private +
  friend SignalBase）**private→public**。理由：唯一现调用点在 `SignalBase::connect` 的
  observer-PMF 重载（signals.hpp:1736 `ptr->add_connection(conn)`），G3 的 lambda 槽无法借道
  PMF 重载；改一个访问说明符 = 最小 diff，且语义无害（把"任意连接挂到 observer、随对象死
  自动断"暴露为正交能力）。**零 EventLoop 字样，防火墙不破**。
- **每 Object 成本**：`std::mutex + std::vector<ScopedConnection>` ≈ 80B/对象。远控主机
  规模（对象千百级）无感；文档备案。
- 弃"ObjectPrivate 自建 mutex+vector"（零 signals.hpp diff 但重造 observer 轮子，违复用梯）。

### 3.1 轴一：锁变体（embedded member 类型）

**裁定：`observer`（std::mutex 版），弃 `observer_st`。**

理由：queued 连接的三个动作发生在不同线程——`connect_queued(..., receiver, ...)` 在连接线程、
emit 快照在发射线程、`disconnect_all` 在 receiver 析构线程（亲和线程或级联析构时的环线程）。
`observer_st`（NullMutex）在任一交错下都是数据竞态；其零锁收益（每对象省 40B mutex）撑不起
正确性缺口。与 `Signal = SignalBase<std::mutex>`（signals.hpp:2638）锁模型对齐，禁混搭。

### 3.2 轴二：析构插入点（eager disconnect_all 在五步序中的位置）

D34/D35 定稿的 `~Object` 五步序（object.cpp:87-133）：

```
① A1 timer stop（亲和线程守卫）
② destroying()           ← 预销毁锚点（虚派发只到 Object 层）
③ purge_pending ×2       ← 亲和环 + current() 残留
④ filter 双向自摘        ← mWatching / mFilters 两个 while
⑤ 级联 children + 摘父链
```

**裁定：disconnect_all 插在 ② 与 ③ 之间**（destroying 之后、purge_pending 之前），并伴随
receiver 令牌早翻于 **① 之前**（§2.4，两件事不同层：令牌是在途闭包的守卫，disconnect 是
槽链的守卫）。

理由：

- **为什么不在 destroying() 之前**：destroying() 是对象可观察行为的最后锚点（Qt
  `destroyed()` 信号的类比位）。在其之前断连 = 连"销毁前最后一次合法 emit 窗口"也掐掉，
  且虚派发只到 Object 层（D33），派生类无感知，无消费者受益——纯过度。
- **为什么不在 ③ 之后/⑤ 之前**：③④ 之间无派发、无 emit 机会（purge 纯锁内摘除），早晚无
  行为差；但**语义序应为"先关生产者、再清存量"**——disconnect（停新 emit→新 post）先于
  purge（清已投 event）是因果自然序，也把"disconnect 与 purge 谁保护谁"的审读负担降为一句。
- **必须早于 ⑤**：级联析构期间任何跨线程 emit 打到半毁 this = UAF；⑤ 前必须完成断连。
- Qt 对照：`~QObject` 先发 `destroyed()` 再全量 disconnect——②→disconnect 的相邻序是同构。

### 3.3 轴三：move_to_loop 过期环（stale pinned loop）

**裁定：显式环绑定，不重定向、不失效——`connect_queued` 的 loop 参数是连接级静态属性。**

设计答案分解：

- queued 连接在 connect 时把 loop 令牌 + loop 裸指针烤进槽闭包；`move_to_loop(receiver, dst)`
  （object.cpp:286）只迁移 receiver 的**亲和元数据与 Event 队列条目**（take_events_for），
  **不触碰信号槽的环绑定**。迁移后既有 queued 连接继续向原环投递。
- 这是特性不是缺陷：v1 语义 = "投递环是连接时**显式声明**的"（与 Qt Auto 连接"投递跟随
  receiver 当时线程"的动态语义刻意分岔——Auto 已在 §四栅栏）。静态绑定可预测、可审计，
  远控主控的会话投递路径（D42 TLS 编排）要的正是确定性。
- 调用方想要"亲和跟随"投递：用 `post_event` 通道（D35 已按 receiver->loop() 路由），
  或迁移后重连。文档一句话指引。
- D35 迁移机零改动——迁移路径不新增任何 signals 知识，`take_events_for`/kLoopChange 原样。
- **若 review 推翻此裁定**（要求亲和跟随）：实现代价 = 闭包改持 receiver 令牌并在 emit 侧
  反查 `receiver->loop()` 动态路由，成本可控但语义翻转为动态——**触发 §四 fallback 评估**。

### 3.4 轴四：锁模型张力（signal 锁 × observer 锁）

**裁定：单向嵌套不变量——唯一嵌套序 `signal mutex → observer mutex`，代码库任何路径禁止反向。**

论证：

- 现状嵌套点唯一：`SignalBase::connect`（observer-PMF 重载，signals.hpp:1736）先
  `add_slot`（signal 锁，signals.hpp:2170 cow_write）后 `ptr->add_connection`（observer 锁，
  signals.hpp:883）——signal→observer 向。
- G3 新增嵌套点：`connect_queued` 形态 2 同序（`sig.connect(lambda)` 完成后 →
  `receiver->add_connection(conn)`）——**顺序执行而非嵌套持锁**，比现状更弱，天然安全。
- 反向路径存在性检查：`ObserverBase::disconnect_all`（observer 锁内 `m_connections.clear()`）
  触发各 `ScopedConnection` 析构 → `SlotState::disconnect` → **不取** signal 锁
  （slot 断连走自身 SlotState 原子状态，signals.hpp:800-808 全 weak_ptr::lock + 状态位，
  无 SignalBase::add_slot 回调）——无 observer→signal 边。**不变量成立**。
- 第三个锁（EventLoop mPostMutex）永不与上两者嵌套：post 在两锁释放后发生（槽在信号锁外
  调用）；purge（mEventMutex）与 observer 锁同上不相遇（③ 步序中 disconnect 已返回）。
- PIT-40 合规复述：槽闭包 = signals 快照锁外调用；posted 闭包 = take_post_queue swap 锁外
  执行（event_loop.cpp:168-178）；两段锁外纪律 G3 全程未破。

### 3.5 轴五：过期环 pending delivery（环亡时在途闭包何往）

**裁定：三分岔全覆盖，无第四态。**

| 时点 | 行为 | 依据 |
|---|---|---|
| emit 时环已亡/正在亡 | loop 令牌 lock 失败 → 静默 skip，不 post | §1.3 emit 侧检查 |
| 闭包已入队，环开始析构 | **照常执行**——~EventLoop 第一动作即 drain post 队列（event_loop.cpp:73-90，"delete_later 闭包必须执行到"），此时 EventLoopPrivate 未析构、loop 令牌仍活；drain 中 receiver 令牌继续守 receiver 死亡（§2.4） | D33 排空不变量 |
| drain 期间闭包内再 post | 既有 L1 已知限制（静默丢失），G3 不新增语义 | event_loop.cpp:75 注释 |

即：**不存在"闭包入队后环亡而被丢弃"的路径**——drain 无条件执行；也不存在"环亡后新闭包
挤进亡环"——emit 令牌拦截。轴五闭合。

### 3.6 五轴 × W3 验收对账

| 轴 | W3 验证用例（进 tst_kernel_event_loop_ext / tst_object 扩展） |
|---|---|
| 锁变体 | 跨线程 connect + emit + ~Object 并发压测（ASAN 零诊断） |
| 析构插入点 | emit 中途 receiver 析构：已运行槽跑完（I7）、后续槽被 skip、disconnect 计数归零 |
| move_to_loop | 迁移后既有 queued 连接仍投原环（显式钉死用例）+ post_event 走新环（对照） |
| 锁模型 | 断连风暴（disconnect_all during emit）不死锁 |
| 过期环 | loop 亡后 emit 零 post（weak token death 用例，W3-SV2）+ 环析构 drain 执行在途闭包 |

---

## 四、G3 fallback 路径（定稿审核裁定，显式化）

**触发条件**：五轴中任一在 W3 实现期/review 期被证伪（最可能：轴三动态路由需求翻案、
或轴四发现未知反向锁边），且修复代价超出 W3 预算 → **G3 整体出局，不降级混发**。

fallback 模式下 W3 的形状：

| 项 | 正常 W3 | fallback W3 |
|---|---|---|
| connect_queued 形态 1（loop 参数 + weak token） | ✅ | ✅ 照发 |
| connect_queued 形态 2（Object receiver） | ✅ | ❌ 删 |
| HC9 receiver 令牌 | 自动（形态 2 内建） | ❌ 无自动路径；文档指引 `PendingTaskSafetyFlag` + `safe_task` 手工包（thread/ 已有，零新码） |
| ObjectPrivate observer / add_connection / signals.hpp 访问符 diff | ✅ | ❌ 全撤 |
| ~Object 插入点改动 | ✅ | ❌ 零改 |
| **W3-SV 判据变化** | SV1-SV7 全量 | **SV4（G3 组合）作废**；SV3（purge 零派发）改为"自带 SafetyFlag 的 opt-in 用例"（safe_task 死旗 → 零执行，ASAN 零 UAF）；SV1/2/5/6/7 不变 |
| 4 消费者迁移 | ✅ | ✅（四站点全是形态 1 消费者，见 §六） |

fallback 下 G3 的重开条件：五轴中被证伪的轴有了收敛答案（例如轴三翻案后给出动态路由 +
锁序完整论证），按新 W2 补遗重启。

---

## 五、G4 续体上下文预埋（HC5，OQ3 定稿：W4 维持门 + 本节预埋）

**裁定：续体执行上下文 = 站点显式参数，两命名上下文封顶——`on_loop`（环亲和）与
`on_pool`（thread_pool），无通用 executor 框架。**

W6 未来形状（仅钉上下文词汇，`then/when_all/finally` 本体不在本文件范围）：

```cpp
// W6 草形（预埋词汇，非 v1 API 承诺）
future<T> f = async_value(...);
f.then(on_loop(my_loop),   [](T v) { ... });   // 续体跳到 my_loop 环线程（connect_queued 同款 hop）
f.then(on_pool(my_pool),   [](T v) { ... });   // 续体进 my_pool 后台并行
```

- **为什么显式参数**：Qt `QFuture::then` 的 context 也是显式（QtFuture::context 对象）；
  隐式上下文 = "当前线程碰巧是谁"的幽灵语义，跨线程调试地狱。两个命名上下文各是零开销
  标签类型（`struct on_loop_t { EventLoop *loop; }` 形），无虚表。
- **为什么拒 executor 框架**：H5——两上下文覆盖全部真实需求（亲和跳变 + 后台并行）；
  通用 executor = 类型擦除层 + 生命周期难题（executor 比 future 先死谁管）+ 当前零消费者。
  YAGNI 栅栏与 §四"明确不做"同构。
- **栅栏条款**：第三上下文（如 `on_strand`、`on_timer`、自定义 executor 适配器）出现需求时，
  **必须重开本节栅栏**（修订本设计文档 + 用户裁定），禁止 W6 实现期顺手加。
- 与 G1 的关系：`on_loop` 的 hop 机制复用 connect_queued 的 loop 令牌 + post 通道
  （同款死环静默 skip）——W6 不重造令牌。

---

## 六、4 消费者迁移草图（HC8）

复核当前树：`tests/tst_event_loop.cpp:185/192`（用例 13）、`:211/216`（用例 14）、
`:228/234`（用例 15）、`examples/exp_qt_embed.cpp:70/73`。四站点全部是**形态 1** 消费者。

| 站点 | 现状 | 迁移动作 |
|---|---|---|
| tst_event_loop.cpp:192（ConnectQueuedDeliversOnLoopThread） | 3 参形态，断言非内联执行 + 环线程投递 + wake_up 计数 | **源形零 diff**（形态 1 签名不变 = 回归即证明 back-compat）；新增姊妹用例 `ConnectQueuedSkipsWhenLoopDead`：先析构 loop 再 emit → fn 零执行、无崩溃（W3-SV2 主证） |
| tst_event_loop.cpp:216（ConnectQueuedReturnsConnectionForDisconnect） | 断言 Connection 生死 + disconnect 停投递 + 零 wake | **零 diff**；disconnect-after-death 组合并入上条姊妹用例 |
| tst_event_loop.cpp:234（ConnectQueuedArgsCopiedNotReferenced） | emit 后改源值，断言投递值不变 | **零 diff**（值拷贝契约在令牌化重构后必须逐字节保持——本用例就是钉子）；G3 落地时追加形态 2 用例组（receiver death skip + observer 计数归零）挂本文件内 |
| exp_qt_embed.cpp:73 | 3 参形态，桥泵投递 `queued: 42` | **零 diff**；紧随其后的注释块补一句死环语义说明（loop 生命周期由 example 的栈序保证，令牌是纵深防御） |

"迁移"的真实含义：四站点以**零源码 diff 通过**证明家族生长未破既有契约（比改写更有力的
回归证据）；新增语义（weak token / receiver token / 形态 2）由新增用例承载。W3-SV5
"迁移并运行" = 四站点回归 + 新增用例全绿，双树。

---

## 七、W2 判据对账（SV1-SV4）

| SV | 判据 | 本文件落点 | 状态 |
|---|---|---|---|
| SV1 | 覆盖家族形状 + weak token + Object registry + eager 释放 + purge 草图 | §1.2/§1.3（家族+token）、§3.0（registry=observer 内嵌）、§3.2（eager 插入点）、§2（purge=token 方案及弃案） | ✅ |
| SV2 | Momus APPROVE | 本文档送审（独立环节） | 待审 |
| SV3 | W3 边界明确（Auto 是否进 v1、purge 落点） | §1.2（Auto/BlockingQueued/Unique/SignalR/SignalUnsafe 全部不进 v1）、§2.3（purge 落点 = 投递侧 token 检查，非队列手术）、§四（fallback 边界） | ✅ |
| SV4 | 与现有 connect_queued.hpp 契约一致 | §1.1 现状核对（返回 Connection 已存在）；值拷贝/I7 disconnect 语义/lifecycle 契约全部延续，§六零 diff 回归钉死 | ✅ |

### 触及文件清单（W3 实现预算，本阶段零改动）

| 文件 | 改动 | 量级 |
|---|---|---|
| `cxxkit/kernel/connect_queued.hpp` | 形态 2 重载 + `detail::token_alive` + 头注释（死环/receiver 死语义、已知窄窗、SafetyFlag 逃生口） | ~80 行 |
| `cxxkit/kernel/event_loop.hpp` / `detail/event_loop_p.hpp` | `alive_token()` + `mAliveToken` | ~15 行 |
| `cxxkit/kernel/object.hpp` / `object.cpp` / `detail/object_p.hpp` | `add_connection` 转发 + `mConnections` observer 成员 + `mAliveToken`（含 ~Object 首行 flip + disconnect_all 插入） | ~30 行 |
| `cxxkit/kernel/signals.hpp` | `ObserverBase::add_connection` private→public（**唯一** diff，零 EventLoop 字样） | 1 词 |
| `tests/tst_event_loop.cpp` + 新用例组 | 死环/receiver 死/形态 2/迁移轴用例 | ~150 行 |
| `examples/exp_qt_embed.cpp` | 注释补句 | ~3 行 |

风险对账（计划 §八）：R1（queued closure UAF）由 §二 token 方案闭合；R5（W3 API 返工）由
本文件形状钉死消解——五轴 + 家族签名 + fallback 全部前置裁定完毕。
