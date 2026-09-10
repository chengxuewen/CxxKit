# Phase 2 事件投递基础设施实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 落地 Qt 形事件投递——send_event/post_event 双入口 + install/remove_event_filter 链 + kDeferredDelete 正名化（Event 队列迁移，自动解除 D33 父子同投限制）。

**Architecture:** 双队列并存（现有 mPostQueue(function) 零改动 + 新 mEventQueue(EventEntry)）；send_event 与派发共用同一条 filter→event() 路径；~Object 清 pending 事件（destroying 后、级联前）。

**Tech Stack:** C++11、kernel 子库既有文件（无新文件、无 CMake 改动）、gtest。

**Spec:** `docs/superpowers/specs/2026-09-09-event-delivery-design.md`（R1-R4 裁定 + 三节设计 + §7 已知限制）。

## Global Constraints

- C++11 硬门禁：禁 auto 变量/if constexpr/泛型 lambda/_t/_v（thread_local 合法，D33 已用）。
- 禁 `rm -rf build`；构建 `cmake --build build --target <t> -j8`。
- 命名 snake_case 函数 / mPascalCase 成员 / kPascalCase 常量；`#pragma once`；尖括号 include。
- 测试 EXPECT_DEATH 空匹配器 `""`；gtest C++11。
- 以 ctest 退出码为准；clang-format 用 `pixi run clang-format`，只碰 .hpp/.cpp。
- 套件基线 **81**（本计划无新套件——全部用例进 tst_object.cpp），终态仍 81。
- 提交身份 `git -c user.name=chengxuewen -c user.email=1398831004@qq.com`。

---

### Task 1: DeferredDeleteEvent + send_event + filter 链（同步路径全通）

**Files:**
- Modify: `cxxkit/kernel/event.hpp` / `event.cpp`（DeferredDeleteEvent 类）
- Modify: `cxxkit/kernel/object.hpp` / `object.cpp`（send_event + install/remove_event_filter + event() kDeferredDelete 分支实装）
- Modify: `cxxkit/kernel/detail/object_p.hpp`（mFilters vector）
- Test: `tests/tst_object.cpp`（追加同步路径用例）

**Interfaces:**
- Consumes: Event::accept()/ignore()/is_accepted()（event.hpp 既有）；event_filter() 虚函数（Phase 0 既有孤岛）；RecordingObject 测试辅助（tst_object 既有）。
- Produces（T2/T3 消费）:
  - `static bool Object::send_event(Object *receiver, Event *event)` —— filter 链（头插序）→ receiver->event()，返回 is_accepted()；filter 拦截返回 false。
  - `void Object::install_event_filter(Object *filter)` / `void Object::remove_event_filter(Object *filter)`。
  - `class DeferredDeleteEvent : public Event`（Type::kDeferredDelete）——默认 ctor。
  - event() 的 kDeferredDelete 分支：`delete this; return true;`（本任务实装——T3 迁移 delete_later 后成为活路径）。

- [ ] **Step 1: 写失败测试**（tst_object.cpp 追加）

```cpp
// ---- send_event / filter 链（同步路径） ----

TEST(Object, send_event_delivers_and_returns_accepted)
{
    RecordingObject obj;
    cxxkit::Event event(cxxkit::Event::Type::kUser); // kUser 走 custom_event，RecordingObject 未覆写
    event.accept();
    EXPECT_TRUE(cxxkit::Object::send_event(&obj, &event));
}

TEST(Object, send_event_returns_false_when_ignored)
{
    RecordingObject obj;
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    event.ignore();
    EXPECT_FALSE(cxxkit::Object::send_event(&obj, &event));
}

TEST(Object, event_filter_can_intercept)
{
    class Interceptor : public cxxkit::Object
    {
    public:
        bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
        {
            CXXKIT_UNUSED(watched);
            return event->type() == cxxkit::Event::Type::kUser; // 拦 kUser
        }
    };
    Interceptor filter;
    RecordingObject watched;
    watched.install_event_filter(&filter);

    cxxkit::Event user(cxxkit::Event::Type::kUser);
    EXPECT_FALSE(cxxkit::Object::send_event(&watched, &user)); // 被拦截 = false
    EXPECT_TRUE(watched.events.empty());                       // 未送达
}

TEST(Object, filter_chain_is_lifo)
{
    class Interceptor : public cxxkit::Object
    {
    public:
        explicit Interceptor(char tag)
            : mTag(tag)
        {
        }
        bool event_filter(cxxkit::Object *watched, cxxkit::Event *event) override
        {
            CXXKIT_UNUSED(watched);
            order.push_back(mTag);
            return false; // 全放行只记录顺序
        }
        char mTag;
        static std::vector<char> order;
    };
    Interceptor::order.clear();
    Interceptor a('a');
    Interceptor b('b');
    RecordingObject watched;
    watched.install_event_filter(&a); // 头插
    watched.install_event_filter(&b); // 头插 → 后装先过滤
    cxxkit::Event event(cxxkit::Event::Type::kUser);
    cxxkit::Object::send_event(&watched, &event);
    ASSERT_EQ(Interceptor::order.size(), 2u);
    EXPECT_EQ(Interceptor::order[0], 'b'); // 后装先过滤
    EXPECT_EQ(Interceptor::order[1], 'a');
}
```

