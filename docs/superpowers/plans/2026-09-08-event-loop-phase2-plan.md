# CxxKit 事件循环二期实施计划：SocketNotifier + TcpSocket + Timer 精化

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 落地 spec §9 二期路线——`SocketNotifier`（uv_poll 底座 + 二期虚函数声明）与 `TcpSocket/TcpServer`（memcached 状态机内核 + beast flat_buffer 形状），叠加 timer 精化小件包（ratas wheel/0ms 直投/aggregate connect/interval CHECK）。

**Architecture:** kernel 仅增 SocketNotifier 抽象 + 二期虚函数（带默认空实现+@since，D2 裁定形态）；实现全部住 uv 子库（跨引擎网络三期按需再议，spec §9 三期默认砍）；TcpSocket 不加虚基类，构造显式传 EventLoop（asio 式，为三期模板化留门）。

**Tech Stack:** C++11（库）/vendored libuv 1.49.2（已就位）/memcached 状态机范式 + beast flat_buffer 接口形状（移植扫描 A3+A5 结论）

**Spec:**
- 路线：`docs/superpowers/specs/2026-09-04-event-loop-design.md` §9（二期行/三期行/不做行）
- 移植依据：`docs/superpowers/specs/2026-09-08-porting-scan-report.md`（A3 状态机/A5 验收标准/规避清单）
- 约束基线：`docs/superpowers/plans/2026-09-04-event-loop-hyperplan-bundle.md`（D1-D16/I1-I7 继续有效）
- 一期实现（只消费不可改，除 T1 授权点）：`cxxkit/kernel/abstract_event_dispatcher.hpp`、`cxxkit/uv/uv_event_dispatcher.*`

## Global Constraints（继承一期全部约束）

- 库代码 C++11 严格门禁；测试跟随主标准；尖括号 include（D8）；uv C API 用 `<cxxkit/3rdparty/libuv/uv.h>`（D6/P5）
- target 注册走 cxxkit_add_library/add_test/add_executable（C10）；clang-format 禁碰 CMakeLists
- 禁 rm -rf build（C6）；时序断言事件驱动禁 sleep（P3）；命令退出码为准（非 grep error_count）
- SDD 纪律：小块写+grep 自验+编译器裁判；单行 replace 多行 lines=插入事故（用 pos+end 范围）
- 每任务收口：build 0 error + 定向 ctest 绿 + format 干净 + 提交（身份 -c user.name=chengxuewen -c user.email=1398831004@qq.com）
- **I1-I7 不变量继续生效**：新 IO 回调同样受 I1（loop 线程）/I3（回调内 stop/start 合法）/I6（析构纪律）约束
- **移植扫描规避清单为硬约束**（porting-scan-report §D 规避设计）：状态转换必重挂兴趣标志/跨线程写必须 post 序列化/写背压只在 writable 回调写

## 任务分解

### Task T0（热身包·A）：timer/信号小件四合一

**Files:**
- Modify: `cxxkit/kernel/event_loop.hpp/.cpp`（0ms 直投分支）
- Create: `tests/tst_kernel_event_loop_ext.cpp`（新用例文件，避免触碰一期 tst_event_loop.cpp 的 15 用例基线）
- Modify: `tests/CMakeLists.txt`（注册）

**Interfaces:**
- Produces:
  1. `EventLoop::start_timer(0, fn, false)` → **直投队列**（等价 post(fn)，绕过引擎 timer 注册——Qt singleShotImpl 同款）；repeat=true 且 interval==0 → 仍走引擎（周期 0 = 每轮触发，Qt 语义）——报告裁定记录
  2. connect_accumulate（A7 nod aggregate）**不做**——与 connect_queued 异步语义冲突（R 值需 emit 栈同步可得）；已入 porting-scan 报告留档
  3. interval CHECK **不做**——qt 侧一期已修（C1 fix），uv 侧 uint64 原生无截断，无可做项
- 注意：0ms 直投路径返回的 timer_id 是**幽灵 id**（无引擎条目，stop_timer 对其 no-op）——one-shot 语义下无影响，头注释文档化
- mHadEvents 置位点文档化**整项挪入 T1**（同文件双写冲突，Momus F1 裁定）

