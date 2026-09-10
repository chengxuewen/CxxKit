# Phase 3 线程亲和实施计划（moveToThread + Cross-Thread Delivery）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** kernel 线程亲和——`thread()`/`move_to_thread()`（静态迁移）+ 跨线程 `post_event`（目标环路由）+ `delete_later` 亲和演进，搭车清 filter 注册表/去重/ctor friend/双队列顺序测试。

**Architecture:** `ObjectPrivate::mThread`（创建即亲和 current）为目标环路由锚点；move_to_thread 静态守卫（两环非运行态）下子树递归 + 队列条目随迁（地址序双锁）；post_event/delete_later 从 current() 语义演进为亲和优先、current 回退。

**Tech Stack:** C++11、kernel 既有文件（无新文件、无 CMake 改动）、gtest、std::thread（测试跨线程用例）。

**Spec:** `docs/superpowers/specs/2026-09-10-thread-affinity-design.md`（R1-R6 裁定 + §3 架构 + §7 限制）。

## Global Constraints

- C++11 硬门禁；禁 `rm -rf build`；snake_case/mPascalCase/kPascalCase；尖括号 include。
- **C18（本计划新约束，首份适用）**：提交消息与代码注释**必须英文**；本计划文档本身中文。检查：`git log --format="%s" -5 | grep -P "[\x{4e00}-\x{9fff}]" | wc -l` 应为 0。
- 测试 EXPECT_DEATH 空匹配器；ctest 退出码为准；`pixi run clang-format` 只碰 .hpp/.cpp。
- 套件基线 **81**（全用例进 tst_object.cpp）；终态 81。
- 提交身份 `git -c user.name=chengxuewen -c user.email=1398831004@qq.com`。
- D33/D34 既有语义不回归：delete_later 无亲和三态（exec 内合法/无环 fatal/未 exec fatal——注意 T2 后者已改未 exec fatal）与 Phase 2 派发语义（pop 锁内互斥/派发锁外）。

---

### Task 1: 亲和元数据 + move_to_thread 核心（含双队列顺序测试搭车）

**Files:**
- Modify: `cxxkit/kernel/detail/object_p.hpp`（mThread 字段）
- Modify: `cxxkit/kernel/object.hpp` / `object.cpp`（thread() 查询 + move_to_thread + 构造即亲和 + kThreadChange 分支实装）
- Modify: `cxxkit/kernel/detail/event_loop_p.hpp` + `event_loop.cpp`（take_events_for(receiver) 随迁辅助——从队列摘出匹配条目返回）
- Test: `tests/tst_object.cpp`（亲和 + 迁移用例 + 双队列顺序搭车用例）

**Interfaces:**
- Consumes: EventLoop::current()（D33）；EventLoop::is_running()（既有）；Event::Type::kThreadChange（既有 22）；children()/d_func()（D33）。
- Produces（T2/T3 消费）:
  - `ObjectPrivate::EventLoop *mThread{nullptr}`（创建即 current()，无环 null）。
  - `EventLoop *Object::thread() const`。
  - `void Object::move_to_thread(EventLoop *target)`——静态守卫（source/target 任一 is_running → fatal；同环 no-op；target null 允许=脱离亲和）+ 子树先序递归改 mThread + 逐个 `send_event(this, &ThreadChangeEvent)` + 队列条目随迁。
  - `EventLoopPrivate::std::deque<EventEntry> take_events_for(Object *receiver)`——锁内摘出 receiver==该对象 的条目（move 随迁的原语；T3 的 purge 复用此形态思路）。
  - event() 的 kThreadChange 分支：no-op 钩子 + doxygen（派生可覆写感知）。

- [ ] **Step 1: 写失败测试**（tst_object.cpp 追加）

```cpp
// ---- thread affinity (Phase 3) ----

TEST(Object, thread_reports_creation_loop)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    RecordingObject obj; // created inside exec? no — creation binds via EventLoop::current()
    // Note: current() is only set during exec. Outside exec, mThread is null.
    // Creation-affinity test must run inside exec closure.
    // ^ Executor: see Step 3 implementation note — binding happens at construction
    //   from EventLoop::current(); outside exec it is null. Adjust assertions accordingly:
    //   objects constructed OUTSIDE any exec have thread() == nullptr (current semantics).
}
```

（**plan 裁定**：spec 说"创建即亲和 current()"——而 current() 仅 exec 期间非空（D33 exec-only）。因此：**exec 闭包内构造的对象** thread()==该环；**环外构造** thread()==null。测试形态：

