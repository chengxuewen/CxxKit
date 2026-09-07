# CxxKit EventLoop 一期实施计划（kernel + uv + qt）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补全 kernel EventLoop 骨架为真实 Qt 式事件循环（dispatcher 抽象 + 跨线程 post + 定时器 + connect_queued），交付 uv（独立循环）与 qt（嵌入宿主）两个 opt-in 引擎子库。

**Architecture:** EventLoop 为用户面壳（持有 `unique_ptr<AbstractEventDispatcher>`），5 纯虚 dispatcher 抽象零平台代码；UvEventDispatcher（vendored libuv，uv_async 门铃 + uv_timer）与 QtEventDispatcher（invokeMethod 门铃 + QTimer）两个注入式引擎；connect_queued header-only 自由函数返回 Connection。

**Tech Stack:** C++11（库）/CMake 3.15+/vendored libuv 1.44+（.7z+stamp）/Qt5.10+（系统包，仅 qt 子库）

**Spec:** `docs/superpowers/specs/2026-09-04-event-loop-design.md` + 收敛 bundle `docs/superpowers/plans/2026-09-04-event-loop-hyperplan-bundle.md`（D1-D16 裁定为硬约束）+ 审核 findings `docs/superpowers/specs/2026-09-04-event-loop-review-findings.md`

## Global Constraints

- 库代码 C++11 严格门禁（禁 `_t<`模板外露/`if constexpr`/init-capture/泛型 lambda/`0b`数字分隔符）；测试 target 默认跟随主标准 11，C++14 语法用 `#if CXXKIT_CC_CPP14_OR_GREATER` 或 `utils::make_move_wrapper`（S12）
- 扁平 `cxxkit::` 命名空间；函数/局部 snake_case；成员 `mPascalCase`；宏 `CXXKIT_*`；枚举值 `kPascalCase`（项目 spec §1 矩阵）
- 内部 include 一律尖括号 `<cxxkit/...>`（D8）；libuv 头用 `<cxxkit/3rdparty/libuv/uv.h>`（D6/S3/P5）
- target 注册全走 `cxxkit_add_library`/`cxxkit_add_test`/`cxxkit_add_executable`（C10）；**clang-format 禁碰 CMakeLists.txt**
- 禁 `rm -rf build`（C6）：stamp 缓存即全库缓存；单包清 `rm -rf build/3rdparty/<pkg>-<buildtype>` 合规
- 头文件守卫全 `#pragma once`；新头配对许可横幅（`/*** Library: CxxKit ... ***/`）+ doxygen 注释
- 头进 target 源列表（GLOB CONFIGURE_DEPENDS，D15）——kernel CMakeLists 预计零改动（L3）
- 时序断言宽松下界 + 事件驱动（Semaphore/cond），禁紧 sleep（P3；C14/PIT-26/task_queue_thread flaky 三前科）
- 每任务完成后：`cmake --build build --parallel` 0 error + 定向 ctest 绿 + `clang-format --dry-run --Werror <touched .hpp/.cpp>` 干净
- 大 CMake 变更后立即 `cmake -S . -B build` 验证（edit-safety：configure 失败按序修第一个）
- SDD 执行纪律：implementer 小块写+grep 自验+编译器裁判；损坏即删重写禁增量修补；edit 工具单行 replace + 多行 lines = 插入事故（PIT 已知）——多行改动用 pos+end 范围

**估时基线：50h ±10%（D16）。每任务标注估时；单任务超 8h 须再拆。**

---

## 前置任务

### Task 0: EventLoopThread 死骨架处置（ODR 前置条件）

**Files:**
- Modify: `cxxkit/thread/event_loop_thread.hpp:35-47`（`#if 0` 块）
- Modify: `.agents/memorys/pitfalls.md`（记录条目）

**Interfaces:**
- Produces: 无 API。消除 `EventLoopThreadPrivate` forward-declare 与 kernel `EventLoopPrivate` 的同名 ODR 隐患（shared 构建前置条件）

- [ ] **Step 1: 删除 `#if 0` 块**（35-47 行整块），文件仅剩 include + 空 namespace

- [ ] **Step 2: 验证**：`cmake --build build --target cxxkit_thread 2>&1 | tail -1` → 0 error；`ctest --test-dir build -R thread` 绿

- [ ] **Step 3: 记 PIT**（forward-declare 同名类跨子库 = shared 构建潜在 ODR；处置=死骨架即删不留半声明），追加 pitfalls.md

