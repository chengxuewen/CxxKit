# UdpSocket 落地 Plan (D44)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**一句话**：给 network 子库补上 UDP——并行 `DgramBackend` 抽象（不碰 StreamBackend）+ 双后端（uv_udp_t / asio udp::socket）+ `UdpSocket` 公共类（对齐 TcpSocket 的 Qt 级错误/状态面）。v1 范围 = 无连接核心。

**用户裁定**（2026-09-16，"ok" = 按推荐全收）：
- R1 架构：**并行 DgramBackend 新接口**（不塞 StreamBackend——流语义 vs 数据报语义不兼容，uv_udp/asio udp API 形状完全不同）
- R2 范围：v1 = bind/send_to/on_datagram/close 无连接核心；connected-UDP + broadcast = v1.5；multicast = v2（Deferred）
- R3 状态机：SocketState 追加 `kBound`（公共枚举追加，值附尾部，向后兼容）
- R4 测试：loopback 收发 fixture，spin_with_worker 模式，PIT-57/58 纪律照抄

**调研来源**（2026-09-16，直营——团队模式被 CPU 过载打死后收编）：
- Qt 6 QUdpSocket 官方文档全文（API 面 + "datagram ≤8192 平台相关/>512 易分片"教训）
- 本地代码：stream_backend.hpp 接口形状、uv backend（uv_tcp_t 直译模式）、asio backend（timer-gated pump + PIT-55 restart 纪律 + alive-weak liveness 桥）、SocketError/SocketState 现有枚举

---

## 大局观

```
StreamBackend (D41, 不动)          DgramBackend (新, 本计划)
  open/connect/write/read_start      open/bind/send_to/receive_start
  accept/adopt/pump                  send_connected(v1.5预留接口位)/close
       │                                  │
  TcpSocket (状态机+pimpl)          UdpSocket (同款形状)
  TcpServer                          (无 server 概念——bind 即收)
       │                                  │
  TlsSocket (D42 组合)               ── 无 TLS-over-UDP (DTLS = Deferred)
```

**分层**：公共头零 uv/asio 类型（D8/D41 纪律）→ UdpSocket pimpl 持 DgramBackend → 编译期 `CXXKIT_NETWORK_BACKEND` 选 uv/asio 实现（PIT-48 接线：CMake option → target_compile_definitions）。

## Global Constraints（与 D41/D42 同款）

- C18: commit + code comments English only; plan + AI dialogue Chinese.
- C4: C++11 lib code; D8: angle includes; 公共头零后端类型。
- D26 naming: snake_case / mPascalCase / kPascalCase。
- PIT-40: 回调经局部拷贝调用（回调可能 close/destroy socket）。
- PIT-48: CMake option ↔ compile definition 接线齐全（双后端 + OFF 三态）。
- PIT-55: asio 侧所有 poll 前置 `io->restart()`。
- PIT-57/58 测试纪律：fresh EventLoop per case + spin_with_worker（禁 run_with_loop/二次 exec）；StringView 字段禁临时 std::string。
- 门禁：build 0 → `ctest --test-dir build -R udp` green → 双树（build + build-asio）→ 全量 ctest → clang-format（禁碰 CMakeLists）。收官全量 + ASAN 定向零诊断。
- 提交身份：`git -c user.name=chengxuewen -c user.email=1398831004@qq.com`。

---

### Task 0: 热身——基线 + DgramBackend 接口

**Files:**
- Create: `cxxkit/network/detail/dgram_backend.hpp`
- Modify: `cxxkit/network/socket_state.hpp`（+kBound，尾追加）

- [ ] **Step 0.1**: 基线——git clean；build 0；全量 ctest 87/87 记录。
- [ ] **Step 0.2**: SocketState 追加 `kBound`（放 kIdle 之后、kConnecting 之前？**值追加在尾部**避免重排既有值——放 kClosed 之后）+ doxygen 注明 UDP-only 稳定态。
- [ ] **Step 0.3**: DgramBackend 纯虚接口（对齐 StreamBackend 风格）：
  - `virtual bool open(EventLoop&) = 0;`（绑定 loop——uv 取 loop、asio 取 io slot，同款）
  - `virtual bool bind(const std::string &ip, uint16_t port) = 0;`（同步语义——UDP bind 无异步性；false 时 native_status 置原因）
  - `virtual uint16_t bound_port() const = 0;`（port 0  ephemeral 回读——TcpServer 同款）
  - `virtual void send_to(const uint8_t *data, size_t len, const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done) = 0;`
  - `virtual void receive_start(std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> on_datagram) = 0;`
  - `virtual void close() = 0;`（幂等；收尾 drain）
  - `virtual EventLoop &loop() const = 0; virtual int native_status() const = 0; virtual void *native_handle() const = 0;`（三件套同 StreamBackend）
- [ ] **Step 0.4**: commit `feat(network): DgramBackend interface + SocketState kBound`。

### Task 1: uv 后端（uv_udp_t 直译）

**Files:**
- Create: `cxxkit/network/detail/dgram_backend_uv.hpp/.cpp`
- Modify: `cxxkit/network/CMakeLists.txt`（源列表追加，双后端条件编译同 StreamBackend 现状）