```cpp
TEST(Object, thread_binds_to_creation_loop)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    RecordingObject outside; // constructed outside exec -> thread() == nullptr
    EXPECT_EQ(outside.thread(), nullptr);
    loop.post([&loop]()
    {
        RecordingObject inside; // constructed inside exec -> thread() == &loop
        EXPECT_EQ(inside.thread(), &loop);
        loop.exit(0);
    });
    EXPECT_EQ(loop.exec(), 0);
}

TEST(Object, move_to_thread_relinks_subtree_and_migrates_pending_events)
{
    FakeDispatcher *d1 = new FakeDispatcher;
    FakeDispatcher *d2 = new FakeDispatcher;
    cxxkit::EventLoop loop1(std::unique_ptr<cxxkit::AbstractEventDispatcher>(d1));
    cxxkit::EventLoop loop2(std::unique_ptr<cxxkit::AbstractEventDispatcher>(d2));
    // Two loops, neither running. Objects created outside (thread()==null).
    RecordingObject parent;
    RecordingObject *child = new RecordingObject(&parent);
    EXPECT_EQ(parent.thread(), nullptr);

    // Migrate to loop2 (source affinity is null — allowed).
    parent.move_to_thread(&loop2);
    EXPECT_EQ(parent.thread(), &loop2);
    EXPECT_EQ(child->thread(), &loop2); // subtree migrated

    // Pending-event migration portion of this test MOVED TO TASK 2 (Momus F4):
    // enqueueing relies on T2's affinity routing; under T1 the post_event call below
    // would fatal (current()-based semantics). T1 verifies subtree relink + thread() only;
    // the `move to loop1 -> post -> move to loop2 -> deliver on loop2` sequence runs as
    // the last test block of Task 2 (affinity routing now live).
    // (pending-event migration block relocated to Task 2 — see Momus F4 above.)
}
```

守卫用例（3 个 EXPECT_DEATH 形态，任一触发即死）：

```cpp
TEST(Object, move_to_thread_fails_when_source_loop_running)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop1(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    FakeDispatcher *d2 = new FakeDispatcher;
    cxxkit::EventLoop loop2(std::unique_ptr<cxxkit::AbstractEventDispatcher>(d2));
    RecordingObject obj;
    obj.move_to_thread(&loop1);
    loop1.post([&loop1, &loop2]()
    {
        RecordingObject victim; // created inside exec -> affinity loop1
        victim.move_to_thread(&loop2); // source (loop1) IS running -> fatal
        loop1.exit(0);
    });
    EXPECT_DEATH(loop1.exec(), "");
}
```

（target 运行中 fatal 用例：同形态对称——闭包在 loop1 里 move 一个亲和 loop2 的对象到 loop1，而 loop2 在另一 std::thread 中 exec——**简化裁定**：本任务只测 source 运行中 fatal；target 运行中场景与 source 对称走同一守卫分支，一个用例覆盖守卫存在性，对称分支代码审查兜底。）

**搭车用例（双队列顺序）**：

```cpp
TEST(EventLoop, post_functions_run_before_posted_events)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    RecordingObject obj;
    std::vector<int> order;
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    event->accept();
    loop.post([&order, &obj]() // also enqueue the event inside the closure
    {
        cxxkit::Object::post_event(&obj, event); // event queued AFTER this closure runs
        order.push_back(1);                      // closure body first
    });
    loop.post([&order]() { order.push_back(2); }); // second closure
    loop.post([&loop]() { loop.exit(0); });
    EXPECT_EQ(loop.exec(), 0);
    // post queue fully drains (1 then 2) before the Event segment dispatches.
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    ASSERT_EQ(obj.events.size(), 1u); // event delivered after both closures
}
```

（执行者按 RecordingObject 既有 events 记录机制核对 kUser 记录路径——T2 时 custom_event 已记录 kUser，直接可用。）

- [ ] **Step 2: 确认失败**：编译错（thread()/move_to_thread 未声明）。
- [ ] **Step 3: 实现**。核心形态：

object_p.hpp：

```cpp
#include <cxxkit/kernel/event_loop.hpp> // EventLoop* member — check include cycle: event_loop.hpp includes object.hpp; use forward declaration instead
// ^ Plan-fixed: forward declare `class EventLoop;` at top of object_p.hpp (it already includes object.hpp
//   which is fine — member is a pointer, forward decl suffices). Do NOT include event_loop.hpp here.
    EventLoop *mThread{nullptr};
```