（`std::vector` include 由执行者按需补；`CXXKIT_UNUSED` 既有宏。static order 跨用例污染——在用例首行 clear。）

- [ ] **Step 2: 确认失败**：编译错（send_event/install_event_filter 未声明）。
- [ ] **Step 3: 实现**。核心代码：

object.hpp（public 段）：

```cpp
    /** @brief 同步投递：filter 链前置，未拦截则 receiver->event()。返回 is_accepted()；被 filter 拦截返回 false。 */
    static bool send_event(Object *receiver, Event *event);

    /** @brief 头插 filter（后装先过滤，Qt 同款）。契约：filter 须比 watched 长寿或自行 remove（spec §3.5 弱化备案）。 */
    void install_event_filter(Object *filter);

    /** @brief 线性查找移除；无此 filter 为 no-op。 */
    void remove_event_filter(Object *filter);
```

object.cpp（send_event + event() 分支）：

```cpp
bool Object::send_event(Object *receiver, Event *event)
{
    CXXKIT_CHECK(receiver != nullptr && event != nullptr) << "send_event requires receiver/event";
    CXXKIT_CHECK(event->type() != Event::Type::kDeferredDelete)
        << "send_event: DeferredDelete is deliverable only via the event queue (owner semantics)";
    ObjectPrivate *priv = receiver->d_func();
    // 头插序遍历：mFilters.back() 最先（后装先过滤）
    for (size_t i = priv->mFilters.size(); i > 0; --i)
    stack_vec: std::vector<Object *> &filters = priv->mFilters;
    for (size_t i = filters.size(); i > 0; --i)
    {
        if (filters[i - 1]->event_filter(receiver, event))
        {
            return false; // 拦截
        }
    }
    receiver->event(event);
    return event->is_accepted();
}
```

（注意：上面片段有一处书写重复——执行者以第二段 `filters` 局部引用形态为准，弃 `priv->mFilters` 直引与 `stack_vec:` 标签行。）

object.cpp event() kDeferredDelete 分支（替换现有注释空壳）：

```cpp
        case Event::Type::kDeferredDelete:
            delete this; // DeleteInEventHandler 正名化（Qt 同款）；仅 Event 队列派发路径会到达
            return true;
```

object_p.hpp：

```cpp
    std::vector<Object *> mFilters; // 头插序（back() = 最新，先过滤）——需 #include <vector>
```

install/remove（object.cpp）：

```cpp
void Object::install_event_filter(Object *filter)
{
    CXXKIT_CHECK(filter != nullptr) << "install_event_filter requires a filter";
    CXXKIT_CHECK(filter != this) << "install_event_filter: self-filtering is not allowed";
    this->d_func()->mFilters.push_back(filter);
}

void Object::remove_event_filter(Object *filter)
remove: 线性查找 erase（std::vector<Object*>::iterator 循环或 std::remove 形态——C++11 用 erase+std::remove 惯用法）。
```

DeferredDeleteEvent（event.hpp/cpp）：

```cpp
class CXXKIT_KERNEL_API DeferredDeleteEvent : public Event
{
public:
    DeferredDeleteEvent();
};

DeferredDeleteEvent::DeferredDeleteEvent()
    : Event(Type::kDeferredDelete)
{
}
```

- [ ] **Step 4: 跑测试确认全过** + 定向回归 `ctest -R "tst_object|tst_kernel" --output-on-failure`。
- [ ] **Step 5: 全量回归** `cmake --build build -j8 && ctest --test-dir build --output-on-failure`（81/81）。
- [ ] **Step 6: format + 提交** `feat(kernel): send_event + event filter chain + DeferredDeleteEvent`。