- [ ] **Step 4: Commit** `fix(thread): remove dead #if 0 EventLoopThread skeleton (ODR hazard, D14)`

---

## Phase A：kernel 抽象 + 壳 + connect_queued（估时 ~14h）

### Task A1: AbstractEventDispatcher 头 + 生命周期契约节 + Glossary（spec 同步）

**Files:**
- Create: `cxxkit/kernel/abstract_event_dispatcher.hpp`
- Modify: `docs/superpowers/specs/2026-09-04-event-loop-design.md`（Glossary/I 集/契约节——文字活与头文件同任务交付）

**Interfaces:**
- Produces: `class AbstractEventDispatcher` —— `virtual bool process_events(EventLoop::ProcessFlags flags) = 0; virtual void wake_up() = 0; virtual void interrupt() = 0; virtual void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) = 0; virtual void stop_timer(int timer_id) = 0;` + `CXXKIT_DISABLE_COPY_MOVE`；前置声明 `class EventLoop;`，include `<cxxkit/kernel/event_loop.hpp>`（ProcessFlags 完整类型，M8 包含方向裁定：dispatcher 头 include event_loop.hpp；EventLoop 侧经 unique_ptr 前置声明反向，无循环）

- [ ] **Step 1: 写头**（许可横幅 + `#pragma once` + 上述 5 纯虚 + 虚析构 + DISABLE_COPY_MOVE + 类注释一行定位句「EventLoop is the user-facing handle; AbstractEventDispatcher is the driver interface; Uv/Qt dispatchers are concrete drivers」（D13））

- [ ] **Step 2: 编译验证**（头尚无消费者，仅语法）: `g++ -std=c++11 -fsyntax-only -I cxxkit cxxkit/kernel/abstract_event_dispatcher.hpp` 或直接全量 build

- [ ] **Step 3: spec 文字同步**：§3 后加 Glossary（两词制 EventLoop/Dispatcher，"engine" 消灭 D13）；新小节「引擎契约不变量」I1-I7 表（bundle §1 原文，含级别列+覆盖用例列）；「生命周期契约」节（M5 四条：析构线程约束/uv 析构纪律/Qt context 顺序/connect_queued loop 存活约束+null CHECK）；§1 非目标加 A5 一行（回调异常=abort 契约 D6）；§4.1 socket 预留段**删除**改 §9 二期行（D2：签名完整 + 新虚函数带默认空实现 + @since + 源码级兼容备注）；§4.1/§4.2 各加线程矩阵两行表；壳层裁定规则（strategist 三问+壳偏置）进附录节

- [ ] **Step 4: format 验证** + 全量 build

- [ ] **Step 5: Commit** `feat(kernel): AbstractEventDispatcher interface + spec contract sections (I1-I7, M5/M8/D2/D6/D13)`

### Task A2: EventLoop 壳填实现（ctor 收紧 + post/exit 耦合 + maximumTime 合成）

**Files:**
- Modify: `cxxkit/kernel/event_loop.hpp` / `event_loop.cpp` / `detail/event_loop_p.hpp`
- Create: `tests/tst_event_loop.cpp` + fake dispatcher `tests/fake_dispatcher.hpp`（header-only，S11 裁定）
- Modify: `tests/CMakeLists.txt`（注册 tst_event_loop）

**Interfaces:**
- Consumes: Task A1 的 AbstractEventDispatcher
- Produces: `explicit EventLoop(std::unique_ptr<AbstractEventDispatcher> dispatcher, Object *parent = nullptr)`（骨架无参 ctor 删除，D 裁定 M8；null → `CXXKIT_CHECK(dispatcher != nullptr, ...)`）；`void post(std::function<void()> fn)`（mutex 队列 + dispatcher->wake_up()）；`int start_timer(uint64_t interval_ms, std::function<void()> fn, bool repeat = true)`（原子自增 id；repeat=false 壳包装：fn 包成「先 stop_timer(id) 再调用」闭包，M3）；`void stop_timer(int id)`；`void exit(int)`/`quit()` 内部调 wake_up（M4/D7）；exec 循环每轮末尾（清空后、阻塞前）复查 mExit/mInterrupt（D7 三检查点）；`bool process_events(ProcessFlags)` 委托；`bool process_events(ProcessFlags, uint64_t maximum_ms)`（骨架 `int maximumTime` 改 uint64_t，D10 壳合成：一次性 interrupt 定时器 + kWaitForMoreEvents 循环）；嵌套 exec：mInExec 置位补齐 + 返回 -1（S7）
- Produces（测试件）: `FakeDispatcher`（tests/fake_dispatcher.hpp）：记录 calls 计数 + start_timer (id,interval,fn) 三元组快照 + 手动 kick 接口 + 可编程 process_events 返回值；防忙旋用法=先 post 一枚 exit 任务再 kick

