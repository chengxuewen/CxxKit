# cxxkit/thread AGENTS.md

## OVERVIEW

线程子库（编译型，`cxxkit::thread` target）：std::thread/condition_variable 原生原语 + 自研 task_queue 抽象，pimpl 隔离平台细节；17 头 + 14 cpp，强依赖 tools（13 次），下游依赖 thread 的子库不得反向引用。

## WHERE TO LOOK

| 任务 | 位置 | 备注 |
|------|------|------|
| 线程池（调度/优先级/active 跟踪） | `thread_pool.hpp` + `detail/thread_pool_p.hpp` + `src/thread/thread_pool.cpp`（706 行，最大） | 内部自持 `mTaskQueue`，push/pop/cancel/clear 直连队列 |
| 任务队列（core 抽象） | `task_queue.hpp` + `src/thread/task_queue.cpp` | 配套 `task.hpp`（任务体）、`task_queue_factory`（工厂）、`task_queue_thread`（队列背线程） |
| 平台线程抽象 | `platform_thread.hpp` + `detail/platform_thread_p.hpp` | 实现分文件：`platform_thread_posix.cpp`（`#if !defined(CXXKIT_OS_WIN)`）`platform_thread_win.cpp`（`#if defined(CXXKIT_OS_WIN)`），公共 `platform_thread.cpp`（344 行） |
| 信号量/同步 | `semaphore.hpp`、`spinlock.hpp`、`mutex.hpp`、`concurrent.hpp` | mutex=std 包装（Mutex/RecursiveMutex + Lock/UniqueLock/Condition 别名） |
| 线程安全诊断 | `context_checker.hpp`、`race_checker.hpp` | 编译期检查 + 运行期竞态检测（依赖 tools/checks） |
| 周期任务 | `repeating_task.hpp` + cpp | 依赖 units/time_delta + tools/clock |
| 线程内事件循环 | `event_loop_thread.hpp` + cpp | 唯一跨 kernel 依赖（`kernel/object.hpp`） |
| future/引用计数 | `future.hpp`、`reference_counter.hpp` | |

## CONVENTIONS

- **平台分支写法**：改平台实现只碰 `platform_thread_posix.cpp` / `platform_thread_win.cpp` 对应文件，条件编译守卫 `#if !defined(CXXKIT_OS_WIN)` vs `#if defined(CXXKIT_OS_WIN)`；公共逻辑（fallback、默认值、状态映射）进 `platform_thread.cpp`。
- **task_queue 生命周期**：队列归线程池/线程私有（`thread_pool.cpp` 经 `mTaskQueue` 直连，无外部注入）；消费端 pop 阻塞，cancel/clear 必须成对考虑，清空后 empty 断言成立。
- **锁选型**：短临界区/自旋可容忍 → `SpinLock`（包内 Locker，无锁睡眠）；可能阻塞/持锁久 → `Mutex` 系列；读多写少 → shared_mutex（`mutex.hpp` 内含）。默认先 mutex，性能数据证明后再换 spinlock。
- **pimpl 纪律**：私有实现进 `detail/*_p.hpp`（platform_thread_p / thread_pool_p），配对 `CXXKIT_DEFINE_DPTR`，公开头不泄漏平台类型。
- **task 语义**：任务按优先级入队，线程池调度入口统一走 `push/executor`；新增任务类型先看 `task.hpp` 已支持形态，别自造队列。

## ANTI-PATTERNS

- **win/posix 双实现只改一边**——提交前 grep 另一实现文件同步语义（改完跑 `cmake --build build`，win 分支无法本地编译验证时至少 review 对称性）。
- **向 thread 头注入 kernel/tools 之外的依赖**——依赖方向 thread → {tools, units, time, kernel}，反向即破坏无循环约束。
- **直接复用 std::thread 裸管理**——统一走 platform_thread 抽象；裸线程逃逸生命周期约束，且平台分支会失配。
- **在 task_queue 上叠加二次队列缓存**——线程池已内建队列语义，外部再包一层 = 双缓冲一致性问题。
- **spinlock 当 mutex 用**——持锁代码路径含 I/O、condition_variable 等待或嵌套锁时禁止 spinlock（自旋饿死）。