### Task 2: post_event + mEventQueue + process_events 双排空

**Files:**
- Modify: `cxxkit/kernel/detail/event_loop_p.hpp`（EventEntry + mEventQueue + take_event_queue + remove_pending_events）
- Modify: `cxxkit/kernel/event_loop.hpp` / `event_loop.cpp`（post_event 需要的入队 API + process_events 第二段排空 + ~EventLoop 排空扩展）
- Modify: `cxxkit/kernel/object.hpp` / `object.cpp`（Object::post_event 静态入口）
- Test: `tests/tst_object.cpp`（异步路径用例）

**Interfaces:**
- Consumes: T1 的 send_event/filter 路径；EventLoop::current()（D33 既有）；take_post_queue 形态（swap-under-lock 样板）。
- Produces（T3 消费）:
  - `static void Object::post_event(Object *receiver, Event *event)` —— current() null = fatal；push {receiver, event}（所有权转移）。
  - `EventLoopPrivate::take_event_queue()` / `EventLoopPrivate::remove_pending_events(Object *receiver)`（T3 的 ~Object 清理消费后者）。
  - process_events 派发语义：post 队列先、Event 队列后；Event 条目派发 = send_event 内部逻辑（filter 生效）后 `delete event`。

- [ ] **Step 1: 写失败测试**

```cpp
TEST(Object, post_event_delivers_on_process_events)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    RecordingObject obj; // 环上对象（同线程）
    cxxkit::Event *event = new cxxkit::Event(cxxkit::Event::Type::kUser);
    event->accept();
    cxxkit::Object::post_event(&obj, event);
    EXPECT_TRUE(obj.events.empty()); // 未派发
    EXPECT_TRUE(loop.process_events(cxxkit::EventLoop::ProcessFlag::kAllEvents));
    ASSERT_EQ(obj.events.size(), 1u); // RecordingObject::event() 需记录 kUser（见 T2 Step3 调整）
    cxxkit::Object::send_event(&obj, &event_after); // （执行者自定：直接断言 events 内容即可）
}
```

（执行者按 RecordingObject 既有 events 记录机制完整写出——RecordingObject::event() 需覆写记录 kUser/kCustom 类型；AAA 三段完整，含派发后 event 已被队列 delete 的不可访问断言——用计数型 Event 子类 DeletedEvent（dtor 计数）验证队列释放所有权。）

- [ ] **Step 2: 确认失败**（post_event 未声明）。
- [ ] **Step 3: 实现**：

event_loop_p.hpp：

```cpp
struct EventEntry
{
    Object *mReceiver;
    Event *mEvent;
};
std::mutex mEventMutex;
std::deque<EventEntry> mEventQueue;

std::deque<EventEntry> take_event_queue()
{
    std::lock_guard<std::mutex> lock(mEventMutex);
    std::deque<EventEntry> entries;
    entries.swap(mEventQueue);
    return entries;
}

// ~Object 清理入口：锁内 remove+delete 匹配条目（含 DeferredDeleteEvent → 父子同投限制解除）
void remove_pending_events(Object *receiver)
{
    std::lock_guard<std::mutex> lock(mEventMutex);
    for (std::deque<EventEntry>::iterator it = mEventQueue.begin(); it != mEventQueue.end();)
    {
        if (it->mReceiver == receiver)
        {
            delete it->mEvent;
            it = mEventQueue.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
```

event_loop.hpp/cpp：EventLoop 增加公开自由函数或静态访问——**形态裁定（plan 定死）**：`Object::post_event` 内部经 `EventLoop::current()->d_func()->mEventQueue` 直推？不行——d_func() 返回 ObjectPrivate，EventLoopPrivate 拿不到。**用 friend 或公开 API**：裁定为 EventLoop 加 **private 静态辅助 + friend class Object** 声明（kernel 内部协作，不公开污染 API）：

```cpp
// event_loop.hpp EventLoop 类内：
private:
    friend class Object; // post_event 入队 + ~Object 清 pending（kernel 内部协作）
```

object.cpp post_event：

