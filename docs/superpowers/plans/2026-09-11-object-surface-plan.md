# Object 能力面补全计划（B+C 组 + EventLoopThread + Application）

日期：2026-09-11 ｜ 前置：D33/D34/D35（Object 树/事件投递/线程亲和已落地，81 套件基线）
来源：Qt 6.11 / UE 5.x 差距分析（2026-09-11 会话，三路团队调研），用户裁定全选 + 新增 ELT。

## 裁定记录（用户 2026-09-11 交互确认）

- **R1** B 组六件套全做：B1 register_event_type / B2 object_name / B3 find_child / B4 Object 级 timer / B5 dump_object_tree / B6 user_data
- **R2** C1 removePostedEvents 公有化 + C2 DeferredDelete 压缩 + C3 事件优先级 + C4 Application 复活（Qt 式完整）
- **R3** EventLoopThread 复活（组合式）：落 cxxkit/thread，构造期持 EventLoop，`move_to_thread(EventLoop*)` 签名不变（kernel→thread 依赖环禁止，用户裁定组合式）
- **R4** user_data 内存语义：Object 拥有（Chromium SupportsUserData 形，析构自动释放）
- **R5** Application 形态：instance() + virtual notify() 漏斗 + 全局 filter + exec() 主环（ctor 注入 dispatcher 工厂）

## 依赖与排序

```
T0(B1)  T1(B2+B5)  T2(B3)  T3(B6)        ← 批次1，互相独立可并行
T4(B4)  T5(C1+C2)                          ← 批次2（B2/B3 完成；队列小改）
T6(C3 优先级)                              ← 批次3（须在 T5 后：压缩先落，优先级再动队列结构，避免二次手术）
T7(C4 Application)                         ← 批次4（须在 T6 后：notify 漏斗落在最终投递语义上）
T8(ELT)                                    ← 任意批次并行（thread 模块，与 kernel 改动零重叠）
T9 文档/记忆收口                            ← 最后
```

硬约束（全程）：
- C++11（禁 auto 局部/泛型 lambda/if constexpr）；`CXXKIT_CHECK` 复合条件整体括号（PIT-45）
- 依赖方向 base→…→text→time→thread→tools→network；thread→kernel 允许，kernel→thread 禁止
- kernel 禁依赖 text（B2 object_name 用 `std::string`，非 cxxkit::String）
- kernel → tools/checks 既有依赖允许（CXXKIT_CHECK 现用）；dump 用 `fprintf(stderr,...)` 不引 logging
- clang-format（pixi）禁碰 CMakeLists；新头 `#pragma once` + 箱式横幅 + doxygen 英文（C18）
- pimpl 用 `CXXKIT_D`（静态函数内不可用——Momus F3 教训）；队列改动后 ASAN 全量复验
- EventEntry 持裸指针所有权（R1/D34 锁内 pop 锁外派发纪律不动摇）

## T0 — B1 Event::register_event_type

- Modify: `cxxkit/kernel/event.hpp/.cpp`
- 语义：`static int register_event_type(int hint = -1);` — 原子计数器从 kUser(1000) 起；hint 在 [kUser,kMax] 内则用 hint 并登记占用（防碰撞），冲突或耗尽返 -1（Qt 同款）。实现：cpp 内 static std::atomic<int> + static std::set<int> 占位表（mutex 保护 hint 分支；无 hint 快路径仅原子递增）。
- Test: 并发注册唯一性（多线程 hint+auto 混合 1000 次无重复）、耗尽返 -1（用假 kMax 边界不可测则测 hint 冲突 + auto 递增序列）、hint 冲突返 -1。
- Commit: `feat(kernel): add Event::register_event_type (Qt parity)`

## T1 — B2 object_name + B5 dump_object_tree

- Modify: `cxxkit/kernel/object.hpp/.cpp` + `detail/object_p.hpp`
- 语义：`const std::string &object_name() const; void set_object_name(std::string name);`（object.hpp 补 `#include <string>`——kernel 现无此 include，Momus F5；ObjectPrivate 挂 mObjectName，移动不迁移名称——名称随对象不随亲和）。`void dump_object_tree(int indent = 0) const;` 递归 stderr 打印 `{name} {typeid(Class).name()}` + children 缩进（Qt dumpObjectTree 对齐；无 moc 用 typeid）。
- Test: set/get 往返、子树 dump 输出包含两级名称与缩进（重定向 stderr 捕获断言）、空名默认打印。
- Commit: `feat(kernel): object_name + dump_object_tree`

## T2 — B3 find_child / find_children

- Modify: `cxxkit/kernel/object.hpp`（header-only 模板，放 object.hpp 类内）
- 语义：
  - `template <typename T> T *find_child(const std::string &name = std::string(), bool recursive = true) const;` — 先序 DFS，dynamic_cast 匹配 + 名称过滤；recursive=false 仅直接 children。
  - `template <typename T> std::vector<T *> find_children(const std::string &name = std::string(), bool recursive = true) const;` — 全部命中。
- Test: 直接子/深孙命中、类型不匹配跳过、名称过滤、recursive=false 边界、空树。
- Commit: `feat(kernel): find_child/find_children typed tree search`