object.cpp 构造（Object(Object*) 与 Object(ObjectPrivate*) 两处链尾——**裁定：在 Object(ObjectPrivate*) 委托目标处设置**，即两个 public ctor 最终都经它）：

```cpp
Object::Object(ObjectPrivate *d)
    : mDPtr(d)
{
    this->d_func()->mThread = EventLoop::current(); // creation-affinity (spec §4-1)
}
```

（**Momus F3 修订——EventLoop 私有替换清掉 mThread 的坑**：EventLoop ctor 在 `Object(parent)` 构造完成后再 `mDPtr.reset(new EventLoopPrivate(this))`——旧 ObjectPrivate（已带 mThread）被销毁，新 EventLoopPrivate 的 mThread=nullptr！plan 原稿"嵌套环构造的 EventLoop 得外层环亲和"论断不成立。修正：EventLoop ctor 的 reset 之后补一行 `d_func()->mThread = EventLoop::current();`（T2 的 event_loop.cpp 编辑点顺带落，或 T1 时先记 TODO 由 T2 落——plan 裁定 T1 即落，避免窗口期语义不一致）。另更正 C++ 事实：委托 ctor 的函数体**会**执行（在目标 ctor 完成后）——"体内不执行"表述有误但结论（只在目标 ctor 设一处）不受影响。）

object.hpp/cpp：

```cpp
public:
    /** @brief Returns the loop this object is affined to (null = no affinity). @since 0.2 */
    EventLoop *thread() const;

    /** @brief Static migration: subtree moves to @p target. Both source and target loops
     *  must not be running; same-loop is a no-op; null target detaches affinity.
     *  Pending queued events for the subtree migrate with it. Timers do NOT migrate.
     *  @note Caller contract (Momus F5): no concurrent post_event to subtree objects
     *        during the call — affinity metadata is unsynchronized by design (static
     *        migration is a setup-phase operation); recorded as a D35 known limitation. */
    void move_to_thread(EventLoop *target);

EventLoop *Object::thread() const
{
    CXXKIT_D(const Object);
    return d->mThread;
}

void Object::move_to_thread(EventLoop *target)
{
    CXXKIT_D(Object);
    EventLoop *source = d->mThread;
    if (source == target)
    {
        return; // same-loop no-op (Qt-tolerant shape)
    }
    CXXKIT_CHECK(source == nullptr || !source->is_running())
        << "move_to_thread: source loop is running";
    CXXKIT_CHECK(target == nullptr || !target->is_running())
        << "move_to_thread: target loop is running";
    // Subtree pre-order: update affinity + notify each object.
    this->migrate_subtree(target); // private recursive helper — see below
    // Migrate pending queue entries: for each migrated object, pull its entries
    // from the source loop's queue and push to the target's.
    // (Executor: gather migrated objects during subtree walk into a local vector,
    //  then for each: source loop (if non-null) -> take_events_for(obj) -> push each
    //  entry into target loop (if non-null); entries dropped to nowhere if target
  //  is null — plan-fixed: delete the events (receiver detached affinity; delivering
    //  on no loop is impossible; deleting avoids leak, matches ~Object purge spirit).)
}
```

私有递归辅助（object.cpp 匿名 ns 或私有静态）：

```cpp
namespace
{
void migrate_object(Object *obj, EventLoop *target)
{
    obj->d_func()->mThread = target;
    cxxkit::ThreadChangeEvent change; // T1 adds this class? NO — Event is enough:
    // plan-fixed: send a plain Event(Type::kThreadChange) — no new class (YAGNI).
    cxxkit::Event change(cxxkit::Event::Type::kThreadChange);
    obj->event(&change); // direct event() call — filter chain NOT applied (Qt same: thread change is not filtered)
}
} // namespace
// Object::move_to_thread walks children() recursively calling migrate_object.
```

（**plan 裁定**：不新建 ThreadChangeEvent 类——用裸 Event(kThreadChange)，YAGNI；**经 event() 直调不经 send_event**——thread change 非可过滤事件（Qt 同款：QEvent::ThreadChange 直发 event()）。）

event_loop_p.hpp（随迁原语）：

```cpp
    /** @brief Locks the queue and extracts all entries whose receiver == @p receiver. */
    std::deque<EventEntry> take_events_for(Object *receiver)
    {
        std::lock_guard<std::mutex> lock(mEventMutex);
        std::deque<EventEntry> taken;
        std::deque<EventEntry> remaining;
        while (!mEventQueue.empty())
        {
            EventEntry entry = mEventQueue.front();
            mEventQueue.pop_front();
            if (entry.mReceiver == receiver)
            {
                taken.push_back(entry);
            }
            else
            {
                remaining.push_back(entry);
            }
        }
        mEventQueue.swap(remaining);
        return taken;
    }
```