- [ ] **Step 1: 失败测试**（tst_kernel_event_loop_ext.cpp）：ZeroShotTimerBypassesEngineRegistration（start_timer(0,fn,false) 后 fake/uv 侧 timers 表无新条目 + fn 在下轮 drain 执行）/ ZeroRepeatStillRegisters（repeat=true interval=0 走引擎）
- [ ] **Step 2: uv 真引擎对照用例**：ZeroShotPostEquivalence（0ms 定时器与 post(fn) 的执行顺序一致性——FIFO 同队列）
- [ ] **Step 3: 实现**（壳层一行分支 + 幽灵 id 文档注释）→ **Step 4: 绿 + 全量回归（75 套件口径）** → **Step 5: format + 提交** `feat(kernel): 0ms one-shot timer delegates to post queue (qt singleShot precedent)`

### Task T1（热身包·B）：二期虚函数声明 + fake/uv/qt 三侧落位

**Files:**
- Modify: `cxxkit/kernel/abstract_event_dispatcher.hpp`（register/unregister_socket_notifier 虚函数声明——**D2 裁定形态**：新声明带默认空实现 + @since 标注 + 源码级兼容注释）
- Modify: `cxxkit/uv/uv_event_dispatcher.*`（真实现：uv_poll_init_socket + uv_poll_start/stop + mHadEvents 置位——**B2 留档项同批闭合**）
- Modify: `cxxkit/qt/qt_event_dispatcher.*`（默认实现改写为 CHECK(false) 升级版：文案引用 PIT 文案 E1——"not supported by this dispatcher (phase-2 feature; UvEventDispatcher will implement it). For file IO use cxxkit thread_pool instead."）
- Modify: `tests/fake_dispatcher.hpp` + `tests/tst_uv_event_dispatcher.cpp`

**Interfaces:**
- Produces（全 dispatcher 家族统一签名，A5 curl socket_cb 验收标准；**Momus F3/F4/F6/F7/F8 修订后**）:
```cpp
// @since 0.2
enum class SocketEventMask { kRead = 1, kWrite = 2 };
// F4 组合语义：kRead|kWrite 必须合法（TcpSocket 双工挂 IN|OUT、curl INOUT、memcached 每转换重挂）
// —— enum class 需显式 operator|（C++11 合法，返回 SocketEventMask）
inline SocketEventMask operator|(SocketEventMask a, SocketEventMask b);
// readiness 通知：level-triggered（对齐 uv_poll），回调后如需继续监听须重挂；
// 回调带 mask 参数（F6 有意偏离 spec §9 字面 std::function<void()>：对齐 curl event_bitmap 回灌）
virtual void register_socket_notifier(int fd, SocketEventMask mask, std::function<void(SocketEventMask)> fn);
virtual void unregister_socket_notifier(int fd);
```
- **F6 偏离标注（三处，T4 D31 收口对账）**：①基类默认实现 = `CXXKIT_CHECK(false, E1 文案)` **有意偏离 D2 字面「默认空实现」**——fail-loud 优于静默 no-op（fd 永不就绪类 bug 静默化更难查）；源码级兼容目标不变（旧子类不 override 仍可编译，调用才 fatal）。②回调签名带 mask（对齐 curl）。③std::function 而非 signals（轻量；signals 版可薄封装其上）
- **F7 fd 生命周期三裁定**：①同 fd 重复 register = **幂等更新兴趣**（uv_poll_start 天然支持——memcached 每状态转换重挂的前提）；②**poll 活跃期关闭 fd = UB**（libuv 调用者契约）——文档「先 unregister 后 close」+ register 撞已活跃 fd = 更新而非 CHECK；③spec §9 独立 `SocketNotifier` RAII 类 **deferred**——TcpSocket 直消费 dispatcher API 无独立消费者（YAGNI），其「析构自动 unregister」价值由 TcpSocket pimpl 析构等价实现
- **F8 竞态策略**：register 回调**先拷贝 std::function 再调用**（回调中 unregister 自身不撕裂执行中的 function；erase 延迟到调用后）
- kernel 默认实现：`CXXKIT_CHECK(false, "not supported by this dispatcher (phase-2 feature; UvEventDispatcher implements it). For file IO use cxxkit thread_pool instead.")`（E1 全文）
- uv 实现：`uv_poll_t` map<fd, handle_t>（handle_t 含 poll_t* + fn + mask，对照 uv_timer map 模式）；**fd 与 socket 双入口**（uv_poll_init / uv_poll_init_socket）；回调里 mHadEvents=true（**B2 留档同批闭合**）；析构 I6：map 全 uv_close + 收尾 run；uv_close 异步 → close_cb delete（防 use-after-free）
- fake：记录型（registered map + 最后 mask + 回调可手动触发）