## T3 — B6 user_data（Object 拥有）

- Modify: `cxxkit/kernel/object.hpp/.cpp` + detail
- 语义：`class Object::UserData { public: virtual ~UserData(); };`（非虚基类，调用方派生）；`void set_user_data(const void *key, std::unique_ptr<UserData> data);`（null key fatal；同 key 覆盖旧值销毁；null data = 移除）；`UserData *user_data(const void *key) const;`。存储：ObjectPrivate `std::map<const void *, std::unique_ptr<UserData>>`；~Object 自动释放（R4）。
- Test: set/get/覆盖（旧析构调用计数）、移除、~Object 级联释放（析构计数）、nullptr data 语义。
- Commit: `feat(kernel): type-keyed user_data attachment (Chromium SupportsUserData shape)`

## T4 — B4 Object 级 timer

- Modify: `cxxkit/kernel/object.hpp/.cpp`
- 语义：`int start_timer(uint64_t interval_ms, bool repeat = true);` / `void kill_timer(int timer_id);` — 经 `thread()` 亲和环注册（EventLoop::start_timer 处复用），回调内**同步直发** `send_event(this, TimerEvent(id))`（非 Qt 队列入列——不过优先级/不经队列压缩，doxygen 声明语义差异）；无亲和 fatal（与 post_event 一致）；**须在对象亲和线程调用**（Qt 同约束 + uv timer 非线程安全——CHECK(current() == thread())）。timer_event 虚函数已声明（D34 前遗留），event() 分派已含 kTimer 路由则接线，缺则补。
- **生命周期契约（Momus A1，防 UAF）**：ObjectPrivate 记 active timer ids；~Object 亲和环线程 best-effort kill 全部；doxygen 同步声明 "析构须在亲和线程，或先 kill_timer，违者 UAF 自担" 双轨。dispatcher 侧 repeating 注册不被队列 purge 覆盖——delete_later 不杀 timer，仅 ~Object kill。
- Test: 定时触发 timer_event 收到正确 timer_id（FakeDispatcher 手动泵）、无亲和 fatal、kill 后不再触发、repeat=false 单发。
- Commit: `feat(kernel): object-level start_timer/kill_timer routing to timer_event`

## T5 — C1 remove_pending_events 公有化 + C2 DeferredDelete 压缩

- Modify: `cxxkit/kernel/event_loop.hpp/.cpp` + `detail/event_loop_p.hpp` + object.cpp（delete_later）
- 语义：
  - C1：`static void EventLoop::remove_pending_events(Object *receiver);` — 全环扫描删除 receiver 的排队条目（事件 delete、不派发；锁内；Qt removePostedEvents 对齐，doc 明示" receiver 析构外调用需自担生命周期"）。
  - C2：delete_later 入队路径查重（Momus F1）：扫描住 `enqueue_event` **锁内** push_back 之前——`mEventQueue` 中存在 `mReceiver == receiver && mEvent->type() == kDeferredDelete` → delete 新事件不入队（保留旧条目位置，Qt postEvent 压缩同款）；object.cpp 侧仅调用，无锁扫描禁止（TOCTOU）。O(n) 扫描仅 delete_later 路径；被压缩时 wake_up 保留（幂等无害）。
- Test: remove 后事件不派发且不泄漏（计数器）；双 delete_later 单次删除（析构计数=1）；压缩后 stop_timer 式 remove 仍正确。
- Commit: `feat(kernel): public remove_pending_events + DeferredDelete compression`

## T6 — C3 事件优先级

- Modify: `cxxkit/kernel/event.hpp`（Event 无改）+ event_loop_p.hpp（EventEntry 加 `int mPriority`）+ event_loop.cpp + object.cpp（post_event 签名）
- 语义：`post_event(receiver, event, int priority = 0)` — 有符号，大者先派发，同优先级严格 FIFO（稳定排序）。队列策略：**有序插入仅住 enqueue_event 单点**（与 C2 扫描同函数同锁路径；std::deque 按优先级降序找第一个严格更小者前插，同值追加在后——全默认 0 时恒退化为 push_back，逐字节等价旧行为），pop 侧零改动（锁内 pop 头部纪律不变）。take_events_for 迁移条目**按扫描序 push_back**（Momus F2：源环已有序，拼接即保序，禁重排）；~EventLoop/~Object purge 不受影响（全量清）。**不引入新容器**（deque + 线性插入——备案 ponytail 上限：若 profiling 证明 post 热点，升级为优先级桶）。
- Test: 高优先级先派发、同优先级 FIFO、默认 0 与旧测试全兼容（既有 81 套件零改动全绿为硬门禁）、跨优先级 delete_later 压缩仍正确、迁移后顺序保持。
- Commit: `feat(kernel): event priority (stable sorted insert, FIFO within priority)`

## T7 — C4 Application 复活（Qt 式完整）

