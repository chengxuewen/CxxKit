# 移植机会扫描报告（Qt 源码 + 主流信号槽/事件/IO 库）

**日期**: 2026-09-08
**流程**: 4 人侦察团队（qt-source-miner / sigslot-scout / eventloop-lib-scout / io-net-scout）并行调研 → lead 交叉汇总
**基线**: 排除已调研项（pal::signal、Boost.Signals2、eventpp、sigc++、CsSignal、rocket、uvw/uv-cpp、libuv、asio、Chromium MessagePump、folly、Glib、Qt dispatcher 嵌入形态、Node tcp_wrap、beast 分层）
**用途**: 二期 SocketNotifier+TcpSocket（spec §9）与后续迭代的候选清单；「不值得」负面清单防重复调研

---

## A. 高价值可移植项（按优先级）

### A1. ratas 层次时间轮 —— 最干净的单件移植
- MIT / C++11 / 单文件 ~700 行 / ~220★ 维护至 2024-04（github.com/jsnell/ratas）
- 机制：分层环形缓冲 + advance(ticks) 纯手动驱动（无线程/无 IO/无时钟）+ schedule_in_range（抖动容忍调度）+ cancel + ticks_to_next_event；TimerEvent 侵入式 RAII（析构自动 cancel）
- 落点：tools 或 thread 子库纯组件；**advance 手动驱动与 fake_clock 确定性测试基建天然咬合**；可选作 UvEventDispatcher timer 后端（uv handle 模型每 timer 有 init/start/stop 开销，wheel 是纯数据结构）
- 交叉：与 qt-source-miner TOP1（coarse timer 粗化）可组合——wheel 的 schedule_in_range 即抖动容忍调度的数据结构化表达

### A2. QTimerInfoList 三档定时器粗化（Qt qtimerinfo_unix.cpp）—— 省电降唤醒纯算法
- 机制：CoarseTimer ≤5% 抖动（<50ms 对齐偶数 50ms、<100ms 对齐 4 倍数、更大按整秒→500ms→250/750→200/400/600/800→100 倍数→25 倍数阶梯选「最多定时器同醒」边界）；registerTimer 自动升降级（≥20s→VeryCoarse 整秒、≤20ms→Precise）；catch-up（timeout+=interval 落后则 resync=now+interval——漏掉的 tick 合并成一次）
- 落点：EventLoop::start_timer 增 `enum TimerType { kPrecise, kCoarse, kVeryCoarse }` 壳层实现；或与 A1 wheel 的 schedule_in_range 合并设计
- C++11 可行（steady_clock + 排序 vector）

### A3. memcached drive_machine 状态机 —— 二期 TcpSocket 内核（**已消费 2026-09-08**，D31）
- C/BSD/极活跃（github.com/memcached/memcached）
- 机制：conn_listening→waiting→read→parse_cmd→nread→swallow→mwrite→closing 显式状态机 + `conn_set_state()` 每次转换**重挂 event 兴趣标志**（读态挂 IN、写态挂 OUT）；固定 rbuf/wbuf 双缓冲 + rcurr/rbytes 游标；超大请求 rbuf_switch_to_malloc 逃生
- 落点：状态枚举→uv read_cb/connect_cb 直译，无虚基类，正合 spec §9 二期路线（uv_tcp 封装/不加虚基类）

### A4. mbedTLS ssl_set_bio 非阻塞状态机 —— 三期 TLS 集成（已 vendored！）
- C/Apache-2.0，cxxkit 已有 WrapMbedTLS vendored 链
- 机制：三回调 f_send/f_recv/f_recv_timeout；handshake/read/write 返 WANT_READ/WANT_WRITE = 「等对应 fd 事件后重入」非错误
- 落点：uv_read/write 回调喂 BIO，WANT_* → 重挂 poll 兴趣。三期 TLS 走此路，不引新 TLS 库

### A5. libcurl multi_socket 契约 —— SocketNotifier 接口验收标准（已 vendored；**已消费 2026-09-08**，D31）
- 机制：CURLMOPT_SOCKETFUNCTION（fd+IN/OUT/INOUT/REMOVE 兴趣推送）+ CURLMOPT_TIMERFUNCTION（全局单发定时器，-1=删/0=立即）+ curl_multi_socket_action(fd, bitmask) 回灌
- 落点：其 socket_cb 形状即二期 register_socket_notifier 的接口验收标准；「HTTP over cxxkit 事件循环」的二期后手（cpr 只用 easy 接口）
- **消费对账（A5 形状对照表，2026-09-08 D31）**：register = CURLMOPT_SOCKETFUNCTION 注册方向（fd + 兴趣 mask + 回调，kRead|kWrite 合法组合 ≈ CURLMOPT_IN|OUT）；回调接 fired mask ≈ `curl_multi_socket_action(fd, bitmask)` event_bitmap 回灌；unregister ≈ CURLMOPT_REMOVE；语义差异声明式披露：curl 是 edge 驱动重注册制，cxxkit 按 libuv uv_poll **level-triggered 常驻注册**（F7：同 fd 重 register = 幂等更新兴趣，交付形态以此为准）

