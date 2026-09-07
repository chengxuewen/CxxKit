# CxxKit 事件循环与信号槽设计（EventLoop + Dispatcher + Queued Signal）

**日期**: 2026-09-04
**状态**: 设计定稿（brainstorming 产出，用户逐节确认）；2026-09-07 A1 同步契约节（Glossary/I1-I7/生命周期/线程矩阵/附录 A-C，见 git log）
**关联**: kernel 子库现有骨架（event_loop.* / event.* / object.* / signals.hpp）

---

## 1. 目标与非目标

### 目标
1. **补全 kernel 的 EventLoop**：从空壳骨架做成真实可跑的 Qt 风格事件循环（exec / process_events / 跨线程 post / 定时器）。
2. **嵌入宿主循环**：典型场景——Qt 应用宿主进程里，cxxkit 的定时器/post 任务挂在 Qt 循环上跑，不另起线程。
3. **跨线程信号投递**：`connect_queued(signal, loop, fn)` 自由函数——emit 线程参数拷贝后投递到目标 loop 执行。

> 术语速查：**EventLoop** = 用户面壳（你调用的）；**Dispatcher** = 构造时注入一次的驱动接口（详见 §3.5 Glossary）。

### 非目标（一期明确不做）
- `register_socket_notifier`（IO 多路复用）——一期不进接口（D2），完整签名记录在 §9 二期行
- Qt 式 `ConnectionType` 连接参数 / thread affinity / 自动断连（signals.hpp 不动）
- 对象级 `postEvent(Object*, Event*)` 投递通道（骨架已留，后续补）
- BlockingQueuedConnection（同步等待语义）
- 异常安全：post/start_timer 回调中抛出异常 = 异常不穿越事件循环，进程终止（abort）；timer 与 post 同契约；排空循环不因单回调异常中断（一期不做异常安全保证）（D6/A5）

## 2. 调研结论支撑（浓缩）

两类调研（librarian 并行，2026-09-04）：

**信号槽三形态**：① 纯同步 sigslot（pal::signal=palacaze/sigslot，我们 signals.hpp 上游；Boost.Signals2）② queue 化 signal（eventpp EventQueue 的 enqueue/process 两阶段）③ 框架级 queued connection（Qt ConnectionType / CsSignal ConnectionKind）。本项目：已有①，选②形态的自由函数包装（connect_queued），拒绝③（需要 thread affinity 概念，改 signals.hpp + Object，工作量大 2-3 倍）。

**事件循环嵌入共识骨架**：接口最小集 5 虚函数（process_events/wake_up/interrupt/start_timer/stop_timer）；双向唤醒（组件→宿主 = eventfd 注册进宿主循环；宿主→组件 = 宿主每轮调 process_events 非阻塞形态）；定时器 = 宿主一等源或折算 poll 超时，无人用轮询。Qt 官方嵌入范例 QEventDispatcherGlib 的关键设计：**dispatcher 持有宿主 context 指针而非自建**。

**libuv 选型**：跨平台（Linux eventfd/epoll、macOS kqueue、Windows IOCP）自己实现 = 3 套平台代码 500+ 行且只有 Linux 一台机器能验证（纯盲写，违背验证诚实红线）。libuv 15 年工业验证，Chromium/Node 同款风险模型——vendored 成熟库，只在 Linux 上验证真实行为。

## 3. 架构总览

```
cxxkit::kernel（零平台代码，零新依赖）
├── EventLoop                    — 壳：exec/process_events/post/start_timer
│                                  （骨架 API 签名不动，填实现=委托 dispatcher）
├── AbstractEventDispatcher      — 新抽象（5 虚函数驱动接口）
├── connect_queued(sig, loop, fn) — 跨线程信号包装（header-only）
│
cxxkit::uv（第 18 子库，CXXKIT_ENABLE_LIB_UV opt-in，vendored libuv）
└── UvEventDispatcher            — 独立循环场景默认驱动实现
│                                  uv_loop_t + uv_async_t（跨线程唤醒）+ uv_timer_t（定时器）
│
cxxkit::qt（第 19 子库，CXXKIT_ENABLE_LIB_QT opt-in）
└── QtEventDispatcher            — 嵌入 Qt 场景驱动实现
                                   持有宿主 QObject 上下文，post→invokeMethod 门铃，
                                   timer→QTimer（宿主一等源）
```

解耦规则：用 uv 子库不引 Qt；用 qt 子库不引 libuv（Qt 驱动直连 Qt）。

## 3.5 术语表（Glossary，两词制）

