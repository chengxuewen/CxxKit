# Phase 3 线程亲和设计（Thread Affinity — moveToThread + Cross-Thread Delivery）

**日期**: 2026-09-10
**状态**: Approved（6 项用户裁定）
**前置**: Phase 1（D33 对象树+WeakPtr，`5d512ef`）+ Phase 2（D34 事件投递，`2c61215`）
**约束**: C18 —— 本 spec 为计划文档用中文；实现阶段提交消息与代码注释必须英文。

## 1. 目标

把 kernel 从"thread_local 单环断言"模型升级为 Qt 形线程亲和：

1. `Object::thread()` 查询 + `move_to_thread(EventLoop *)` 迁移（静态迁移语义）
2. 跨线程 `post_event`（目标环路由：receiver→所属环→投递+wake，Qt postEvent 同构）
3. **顺手项**（搭车，不单独立任务）：filter 反向清理注册表、filter 双 install 去重、DeferredDeleteEvent ctor friend 收敛、双队列顺序测试

**不在范围**：动态迁移（运行中环间迁移——静态迁移守卫排除）、事件压缩、多优先级、worker 线程池集成（thread 子库桥接另立）。

## 2. 已确认裁定

| # | 裁定 | 理由 |
|---|---|---|
| R1 | 范围 = 方案 B：线程亲和主线 + 卫生项搭车 | 卫生项中 3 个与线程模型耦合，先做必返工；独立成批不值得任务粒度 |
| R2 | 跨线程投递 = **目标环路由**（Qt 同构） | post_event(receiver, e) 从 receiver 查所属环，投到目标环队列 + wake 目标环；API 面与 Qt 心智一致 |
| R3 | moveToThread = **静态迁移**（仅两环都非运行态） | 运行中环间迁移重开 Phase 2 已闭合的 purge/pop 竞态窗口；Qt moveToThread 同样禁止目标线程运行中 |
| R4 | filter 反向清理注册表：watched→filter 反查表 | 消除 D34 弱化契约（"filter 必须比 watched 长寿"）——~filter 时自动从所有 watched 摘除 |
| R5 | filter 双 install 去重：remove-then-insert（Qt 同款） | 同 filter 重装 = 移到最新位（单次过滤） |
| R6 | DeferredDeleteEvent ctor 收敛 friend class Object | 用户无法构造 → 禁直发语义获得语言级封锁（Qt QDeferredDeleteEvent 同款） |

## 3. 架构

### 3.1 线程亲和元数据（ObjectPrivate）

```cpp
// object_p.hpp 新增
EventLoop *mThread{nullptr};   // 所属环（null = 无亲和，宿主线程未绑定）——命名 mThread 对齐 Qt thread()
```

- 构造时：`mThread = EventLoop::current()`（创建即亲和当前环——Qt 对象创建于其线程同款语义）。
- `Object::thread()` 公开查询（返回 Object* 层 EventLoop*，可能 null）。

### 3.2 move_to_thread（静态迁移）

```cpp
// object.hpp
public:
    /** @brief Migrates this object and its whole child subtree to @p target thread's loop.
     *  Static migration only: both source and target loops must not be running (exec()ing),
     *  or the call is fatal. The ThreadChangeEvent is sent to every migrated object. */
    void move_to_thread(EventLoop *target);
```

语义（全部 fatal 守卫——CXXKIT_CHECK 流式）：
1. `target != this->thread()` 检查（同环迁移 = no-op 或 fatal？**裁定：同环 no-op**——Qt 同款宽容）。
2. **静态守卫**：source 环（this->thread()）与 target 环都**非运行态**（`is_running()`——EventLoop 既有 API）。任一运行中 = fatal。target 为 null = 允许（脱离亲和，回 thread_local 语义）。
3. **子树递归**：先序遍历 children，逐个改 mThread + 发 ThreadChangeEvent（kThreadChange 已在 Event::Type 表，event() 既有空壳分支 T3 后语义为"记录新环"——实装为 no-op 钩子 + doxygen）。
4. **Event 队列条目处理**：静态守卫保证两环皆未运行 → 队列无并发派发；该对象在旧环的 pending 条目**原地保留**（EventEntry.mReceiver 指针不变，目标环变了但条目在旧环队列）→ **问题**：旧环醒来会派发给已属新环的对象。**裁定：move 时把旧环队列中该子树的 pending 条目迁移到新环队列**（purge-from-old + enqueue-to-new，锁两环队列——静态守卫下无死锁风险：两环皆未运行无派发线程竞争，按地址序加锁防未来扩展）。Event 对象指针随条目走（所有权不重分配）。
5. **定时器不迁移**：EventLoop::start_timer 是环级 API（timer 在环上不在对象上），对象迁移不影响既有 timer（它们是 std::function 闭包，无 Object 绑定）——doxygen 明示。

### 3.3 跨线程 post_event（目标环路由）

Phase 2 形态：`post_event` 强制 `EventLoop::current()`（投递者所在环）。
Phase 3 形态：

```cpp
void Object::post_event(Object *receiver, Event *event)
{
    // 守卫不变：null / kDeferredDelete 拒绝
    EventLoop *loop = receiver->d_func()->mThread;   // 目标环路由（R2）
    CXXKIT_CHECK(loop != nullptr) << "post_event: receiver has no thread affinity";
    EventLoop::enqueue_event(loop, receiver, event);  // 签名扩环参数
    loop->wake_up();                                   // 跨环唤醒
}
```