随迁整合（move_to_thread 内，子树更新后）：

```cpp
    // gather list of migrated objects (subtree) during the walk into std::vector<Object*> moved;
    for (size_t i = 0; i < moved.size(); ++i)
    {
        std::deque<EventLoopPrivate::EventEntry> entries;
        if (source != nullptr)
        {
            entries = source->d_func()->take_events_for(moved[i]);
        }
        if (target != nullptr)
        {
            // push into target queue (lock inside — sequential here, no deadlock:
            // source queue already drained for this receiver)
            EventLoopPrivate *td = target->d_func();
            std::lock_guard<std::mutex> lock(td->mEventMutex);
            while (!entries.empty())
            {
                td->mEventQueue.push_back(entries.front());
                entries.pop_front();
            }
        }
        else
        {
            while (!entries.empty())
            {
                delete entries.front().mEvent; // detached affinity: drop, avoid leak
                entries.pop_front();
            }
        }
    }
```

（**锁序说明**：source 与 target 锁不同时持有——逐 receiver 先排空 source 再入 target，无嵌套加锁，无地址序需求（spec §4-3 的 lock_two_loops 简化为顺序操作）。）

event() kThreadChange 分支（object.cpp 既有空壳）：

```cpp
        case Event::Type::kThreadChange:
        {
            // Migration notification: affinity already updated by move_to_thread.
            // Deliberate no-op hook — derived classes may override to react.
            break;
        }
```

（既有分支已有 `CXXKIT_D(Object);` 读 d 的死代码——替换为上述 no-op + doxygen。）

- [ ] **Step 4: 跑测试确认全过** + 定向回归（tst_object + tst_kernel 全组）。
- [ ] **Step 5: 全量回归 81/81**。
- [ ] **Step 6: format + 提交** `feat(kernel): thread affinity — thread(), move_to_thread static migration, queue entry migration`。

### Task 2: 跨线程投递路由（post_event/delete_later 演进 + wake_up 契约 + ctor friend 搭车）

**Files:**
- Modify: `cxxkit/kernel/object.cpp`（post_event 路由 + delete_later 演进 + purge 双清）
- Modify: `cxxkit/kernel/event_loop.hpp` / `event_loop.cpp`（enqueue_event/purge_pending 扩环签名）
- Modify: `cxxkit/kernel/event.hpp` / `event.cpp`（DeferredDeleteEvent ctor friend 收敛——搭车 R6）
- Modify: `cxxkit/kernel/abstract_event_dispatcher.hpp`（wake_up thread-safe 契约 doxygen）
- Modify: `tests/fake_dispatcher.hpp`（wake_up 加锁——测试基建）
- Test: `tests/tst_object.cpp`（跨线程用例）

**Interfaces:**
- Consumes: T1 的 mThread/thread()；enqueue_event/purge_pending 静态（Phase 2 形态）。
- Produces:
  - `post_event` 路由：`receiver->thread()` 非空 → 投亲和环 + `wake_up()`；null → **fatal**（指引构造于环内）。**变更面**：Phase 2 的"current() 语义"移除——receiver 亲和即目标，与投递者位置无关（Qt postEvent 同构）。
  - `delete_later` 演进：`thread()` 非空 → enqueue 亲和环；null → current() 回退（null fatal——三态测试保留）。
  - `EventLoop::enqueue_event(EventLoop *loop, Object *, Event *)` / `purge_pending(EventLoop *loop, Object *)`——**显式环参数**（静态函数从 current() 内取改为调用方传入；调用方负责非空）。
  - purge 双清：~Object 时 `purge(thread())` + `purge(current())`（去重同环只清一次——防静态迁移窗口不一致）。
  - `DeferredDeleteEvent` ctor private + `friend class Object`（delete_later 可构造；用户不可——R6 语言级封锁）。
  - `AbstractEventDispatcher::wake_up()` doxygen 契约："must be thread-safe"。

- [ ] **Step 1: 写失败测试**