- [ ] **Step 1: 失败测试**（tst_uv 新增 5 用例）：RegisterPollFiresOnReadable（socketpair 写端触发读通知）/ PollInterestMaskRespected（只挂 Write 不误报 Read；含 kRead|kWrite 组合态）/ PollUnregisterStopsDelivery / PollReregisterSameFdUpdates（F7-①幂等更新）/ PollUnregisterInsideCallback（F8-①自拆不撕裂）+ 析构清理用例（I6 ASAN watch 同 B2 先例）
- [ ] **Step 2: 三侧落位**（kernel 默认 → uv 真 → fake 记录 → qt CHECK 升级）→ **Step 3: 绿**（uv 用例 9 基线 +5，以实跑为准）→ **Step 4: asan 树 UV=ON 零诊断** → **Step 5: 提交** `feat(kernel+uv): socket notifier phase-2 — uv_poll backend + family-wide declaration (D2/S13/E1)`

### Task T2：TcpSocket（memcached 状态机内核）

**Files:**
- Create: `cxxkit/uv/tcp_socket.hpp/.cpp` + `detail/tcp_socket_p.hpp`
- Modify: `cxxkit/kernel/event_loop.hpp/.cpp`（**F3 裁定**：新增 `AbstractEventDispatcher& dispatcher()` public accessor——TcpSocket 经 loop 拿注册入口；EventLoopPrivate::mDispatcher 是 kernel 私有不可达）
- Modify: `cxxkit/uv/CMakeLists.txt`（源文件进 GLOB 自动，确认即可）
- Create: `tests/tst_tcp_socket.cpp`
- Modify: `tests/CMakeLists.txt`（**F5**：注册 cxxkit_tst_tcp_socket，UV 门控块内）

**Interfaces:**
- Produces:
```cpp
class CXXKIT_UV_API TcpSocket
{
public:
    explicit TcpSocket(EventLoop& loop);   // 构造显式传 loop（asio 式，无虚基类，三期模板化留门）
    ~TcpSocket();                           // I6：uv_close + 收尾排空

    void connect(const std::string& ip, uint16_t port, std::function<void(bool ok)> on_connected);
    void write(const uint8_t* data, size_t len, std::function<void(bool ok)> on_written);
    void read_start(std::function<void(const uint8_t*, ssize_t)> on_data);  // nread<=0 = EOF/错误
    void read_stop();
    void close();

    // 连接信息
    bool is_open() const;
};
```
- **状态机（memcached 范式）**：`enum class State { kIdle, kConnecting, kConnected, kReading, kWriting, kClosing, kClosed }` + 每次转换**重挂兴趣**（读态挂 IN、写态挂 OUT——porting-scan 规避清单硬约束）
- **写背压纪律（lws）**：write 请求在 awaiting_on_written 期间排队（pending_writes deque），on_written 回调后才发下一条；**跨线程写拒绝**——write/read_start 仅限 loop 线程（I1；**有意比规避清单更严**：清单允许 post 序列化，我们直接拒绝——序列化由调用方显式做更清晰）+ 文档明示
- **缓冲（beast flat_buffer 形状）**：read 侧单块连续 std::vector<uint8_t> + 游标（rcurr/rbytes memcached 式）；**不做** prepare/commit 泛化（beast 接口形状是二期读缓冲的实现指引非 API 复刻——报告裁定记录）
- **错误处理**：uv 回调 status<0 → 映射 close（状态机 kClosing）+ on_data(EOF)/on_connected(false)——**错误态粘滞规避**（关闭后一切操作 CHECK/no-op）
- **F8-② close 窗口裁定**：`mCloseRequested` 幂等（二次 close no-op——uv_close 异步下二次 close = use-after-free）；close() 时 pending_writes 全部补发 `on_written(false)`（丢弃语义，文档化）；析构若 close_cb 未跑完 → 收尾 run 排空（一期 uv dispatcher 同款）