```cpp
void Object::post_event(Object *receiver, Event *event)
{
    CXXKIT_CHECK(receiver != nullptr && event != nullptr) << "post_event requires receiver/event";
    CXXKIT_CHECK(event->type() != Event::Type::kDeferredDelete)
        << "post_event: use delete_later() for deferred delete (owner semantics)";
    EventLoop *loop = EventLoop::current();
    CXXKIT_CHECK(loop != nullptr) << "post_event: no running EventLoop on this thread";
    // friend 直推队列（锁内 push_back）
    loop->d->mEventQueue.push_back(EventEntry{EventEntry初值化形态});
    // ↑ EventLoop::d 访问：Object 非 EventLoop 派生——friend class Object 给了私有访问权，
    //   但 d（mDPtr）是 protected。裁定：EventLoop 加 private 静态方法作唯一入队/清理通道：
}
```

（**plan 定死形态**：EventLoop 加两个 private 静态成员函数，friend Object 之外再 friend 这两个函数——最简是 EventLoop 上加 `private: static void enqueue_event(Object*, Event*);` 与 `static void purge_pending(Object*);`，定义在 event_loop.cpp，friend class Object 后 Object 直接调 `EventLoop::enqueue_event(receiver, event)`。执行者照此落地，勿自行发明访问路径。）

```cpp
// event_loop.hpp
private:
    friend class Object; // Object 调用两个私有静态（Momus F3：仅此一个 friend 即可——
                         // "再 friend 两个函数"的表述冗余且 friend 私有成员函数顺序不合法，勿采用）
    static void enqueue_event(Object *receiver, Event *event); // 锁内 push {receiver, event}
    static void purge_pending(Object *receiver);               // 锁内 remove+delete；null 容忍

// event_loop.cpp —— Momus F3 修订：静态函数无 this，CXXKIT_D 宏（= d_func() 成员调用）编不过！
// 正确形态：current() 取环 → 经对象指针 loop->d_func() 访问私有（成员访问私有合法）。
void EventLoop::enqueue_event(Object *receiver, Event *event)
{
    EventLoop *loop = EventLoop::current();
    CXXKIT_CHECK(loop != nullptr) << "enqueue_event: no running EventLoop";
    EventLoopPrivate *d = loop->d_func();
    std::lock_guard<std::mutex> lock(d->mEventMutex);
    EventEntry entry;
    entry.mReceiver = receiver;
    entry.mEvent = event;
    d->mEventQueue.push_back(entry);
}

void EventLoop::purge_pending(Object *receiver)
{
    EventLoop *loop = EventLoop::current();
    if (loop == nullptr)
    {
        return; // null 容忍（spec §3.3：无环线程无 pending——防御性 no-op）
    }
    loop->d_func()->remove_pending_events(receiver); // EventLoopPrivate 成员，锁内 remove+delete
}
```

（执行者整合：take_event_queue / remove_pending_events / pop_event_entry 都放 EventLoopPrivate。）

process_events 第二段（event_loop.cpp——**Momus F2 结构性修订：弃 take-snapshot，改锁内逐条 pop + 锁外派发**）：

快照形态的致命窗口：父子同投 [{parent,dd},{child,dd}] → 快照取走 → 派发 parent → delete parent → 级联 delete child → ~child → purge(child) 只能清**队列本体**，child 条目已在**局部快照**里够不着 → 循环下一条 receiver=悬垂 child → UAF。Qt 不走快照正是为此（~QObject 从队列本体 remove + 条目失效标记）。

```cpp
    // Event 队列：锁内逐条 pop + 锁外派发（Qt 忠实形态）——派发期间级联析构触发的
    // purge_pending 在锁内从队列本体删掉未派发条目，pop 到即不存在，无悬垂窗口。
    bool had_event = false;
    for (;;)
    {
        EventEntry entry;
        {
            std::lock_guard<std::mutex> lock(d->mEventMutex);
            if (d->mEventQueue.empty())
            {
                break;
            }
            entry = d->mEventQueue.front();
            d->mEventQueue.pop_front();
        } // 锁外派发
        if (entry.mReceiver != nullptr)
        {
            entry.mReceiver->send_event(entry.mReceiver, entry.mEvent); // filter 链生效
        }
        delete entry.mEvent; // 队列释放所有权（派发后必删；receiver null = 已失效条目，仅删）
        had_event = true;
    }
    return had_event; // 与 post 段合并：整合时 post 段改设 had_post 标志，最终 return had_post || had_event（两队列任一非空 = true，语义与原三路 return 等价）
```

配套：EventLoopPrivate 新增 `EventEntry pop_event_entry()`（锁内取头，空队列返回 {nullptr, nullptr} 哨兵——执行者照 take_post_queue 形态实现）；take_event_queue 仅剩 ~EventLoop 排空消费（快照 delete 不触 receiver，安全）。