```cpp
TEST(Object, post_event_routes_to_receiver_affinity_across_threads)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    RecordingObject *obj = nullptr;
    // Create the object inside exec so it binds affinity to `loop`.
    loop.post([&loop, &obj]()
    {
        obj = new RecordingObject();
        EXPECT_EQ(obj->thread(), &loop);
        loop.exit(0);
    });
    EXPECT_EQ(loop.exec(), 0);
    ASSERT_NE(obj, nullptr);

    // From ANOTHER thread, post to obj — must route to `loop` and wake it.
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    event->accept();
    std::thread poster([obj, event]()
    {
        cxxkit::Object::post_event(obj, event); // cross-thread: routes to loop, wakes it
    });
    poster.join();
    // Drain on this thread (loop not exec'ing — process_events pulls the entry).
    EXPECT_TRUE(loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents));
    ASSERT_EQ(obj->events.size(), 1u);
    delete obj;
}

TEST(Object, post_event_without_affinity_fails)
{
    RecordingObject obj; // created outside exec -> no affinity
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    EXPECT_DEATH(cxxkit::Object::post_event(&obj, event), "");
    delete event; // ownership not transferred on fatal path
}

TEST(Object, move_to_thread_migrates_pending_events_cross_loop)
{
    // Relocated from Task 1 (Momus F4): requires T2 affinity routing for the enqueue.
    FakeDispatcher *d1 = new FakeDispatcher;
    FakeDispatcher *d2 = new FakeDispatcher;
    cxxkit::EventLoop loop1(std::unique_ptr<cxxkit::AbstractEventDispatcher>(d1));
    cxxkit::EventLoop loop2(std::unique_ptr<cxxkit::AbstractEventDispatcher>(d2));
    RecordingObject parent;
    parent.move_to_thread(&loop1);   // affinity loop1
    cxxkit::Event *pending = new cxxkit::Event(cxxkit::Event::Type::kUser);
    pending->accept();
    cxxkit::Object::post_event(&parent, pending); // routed to loop1 (affinity, T2 semantics)
    EXPECT_FALSE(loop2.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // nothing on loop2
    parent.move_to_thread(&loop2);   // migrate WITH pending entry
    EXPECT_TRUE(parent.events.empty());
    EXPECT_TRUE(loop2.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // delivered on loop2
    ASSERT_EQ(parent.events.size(), 1u);
    EXPECT_EQ(parent.events[0], cxxkit::Event::Type::kUser);
}

TEST(Object, delete_later_uses_affinity_loop_when_present)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    Counter::alive = 0;
    CountedObject *victim = nullptr;
    CountedObject *victim = new CountedObject(); // OUTSIDE exec — Phase 2 would fatal here
    victim->move_to_thread(&loop);               // bind affinity (Momus F4: no in-closure enqueue —
                                                 //  a delete_later inside the closure would dispatch
                                                 //  during exec and UAF the post-exec call below)
    EXPECT_EQ(loop.exec(), 0);                   // loop ran empty — victim untouched
    // Loop NOT exec'ing now: Phase 2 would fatal (no current); Phase 3 routes to affinity.
    victim->delete_later();
    EXPECT_TRUE(loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents)); // drains the DeferredDelete
    EXPECT_EQ(Counter::alive, 0);
    // cleanup: obj deleted via queue
}
```

**Momus F1 修订——既有用例适配总账（T2 落地时逐个执行）**：T2 亲和路由后，**3 个既有 post_event 用例的接收方是环外构造（mThread=null）→ 会 fatal 而非送达**：
- `post_event_delivers_on_process_events`（obj 环外栈构造）
- `post_event_fifo_order_within_one_drain`（obj 环外栈构造）
- `post_event_respects_filter_chain`（watched 环外栈构造）

修正形态（统一）：每处 `RecordingObject obj;` 声明后加一行 `obj.move_to_thread(&loop);`（T1 API，静态守卫下合法——两环未运行）再 `loop.post(...)`。用例名不变、总数不变。

- `post_event_without_current_loop_fails`：实测该用例**原样通过**（obj 环外构造 → mThread=null → 新守卫 fatal 同形态）——仅重命名 `post_event_without_affinity_fails`（语义名实相符），体不变。
- （更正 plan 原文的两处事实错误：spec §4.6 与本 plan 原"exec 闭包内构造天然有亲和故兼容"的论断不实——既有用例对象全在环外构造；"该用例在新语义下不再 fatal"亦不实。以本修订为准。）

**搭车用例（ctor friend 收敛，编译期验证）**：

```cpp
// DeferredDeleteEvent default construction must be inaccessible outside Object.
// Runtime probe: cannot compile a construction — verified by this file NOT compiling
// if uncommented. Guard with a comment block? No — plan-fixed: static_assert on
// constructibility via SFINAE is overkill for C++11; instead rely on the friend
// declaration and a negative compile check OUTSIDE the test target:
// (executor: add nothing here; the R6 closure is verified by code review +
// the fact that only delete_later constructs it — grep evidence in report.)
```