### A6. Chromium Once/Repeating 契约层 —— 信号槽唯一实质增量
- 机制：OnceCallback move-only（bind 后不可重复触发）vs RepeatingCallback 二分；配合已有 unique_function 天然可落
- 价值：消灭「连接回调被重复执行」bug 类；作为 connect 的可选约束层而非新信号库

### A7. nod 风格 aggregate 返回值聚合 —— ~30 行小件
- 机制：`T accumulate = combine(所有槽返回值)`（求和/收集/短路）
- 落点：pal::signal 上游移植时砍掉的 combiner 真空；可选 connect 返回值收集器

### A8. libhv 事件优先级次序契约 —— 纯文档动作
- 机制：hloop_process_events 固定次序 ios→timers→idles→pendings + 五级优先级常量
- 落点：AbstractEventDispatcher 文档明确「pending tasks vs timers vs posted events 处理次序」契约（零代码，反哺接口语义）

## B. 中价值（条件触发再做）

| 项 | 触发条件 | 内容 |
|---|---|---|
| compressEvent 压缩事件 | 二期 postObject 通道（UI 式重绘/布局合并） | Timer 按 (receiver,timerId) 合并、Resize 改写已入队事件 payload、UpdateLater 区域求和；设计带 compressible 标志 |
| 排序 vector 定时器表 + activateRef 重入防护 | 未来自研 epoll/kqueue dispatcher | timerInsert upper_bound 插入 + activateTimers rotate 保序 + activateRef 自指防「回调内注销自身」；对照确认 I3 已被 uv_close 覆盖 |
| QThreadPipe 唤醒去重门 | 自研引擎 | wakeUps.fetch_and(1)&1==0 才写 eventfd（N 线程并发唤醒一次系统调用）+ EAGAIN 排空 + testAndSetRelease 复位 |
| 0ms 单发定时器直投队列 | 随时（一行） | `interval==0 && !repeat → post(fn)` 绕过 uv_timer 注册（Qt singleShotImpl 同款）|
| Glib postEventSource serialNumber 形态 | cxxkit::qt 精化 | wake_up 状态驻留为 GSource ready 位（serialNumber 比较）替代 QTimer 泵；idle 优先级降级定时器 |
| evpp InvokeTimer 自持有 | thread/tools 定时器需「可取消+自动保险」语义 | enable_shared_from_this self_ 持有 + Cancel 安全，~150 行独立移植 |
| folly::Observer / seastar foreign_ptr | 线程池分片远期 | 派生观察者自动重算 / 对象记录 home thread 迁移销毁 |

## C. 不值得移植·负面清单（防重复调研）

**信号槽**：KDBindings（scoped connection 已被 Connection+RAII 习惯覆盖）、react-cpp（响应式数据流超纲）、Chromium 整套 base::ObserverList、sigslot 全家（pal::signal 已覆盖）、CsSignal（已评）、DelegateMQ/BSignals（连接时指定执行上下文——connect_queued 已等价）
**事件循环**：Bombash/wheels（不存在，情报有误）、srs state-threads（setjmp/longjmp + 单线程假设与多线程模型冲突）、libco（license NOASSERTION 商用风险，被 coost 替代）、evpp 整库（libevent 绑定+停更 2017）、Thrift eventor（folly EventBase 同源重复）、seastar 整库（C++14~20 + DPDK）、libhv 整库（iowatcher 重复造轮子，只借次序契约）
**IO/网络**：coost 整库（2025 转 C++17+xmake）、wangle（folly 硬依赖）、libwebsockets 整库（只取写背压纪律）、beast composed ops（asio 绑定）
**Qt 内部**：QMetaType 注册体系（C++11 无反射，lambda 捕获已等价）、sendEvent 多层分发、QPostEvent 优先级排序、postEventList offset 排空复杂度（swap 已等价）、childEvent 传播、Windows SetCoalescableTimer、DeferredDelete 嵌套语义、QChronoTimer 双类（无 ABI 包袱）、QSocketNotifier::Exception 类型（POSIX 无用）

## D. 二期骨架推荐组合

**SocketNotifier 接口** = libcurl multi_socket socket_cb 形状（A5 验收标准）
**TcpSocket 内核** = memcached 状态机（A3 控制流）+ beast flat_buffer 接口形状（单块连续+prepare/commit/consume+max_size）
**TLS（三期）** = mbedTLS BIO（A4，已 vendored）
**timer 精化** = ratas wheel（A1）或 Qt 三档粗化（A2），两者可组合（wheel 结构 + schedule_in_range 语义）
**写背压** = lws 纪律（只在 writable 回调写，欲写者排队等通知）
**已知陷阱规避** = TLS 缓冲稳定性无保证（mbedtls #10500，重入前拷贝）/ WANT_* 忙轮询（必须先置非阻塞）/ 错误态粘滞（非 WANT_* 后 context 报废须 reset）/ curl timeout=0 禁递归 / 状态转换必重挂兴趣标志 / 跨线程写必须 post 序列化
