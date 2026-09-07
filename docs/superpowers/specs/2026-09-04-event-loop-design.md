# CxxKit 事件循环与信号槽设计（EventLoop + Dispatcher + Queued Signal）

**日期**: 2026-09-04
**状态**: 设计定稿（brainstorming 产出，用户逐节确认）
**关联**: kernel 子库现有骨架（event_loop.* / event.* / object.* / signals.hpp）

---

## 1. 目标与非目标

### 目标
1. **补全 kernel 的 EventLoop**：从空壳骨架做成真实可跑的 Qt 风格事件循环（exec / process_events / 跨线程 post / 定时器）。
2. **嵌入宿主循环**：典型场景——Qt 应用宿主进程里，cxxkit 的定时器/post 任务挂在 Qt 循环上跑，不另起线程。
3. **跨线程信号投递**：`connect_queued(signal, loop, fn)` 自由函数——emit 线程参数拷贝后投递到目标 loop 执行。

### 非目标（一期明确不做）
- `register_socket_notifier`（IO 多路复用——libuv 后端留了门，需求触发再加）
- Qt 式 `ConnectionType` 连接参数 / thread affinity / 自动断连（signals.hpp 不动）
- 对象级 `postEvent(Object*, Event*)` 投递通道（骨架已留，后续补）
- BlockingQueuedConnection（同步等待语义）

## 2. 调研结论支撑（浓缩）

两类调研（librarian 并行，2026-09-04）：

**信号槽三形态**：① 纯同步 sigslot（pal::signal=palacaze/sigslot，我们 signals.hpp 上游；Boost.Signals2）② queue 化 signal（eventpp EventQueue 的 enqueue/process 两阶段）③ 框架级 queued connection（Qt ConnectionType / CsSignal ConnectionKind）。本项目：已有①，选②形态的自由函数包装（connect_queued），拒绝③（需要 thread affinity 概念，改 signals.hpp + Object，工作量大 2-3 倍）。

**事件循环嵌入共识骨架**：接口最小集 4 虚函数（processEvents/wakeUp/interrupt/registerTimer）；双向唤醒（组件→宿主 = eventfd 注册进宿主循环；宿主→组件 = 宿主每轮调 process_events 非阻塞形态）；定时器 = 宿主一等源或折算 poll 超时，无人用轮询。Qt 官方嵌入范例 QEventDispatcherGlib 的关键设计：**dispatcher 持有宿主 context 指针而非自建**。

**libuv 选型**：跨平台（Linux eventfd/epoll、macOS kqueue、Windows IOCP）自己实现 = 3 套平台代码 500+ 行且只有 Linux 一台机器能验证（纯盲写，违背验证诚实红线）。libuv 15 年工业验证，Chromium/Node 同款风险模型——vendored 成熟库，只在 Linux 上验证真实行为。

## 3. 架构总览

```
cxxkit::kernel（零平台代码，零新依赖）
├── EventLoop                    — 壳：exec/process_events/post/start_timer
│                                  （骨架 API 签名不动，填实现=委托 dispatcher）
├── AbstractEventDispatcher      — 新抽象（4 虚函数）
├── connect_queued(sig, loop, fn) — 跨线程信号包装（header-only）
│
cxxkit::uv（第 18 子库，CXXKIT_ENABLE_LIB_UV opt-in，vendored libuv）
└── UvEventDispatcher            — 独立循环场景默认引擎
│                                  uv_loop_t + uv_async_t（跨线程唤醒）+ uv_timer_t（定时器）
│
cxxkit::qt（第 19 子库，CXXKIT_ENABLE_LIB_QT opt-in）
└── QtEventDispatcher            — 嵌入 Qt 场景引擎
                                   持有宿主 QObject 上下文，post→invokeMethod 门铃，
                                   timer→QTimer（宿主一等源）
```

解耦规则：用 uv 子库不引 Qt；用 qt 子库不引 libuv（Qt 引擎直连 Qt）。

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