- **EventLoop**：用户面壳——你调用的对象。提供 exec/process_events/post/start_timer 等公开 API，内部全部委托 dispatcher。
- **Dispatcher**：构造时注入一次的驱动接口（`AbstractEventDispatcher`）——平台事件源的真实所有者；EventLoop 是句柄，dispatcher 是驱动。
- 定位句：`EventLoop is the user-facing handle; AbstractEventDispatcher is the driver interface; concrete drivers (e.g. Uv/Qt dispatchers) implement this.`
- 全文档两词制：不用"引擎/engine"一词——uv/qt 侧类名是 UvEventDispatcher/QtEventDispatcher，泛称一律"驱动/驱动实现"。

## 4. 接口设计

### 4.1 AbstractEventDispatcher（kernel）

```cpp
class CXXKIT_KERNEL_API AbstractEventDispatcher
{
public:
    virtual ~AbstractEventDispatcher() = default;

    // 处理事件。flags 复用 EventLoop::ProcessFlags（kWaitForMoreEvents 控制阻塞）
    // 返回 true = 处理了至少一个事件
    virtual bool process_events(ProcessFlags flags) = 0;

    // 跨线程安全：踢醒阻塞在 process_events 里的线程
    virtual void wake_up() = 0;

    // 让 process_events 尽快返回（exec 收尾）
    virtual void interrupt() = 0;

    // 定时器注册。回调在 loop 线程执行
    virtual void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) = 0;
    virtual void stop_timer(int timer_id) = 0;
};
```

- timer 用 id（与 EventLoop 公开 API 对齐，Qt startTimer 风格）
- **所有权**：EventLoop 构造时注入 dispatcher（`std::unique_ptr`）。kernel 只暴露注入接口、不依赖任何驱动子库（依赖方向：kernel ← uv/qt）；默认装配由启用方完成——uv 子库提供 `make_uv_dispatcher()` 工厂，消费方一行 `EventLoop loop(make_uv_dispatcher())`；嵌入场景本来就显式传驱动
- 线程矩阵（dispatcher 接口面）：

| API | 线程 |
|---|---|
| wake_up | 任意线程 |
| process_events / start_timer / stop_timer / 析构 | 仅 loop 线程（I1；析构另见 I6） |

### 4.2 EventLoop 公开 API（骨架增补）

```cpp
// 已有（填实现，委托 dispatcher）：exec/process_events/wake_up/exit/quit/is_running
// 新增：
void post(std::function<void()> fn);            // 任意线程可调
int  start_timer(uint64_t interval_ms, std::function<void()> fn, bool repeat = true);
void stop_timer(int timer_id);                  // id 由 start_timer 返回（原子自增分配）
```

### 4.3 post 数据流（竞态核心设计）

```
任意线程                                  loop 线程
   │ post(fn) ──► mutex 锁队列 push        │
   │ wake_up() ──► uv_async_send ────────► │  uv_async 回调（门铃）
   │  （send 幂等，多次合并）                │  └─► 锁内 swap 排空整队，锁外逐个 fn()
```

- 队列独立于 libuv（mutex + deque），uv_async 只当门铃；send 合并竞态由 libuv 处理
- 排空用 swap——回调里再 post 不会死锁
- Qt 版同构：队列 + QMetaObject::invokeMethod 门铃；门铃接收者的 thread affinity = cxxkit 的"loop 线程"（文档写明 context 必须属于目标线程）
- 线程矩阵（EventLoop 公开 API 面）：

| API | 线程 |
|---|---|
| post / wake_up / exit / quit | 任意线程 |
| process_events / exec / start_timer / stop_timer / 析构 | 仅 loop 线程（I1/I6） |

### 4.4 connect_queued（header-only，自由函数）

```cpp
template <typename... Args>
void connect_queued(Signal<Args...>& sig, EventLoop* loop, std::function<void(Args...)> fn)
{
    sig.connect([loop, fn](const Args&... args) {
        loop->post([fn, args...]() { fn(args...); });   // emit 时参数值拷贝一次
    });
}
```

- 参数值语义拷贝（Qt QueuedConnection 同款要求：类型须可拷贝）
- **生命周期保护明示差距**：无自动断连，文档要求"接收方销毁须先 disconnect"；后续可加 weak_ptr 版重载
- 与 PendingTaskSafetyFlag（已有，防异步 use-after-free）互补

## 5. qt 子库（嵌入形态）

```cpp
class CXXKIT_QT_API QtEventDispatcher : public AbstractEventDispatcher
{
public:
    explicit QtEventDispatcher(QObject* context = nullptr);  // 默认 QCoreApplication::instance()
    // ... 5 虚函数实现
};
```

| cxxkit 接口 | Qt 翻译 |
|---|---|
| post(fn) | 队列 push + invokeMethod 空参门铃（QueuedConnection），回调排空 |
| wake_up() | 同上门铃；mBellPending 标志自合并（一次排空前至多挂一个控制事件） |
| process_events(非阻塞) | QCoreApplication::processEvents() 直通 |
| process_events(阻塞) | processEvents() 直通（阻塞由宿主循环承担，诚实降级） |
| start_timer | 每定时器一个 QTimer（宿主一等源） |
| interrupt | 原子 flag + 门铃，process_events 提前返回 |

