# D43.5 收尾小波 Plan：ELT×默认 dispatcher fatal + 信号槽两项收尾

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**一句话**：D43 撞上的 ELT×默认 dispatcher fatal（uv 在**构造线程**捕获 loop 线程 id → worker `exec()` 必 fatal）+ 两个信号槽直系小项（SignalBaseR move、move-assign 出槽位重路由），一个小波清掉。

**三项内容**：
1. **ELT fatal 修复**（根因已在 D43 记录）：`EventLoopThread` 构造时在**构造线程**调用 factory()（event_loop_thread.cpp:59-60），uv dispatcher 在 ctor 捕获 `mLoopThreadId`（uv_event_dispatcher.cpp:84）→ worker 线程 `exec()` 时 `check_loop_thread` 必 fatal。修法：**factory 延迟到 loop 线程执行**（Runner 启动后在 worker 线程调 factory() 再建 EventLoop）——dispatcher 从出生就在正确的线程，零 API 变更。
2. **SignalBaseR move 支持**：R 族信号目前不可移动（copy deleted 无 move）；照 T2 模式（mCleaner 已是指针，重路由函数现成）加 move ctor+assign。
3. **move-assign 出槽位重路由**（终审 parked 项）：`SignalBase::operator=(SignalBase&&)` 现只重路由换入的槽位；换出到源 `o` 的槽位 cleaner 仍指 `*this`——补对称重路由（~4 行）。

## Global Constraints（与 D43 同款）

- C18/C4/D26/D8 同 D43 计划；NEVER rm -rf build*；pixi clang-format（禁 CMakeLists）。
- 每任务门禁：build 0 → `ctest --test-dir build -R signal` / `-R event_loop_thread` green → 全量 ctest。
- 提交身份：`git -c user.name=chengxuewen -c user.email=1398831004@qq.com`。

---

### Task 1: ELT factory 延迟到 loop 线程（核心修复）

**Files:**
- Modify: `cxxkit/thread/event_loop_thread.cpp/.hpp` + `detail/event_loop_thread_p.hpp`

- [ ] **Step 1.1**: Runner 主循环改为：worker 线程先 `std::unique_ptr<AbstractEventDispatcher> d = factory()` → `mLoop.reset(new EventLoop(std::move(d)))` → exec → 析构也在 worker 线程。ctor 只存 factory 不再调用。
- [ ] **Step 1.2**: 处理时序：ctor 返回后、worker 建好 loop 前，`loop()` 访问需等待（EventLoopThreadPrivate 加 `std::promise<bool>/future<bool>` 或 condition_variable ready 标志；`loop()` 在 ready 前阻塞或返回前等待——按现有 API 形状选最小者）。
- [ ] **Step 1.3**: `~EventLoopThread` 时序：stop → join（loop 与 dispatcher 在 worker 线程完整析构）。
- [ ] worker-first 构造对 QtEventDispatcher 同样成立（Qt dispatcher 无线程捕获问题，worker 构造无害）。
- [ ] **Step 1.4**: 新测试（tst_event_loop_thread）：默认 dispatcher（uv）+ worker exec + 主线程 post → 不 fatal、任务执行。这就是 D43 撞 fatal 的原始场景。
- [ ] **Step 1.5**: 全量 ctest + exp_kernel 跨线程节回归（仍 rc=0）。
- [ ] **Step 1.6**: commit `fix(thread): construct dispatcher on the loop thread in EventLoopThread (uv thread-affinity fatal)`。

### Task 2: SignalBaseR move + 出槽位重路由（信号槽两小项打包）

**Files:**
- Modify: `cxxkit/kernel/signals.hpp`

- [ ] **Step 2.1**: SignalBaseR 加 move ctor+assign（copy 仍 deleted）：锁内 swap 槽位表 + 对换入槽位调既有 reroute 逻辑（mCleaner 已是指针——D43 T2 的地基直接复用）。
- [ ] **Step 2.2**: `SignalBase::operator=(SignalBase&&)` 补出槽位重路由：swap 后对 `o` 现持有的槽位也 reroute 到 `o`（~4 行，消 parked 的静默 no-op disconnect + UAF 边角）。
- [ ] **Step 2.3**: 测试：SignalR move 后旧 Connection disconnect 正确作用于新 signal（mirror B2a）；move-assign 后双向（换入+换出）Connection 各自 disconnect 正确。
- [ ] **Step 2.4**: gates + commit `fix(kernel): SignalBaseR move support + move-assign outgoing-slot cleaner reroute`。

### Task 3: 收官——9.2c 补课 + 文档 + 记忆

**Files:**
- Modify: `examples/exp_kernel.cpp`、`.agents/memorys/{status,decisions}.md`

- [ ] **Step 3.1**: exp_kernel 跨线程节升级：std::thread demo 保留，**补 EventLoopThread 版**（worker emit → 主线程收，join-before-print 保确定性）——9.2c 的债清了。
- [ ] **Step 3.2**: README kernel/thread 行措辞如需同步则改；exp rc=0 验证。
- [ ] **Step 3.3**: 记忆：status.md（D43.5 小波条目）、decisions.md（D43.5：ELT factory-worker-first 裁定 + 两信号槽小项）。
- [ ] **Step 3.4**: 全量门禁（build/ctest/format）；commit `docs: D43.5 records + exp_kernel ELT cross-thread demo`。

---

## Deferred（需求触发再取）

- check_loop_thread 改成 exec 时重绑（比 worker-first 更大的语义变更，worker-first 已消除需求）
- UDP / Part Buffer / GLFW / 历史低分补测 / CI 侧——维持 YAGNI