（**plan 裁定**：R6 无运行期测试——C++11 无干净的负编译测试形态；验证 = grep 全库仅 object.cpp 构造 DeferredDeleteEvent + 代码审查。）

- [ ] **Step 2: 确认失败**（新用例编译/断言失败）。
- [ ] **Step 3: 实现**：

post_event 路由（object.cpp，替换 current 逻辑）：

```cpp
void Object::post_event(Object *receiver, Event *event)
{
    CXXKIT_CHECK(receiver != nullptr && event != nullptr) << "post_event requires receiver/event";
    CXXKIT_CHECK(event->type() != Event::Type::kDeferredDelete)
        << "post_event: use delete_later() for deferred delete (owner semantics)";
    EventLoop *loop = receiver->d_func()->mThread; // target-affinity routing (R2)
    CXXKIT_CHECK(loop != nullptr)
        << "post_event: receiver has no thread affinity — create it inside a loop's exec or move_to_thread";
    EventLoop::enqueue_event(loop, receiver, event);
    loop->wake_up(); // cross-thread wake (thread-safe contract)
}
```

enqueue/purge 扩签名（event_loop.hpp/cpp——静态函数改为显式环参数）：

```cpp
// event_loop.hpp (private, friend class Object unchanged)
    static void enqueue_event(EventLoop *loop, Object *receiver, Event *event);
    static void purge_pending(EventLoop *loop, Object *receiver);

// event_loop.cpp
void EventLoop::enqueue_event(EventLoop *loop, Object *receiver, Event *event)
{
    CXXKIT_CHECK(loop != nullptr) << "enqueue_event: null loop";
    EventLoopPrivate *d = loop->d_func();
    std::lock_guard<std::mutex> lock(d->mEventMutex);
    EventEntry entry;
    entry.mReceiver = receiver;
    entry.mEvent = event;
    d->mEventQueue.push_back(entry);
}

void EventLoop::purge_pending(EventLoop *loop, Object *receiver)
{
    if (loop == nullptr)
    {
        return;
    }
    loop->d_func()->remove_pending_events(receiver);
}
```

delete_later 演进（object.cpp）：

```cpp
void Object::delete_later()
{
    EventLoop *loop = this->d_func()->mThread; // affinity-first (D35 evolution)
    if (loop == nullptr)
    {
        loop = EventLoop::current(); // legacy fallback for affinity-less objects
    }
    CXXKIT_CHECK(loop != nullptr)
        << "Object::delete_later: no affinity and no running EventLoop on this thread";
    EventLoop::enqueue_event(loop, this, new DeferredDeleteEvent); // R6: only Object constructs it
}
```

~Object purge 双清（object.cpp——替换 Phase 2 的单 purge）：

```cpp
    EventLoop::purge_pending(d->mThread, this);      // affinity loop
    EventLoop::purge_pending(EventLoop::current(), this); // caller-thread residual (dedup harmless)
```

（同环两次 purge 第二次 no-op——remove 已删空，无害。）

ctor friend 收敛（event.hpp/cpp——搭车 R6）：

```cpp
class CXXKIT_KERNEL_API DeferredDeleteEvent : public Event
{
public:
    // No public constructor — only Object::delete_later() creates this event (R6).
private:
    friend class Object;
    DeferredDeleteEvent();
};
```

（event.cpp 定义不变。）

wake_up 契约（abstract_event_dispatcher.hpp doxygen——wake_up 声明处）：

```cpp
    /** @brief Wakes the dispatcher from a possibly different thread.
     *  @note Thread-safe: implementations must be callable from any thread
     *        (uv_async_send / QMetaObject::invokeMethod(QueuedConnection) satisfy this). */
    virtual void wake_up() = 0;
```

FakeDispatcher wake_up 加锁（tests/fake_dispatcher.hpp——mWakeUpCount 原子化或 mutex；**plan 裁定**：`std::atomic<int> mWakeUpCount{0};` 最简——检查既有成员类型，若非原子改之；跨线程读在 poster.join() 后无竞争，原子化仅为契约演示，`++mWakeUpCount` 保持）：

```cpp
    void wake_up() override { mWakeUpCount.fetch_add(1); } // thread-safe (contract)
    std::atomic<int> mWakeUpCount{0}; // was plain int — upgrade for the thread-safe contract
```