端到端用户视角：

```cpp
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    cxxkit::EventLoop loop(std::make_unique<cxxkit::QtEventDispatcher>());
    cxxkit::connect_queued(worker.on_data, &loop, [](Data d) { /* Qt 主线程 */ });
    loop.start_timer(100, [&]{ /* 周期任务 */ });
    // 嵌入桥：宿主侧周期调壳 process_events（qt 引擎约束——壳的 post 队列只有桥/显式调用会排空，见头文件注释）
    QTimer bridge;
    QObject::connect(&bridge, &QTimer::timeout, [&]{ loop.process_events(); });
    bridge.start(10);
    return app.exec();   // cxxkit 全由 Qt 循环驱动，零额外线程
}
```

## 6. 依赖与构建

| 子库 | 依赖 | 开关 | vendored |
|---|---|---|---|
| kernel | 无新增 | 既有 | — |
| cxxkit::uv | libuv（C，MIT） | CXXKIT_ENABLE_LIB_UV（默认 OFF） | libuv.7z + FindWrapLibuv（breakpad 同款链路：stamp + find_package + 门禁） |
| cxxkit::qt | Qt5/6 Core（系统包，**不 vendored**——体量；Qt 官方 CMake 即 Config.cmake，D11 天然兼容） | CXXKIT_ENABLE_LIB_QT（默认 OFF） | — |

安装/导出跟随现有模式：cxxkitConfig find_dependency 条件化（C11）、.pc 链、install_public_wrap_headers。

## 7. 测试矩阵（分层诚实）

| 层 | 验证方式 |
|---|---|
| kernel 抽象 + connect_queued + EventLoop | gtest 全自动（fake dispatcher 手动踢 process_events——imgui fake backend 先例） |
| UvEventDispatcher | gtest 全断言（真实 libuv；跨线程 post/timer/exec/exit；本机 Linux 可全验证） |
| QtEventDispatcher | CI 编译门禁（qt6-base-dev）+ `docs/qt-embed-smoke.md` 手工门禁（8 步：post 跨线程/定时器/嵌套 exec/退出/断连安全——用户有 Qt 环境时跑；imgui/crash 分层验证同款） |
| examples | exp_event_loop（uv 版，CI rc=0）+ exp_qt_embed（编译验证 + smoke 文档） |

## 8. 风险与开放点

1. **libuv wrap 搭建**是新 wrap（第 24 个 FindWrap）——成熟链路照抄 breakpad，风险低
2. **Qt 双版本（5/6）兼容**：接口面只碰 QCoreApplication/QObject/QTimer/invokeMethod，5/6 同 API，编译门禁装 Qt6 即可（文档注明 Qt5 未验）
3. **嵌套 process_events 语义**（Qt QEventLoop 嵌套）一期按 Qt 直通处理，语义等价性靠 smoke 验证
4. EventLoop 骨架的 `mInExec`/`mExit`/`mRetCode` 原子量已存在，exec 循环改成调 dispatcher——骨架兼容性实现期确认
5. **asio 驱动二期备忘**（2026-09-04 调研）：接口映射 3 原生（process_events/run_one_for、timer）+ 3 可绕或缺失（wake_up 无公开跨线程唤醒、interrupt 的 stop() 是放弃一切+需 restart 语义、socket_notifier 需 0-length read re-arm 状态机模拟、嵌入不公开 backend fd）。一期不做（libuv 两头占优）；二期触发条件：network 子库迁 asio / 协程需求 / Qt 循环跑 asio 回调。形态=并存不替代（uSockets 三后端先例），standalone asio pin 1.28.x~1.30.x 保 C++11 + extract-only vendored。**asio 准入判据（D15）**：契约级不变量零豁免——shim（翻译式实现：0-length re-arm、内部 async 通道）合法；须修改任何契约级 I 项文本 = 默认否决；能力面差异（嵌入/嵌套/socket_notifier）声明式披露。

## 9. IO/网络扩展分期路线图（2026-09-04 调研后补充）

**封装库选型结论**：直包 libuv C API，不引第三方封装——uvw 需 C++17（header-only 传染消费 TU，一票否决 C4/D3）；uv-cpp 休眠 5 年且网络栈与 cxxkit::network(cpr) 重叠；libuv 官方 LINKS 里的 C++ 使用者（Node.js HandleWrap/ReqWrap、mediasoup）全部自包 C API，Node 的薄 wrap 模式即范本。