- [ ] **Step 1: 写失败测试**（tst_event_loop.cpp）：≥8 用例（P1 清单）——ExecRunsUntilExitRetCode / ExitBeforeExecReturnsSentinel / NestedExecReturnsMinusOne / PostFifoDrainOrder / StartTimerAssignsUniqueIds / StartTimerOneShotFiresOnce（M3 验证）/ StopTimerForwardsId / ProcessEventsTimeoutReturns（maximumTime 合成）/ ProcessEventsReturnPassthrough。fake dispatcher kick 模型：kick → 处理 post 队列 → 回调已 post 的 exit → exec 结束

- [ ] **Step 2: 跑测试确认失败**（编译失败=EventLoop 无新签名，符合预期）

- [ ] **Step 3: 写 EventLoop 实现**（按 Interfaces 逐条；exit→wake_up 耦合在 exit()/quit()/interrupt 委托里）

- [ ] **Step 4: 跑测试至绿**：`cmake --build build --parallel && ctest --test-dir build -R event_loop --output-on-failure`

- [ ] **Step 5: 全量回归**：`ctest --test-dir build` 72/72 不破 + format

- [ ] **Step 6: Commit** `feat(kernel): EventLoop shell — dispatcher injection, post, one-shot timer wrap, exit→wake_up coupling, maximumTime synthesis (M3/M4/M8/D7/D10)`

### Task A3: connect_queued（header-only）

**Files:**
- Create: `cxxkit/kernel/connect_queued.hpp`
- Modify: `tests/tst_event_loop.cpp`（追加用例）

**Interfaces:**
- Consumes: `cxxkit::Signal<Args...>`（kernel/signals.hpp 已有）+ EventLoop::post
- Produces: `template <typename... Args> Connection connect_queued(Signal<Args...>& sig, EventLoop* loop, std::function<void(Args...)> fn)`——返回 Connection（M1）；实现 `sig.connect([loop, fn](const Args&... args) { loop->post(std::bind(fn, args...)); });`（M2 C++11：std::bind decay 拷贝一次，禁 init-capture）；loop null → CXXKIT_CHECK

- [ ] **Step 1: 失败测试**：ConnectQueuedDeliversOnLoopThread（fake loop 线程标记）/ ConnectQueuedReturnsConnectionForDisconnect / ConnectQueuedArgsCopiedNotReferenced（emit 后改源，投递值不变）

- [ ] **Step 2: 确认失败** → **Step 3: 实现**（注意：std::bind 绑定 Args 的 decay 拷贝；文档注释写明值拷贝语义 + I7 引用「disconnect 停后续投递不追已入队」）→ **Step 4: 绿** → **Step 5: format+全量** → **Step 6: Commit** `feat(kernel): connect_queued — thread-hopping signal wrapper returning Connection (M1/M2)`

---

## Phase B：uv 子库（估时 ~16h，与 Phase C 可并行但共享文件须串行合并点）

### Task B1: libuv vendored + FindWrapLibuv（与 A 并行的基建）

**Files:**
- Create: `3rdparty/libuv-v1.49.2.7z`（从零打包：官方 tarball → `cmake -E tar cvf libuv-v1.49.2.7z --format=7zip <dir>`，P6：归档当前不存在）
- Create: `cmake/wrap/FindWrapLibuv.cmake`（**照 FindWrapLibyuv 模板**，S1：vendored 源码链非 breakpad vcpkg 链）
- Modify: 顶层 CMakeLists（uv 子目录 add + `CXXKIT_ENABLE_LIB_UV` option + VENDORED_FIND_DEPS append `WrapLibuv`）