- Modify: `cxxkit/kernel/application.hpp/.cpp`（现注释态 7 行）+ object.cpp/event_loop.cpp（投递漏斗接线）
- 语义：
  - `class Application : public Object` — 单例：`static Application *instance();`；ctor `Application(std::function<std::unique_ptr<AbstractEventDispatcher>()> factory, Object *parent = nullptr)`；dtor 清 instance（doc：须最后析构，Qt qApp 同约束）。
  - 主环：构造期创建 `EventLoop mMainLoop`（factory 注入）；`int exec();` / `void quit();`（转发主环）。
  - notify 漏斗（Momus F3，单一漏斗点防递归）：现有 send_event 体改名 `send_event_core`（internal static，无 instance 检查）；public `send_event` 只做漏斗分派 `app ? app->notify(receiver, event) : send_event_core(...)`；`notify` 默认实现 = **自身 filter 链**（App 上 install_event_filter 的对象即全局 filter，Qt 同机制）→ `send_event_core`。事件队列派发（process_events）调用 public send_event → **零改动自动过漏斗**。无 App 行为零变化（instance() 判空短路）。覆写 notify 内再调 send_event 同 receiver = 用户级递归（Qt 同责，doxygen 声明）。
  - 全局 filter 顺序：App filters（LIFO）→ receiver filters（LIFO）→ receiver->event()（Qt 对齐）。
- Test: 无 App 时 send/post 全旧行为（81 套件零改动）；App 存在时 notify 覆写可达；全局 filter 拦截任意 receiver 事件；App filter + receiver filter 叠加顺序；exec/quit 主环；instance 生命周期（dtor 后回 null）。
- Add: `tests/tst_application.cpp` + tests/CMakeLists.txt 注册（cxxkit_add_test）（Momus A2）
- Commit: `feat(kernel): Application revival (instance + notify funnel + global filters + main loop)`

## T8 — EventLoopThread 复活（组合式，cxxkit/thread）

- Modify: `cxxkit/thread/event_loop_thread.hpp/.cpp`（现死骨架 30 行）+ README 文档债
- 语义：
  ```cpp
  class EventLoopThread final : public Object {
  public:
      explicit EventLoopThread(std::function<std::unique_ptr<AbstractEventDispatcher>()> factory,
                               Object *parent = nullptr);
      ~EventLoopThread() override;          // 未 stop 则 stop
      EventLoop &loop();                     // 构造期创建（R3：亲和绑定目标先行）；**裸成员直持、parent=nullptr**（Momus F4：挂 ELT 父下 = member+级联双所有权 + 构造期 ChildEvent 派发噪音）
      void start();                          // 平台线程跑 mLoop.exec()（重复 start fatal）
      void stop();                           // exit + join（未 start no-op）
      bool is_running() const;
  };
  ```
  用法（doc 示例）：`EventLoopThread elt(factory); obj.move_to_thread(&elt.loop()); elt.start();` — 亲和绑定在 start 前完成（D35 静态迁移契约天然满足：loop 未运行）。线程载体走 `platform_thread` 抽象（thread AGENTS 反模式：禁裸 std::thread；is_running/is_finished/wait API 面够用——Momus F4 核实）。stop() 的 exit() 跨线程合法（wake_up 线程安全契约，D35 I1）。
- Add: `tests/tst_event_loop_thread.cpp` + tests/CMakeLists.txt 注册（cxxkit_add_test）（Momus A2）
- Test: start/stop 往返、post 任务跨线程执行、亲和对象在 ELT 环上 delete_later 跨线程回收（复用 D35 I1 用例形态）、未 stop 析构安全、重复 start fatal、stop 未 start no-op。
- Commit: `feat(thread): revive EventLoopThread (composition over kernel EventLoop)`

## T9 — 收口

- README：thread 行补 EventLoopThread；kernel 行补新 API 关键词；example 无需新增（exp_kernel 可选加 find_child 段——从简则跳过）
- `.agents/memorys/`：decisions.md D36（差距分析裁定 R1-R5 + C3 备案上限 + Application 契约）+ pitfalls（如有新坑）+ status.md
- Commit: `docs: object surface completion records (D36)`

## 不变量（验收门禁）

1. 既有 81 套件**零改动**全绿（每 Task 提交前 `ctest --test-dir build`；T5/T6/T7 后额外 ASAN 全量零诊断——队列/投递语义改动红线）
2. T6 后 post 零优先级调用 = 旧行为逐字节等价（顺序敏感测试不迁移不放松）
3. T7 无 App 路径 = 旧行为（漏斗判空短路）
4. 全库 grep 无 `auto ` 局部推导新增 / 无 kernel→text/thread 反向 include
5. coverage 不低于 80%（新文件各自 >85%）

## 已知限制（备案，非遗漏）

- C3 线性插入 O(n)：post 高频热点场景需优先级桶升级（ponytail 备案）
- T4 timer 须亲和线程调用（uv 约束传导）；TimerType（coarse/precise）未纳入（C5 驳回本次）
- T7 Application 无 spontaneous 事件源、无 removePostedEvents(type) 过载（按需再取）
- T8 ELT 的 loop() 引用生命周期 = EventLoopThread 本体（doc 声明；WeakPtr 组合可解，不强制）
- T4 timer 回调为同步直发（非 Qt 队列形态）：不过优先级、不经压缩——语义差异成文
