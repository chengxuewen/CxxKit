# event-loop 一期实施计划输入 bundle（hyperplan 对抗收敛终态）

**日期**: 2026-09-04
**流程**: spec（2026-09-04-event-loop-design.md）→ 四方审核（2026-09-04-event-loop-review-findings.md）→ hyperplan 对抗三轮（pragmatist/adversary/strategist/synthesist）→ 本 bundle = 唯一权威输入
**计划产出要求**: 一期实施计划（任务分解 + 依赖 + 并行化 + 每步验证标准），计划者不得偏离本 bundle 的已收敛裁定

## 0. 已收敛终局裁定（对抗三轮确认，零异议）

| # | 裁定 | 内容 |
|---|---|---|
| D1 | **qt 一期保留最小面** | 嵌入 Qt 是用户立项需求（spec 目标 2）；推二期 = scope change 须用户确认。qt 引擎 5h（QPointer 守门铃 + context 析构契约 + 嵌套直通测试上调后） |
| D2 | **socket_notifier 预留虚函数整体砍** | 签名完整记 spec §9 二期行；二期 SocketNotifier 需要时**新声明虚函数带默认空实现 + @since 标注**（源码级兼容非 ABI 兼容，半句备注进 §9） |
| D3 | **I2 行为级** | 「kWaitForMoreEvents 下到期 timer 有界触发」；禁"poll 超时"机制字样；每引擎自证附表（uv=timeout 折算 / Qt=QTimer 一等源 / asio 若来=run_one_for） |
| D4 | **不变量集两级化** | I1-I7 编号 + 契约级/实现级标注 + 每条标覆盖用例行。契约级后端零豁免、文本变更默认否决须全后端重议；实现级各引擎声明式披露 |
| D5 | **A1 → I7 契约级** | 「disconnect 停止后续投递、不追已入队；安全撤销须 SafetyFlag 或先停 emit 源」。M1 Connection 文档与 S8 **引用** I7（一处定义三处引用防漂移） |
| D6 | **A5 → SHOULD 一行** | 「post/start_timer 回调抛异常 = 异常不穿越事件循环，进程终止（abort），timer 与 post 同契约；排空循环不因单回调异常中断」进 §1 非目标 |
| D7 | **A3 → M4 三检查点** | exit/interrupt 内部调 wake_up（M4 原）+ interrupt flag 检查点（入口 + 每轮事件处理前）+ **排空循环每轮末尾（清空后、再阻塞前）复查 mExit/mInterrupt** |
| D8 | **A9 → S10 补充** | 「排空幂等兜底正确性；Qt 事件风暴下门铃延迟上界不承诺；不做补偿机制（一期 YAGNI）」 |
| D9 | **A2 → P4 一行** | check.sh/asan 树 UV=ON（否则 M5 析构交叠 ASAN 用例与 S11 85% 水位双双悬空） |
| D10 | **maximumTime 真实现** | 壳层合成（一次性 interrupt 定时器 + kWaitForMoreEvents 循环，Qt QEventLoop 同款）1.5h——非空壳（空壳=API 债，与砍预留同构双标被否） |
| D11 | **uv 测试 7 场景维持** | wake_up 风暴（S10/I4 唯一兜底）不砍；空队列返回值透传不砍（P1 依赖） |
| D12 | **首跑例钉板三件** | 工厂扁平名 `make_uv_dispatcher()`（守 D2 扁平命名空间）；头 `cxxkit/uv/uv_event_dispatcher.hpp`；find_package 组件名 `uv`。adversary 可编译全文采纳（EventLoop loop(cxxkit::make_uv_dispatcher()) + 3 tick timer exit）；标注「CI 编译门禁验证」而非承诺用户耗时 |
| D13 | **Glossary 两词制** | EventLoop（用户面壳）+ Dispatcher（驱动接口）；"engine" 一词全文档消灭；AbstractEventDispatcher 定位句进 Glossary |
| D14 | **EventLoopThread** | 禁令降级「已知冲突记录」（kernel spec 不立法 thread 子库）；#if 0 死骨架本期廉价处置（0.5h，forward-declare EventLoopPrivate 与 kernel 同名 ODR 风险 → plan 前置条件） |
| D15 | **asio 准入判据修正** | 「契约级不变量零豁免」——shim 合法（0-length re-arm、内部 async 通道 = 翻译式实现，局部有界可测），豁免非法；须改 I 集文本 = 默认否决；能力面差异（嵌入/嵌套/socket_notifier）声明式披露（uSockets 并存先例） |
| D16 | **最终估算 50h ±10%** | （40-59h）≈ 6.5 工作日。明细：核心实现 8h + uv 引擎+wrap 10h + qt 引擎+M9/M10 6.5h + kernel fake 测试 4h + uv 7 场景 5h + maximumTime 1.5h + M1-M7 4h + SHOULD S1-S14 8h + 文档口径 2h + EventLoopThread 处置 0.5h |