（D34 须修正 spec §4.2 不变量表述：从"~Object 与派发不并发"改为"purge 与 pop 锁内互斥、派发在锁外——穿插级联析构下条目本体移除先于 pop"。）

~EventLoop 排空扩展（现有 post 排空后追加）：

```cpp
    // Event 队列排空：剩余 Event 一律 delete 不派发（环已死）
    std::deque<EventEntry> entries = d->take_event_queue();
    for (size_t i = 0; i < entries.size(); ++i)
    {
        delete entries[i].mEvent;
    }
```

- [ ] **Step 4: 定向 + 全量回归**（81/81）+ **Step 5: format + 提交** `feat(kernel): post_event + event queue with dual drain in process_events`。

### Task 3: delete_later 迁移 + ~Object 清 pending（父子同投限制解除）

**Files:**
- Modify: `cxxkit/kernel/object.cpp`（delete_later 改投 Event 队列 + ~Object 加清理步骤）
- Test: `tests/tst_object.cpp`（迁移用例 + 限制解除正名测试）

**Interfaces:**
- Consumes: T2 的 post_event/enqueue_event/purge_pending；D33 既有 delete_later 三态测试（必须原语义通过）。
- Produces:
  - `delete_later()` = `post_event(this, new DeferredDeleteEvent)`（T1 禁 send_event(kDeferredDelete)/T2 禁 post_event(kDeferredDelete) 的守卫反而逼迫唯一通道——设计如此）。
  - `~Object` 步骤序：destroying → **purge_pending(this)** → 级联 → 摘链。

- [ ] **Step 1: 写失败测试**

```cpp
// DeletedEvent 计数型辅助（T2 已建，此处消费）
TEST(Object, delete_later_uses_deferred_delete_event)
{
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventLoop>(dispatcher));
    // ↑ 类型笔误自查：AbstractEventDispatcher
    Counter::alive = 0;
    CountedObject *victim = new CountedObject;
    victim->delete_later();
    loop.post([&loop]() { loop.exit(0); });
    EXPECT_EQ(loop.exec(), 0);
    EXPECT_EQ(Counter::alive, 0);
}

TEST(Object, delete_later_parent_and_child_no_double_delete)
{
    // D33 限制①解除的正名测试
    FakeDispatcher *dispatcher = new DynamicDispatcher;
    // ↑ 类型名自查：FakeDispatcher
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    Counter::alive = 0;
    CountedObject *parent = new CountedObject;
    new CountedObject(parent); // child
    parent->delete_later();    // 父子同投——D33 时代 = double delete
    child_ptr->delete_later(); // 执行者持 child 指针变量
    loop.post([&loop]() { loop.exit(0); });
    EXPECT_EQ(plan.exec(), 0);
    // ↑ 变量名自查：loop
    EXPECT_EQ(Counter::alive, 0); // 双亡无二删——ASAN 树兜底
}
```

（片段中的三处笔误已标注，执行者以正确形态落地；AAA 完整。）

- [ ] **Step 1.5: 迁移断言**——现有 `delete_later_runs_on_loop_exec` 用例的 FIFO 串联形态（post 闭包内 delete_later + 同闭包 exit）应**原样通过**（签名与 fatal 纪律不变，只换内部通道）。

- [D33 既有三态测试回归]：delete_later_runs_on_loop_exec / delete_later_without_current_loop_fails / delete_later_before_exec_fails——全过为迁移正确性证据。

- [ ] **Step 2: 确认失败**——迁移后 event() kDeferredDelete 分支 delete this 生效，但 ~Object 无清理时父子同投测试 FAIL（child 二次 delete/ASAN 报 UAF）。
- [ ] **Step 3: 实现**：

```cpp
void Object::delete_later()
{
    EventLoop *loop = EventLoop::current();
    CXXKIT_CHECK(loop != nullptr) << "Object::delete_later: no running EventLoop on this thread";
    EventLoop::enqueue_event(this, new DeferredDeleteEvent); // Momus F1：直推 enqueue，不经 post_event——
                                                             // post_event 的 kDeferredDelete 守卫会 fatal
}
```

（**Momus F1 修订**：原形态 post_event(this, DeferredDeleteEvent) 会踩死 post_event 自身的 kDeferredDelete 守卫（唯一合法生产者恰好被唯一通道拒绝——自相矛盾）。裁定：delete_later 直推 EventLoop::enqueue_event 绕过守卫；delete_later 自身保留 current() null fatal（D33 文案），post_event 守卫照旧保留（用户侧禁投 DeferredDelete 的 API 纪律不变）。）

