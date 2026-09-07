# event-loop spec 审核发现汇总（四方 APPROVE-WITH-FIXES）

日期：2026-09-04。四个独立审核员（API/并发/构建/测试）对 `docs/superpowers/specs/2026-09-04-event-loop-design.md` 的全部发现，经 lead 归并去重。分级：MUST = spec 修订必落（实施前钉死）；SHOULD = spec 补条目；PLAN = 落实施计划即可。

## 归并后的必修项（MUST，按依赖顺序）

### M1. connect_queued 返回 Connection（API-CRITICAL F8）
返回 void 丢弃 Connection → "接收方销毁须先 disconnect" 的安全声明在 API 上不可实现。签名改 `template <typename... Args> Connection connect_queued(Signal<Args...>&, EventLoop*, std::function<void(Args...)>)`。

### M2. connect_queued 示例实现是 C++14（API-HIGH F9）
`[fn, args...]` init-capture 是 C++14 语法，C4 门禁（库代码 C++11）编译失败。C++11 写法：`loop->post(std::bind(fn, args...))`（bind decay 拷贝一次，转 std::function<void()> 天然成立）。std::bind 惯用但注意：参数是 decay-copy，文档写明"emit 时值拷贝一次"不变。

### M3. repeat 标志翻译策略（API-HIGH F1 + TEST-HIGH T1）
EventLoop 公开 API 有 `repeat=true`，dispatcher 纯虚没有——单次定时器在引擎层无法表达。**裁定：壳层包装**——EventLoop::start_timer 在 repeat=false 时把 fn 包成"先 stop_timer(id) 再调用"的闭包传给 dispatcher，对 uv/qt 零改动（uv_timer/QTimer 均默认重复，语义一致）。测试用例 `StartTimerOneShotFiresOnce` 进 kernel 壳层清单。

### M4. exit/interrupt 必须耦合 wake_up（CONC-HIGH F1）
骨架 exec = `while(!mExit) process_events(kWaitForMoreEvents)`；跨线程 exit() 只置 flag 不踢 dispatcher → loop 线程阻塞永不醒。spec 明文：exit()/interrupt() 实现内部必须调 wake_up()。interrupt flag 检查点：process_events 入口 + 每轮事件处理前。

### M5. 生命周期契约章节（CONC-HIGH F2 + F8-Qt + API-F10 loop 侧）
新增 spec 小节：
- dispatcher/EventLoop 析构约束：必须在 loop 线程，或 loop 线程已确认停止；uv 侧析构纪律 = uv_close(async/timers) + 收尾 uv_run 排空 + 无在飞 send 保护（停止标志）；析构后 post() = UB 文档明示
- Qt context 顺序契约：dispatcher 必须先于 context 析构（声明顺序示例进 spec）；实现建议 QPointer 守门铃
- connect_queued loop 侧：loop 必须比 connection 活得久；loop 为 null → CXXKIT_CHECK
- 测试矩阵加跨线程 post 与析构交叠用例（ASAN 下跑）

### M6. 单消费者契约（CONC-HIGH F3）
明文：同一 dispatcher 的 process_events/exec 只允许单一 loop 线程调用；post/wake_up/exit 可任意线程（exit 经 wake_up 耦合后安全）。uv 版 debug 构建加 thread id 断言（race_checker 先例）。

### M7. timer 控制线程契约（TEST-HIGH T2）
libuv 除 async_send 外全非线程安全。裁定：**start_timer/stop_timer 仅限 loop 线程调用**（Qt 同款限制），uv 版 debug 断言，文档写明；跨线程需要时用 post 包装。