## 1. 不变量集终形态（进 spec 新小节「引擎契约不变量」）

| # | 级别 | 内容 | 覆盖用例 |
|---|---|---|---|
| I1 | 契约级 | 单消费者：process_events/exec 仅 loop 线程；post/wake_up/exit 任意线程 | debug thread id 断言 + race_checker 先例 |
| I2 | 契约级 | kWaitForMoreEvents 下到期 timer 有界触发（D3 行为级） | timer 时序用例（宽松下界） |
| I3 | 契约级 | 回调内 start_timer/stop_timer 合法（M3 one-shot 壳包装的实现前提） | 回调内自停用例 |
| I4 | 契约级 | wake_up 可合并且门铃回调无条件锁内排空整队（含空队列虚警） | wake_up 风暴 N×M |
| I5 | 实现级 | 嵌套支持度是每引擎声明属性（uv 不支持→debug 断言；Qt=QEventLoop 直通） | 嵌套钉住用例 |
| I6 | 契约级 | 析构纪律：dispatcher/EventLoop 于 loop 线程或 loop 已停析构；uv_close 全 handle+收尾排空；Qt context 顺序（dispatcher 先亡）| 析构清理 ASAN 用例 |
| I7 | 契约级 | disconnect 停后续投递不追已入队（D5） | disconnect+在飞闭包 ASAN 用例 |

壳层包装裁定规则（strategist 三问：可组合性/已成文不变量/可观测漂移 + 壳偏置）进 spec 附录；壳包装仅可依赖契约级 I 项。

## 2. 最终一期范围（scope freeze）

**做**：kernel（AbstractEventDispatcher 5 纯虚 + EventLoop 壳填实现 + ctor 收紧 `EventLoop(std::unique_ptr<AbstractEventDispatcher>, Object* = nullptr)` + null CHECK + post/start_timer/stop_timer + maximumTime 壳合成 + connect_queued 返回 Connection + std::bind C++11 实现 + Glossary + I 集节 + 生命周期契约节 M5 + 线程矩阵两表 §4.1/§4.2）+ uv 子库（FindWrapLibuv libyuv 形态 + libuv.7z vendored + UvEventDispatcher + make_uv_dispatcher + install 四件套 imgui/media 模板 + .pc + VENDORED_FIND_DEPS append + D6 命名空间头）+ qt 子库（QtEventDispatcher 5h + 目标级 CXX_STANDARD 17 + cxxkitConfig 独立 Qt 分支 M10 + CMake-only 消费不出 .pc + smoke 文档 + CI 编译门禁 qt6-base-dev）+ 2 examples（exp_event_loop CI rc=0 / exp_qt_embed 编译验证）+ M1-M10 全部 + SHOULD S1-S14（含 S5 crash.pc 既有 bug 顺手修、S11 覆盖口径 43-file 基线更新、S12 测试 C++11 备注、S13 smoke 格式）+ EventLoopThread 死骨架处置。

**不做（一期出界）**：register_socket_notifier 任何形态（D2）、asio 后端（D15 判据备案）、普通文件异步 IO（thread_pool 域）、BlockingQueuedConnection、泛型 safe_task（S9 仅改表述）、thread 子库任何立法。

## 3. 依赖与并行度（供计划者排布）

- Phase A（kernel 抽象+壳+connect_queued+fake 测试）无外部依赖，先行
- Phase B（uv）与 Phase C（qt）均只依赖 A 的 header，**可并行**；但两者都改 CxxKitConfig.cmake.in / README——计划者须安排串行合并点或分文件冲突消解
- wrap 基建（libuv.7z vendored + FindWrapLibuv）可与 Phase A 并行（纯基础设施）
- 前置条件：EventLoopThread ODR 冲突处置先于 shared 构建验证；libuv.7z 从零打包（归档不存在）

## 4. 验收门禁（每 Phase 的完成标准）

- 全量 build 0 error + ctest 72/72 基线不破（新增测试全绿）
- clang-format 干净；C++11 门禁（库代码）/测试默认主标准
- 覆盖率 43-file 全口径 ≥80%（S11 新基线），uv 测试水位目标 85%+
- asan 树 UV=ON 零诊断（D9）
- example rc 矩阵：exp_event_loop rc=0（CI）；exp_qt_embed 编译门禁 + smoke 文档人工门禁
- spec/README/AGENTS 口径同步（Glossary/I 集/§9 二期行/子库表 19 库）