- [ ] **Step 4: 定向 + 全量回归 81/81**（既有 post_event/delete_later 用例语义替换面已裁定——`post_event_without_current_loop_fails` 改造为无亲和形态）。
- [ ] **Step 5: format + 提交** `feat(kernel): cross-thread post_event routing + delete_later affinity evolution (D35)`。

### Task 3: filter 体系收口（反向注册表 + 双 install 去重——搭车）

**Files:**
- Modify: `cxxkit/kernel/detail/object_p.hpp`（mWatching 反向表）
- Modify: `cxxkit/kernel/object.cpp`（install 去重 + install/remove 维护 mWatching + ~Object filter 亡自摘）
- Test: `tests/tst_object.cpp`

**Interfaces:**
- Consumes: T1 无依赖（纯 object.cpp 内聚）；Phase 2 install/remove 形态。
- Produces:
  - install_event_filter：**先 remove 再 push_back**（R5 去重——重装=移到最新位）+ `filter->d_func()->mWatching.push_back(this)`。
  - remove_event_filter：同步从 filter->mWatching 移除 watched（本对象）。
  - **~Object（filter 侧）**：遍历 mWatching 逐个 `watched->remove_event_filter(this)`——D34 弱化契约解除（filter 可先亡）。

- [ ] **Step 1: 写失败测试**

```cpp
TEST(Object, destroyed_filter_is_auto_removed_from_watched)
{
    RecordingObject watched;
    {
        Interceptor filter('x'); // records into Interceptor::order — reuse existing helper
        watched.install_event_filter(&filter);
    } // filter dies here — Phase 2: dangling entry; Phase 3: auto-removed
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    event.accept();
    EXPECT_TRUE(cxxkit::Object::send_event(&watched, &event)); // no dangling call, delivered
    EXPECT_EQ(Interceptor::order.size(), 0u); // dead filter never invoked
}

TEST(Object, reinstalled_filter_moves_to_front)
{
    Interceptor a('a');
    Interceptor b('b');
    RecordingObject watched;
    watched.install_event_filter(&a);
    watched.install_event_filter(&b); // front: b
    watched.install_event_filter(&a); // reinstall a — moves to front (R5)
    Interceptor::order.clear();
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &event);
    ASSERT_EQ(Interceptor::order.size(), 2u);
    EXPECT_EQ(Interceptor::order[0], 'a'); // reinstalled a fires FIRST now
    EXPECT_EQ(Interceptor::order[1], 'b');
}
```

- [ ] **Step 2: 确认失败**（destroyed_filter 用例 Phase 2 下悬垂调用——ASAN 抓或 order 残留）。
- [ ] **Step 3: 实现**：

object_p.hpp：

```cpp
    std::vector<Object *> mWatching; // reverse registry: filters installed ON other objects by me
```

install（object.cpp——替换 Phase 2 形态）：

```cpp
void Object::install_event_filter(Object *filter)
{
    CXXKIT_CHECK(filter != nullptr) << "install_event_filter requires a filter";
    CXXKIT_CHECK(filter != this) << "install_event_filter: self-filtering is not allowed";
    this->remove_event_filter(filter); // R5 dedup: reinstall moves to front
    this->d_func()->mFilters.push_back(filter);
    filter->d_func()->mWatching.push_back(this); // reverse registry
}
```

remove（同步双向摘除）：

```cpp
void Object::remove_event_filter(Object *filter)
{
    CXXKIT_D(Object);
    std::vector<Object *> &filters = d->mFilters;
    filters.erase(std::remove(filters.begin(), filters.end(), filter), filters.end());
    if (filter != nullptr)
    {
        std::vector<Object *> &watching = filter->d_func()->mWatching;
        watching.erase(std::remove(watching.begin(), watching.end(), this), watching.end());
    }
}
```

~Object **双向自摘**——在 purge_pending 之后、级联之前插入（**Momus F2 修订：必须双向**）：

```cpp
    // (a) I die as a FILTER: remove myself from every watched object's chain.
    while (!d->mWatching.empty())
    {
        d->mWatching.back()->remove_event_filter(this);
    }
    // (b) I die as a WATCHED: detach my installed filters' registry entries so a
    // surviving filter's later teardown never dereferences me (UAF without this).
    while (!d->mFilters.empty())
    {
        d->mFilters.back()->remove_event_filter(this);
    }
```

**F2 缺口说明**：原 plan 只有 (a) filter 先亡分支——watched 先亡时幸存 filter 的 mWatching 残留悬垂指针，filter 随后析构即 UAF（既有用例 `delete_later_uses_deferred_delete_event` 的 ObserverFilter 在测试作用域退出时即触发）。(b) 补对称清理后两个方向都闭合：remove_event_filter 内部双向摘除保证任一侧 while 循环每轮恰好消一项，终态空集，无死循环。