**Interfaces:**
- Produces: `CxxKitWrapLibuv::WrapLibuv` STATIC IMPORTED target；头暂存 `install/include/cxxkit/3rdparty/libuv/`（D6）；`INTERFACE_LINK_LIBRARIES Threads::Threads;${CMAKE_DL_LIBS}`（S1 手搓分支必备；libuv ≥1.44 自带 libuvConfig.cmake 时优先 find_package(PATHS ... NO_DEFAULT_PATH)）；wrap 头部 `if(TARGET CxxKitWrapLibuv::WrapLibuv) return() endif()`（PIT-34）

- [ ] **Step 1: 下载 libuv v1.49.2 tarball → 7z 打包入库**（`cmake -E tar cvf` 命令见上；禁第三方 7z 工具依赖）

- [ ] **Step 2: FindWrapLibuv.cmake**（Libyuv 模板复制改造：源目录 stamp + 外部 cmake 构建 `-DLIBUV_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF` + install 暂存 + find_package libuv PATHS 尝试 + fallback 手搓 IMPORTED + INTERFACE_LINK_LIBRARIES 线程/dl + stamp 幂等）

- [ ] **Step 3: 配置验证**：`cmake -S . -B build -DCXXKIT_ENABLE_LIB_UV=ON` 0 error；`ls build/3rdparty/libuv*` stamp 存在；二次 configure 秒级（stamp 命中）

- [ ] **Step 4: Commit** `feat(uv): vendored libuv 1.49.2 + FindWrapLibuv (libyuv-style, D6 namespace staging)`

### Task B2: UvEventDispatcher 实现 + 工厂

**Files:**
- Create: `cxxkit/uv/uv_global.hpp`（CXXKIT_UV_API）/ `uv_event_dispatcher.hpp` / `uv_event_dispatcher.cpp` / `detail/uv_event_dispatcher_p.hpp` / `dispatcher_factory.hpp`（`std::unique_ptr<AbstractEventDispatcher> make_uv_dispatcher();`——D12 扁平名）/ `dispatcher_factory.cpp`
- Create: `cxxkit/uv/CMakeLists.txt`（cxxkit_add_library + EXPORT_NAME uv + install 四件套 S4：pkg_config + install(TARGETS EXPORT cxxkitTargets) + install(DIRECTORY) + install_public_wrap_headers）

**Interfaces:**
- Consumes: AbstractEventDispatcher（A1）+ WrapLibuv
- Produces: `class CXXKIT_UV_API UvEventDispatcher : public AbstractEventDispatcher`——pimpl 持 `uv_loop_t` + `uv_async_t`（门铃）+ `std::map<int, uv_timer_t*>`；process_events=uv_run(NOWAIT/NONBLOCK 两态)+排空 post 队列+空队列返回 false 显式跟踪（P2-7）；wake_up=队列 push 后 uv_async_send（幂等合并 libuv 保证，I4）；interrupt=原子 flag+async 踢；start_timer/stop_timer=uv_timer_start/stop + debug thread-id 断言（I1/M7 仅限 loop 线程）；析构=uv_close 全 handle + 收尾 uv_run 排空（I6）；嵌套 process_events → debug 断言（I5/S6：文案「re-entered from a callback — libuv has no nested iteration. Return and schedule with post()」E2）
- Produces: `make_uv_dispatcher()`（唯一工厂，非 make_default——D12 扁平）

- [ ] **Step 1: 失败测试**（tests/tst_uv_event_dispatcher.cpp，真实 libuv）：7 场景（D11 维持）——PostCrossThreadDrains / TimerFiresWithLowerBound / TimerOneShotViaShell / CallbackStopRestart（I3）/ WakeUpStormNThreadsMTasks（I4）/ NestedProcessEventsAsserts（I5，debug 断言路径）/ DestructorCleansUpHandles（I6，ASAN watch）+ EmptyQueueReturnsFalseAfterDrain。时序纪律：Semaphore 事件驱动禁 sleep（P3）

- [ ] **Step 2: 确认失败** → **Step 3: 实现**（小块写+grep 自验；uv 回调是 C 函数指针 → 静态 trampoline + handle->data 回 C++ 对象，Node HandleWrap 模式）→ **Step 4: 定向绿** `ctest -R uv_event_dispatcher` → **Step 5: 全量 + asan 树**（D9：`cmake -S . -B build-asan -DCXXKIT_BUILD_SANITIZERS=ON -DCXXKIT_ENABLE_LIB_UV=ON` 已存在时增量构建 + LSAN supp）→ **Step 6: Commit** `feat(uv): UvEventDispatcher + make_uv_dispatcher factory (I1-I6, D9/D11/D12)`