- [ ] **Step 1: 失败测试**（tst_tcp_socket.cpp，真实 loopback）：ConnectEchoRoundTrip（本地 echo server + write + on_data 收回）/ WriteBackpressureQueueing（on_written 前多次 write 排队按序发）/ ReadStopRespectsMask / CloseInsideCallback（I3：on_data 里 close 自身不崩）/ DestructorMidTransfer（I6 ASAN）/ ErrorPathRemoteClose（对端关闭 → on_data(0)）/ CloseTwiceIdempotent（F8-②幂等 + pending writes 收 false）/ CrossThreadWriteRejected（F11：非 loop 线程 write → fatal）
- [ ] **Step 2: 实现**（状态机 + uv_tcp_t 直包，Node tcp_wrap 模式 handle->data 回 C++）→ **Step 3: 绿** → **Step 4: asan 零诊断** → **Step 5: 提交** `feat(uv): TcpSocket — memcached state machine over uv_tcp (A3/背压纪律/I1/I3/I6)`

### Task T3：TcpServer + echo example

**Files:**
- Create: `cxxkit/uv/tcp_server.hpp/.cpp`（**F10 裁定**：detail/tcp_socket_p.hpp 提供 `static std::unique_ptr<TcpSocket> adopt(uv_tcp_t*, EventLoop&)` 内部构造——写死一种）
- Create: `tests/tst_tcp_server.cpp`（**F5 补**）
- Create: `examples/exp_tcp_echo.cpp`
- Modify: `tests/CMakeLists.txt`（注册）、`examples/CMakeLists.txt`（UV=ON 门控）、`README.md`（uv 行扩展：tcp echo）

**Interfaces:**
- Produces:
```cpp
class CXXKIT_UV_API TcpServer
{
public:
    explicit TcpServer(EventLoop& loop);
    ~TcpServer();
    bool listen(const std::string& ip, uint16_t port, int backlog = 128);
    // 每 accept 一个连接回调一次；socket 所有权归消费者（unique_ptr 语义）
    void on_connection(std::function<void(std::unique_ptr<TcpSocket>)> fn);
};
```
- uv_tcp_t + uv_listen + accept 产出新 handle → 包成 TcpSocket（accept 时机即 handle 已就绪）

- [ ] **Step 1: 失败测试**：ListenAcceptEcho（server+client 全环）+ AcceptDuringActiveConnections + ServerDestructorWithLiveConnections（I6）→ **Step 2: 实现** → **Step 3: 绿** → **Step 4: 提交** `feat(uv): TcpServer + exp_tcp_echo (state-machine family completion)`

### Task T4：收尾（文档/CI/memory/全链）

**Files:**
- Modify: spec §9（二期行 → 已交付标注 + 实际签名对齐）、`docs/superpowers/specs/2026-09-08-porting-scan-report.md`（A3/A5 标注已消费 + F6 对账）
- Modify: `README.md`（子库表 uv 行内容更新：socket notifier + tcp）、AGENTS.md 结构段（如有必要）
- Modify: `.agents/memorys/`（status.md 二期段落 + decisions.md D31 + pitfalls 查 Qt/uv 新坑——如 uv_poll fd vs socket 双入口的陷阱）
- **全链验收**：主树（UV+QT=ON）75+N 全绿 / shared 74+ / asan 零诊断 / coverage ≥80%（uv 新文件水位 ≥85%）/ check.sh 8/8 exit=0 / rc 矩阵（exp_event_loop + exp_qt_embed + exp_tcp_echo 三例）

- [ ] 全链执行 + evidence 记录 + memory 提交 `docs(memory): event-loop phase-2 record (D31)`

## 任务依赖与执行序

```
T0（热身 0ms 直投）─┐
T1（notifier 三侧）─┼─→ T2（TcpSocket）─→ T3（TcpServer+example）─→ T4（收尾）
（T0/T1 可并行，不同文件）
```

## 验收门禁（继承 D2 证据链标准）

- 主树 UV+QT=ON：全绿（**套件 75→78**：+tst_kernel_event_loop_ext/+tst_tcp_socket/+tst_tcp_server；用例数以实跑为准）
- shared/asan（UV=ON）：绿/零诊断（新增 ASAN watch 用例同 B2 先例）
- coverage：≥80% 全口径，uv 新文件 ≥85%
- check.sh 8/8 exit=0；rc 矩阵三例
- 二期验收特有：A5 curl socket_cb 形状对照表（register/unregister 参数与语义逐项对照）记入报告

## 明确不做（scope freeze）

- 三期网络抽象/QTcpSocket 复用（spec §9 三期，默认砍）
- TLS/mbedTLS BIO（三期；规避清单已备）
- UDP/Pipe（需求触发）
- aggregate connect / coarse timer（T0 修正中已 deferred——与 queued 异步语义冲突/需求触发）
- Windows 专项验证