| 期 | 交付 | 关键决策 |
|---|---|---|
| **一期**（本 spec） | EventLoop + 5 虚函数 dispatcher + uv/qt 驱动 + connect_queued | register_socket_notifier 不进一期接口（D2） |
| **二期** | `SocketNotifier` 实现（uv_poll 底座）+ `TcpSocket/TcpServer/UdpSocket/PipeStream`（仅 uv 子库） | SocketNotifier 构造**显式传 dispatcher**（asio 式，不照搬 Qt thread-local 隐式注册）；scope 限 fd-pollable（socket/pipe/tty）；回调接现有 cxxkit::signals（不引入新 emitter）；不加虚基类，构造传 dispatcher 为三期模板化留门；socket 注册 API 记录：`register_socket_notifier(int fd, SocketEventMask events, std::function<void()>)` / `unregister_socket_notifier(int fd)`——二期实现；需要时**新声明虚函数带默认空实现 + @since 标注**（源码级兼容而非 ABI 兼容） |
| **三期** | 跨驱动网络抽象 | **默认不做**——Qt 嵌入场景直接复用 QTcpSocket（零成本）；真要跨驱动走 asio 式模板化（编译期），不建第二层虚接口（dispatcher 已虚一层） |
| **不做** | 普通文件异步 IO | epoll 不支持磁盘文件；uv_fs 本质 threadpool（默认 4 线程）——异步文件 API = thread_pool 提交包装，非 loop 集成（Chromium base::File 纯同步同款共识） |

---

## 附录 A. 壳层包装裁定规则（strategist 三问 + 壳偏置）

新增壳层包装（EventLoop 之上的便利层）前依次回答：

1. **可组合性**：能否仅用契约原语（EventLoop 公开 API + 契约级不变量）表达？不能 → 上推到驱动实现（各驱动自己做）。
2. **不变量**：只依赖已成文的契约级 I 项？依赖未成文行为 → 先成文再下沉（先在本文档立契约，再写包装）。
3. **可观测漂移**：包装引入的延迟/乱序有界且小？改变量级 → 拒绝该包装。
4. **成本偏置**：默认壳——kernel 层测一次 vs 每驱动实现测 N 次；壳偏置（能进壳就不进驱动）。

壳层包装仅可依赖契约级 I 项；实现级 I 项由各驱动声明式披露（见各驱动文档）。

---

## 附录 B. 驱动契约不变量（I1-I7）

> 注：两词制下本节读作「驱动契约不变量」；「引擎」为 brainstorm/hyperplan 阶段历史用词，仅此处保留说明。

| # | 级别 | 内容 | 覆盖用例 |
|---|---|---|---|
| I1 | 契约级 | 单消费者：process_events/exec 仅 loop 线程；post/wake_up/exit 任意线程 | debug thread id 断言 + race_checker 先例 |
| I2 | 契约级 | kWaitForMoreEvents 下到期 timer 有界触发（D3 行为级） | timer 时序用例（宽松下界） |
| I3 | 契约级 | 回调内 start_timer/stop_timer 合法（M3 one-shot 壳包装的实现前提） | 回调内自停用例 |
| I4 | 契约级 | wake_up 可合并且门铃回调无条件锁内排空整队（含空队列虚警） | wake_up 风暴 N×M |
| I5 | 实现级 | 嵌套支持度是每驱动声明属性（uv 不支持→debug 断言；Qt=QEventLoop 直通） | 嵌套钉住用例 |
| I6 | 契约级 | 析构纪律：dispatcher/EventLoop 于 loop 线程或 loop 已停析构；uv_close 全 handle+收尾排空；Qt context 顺序（dispatcher 先亡） | 析构清理 ASAN 用例 |
| I7 | 契约级 | disconnect 停后续投递不追已入队（D5） | disconnect+在飞闭包 ASAN 用例 |

壳层包装仅可依赖契约级 I 项；实现级 I 项由各驱动声明式披露（见各驱动文档）。

---

## 附录 C. 生命周期契约（M5 四条）

1. **析构线程约束**：dispatcher/EventLoop 必须在 loop 线程析构，或 loop 线程已确认停止；析构后 post() = 未定义行为（UB）。
2. **uv 侧析构纪律**：uv_close 全 handle + 收尾 uv_run 排空 + 无在飞 send 保护（停止标志）——析构时不再有其它线程能触发 uv_async_send。
3. **Qt context 顺序**：dispatcher 必须先于 context 析构。声明顺序示例（局部变量逆序析构，先声明者后亡——dispatcher 声明在后即先亡，正确）：

```cpp
// 正确：dispatcher 先亡
QCoreApplication app(argc, argv);
cxxkit::EventLoop loop(std::make_unique<cxxkit::QtEventDispatcher>());  // 后声明 → 先析构 ✓
```

4. **connect_queued 存活约束**：loop 必须比 connection 活得久；loop 为 null → `CXXKIT_CHECK` 失败（fatal）。