### Task B3: exp_event_loop example + README 口径

**Files:**
- Create: `examples/exp_event_loop.cpp`（D12 钉板全文：include `<cxxkit/uv/uv_event_dispatcher.hpp>` + `<cxxkit/kernel/event_loop.hpp>`；`EventLoop loop(cxxkit::make_uv_dispatcher());` + start_timer 3 次 tick 后 post exit；打印 tick 计数；CI 可跑 rc=0）
- Modify: `examples/CMakeLists.txt`（cxxkit_add_executable + UV=ON 门控）+ `README.md`（子库表 +19 行：`cxxkit::uv` / Examples 表 +1 行）

- [ ] **Step 1: example 实现** → **Step 2: 手跑 rc=0 验证** → **Step 3: README 两表 + spec §10 口径同步** → **Step 4: Commit** `feat(examples): exp_event_loop + README uv sublibrary rows (D12)`

---

## Phase C：qt 子库（估时 ~7h，与 B 并行、共享文件串行合并）

### Task C1: QtEventDispatcher + M9/M10

**Files:**
- Create: `cxxkit/qt/qt_global.hpp`（CXXKIT_QT_API）/ `qt_event_dispatcher.hpp` / `qt_event_dispatcher.cpp` / `cxxkit/qt/CMakeLists.txt`
- Modify: 顶层 CMakeLists（`CXXKIT_ENABLE_LIB_QT` option + add_subdirectory）/ `CxxKitConfig.cmake.in`（**独立 Qt 分支 M10**：`#cmakedefine CXXKIT_QT_ENABLED` + `if(CXXKIT_QT_ENABLED) find_dependency(Qt6 COMPONENTS Core) endif()`——不带 NO_DEFAULT_PATH）

**Interfaces:**
- Consumes: AbstractEventDispatcher（A1）
- Produces: `class CXXKIT_QT_API QtEventDispatcher : public AbstractEventDispatcher`——ctor `explicit QtEventDispatcher(QObject* context = nullptr)`（默认 qApp；**QPointer 守门铃** receiver，F8-Qt）；post=队列+`QMetaObject::invokeMethod(receiver, [this]{drain()}, Qt::QueuedConnection)`（S10：不合并靠排空幂等，D8 延迟不承诺）；timer=每 id 一 QTimer（宿主一等源）；process_events 非阻塞=processEvents() 直通、嵌套=QEventLoop 直通（I5 Qt 侧）；析构顺序契约注释（I6：dispatcher 必须先于 context 析构）；**CMakeLists 内 `set_property(TARGET cxxkit_qt PROPERTY CXX_STANDARD 17)`**（M9，helper 无此参数勿扩）；Qt ≥5.10 注释（invokeMethod functor 下限）
- Produces: qt 子库 CMake-only 消费声明（S2：不出 .pc，Qt6Core.pc Requires 链复杂收益低）

- [ ] **Step 1: 失败测试**（tests/tst_qt_event_dispatcher.cpp——**仅编译目标**：cxxkit_add_test 注册但 CI 不跑（headless 无 Qt 运行时），本机有 Qt 时 `Qt6::Core` 纯 QCoreApplication 可跑部分用例；测试写全 7 场景同 uv 对齐+context 析构顺序用例）→ **Step 2: 实现** → **Step 3: 本机无 Qt 时的验证路径**：`g++ -std=c++17 -fsyntax-only -I/usr/include/x86_64-linux-gnu/qt6 -I<qt include 链>` 若本机装 qt6-base-dev；否则编译验证推迟到 CI 步骤并在任务内注明 → **Step 4: Commit** `feat(qt): QtEventDispatcher — QPointer bell, QTimer map, C++17 target property (M9/M10/I5/I6)`

### Task C2: exp_qt_embed + smoke 文档 + CI 门禁

**Files:**
- Create: `examples/exp_qt_embed.cpp`（spec §5.2 端到端骨架可编译版：QCoreApplication + EventLoop(make_qt_dispatcher) + connect_queued + timer + app.exec）+ `docs/qt-embed-smoke.md`（imgui-smoke 格式 S13：Prerequisites/8 步/每步可观察 Expected：线程 id/tick 计数/rc；断连步=「documented-pattern」表述：析构前先 disconnect → 无迟到回调，引用 I7）
- Modify: `examples/CMakeLists.txt`（QT=ON 门控）+ `.github/workflows`（qt6-base-dev 安装步 + qt 子库编译门禁 job/step）+ README Examples 行