~Object（object.cpp，现有级联之前插一步）：

```cpp
Object::~Object()
{
    CXXKIT_D(Object);
    this->destroying();
    EventLoop::purge_pending(this); // 新增：清 pending（含 DeferredDelete → 父子同投限制解除）
    while (!d->mChildren.empty())
    {
        delete d->mChildren.front();
    }
    if (d->pParent != nullptr) // ↑ 变量名自查：d->mParent
    {
        d->mParent->d_func()->detach_child(this);
    }
}
~Object 里 purge_pending 的调用形态：静态函数，直接 EventLoop::purge_pending(this)（current() 内部判空——**purge 静态内部需 current() null 容忍**：无环时 no-op（spec §3.3：投递前置已保证无环线程无 pending，防御性 no-op），实现时 purge_pending 静态函数第一行 `EventLoop *loop = EventLoop::current(); if (loop == nullptr) return;`。
```

- [ ] **Step 4: 全量回归 81/81 + ASAN 定向**（`cmake --build build-asan --target cxxkit_tst_object -j8 && LSAN_OPTIONS="suppressions=$(pwd)/scripts/lsan.supp" ./build-asan/tests/cxxkit_tst_object`——父子同投用例零诊断）+ **Step 5: format + 提交** `feat(kernel): delete_later migrates to event queue — parent/child co-post limit lifted`。

### Task 4: D34 + status + check.sh 收口

**Files:**
- Modify: `.agents/memorys/decisions.md`（D34）
- Modify: `.agents/memorys/status.md`
- 无 CMake、无库码改动。

**Interfaces:**
- Consumes: T1-T3 全落地。
- Produces: D34 决策记录 + status 段落。

- [ ] **Step 1: D34**（decisions.md）：
  - R1-R4 裁定全录（裸 Event\*/迁移/filter 本期做/双队列并存）
  - send_event 禁 kDeferredDelete、post_event 禁 kDeferredDelete（唯一通道 = delete_later）的设计
  - filter 生命周期弱化契约（spec §3.5）
  - ~Object 清理序：destroying → purge_pending → 级联 → 摘链
  - **§4.2 不变量修正**（Momus F2）：spec 原文"~Object 与派发不并发"在单线程级联析构下即为假——实际不变量 = "purge 与 pop 锁内互斥、派发在锁外；派发期间的穿插级联析构从队列本体移除未派发条目，pop 到即不存在"。派发形态 = 锁内逐条 pop + 锁外派发（Qt 忠实），非快照
  - 双队列顺序语义（post 先 Event 后，跨队列无全局序——Qt 同款）
  - 已知限制：跨线程投递 fatal（Phase 3 重审）/无压缩/无多优先级
- [ ] **Step 2: pitfalls 查补**（grep 查重后五段式）：无新 pitfall 则跳过——T1-T3 执行中如有新教训即时补
- [ ] **Step 3: status.md** 段落（Phase 2 落地、用例数、提交链）
- [ ] **Step 4: check.sh 全链** `bash scripts/check.sh > /tmp/opencode/check-p2.log 2>&1; echo exit=$?` 必须退 0（81 套件主/asan/shared 三树）
- [ ] **Step 5: 提交** `docs(memory): phase 2 event delivery record (D34)`

---

## Self-Review（已跑）

1. **Spec 覆盖**：§3.2 send_event/filter（T1）✓；§3.3 post_event/双排空（T2）✓；kDeferredDelete 正名化+迁移（T1 分支实装 + T3 迁移）✓；~Object 清 pending（T3）✓；§3.4 ~EventLoop 排空扩展（T2）✓；§4 语义裁定 → D34（T4）✓；§7 限制 → D34（T4）✓。
2. **占位符扫描**：T1 send_event 片段有一处书写重复已标注（以第二段为准）；T2 process_events 整合形态已给出（had_event 统一返回）；T3 片段笔误已标注正确形态。三处标注均为"执行者照正确形态落地"——非 TBD。
3. **类型一致性**：EventEntry{mReceiver, mEvent} T2 定义 T2/T3 使用一致；enqueue_event/purge_pending 签名 T2 定义 T3 消费一致；DeferredDeleteEvent T1 定义 T3 消费一致；send_event 签名 T1 定义 T2 派发路径复用一致。