- `register_socket_notifier(fd, Read/Write, fn)` / `unregister_socket_notifier(fd)`：**预留纯虚**（一期不实现，但接口进抽象——uv/qt/未来 asio 都留 override 点）。一期 scope 限 fd-pollable（socket/pipe/tty）；普通文件排除（epoll 不支持，异步文件走 thread_pool——libuv uv_fs 本质也是 threadpool，Chromium base::File 纯同步同款共识）
- timer 用 id（与 EventLoop 公开 API 对齐，Qt startTimer 风格）
- **所有权**：EventLoop 构造时注入 dispatcher（`std::unique_ptr`）。kernel 只暴露注入接口、不依赖任何引擎子库（依赖方向：kernel ← uv/qt）；默认装配由启用方完成——uv 子库提供 `make_default_dispatcher()` 工厂，消费方一行 `EventLoop loop(make_default_dispatcher())`；嵌入场景本来就显式传引擎

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
    // ... 4 虚函数实现
};
```

| cxxkit 接口 | Qt 翻译 |
|---|---|
| post(fn) | 队列 push + invokeMethod 空参门铃（QueuedConnection），回调排空 |
| wake_up() | 同上门铃 |
| process_events(非阻塞) | QCoreApplication::processEvents() 直通 |
| process_events(阻塞) | 短暂 QEventLoop（Qt 官方嵌套循环机制） |
| start_timer | 每定时器一个 QTimer（宿主一等源） |
| interrupt | 原子 flag + 门铃，process_events 提前返回 |

端到端用户视角：

```cpp
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    cxxkit::EventLoop loop(std::make_unique<cxxkit::QtEventDispatcher>());
    cxxkit::connect_queued(worker.on_data, &loop, [](Data d) { /* Qt 主线程 */ });
    loop.start_timer(100, [&]{ /* 周期任务 */ });
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
5. **asio 后端二期备忘**（2026-09-04 调研）：接口映射 3 原生（process_events/run_one_for、timer）+ 3 可绕或缺失（wake_up 无公开跨线程唤醒、interrupt 的 stop() 是放弃一切+需 restart 语义、socket_notifier 需 0-length read re-arm 状态机模拟、嵌入不公开 backend fd）。一期不做（libuv 两头占优）；二期触发条件：network 子库迁 asio / 协程需求 / Qt 循环跑 asio 回调。形态=并存不替代（uSockets 三后端先例），standalone asio pin 1.28.x~1.30.x 保 C++11 + extract-only vendored

## 9. IO/网络扩展分期路线图（2026-09-04 调研后补充）

**封装库选型结论**：直包 libuv C API，不引第三方封装——uvw 需 C++17（header-only 传染消费 TU，一票否决 C4/D3）；uv-cpp 休眠 5 年且网络栈与 cxxkit::network(cpr) 重叠；libuv 官方 LINKS 里的 C++ 使用者（Node.js HandleWrap/ReqWrap、mediasoup）全部自包 C API，Node 的薄 wrap 模式即范本。

| 期 | 交付 | 关键决策 |
|---|---|---|
| **一期**（本 spec） | EventLoop + 4 虚函数 dispatcher + uv/qt 引擎 + connect_queued | register_socket_notifier 只留纯虚预留位 |
| **二期** | `SocketNotifier` 实现（uv_poll 底座）+ `TcpSocket/TcpServer/UdpSocket/PipeStream`（仅 uv 子库） | SocketNotifier 构造**显式传 dispatcher**（asio 式，不照搬 Qt thread-local 隐式注册）；scope 限 fd-pollable（socket/pipe/tty）；回调接现有 cxxkit::signals（不引入新 emitter）；不加虚基类，构造传 dispatcher 为三期模板化留门 |
| **三期** | 跨引擎网络抽象 | **默认不做**——Qt 嵌入场景直接复用 QTcpSocket（零成本）；真要跨引擎走 asio 式模板化（编译期），不建第二层虚接口（dispatcher 已虚一层） |
| **不做** | 普通文件异步 IO | epoll 不支持磁盘文件；uv_fs 本质 threadpool（默认 4 线程）——异步文件 API = thread_pool 提交包装，非 loop 集成（Chromium base::File 纯同步同款共识） |

行业共识佐证：循环绑定一律构造期决定；watcher 全行业向回调收敛（Chromium 亲手废了自己的虚接口 Watcher，迁到回调+RAII Controller）。

## 10. 实施顺序（预告，详细计划走 writing-plans）