- [ ] **Step 1: example + smoke 落盘** → **Step 2: 本机若可编译则编译验证，否则标注 CI-only** → **Step 3: CI workflow 改** → **Step 4: Commit** `feat(examples): exp_qt_embed + qt-embed-smoke manual gate + CI qt compile check (S13)`

---

## 收尾 Phase D（估时 ~6h）

### Task D1: S5 既有 bug 顺手修 + S11 覆盖基线 + spec 终稿口径

**Files:**
- Modify: `cmake/CxxKitPkgConfigHelpers.cmake`（删重复 Breakpad 分支：首分支误映射 `-lTracyClient`，S5——独立小提交）
- Modify: `scripts/coverage.sh` 无需改（全口径自动），核对 `build-cov/coverage/summary.txt` 43-file 基线 + 新增 uv .cpp 行水位 ≥85%（S11）
- Modify: spec §8 asio 备忘措辞（D15 判据「契约级不变量零豁免——shim 合法豁免非法」）+ §9 二期行终稿

- [ ] **Step 1: crash.pc 修复**：`pkg-config --libs cxxkit-crash` 前后对照（修前含 -lTracyClient 错链）→ 修后 `-lbreakpad_client -pthread` → Commit `fix(cmake): crash pc link chain — remove duplicated Breakpad branch mapping TracyClient (S5)`
- [ ] **Step 2: 覆盖率核对**：`bash scripts/coverage.sh build-cov | grep "Line-weighted"` ≥80% 且 uv 行 ≥85%
- [ ] **Step 3: spec 终稿** → Commit `docs(spec): asio admission criteria final wording (D15) + phase-2 notifier line (D2)`

### Task D2: 全链验收（bundle §4 门禁）

- [ ] **Step 1: 主树全量**：build 0 error + `ctest --test-dir build` 全绿（72+新增）
- [ ] **Step 2: shared 树**：build-shared 配置 UV=ON 增量 + ctest 全绿（PIC 先例：新静态 libuv 必须容忍链入 .so——wrap 已带 PIC 需确认）
- [ ] **Step 3: asan 树 UV=ON**：零诊断（D9；libuv 自身 LSAN 抑制若需 → scripts/lsan.supp 追加）
- [ ] **Step 4: coverage**：43+file 全口径 ≥80%
- [ ] **Step 5: check.sh 全链**：`bash scripts/check.sh` exit=0 ALL CHECKS PASSED
- [ ] **Step 6: rc 矩阵**：exp_event_loop rc=0；exp_qt_embed 编译产物存在（运行=smoke 人工门禁）
- [ ] **Step 7: memory 收录**：status.md 新段落 + decisions.md D30（事件循环子系统一期落地：I 集/D1-D16 关键裁定/50h 实际对照）+ conventions 若有新约定
- [ ] **Step 8: 最终提交**（按任务逐个已提交，此处仅 fixup 若有）

---

## Self-Review 记录

- **Spec 覆盖**：M1-M10 全部落位（M1/A3、M2/A3、M3/A2、M4/A2、M5/A1 文字+A2 测试、M6/A1+I1、M7/A1+I1、M8/A1/A2、M9/C1、M10/C1）；S1-S14 落位（S1/B1、S2/C1、S3/B1、S4/B2、S5/D1、S6/B2、S7/A2、S8/A3 注释、S9/D1 措辞、S10/B2+C1、S11/D1、S12/全局约束、S13/C2、S14/A1 spec 同步）；P1-P6 落位（P1/A2、P2/B2、P3/全局、P4/D2、P5/全局+D14、P6/B1）
- **占位符扫描**：无 TBD/TODO；C1 Step 3 有条件分支（本机 Qt 有无）已给两条显式路径
- **类型一致性**：`make_uv_dispatcher()`（B2/B3/A3 无涉）/`make_qt_dispatcher`（C1 未显式给——C1 补充说明：qt 侧用户直接 `new QtEventDispatcher()` 或后续补工厂；exp_qt_embed 用直接构造，避免一期再扩 API）修正：exp_qt_embed 用 `std::make_unique<QtEventDispatcher>()`（C++11 下 `std::unique_ptr<QtEventDispatcher>(new QtEventDispatcher())`）