- **enqueue_event 扩签名**：`static void enqueue_event(EventLoop *loop, Object *, Event *)`（Phase 2 是 current() 内部取环——改为显式入参，current 形态由调用方决定）。
- **wake_up 跨线程安全性**：AbstractEventDispatcher::wake_up 契约审查——uv 形态（uv_async_send 线程安全 ✓）、qt 形态（QMetaObject::invokeMethod QueuedConnection 线程安全 ✓）、FakeDispatcher（测试用，加锁或原子）。**spec 要求**：wake_up 必须 thread-safe（已是两引擎事实标准，落 doxygen 契约）。
- **delete_later 同步升级**：delete_later 从"current() 语义"改为"this->thread() 目标环路由"——**D33 的 exec-only fatal 语义变更**：对象有亲和环（哪怕别的线程在跑）即可 delete_later（投过去由目标环执行）。无亲和（mThread null）= 保持 current() 回退 + fatal（兼容现有三态测试）。**这是 D33 语义的正式演进，D35 记录**。
- purge_pending 同理扩签名（EventLoop *loop 显式参数——~Object 清理时按 this->thread() 路由，兼清 current() 环的残留——**裁定：purge 清两处**：mThread 环 + current() 环（若有），双保险静态迁移期不一致窗口）。

### 3.4 顺手项

1. **filter 反向注册表**（R4）：`ObjectPrivate::mFilters`（我过滤谁不改变）+ 新增 watched 侧不建表——反向表挂在 **filter 对象侧**：`ObjectPrivate::mWatching`（我在过滤谁）。install 时 filter->d->mWatching.push_back(watched)；~Object（filter 亡）时遍历 mWatching 逐个 remove_event_filter 自身。开销：install/remove O(watched 数)。
2. **filter 双 install 去重**（R5）：install 先 `remove_event_filter(filter)` 再 push_back——语义 = 重装移到最新位。
3. **DeferredDeleteEvent ctor friend**（R6）：`DeferredDeleteEvent() {}` 改 private + `friend class Object;`——delete_later（Object 成员）可构造，用户不可。
4. **双队列顺序测试**：一个用例钉 post(fn) 先于 post_event(Event) 执行（同轮两段顺序）。

## 4. 语义裁定细节（写入 D35）

1. **创建即亲和**：`Object::Object(parent)` 时 mThread = current()——嵌套环场景取最内层（thread_local 即最内层）。
2. **静态迁移守卫的检查点**：move_to_thread 进入时检查 source/target is_running()——若 target == source no-op；若 target null 仅检查 source。
3. **队列条目随迁的锁序**：静态守卫下无派发竞争，但 purge/enqueue 拿锁仍需顺序——按 EventLoop 地址序（this 排序）加锁（防御未来动态迁移扩展；实现为辅助函数 lock_two_loops）。
4. **ThreadChangeEvent 派发方式**：send_event 直发（同步，迁移调用栈内）——对象此时逻辑上已在新线程上下文。event() 的 kThreadChange 分支实装：更新内部状态（当前为 no-op 钩子 + doxygen"派生类可覆写感知迁移"）。
5. **wake_up 契约**：AbstractEventDispatcher::wake_up doxygen 升级为"must be thread-safe"；uv_async_send/QMetaObject::invokeMethod 已满足；FakeDispatcher 补 mutex（测试基建）。
6. **post_event 无亲和 receiver**：mThread null = fatal（消息指引：构造于环内或 move_to_thread）。**变更面**：Phase 2 测试里 current 形态投递的对象（创建于 exec 闭包内）天然有 mThread=current——既有用例兼容。

## 5. 测试策略

- tst_object 扩展：thread() 查询、创建即亲和、move_to_thread 静态守卫（运行中 fatal ×2）、子树递归迁移（ChildEvent 链 + thread() 全对）、队列条目随迁（旧环 pending → move → 新环派发收到）、跨线程 post_event（线程 A 投、线程 B 环派发——std::thread 同步栅栏）、filter 反向清理（filter 先亡 → watched 链自动摘除 → 事件直达）、双 install 去重（单次回调）、DeferredDeleteEvent 构造不可达（编译期——static_assert 或 SFINAE 探针，运行期用 deleted 形态测不了则 doxygen+代码审查兜底）、双队列顺序。
- delete_later 语义演进：有亲和对象跨线程 delete_later（新）；无亲和 current fatal（既有三态保留）。
- ASAN：跨线程用例 + 队列随迁零诊断。
- 门禁：81 套件基线 + check.sh 8/8。

## 6. 量级

- kernel：object.hpp/cpp（thread/move_to_thread/mThread/post_event 路由/delete_later 演进/purge 扩签名）+ event_loop.hpp/cpp（enqueue 扩签名/purge 两环/wake_up 契约）+ abstract_event_dispatcher.hpp（wake_up doxygen）+ event.hpp/cpp（ctor friend）+ object_p.hpp（mThread/mWatching）≈ **350-400 行**
- 测试：tst_object +10 用例 ≈ 300 行
- 无新文件、无 CMake 改动、FakeDispatcher 补 mutex（测试基建 ~10 行）

## 7. 已知限制（D35 备案）

1. 动态迁移（运行中环间）不支持——静态守卫 fatal；Phase 4+ 需求触发重审（需队列条目失效标记机制）
2. moveToThread 不迁移定时器（环级资源）；不迁移 signal 连接（signals 与线程无关——连接是对象级）
3. Application 单例仍注释态——线程亲和无需 qApp（环指针直查）；Qt 的 QThread::current 概念不引入
4. 无亲和对象（mThread null）的 post_event 仍 fatal——指引构造于环内或显式迁移