### M8. dispatcher 接口签名修正（API-HIGH F2 + F5 + F6）
- start_timer 不加 repeat（M3 壳层包装已定）；签名最终形态：`virtual bool process_events(ProcessFlags flags) = 0; virtual void wake_up() = 0; virtual void interrupt() = 0; virtual void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) = 0; virtual void stop_timer(int timer_id) = 0;`
- register_socket_notifier 预留：**改非纯虚**（默认实现 CHECK(false)），写全签名（`int fd` 一期 Linux 语义 + `enum class SocketEvent { kRead, kWrite }` + 回调 std::function<void(SocketEvent)>；Windows 语义二期再议）——二期实现时已有明确落点
- EventLoop ctor 写死：`explicit EventLoop(std::unique_ptr<AbstractEventDispatcher> dispatcher, Object *parent = nullptr)`（骨架无参 ctor 删除——不再有无 dispatcher 的合法态）；null dispatcher 语义随 ctor 收紧而消失
- process_events(flags, int maximumTime) 重载：壳层合成（一次性 interrupt 定时器 + kWaitForMoreEvents 循环，Qt QEventLoop 同款）；`int maximumTime` 与 `uint64_t interval_ms` 统一为 uint64_t（骨架签名改造，int 溢出语义顺带消除）
- AbstractEventDispatcher 补 `CXXKIT_DISABLE_COPY_MOVE`；ProcessFlags 限定 `EventLoop::ProcessFlags`，包含方向：dispatcher 头 include event_loop.hpp（EventLoop 侧前置声明，无循环）

### M9. qt 子库 C++17（BUILD-HIGH H1）
cxxkit_qt 目标级 `set_property(TARGET cxxkit_qt PROPERTY CXX_STANDARD 17)`（PIT-36 同款，勿扩 helper）；kernel 抽象层保持 C++11（ABI 无涉：unique_ptr 前置声明持有）。"Qt5/6 同 API"修正为 **Qt ≥5.10**（invokeMethod functor 版下限），CI 门禁装 Qt6。

### M10. cxxkitConfig 系统 Qt 分支（BUILD-HIGH H2）
现有 C11 `VENDORED_FIND_DEPS` 带 NO_DEFAULT_PATH 找不到系统 Qt。CxxKitConfig.cmake.in **新增独立分支**：configure 时 `@CXXKIT_QT_ENABLED@`，模板 `if(@CXXKIT_QT_ENABLED@) find_dependency(Qt6 COMPONENTS Core) endif()`（不带 NO_DEFAULT_PATH）。显式交付项，防 cf9e1e9 同类消费断链。

## SHOULD 项（spec 补条目）