（跨线程场景沿用 D33 单线程语义备案——无新守卫，D35 记一句。）

- [ ] **Step 4: 定向 + 全量回归 81/81** + **Step 5: format + 提交** `feat(kernel): filter reverse registry + install dedup (D34 contract lifted)`。

### Task 4: D35 + status + check.sh 收口

**Files:**
- Modify: `.agents/memorys/decisions.md`（D35）
- Modify: `.agents/memorys/status.md`
- 无 CMake、无库码改动。

**Interfaces:**
- Consumes: T1-T3 全落地。
- Produces: D35 决策记录 + status 段落。

- [ ] **Step 1: D35**（decisions.md）：
  - R1-R6 裁定全录（方案 B 范围/目标环路由/静态迁移/反向注册表/去重/ctor friend）
  - **delete_later 语义演进**（D33 exec-only → 亲和优先 current 回退——正式演进记录，含三态测试改造说明）
  - 创建即亲和语义（exec 内构造绑定 / 环外 null）
  - 队列条目随迁形态（逐 receiver 先排空 source 再入 target——无嵌套锁，spec §4-3 的 lock_two_loops 简化为顺序操作）
  - ThreadChangeEvent 经 event() 直调不经 send_event（非可过滤事件，Qt 同款）+ 不新建事件类（YAGNI）
  - wake_up thread-safe 契约升级
  - 已知限制：动态迁移 fatal/定时器不迁/无亲和 post fatal/Application 仍注释态
  - **Momus F5 备案**：move_to_thread 调用方契约——迁移期间禁止对子树并发 post_event（亲和元数据不同步，静态迁移=初始化期操作）；动态迁移需求时重审
  - **Momus F6 备案**：ThreadChange 派发形态= event() 直调不经 send_event（非可过滤，plan 裁定）与 spec §4.4 "send_event 直发"表述不一致——以 plan 裁定为准，spec 措辞在 D35 中更正
  - **Momus F2 备案**：filter 双向自摘（filter 亡→mWatching 清理 + watched 亡→mFilters 清理）——D34 弱化契约解除的双向闭环
- [ ] **Step 2: pitfalls 查补**（grep 查重）：执行中新教训即时五段式；无则跳过
- [ ] **Step 3: status.md** 段落（P3 落地/用例数/提交链）
- [ ] **Step 4: check.sh 全链** `bash scripts/check.sh > /tmp/opencode/check-p3.log 2>&1; echo exit=$?` 必须 ALL CHECKS PASSED（81 三树）
- [ ] **Step 5: 提交** `docs(memory): phase 3 thread affinity record (D35)`

---

## Self-Review（已跑）

1. **Spec 覆盖**：§3.1 亲和元数据→T1 ✓；§3.2 move_to_thread→T1 ✓（含队列随迁原语 take_events_for）✓；§3.3 跨线程路由→T2 ✓（post_event/delete_later/enqueue 扩签名/purge 双清/wake_up 契约）✓；§3.4 顺手项 4 个→T1(双队列测试)/T2(ctor friend)/T3(注册表+去重) ✓；§4 裁定细节→D35（T4）✓；§7 限制→D35 ✓。
2. **占位符扫描**：T1 测试形态的"Executor: see Step 3 note"是裁定说明非 TBD（亲和语义 exec-only 约束的测试形态已在 plan 定死）；T2 ctor friend 的负编译验证裁定为 grep+审查（C++11 无干净形态，plan 明示）✓。
3. **类型一致性**：mThread T1 定义 T2/T3 消费一致；take_events_for T1 定义 T1 消费（move 随迁）；enqueue_event/purge_pending 新签名 T2 定义并消费一致；mWatching T3 定义自洽；EventEntry 复用 Phase 2 形态。
4. **风险预判**（给 Momus 的重点）：①T2 既有用例改造面（post_event_without_current_loop_fails → 无亲和形态）——语义替换裁定已写明；②move_to_thread 随迁的"source 排空后 target 入队"顺序操作 vs spec §4-3 lock_two_loops——简化是否引入窗口（静态守卫下两环未运行、单线程调用 move——无并发派发者，顺序操作安全；但若未来动态迁移需重审）；③创建即亲和在 Object(ObjectPrivate*) 委托目标处设置——EventLoop 自身构造（也是 Object）时 current 为 null/自身——EventLoop 构造于 exec 内的场景（嵌套环）mThread=外层环，合理。