- [ ] **Step 1.1**: uv_udp_t + alloc_cb/recv_cb 直译；uv_udp_send req 管理（一次性 req + 局部拷贝数据——PIT-40）。
- [ ] **Step 1.2**: fill_sockaddr 复用（stream_backend_uv 已有，提取共用或复制——最小 diff 优先，选复制到 dgram 文件内 static）。
- [ ] **Step 1.3**: 错误映射：UV_EMSGSIZE→kMessageTooLarge（新枚举值，Task 2 一并加）、UV_EADDRINUSE→kAddressInUse、UV_ENETUNREACH→kNetworkUnreachable、uv_udp_recv cb nread<0→map_transport_error 复用。
- [ ] **Step 1.4**: close：uv_close 收尾 + recv 停止；幂等。
- [ ] **Step 1.5**: gates + commit `feat(network): uv dgram backend (uv_udp_t)`。

### Task 2: 公共错误枚举扩展 + UdpSocket 公共类

**Files:**
- Create: `cxxkit/network/udp_socket.hpp/.cpp` + `detail/udp_socket_p.hpp`
- Modify: `cxxkit/network/socket_error.hpp`（+kMessageTooLarge/kAddressInUse/kNetworkUnreachable，尾追加 + to_string）

- [ ] **Step 2.1**: SocketError 三枚举值 + to_string（照现有表格式）。
- [ ] **Step 2.2**: UdpSocket 对齐 TcpSocket 形状：pimpl + 状态机（kIdle→kBound→kClosed；kConnecting/kConnected **不在** v1——connected-UDP 是 v1.5）+ `set_on_error/set_on_state_change/state/last_error` Qt 级面。
- [ ] **Step 2.3**: 公共 API：
  - `bool bind(const std::string &ip, uint16_t port);`（同步；成功→kBound）
  - `uint16_t bound_port() const;`（loop-thread 断言同 TcpSocket）
  - `void send_to(const std::string &data, const std::string &ip, uint16_t port);`（重载 span 形态可选）
  - `void set_on_datagram(std::function<void(const std::string &data, const std::string &sender_ip, uint16_t sender_port)>);`
  - `void close();`
  - `set_transport`-式后端注入 `set_backend(std::unique_ptr<DgramBackend>)`（测试注入 + TlsSocket 先例）
- [ ] **Step 2.4**: 析构安全：未 close 的 socket dtor 内 close；回调置空（PIT-40 族）。
- [ ] **Step 2.5**: gates + commit `feat(network): UdpSocket public class (connectionless core)`。

### Task 3: asio 后端

**Files:**
- Create: `cxxkit/network/detail/dgram_backend_asio.hpp/.cpp`

- [ ] **Step 3.1**: asio::ip::udp::socket + async_receive_from/send_to；alive-weak liveness 桥（照抄 stream_backend_asio 的 ensure_pump 模式）。
- [ ] **Step 3.2**: PIT-55：所有 poll 前 restart()（pump_tick 复制或复用——**复用泵机制但要独立 has_work**（udp socket 单独跟踪））。
- [ ] **Step 3.3**: 错误映射：error_code → 公共枚举（EMSGSIZE/EADDRINUSE/ENETUNREACH 同表）。
- [ ] **Step 3.4**: gates：build-asio 树 build + `ctest --test-dir build-asio -R udp` green；commit `feat(network): asio dgram backend (udp::socket)`。

### Task 4: 测试全量 + 收官

**Files:**
- Create: `tests/tst_udp_socket.cpp`
- Modify: `tests/CMakeLists.txt`（注册，kernel/network 门同 tst_tcp_socket）
- Modify: `.agents/memorys/{status,decisions}.md`、`README.md`（network 行补 UDP）

- [x] **Step 4.1**: 测试用例（loopback fixture，spin_with_worker）：
  - bind 回读（port 0 → ephemeral bound_port）
  - bind 冲突（同端口第二个 socket → false + kAddressInUse）
  - send_to → on_datagram 收到（数据/发送方地址/端口全对）
  - 发送方未 bind 也能发（ephemeral 源）
  - close 后 send_to → 错误面（state kClosed + on_error）
  - dtor 未 close → 自动 close 不崩
  - 大包（接近 8192）往返
  - 双 socket 互发
- [x] **Step 4.2**: 双树全量：build（uv）+ build-asio 全绿；ASAN 定向 udp 零诊断。
- [x] **Step 4.3**: README network 行补 UdpSocket；记忆：status.md D44 条目 + decisions.md D44（R1-R4 裁定 + 直营调研说明）。
- [x] **Step 4.4**: commit `test(network): UDP suite + docs (D44 closeout)`。

---

## Deferred（需求触发再取，YAGNI）

- connected-UDP（connectToHost 等价 + ICMP ECONNREFUSED 语义）——v1.5
- broadcast 选项（SO_BROADCAST 包装）——v1.5
- multicast 全家（join/leave/interface/TTL/loopback）——v2
- DTLS（TLS over UDP）——远期
- Windows/CI 侧验证——维持项目级备案