- S1. wrap 形态改口径：FindWrapLibuv 照 **FindWrapLibyuv 模板**（vendored 源码链），非"breakpad 同款"（那是 vcpkg 链）；libuv ≥1.44 自带 libuvConfig.cmake，优先 find_package(PATHS install NO_DEFAULT_PATH)；手搓 IMPORTED 则必须补 `INTERFACE_LINK_LIBRARIES Threads::Threads ${CMAKE_DL_LIBS}`
- S2. .pc 映射：uv 出 .pc（`WrapLibuv → -luv`）；**qt 子库声明 CMake-only 消费、不出 .pc**（Qt6Core.pc 存在但 Requires 链复杂，收益低）
- S3. D6 区分：uv 头走 `cxxkit_install_public_wrap_headers`（`<cxxkit/3rdparty/libuv/uv.h>` 命名空间路径写死）；qt 系统依赖豁免 D6
- S4. install 四件套以 **imgui/media 为模板**（crash 实测缺 install 环节，勿照抄）：cxxkit_add_library + pkg_config + install(TARGETS EXPORT cxxkitTargets) + install(DIRECTORY) + uv 加 install_public_wrap_headers + 顶层 VENDORED_FIND_DEPS append（uv）/新分支（qt→M10）
- S5. 附带修复（独立小提交）：CxxKitPkgConfigHelpers Breakpad 分支重复且首分支误映射 `-lTracyClient` → crash.pc 现输出错误链接链；删一个 elseif 即修
- S6. uv 嵌套语义：qt = QEventLoop 直通；**uv 后端不支持嵌套 process_events**（libuv 无嵌套迭代语义），实现 debug 断言/告警；"回调内调 process_events"用户警示进文档
- S7. 嵌套 exec：mInExec 守卫补实现（骨架从未置位），嵌套 exec 返回 -1；exit-before-exec 语义裁定（mExit 初始值与 exec 入口复位策略写明）
- S8. disconnect 竞态警示：pal::signal 的 disconnect 不等待在飞 emit（COW 快照照常跑完）——"emit 中断连不终止当前轮"明文；安全模式 = 先断信号源或 SafetyFlag 模式
- S9. PendingTaskSafetyFlag"互补"声明修正：safe_task 仅 void() 版，带参信号类型不匹配——改表述为"仅无参信号可用，带参在 fn 体内手写 alive 检查"，或二期补泛型版
- S10. wake_up 不变量：门铃回调必须无条件锁内排空整队（含空队列虚警）；Qt invokeMethod(QueuedConnection) **不合并**（每 post 一事件），靠排空幂等兜底——"Qt 版同构"表述修正
- S11. 覆盖率口径更新：现状 43-file 80.7%（spec 写 29 是旧数）；新 .cpp 进全口径门禁，uv 测试水位承诺 85%+；fake dispatcher 放 tests/ 侧 header-only（不进门禁）；connect_queued header-only 自然不进门禁（注明即可）
- S12. 测试 C++11 备注：tests/ 默认跟随主标准（11）——闭包测试禁 init-capture/make_unique，move-only 用 make_move_wrapper（tst_task_queue_thread 惯例）或 `#if CXXKIT_CC_CPP14_OR_GREATER`
- S13. smoke 可判定性：qt-embed-smoke.md 按 imgui-smoke.md 格式（每步含可观察 Expected：线程 id/tick 计数/rc）；"断连安全"步改"documented-pattern"表述（析构前先 disconnect → 无迟到回调）
- S14. 已评估不引入记录：has_pending_events/remaining_time/awake 钩子（防后人重复发问）；"4 虚函数"计数笔误改"5 虚函数 + 1 预留"

## PLAN 项（落实施计划）

- P1. fake dispatcher 形态：记录型 test double（calls 计数 + flags/params 快照 + 手动 kick），先 post 一枚 exit 任务再 kick 防忙旋挂死；kernel 壳层 ≥8 用例（exec/exit retcode/is_running 翻转/post FIFO/timer id 原子唯一/one-shot/maximumTime 合成/process_events 返回值透传/嵌套 exec -1）
- P2. uv gtest 7 场景：多定时器交错/回调内 stop+restart/stop 未注册 id no-op/wake_up 风暴 N×M 全执行/析构清理（ASAN）/嵌套 process_events 钉住/空队列 false + 有事件 true
- P3. 时序纪律：timer 断言宽松下界 + 事件驱动（Semaphore），禁紧 sleep（C14/PIT-26 前科）
- P4. check.sh 策略：uv 进主树（构建 ~1 分钟，NETWORK=ON 先例）；asan/cov 树 crash 保守态先例（OFF）；CI 加 qt6-base-dev 编译门禁步
- P5. uv 子库源码 include 写死 `<cxxkit/3rdparty/libuv/uv.h>`；FindWrapLibuv 头部 `if(TARGET ...) return()` 守卫（PIT-34）
- P6. libuv.7z 尚不存在（30 归档核对），vendored 入库是 Phase 2 首任务

## 总体

四位审核员一致 **APPROVE-WITH-FIXES**，架构方向（壳/引擎分离、5 虚函数最小接口、mutex 队列+门铃+swap 排空、双 opt-in 子库、vendored libuv）零否决。必修 10 项全是"签名/契约/标准/导出链"级别的 spec 补条目，不动架构。修订顺序：MUST M1-M10 落 spec → SHOULD 批量补条目 → writing-plans 出实施计划（吸收 PLAN 项）。
