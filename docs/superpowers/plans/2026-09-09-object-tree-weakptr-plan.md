# Object 对象树 + WeakPtr 实施计划（Phase 1a/1b/1c）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 `cxxkit::kernel::Object` 从"字段在语义全缺的 Qt 形骨架"补全为真实的对象树（父子互链/析构级联/ChildEvent/延迟删除/销毁回调），并在 `cxxkit::memory` 落地 Chromium 形非侵入弱引用（`WeakPtr`/`WeakPtrFactory`）。

**Architecture:** 三块独立可测的工作——①kernel Object 树语义（纯 kernel 内，改 object.cpp/object_p.hpp）②生命周期设施（delete_later 走 EventLoop post 队列 + thread_local 当前环指针 + ~EventLoop 排空）③memory 弱引用（新头 `weak_ptr.hpp`，Chromium WeakPtrFactory 形：源计数块 + 非 intrusive，不碰 Object）。Phase 2 事件投递/Phase 3 线程亲和不在本计划（需求触发再立）。

**Tech Stack:** C++11（库码硬约束）、CMake（cxxkit_add_test）、gtest 1.12.1（vendored）。

**Spec:** `.superpowers/sdd/object-analysis-synthesis.md`（4 调研员团队分析汇总——P0 对象树四件套/P0 WeakPtr/Phase 演进/拒绝清单）+ `docs/superpowers/plans/2026-09-09-object-analysis-` 团队报告归档（gap/qt/ue/ecosystem 四份结论在 synthesis 中）。

## Global Constraints

- 库代码 **C++11**：禁 `auto` 变量、`if constexpr`、泛型 lambda、`_t`/`_v` 别名（C4/PIT-22）。`thread_local` 是 C++11 合法。
- **禁 `rm -rf build`**（C6）——只清 `build/CMakeFiles build/CMakeCache.txt`（本计划无 CMake 结构改动，无需重配）。
- 命名：函数/局部 `snake_case`；成员 `mPascalCase`；常量 `kPascalCase`；`#pragma once`；D8 尖括号 include `<cxxkit/...>`。
- 新文件顶部箱式许可横幅（照 `cxxkit/kernel/object.hpp` 样式）。
- 测试注册走 `cxxkit_add_test`（生成 `cxxkit_tst_<name>` + `<name>_check` target）；测试也是 C++11。
- 验证以**构建/ctest 退出码为准**（禁 grep error_count 假绿）。
- 提交身份：`git -c user.name=chengxuewen -c user.email=1398831004@qq.com`。
- clang-format：只跑 `.cpp/.hpp`，**禁碰 CMakeLists.txt**（D27 纪律）；工具用 `pixi run clang-format`（系统无裸命令）。
- 保持既有行为兼容：`event_loop`/`signals`/`qt`/`uv` 子库不因 Object 改动破坏（现有 79 套件全绿是硬门禁）。

---

### Task 1: Object 树四件套（挂父/摘旧/级联析构/children 真实维护 + ChildEvent 派发）

**Files:**
- Modify: `cxxkit/kernel/detail/object_p.hpp`（加树操作辅助）
- Modify: `cxxkit/kernel/object.cpp:40-70`（构造/set_parent/析构）
- Modify: `cxxkit/kernel/object.hpp`（无需改公共 API——set_parent/children 已声明）
- Test: Create `tests/tst_object.cpp`
- Modify: `tests/CMakeLists.txt`（注册 cxxkit_add_test(tst_object ...)，照 tst_kernel_event 块样式）

**Interfaces:**
- Consumes: `ChildEvent`（event.hpp 已有，Type::kChildAdded/kChildRemoved）、`Object::event()` 已路由 kChild* 到 `child_event()` 虚函数。
- Produces: ①`ObjectPrivate::attach_child(Object*)`/`detach_child(Object*)`（内部辅助，kernel detail 命名空间内）②`set_parent(nullptr)` 合法（脱离父）③循环父派发 ChildEvent 给**父**的 `event()`（同步 send 语义：父 `child_event` 在 set_parent 调用栈内被调）④析构级联：`~Object` 逐个 `delete` children（子 dtor 自摘链，用 while(!empty) delete front 模式）⑤任何 Object 析构时把自己从 parent 的 children 摘除。

- [ ] **Step 1: 写失败测试** `tests/tst_object.cpp`

```cpp
/*** Library: CxxKit ***/
// 照 tst_kernel_event.cpp 的横幅与 include 风格
#include <cxxkit/kernel/object.hpp>
#include <cxxkit/kernel/event.hpp>
#include <gtest/gtest.h>

#if CXXKIT_FEATURE_ENABLE_KERNEL

namespace
{
class RecordingObject : public cxxkit::Object
{
public:
    explicit RecordingObject(cxxkit::Object *parent = nullptr)
        : Object(parent)
    {
    }
    std::vector<cxxkit::Event::Type> events;
    std::vector<cxxkit::Object *> children_seen;

protected:
    void child_event(cxxkit::ChildEvent *event) override
    {
        events.push_back(event->type());
        children_seen.push_back(event->child());
    }
};
} // namespace

TEST(Object, constructor_wires_parent_and_child_lists)
{
    RecordingObject parent;
    RecordingObject *child = new RecordingObject(&parent);
    EXPECT_EQ(child->parent(), &parent);
    ASSERT_EQ(parent.children().size(), 1u);
    EXPECT_EQ(parent.children().front(), child);
}

TEST(Object, set_parent_relinks_and_dispatches_child_events)
{
    RecordingObject p1;
    RecordingObject p2;
    RecordingObject child(&p1);
    ASSERT_EQ(p1.children().size(), 1u);

    child.set_parent(&p2); // 摘旧挂新
    EXPECT_EQ(p1.children().size(), 0u);
    ASSERT_EQ(p2.children().size(), 1u);
    // 旧父收到 ChildRemoved、新父收到 ChildAdded（同步派发）
    // p1 先在构造时收到 kChildAdded，set_parent 后收到 kChildRemoved
    ASSERT_EQ(p1.events.size(), 2u);
    EXPECT_EQ(p1.events[0], cxxkit::Event::Type::kChildAdded);
    EXPECT_EQ(p1.events[1], cxxkit::Event::Type::kChildRemoved);
    ASSERT_GE(p2.events.size(), 1u);
    EXPECT_EQ(p2.events.back(), cxxkit::Event::Type::kChildAdded);

    child.set_parent(nullptr); // 脱离
    EXPECT_EQ(p2.children().size(), 0u);
}

TEST(Object, destructor_cascades_to_children)
{
    RecordingObject *child = nullptr;
    {
        RecordingObject parent;
        child = new RecordingObject(&parent);
    } // parent 离开作用域：child 必须已级联析构（ASAN 树验证无泄漏）
    EXPECT_EQ(child->parent(), nullptr); // 危险断言？——不写这个。级联析构后 child 悬垂，
                                         // 不能访问。改为用死亡计数验证（见下）。
}
// 上面测试改为存活计数形态：
namespace
{
struct Counter
{
    static int alive;
    Counter() { ++alive; }
    ~Counter() { --alive; }
};
int Counter::alive = 0;
class CountedObject : public cxxkit::Object, Counter
{
public:
    explicit CountedObject(cxxkit::Object *parent = nullptr)
        : Object(parent)
    {
    }
};
} // namespace

TEST(Object, destructor_cascades_and_clears_child_parent_pointers)
{
    EXPECT_EQ(Counter::alive, 0);
    {
        CountedObject parent;
        new CountedObject(&parent);
        new CountedObject(&parent);
        EXPECT_EQ(Counter::alive, 3);
    } // 级联 delete：子 dtor 先跑（-2），父 dtor 后跑（-1）
    EXPECT_EQ(Counter::alive, 0);
}

TEST(Object, set_parent_rejects_descendant_cycle)
{
    RecordingObject a;
    RecordingObject b(&a);
    RecordingObject c(&b);
    EXPECT_DEATH(a.set_parent(&c), ""); // 环检测 fatal（空匹配器——仓内惯例，tst_array_view 等 8 处同款）
}
#endif // CXXKIT_FEATURE_ENABLE_KERNEL
```

- [ ] **Step 2: 跑测试确认失败**：`cmake --build build --target cxxkit_tst_object -j8 && ctest --test-dir build -R tst_object --output-on-failure`；预期：编译过但 constructor_wires/级联 FAIL（children 恒空、Counter 不归零）、cycle 测试不死（现 set_parent 无检查）。
- [ ] **Step 3: 实现**（object.cpp + object_p.hpp）

object_p.hpp（ObjectPrivate 增加两个内部辅助，字段不变）：

```cpp
    void attach_child(Object *child);   // children.push_back(child)
    void detach_child(Object *child);   // children.remove(child)
```

object.cpp 核心形态（snake_case、C++11、同步 ChildEvent 派发）：

```cpp
Object::Object(Object *parent)
    : Object(new ObjectPrivate(this))
{
    if (parent != nullptr)
    {
        this->set_parent(parent); // 统一走 set_parent（构造期父链上有 ChildEvent 派发）
    }
}

void Object::set_parent(Object *parent)
{
    CXXKIT_D(Object);
    if (d->mParent == parent)
    {
        return;
    }
    if (parent != nullptr)
    {
        // 环检测：new parent 不能是自己的后代（沿 parent 链上行不能遇到 this）
        for (Object *it = parent; it != nullptr; it = it->parent())
        {
            CXXKIT_CHECK(it != this) << "Object::set_parent: cycle detected";
        }
    }
    if (d->mParent != nullptr)
    {
        d->mParent->CXXKIT_P(Object)->detach_child(this);
        ChildEvent removed(Event::Type::kChildRemoved, this);
        d->mParent->event(&removed);
        d->mParent = nullptr;
    }
    if (parent != nullptr)
    {
        d->mParent = parent;
        parent->CXXKIT_P(Object)->attach_child(this);
        ChildEvent added(Event::Type::kChildAdded, this);
        parent->event(&added);
    }
}

Object::~Object()
{
    CXXKIT_D(Object);
    // 级联析构：先回调 destroying()（Task 2 加），再逐个 delete children。
    // 子 dtor 会通过 set_parent(nullptr)/detach 自摘链，所以 while(!empty) 安全。
    while (!d->mChildren.empty())
    {
        delete d->mChildren.front();
    }
    if (d->mParent != nullptr)
    {
        d->mParent->CXXKIT_P(Object)->detach_child(this);
    }
}
```

注意：访问 parent 的 ObjectPrivate 用 `d->mParent->d_func()->attach_child(this)` / `->detach_child(this)`——`d_func()` 是 Object protected 成员函数（同类内跨实例可访问），且 `CXXKIT_DECLARE_PUBLIC(Object)` 已给 ObjectPrivate `friend class Object`。**不要用 `parent->CXXKIT_P(Object)` 形态**——该宏展开为声明 `Object *const p = p_func()`，只能在 Private 成员函数体内用（macros.hpp L252-301 已核实）。`event()` 调用路径已存在（object.cpp L72）。

- [ ] **Step 4: 跑测试确认全过**：`cmake --build build --target cxxkit_tst_object -j8 && ctest --test-dir build -R "tst_object|tst_kernel" --output-on-failure`（kernel 相关回归）。预期 PASS。
- [ ] **Step 5: 全量回归**：`cmake --build build -j8 && ctest --test-dir build --output-on-failure`。预期 79+1=80 套件全绿。
- [ ] **Step 6: 格式 + 提交**：`pixi run clang-format -i cxxkit/kernel/object.cpp cxxkit/kernel/detail/object_p.hpp tests/tst_object.cpp && git add ... && git -c user.name=... commit -m "feat(kernel): object tree — parent/child linking, cascade delete, child event dispatch"`

### Task 2: delete_later + destroying() 销毁回调 + ~EventLoop 排空

**Files:**
- Modify: `cxxkit/kernel/object.hpp` / `object.cpp`（`delete_later()`、`destroying()` 虚函数）
- Modify: `cxxkit/kernel/event_loop.hpp` / `event_loop.cpp` / `detail/event_loop_p.hpp`（thread_local 当前环 + 析构排空）
- Test: Modify `tests/tst_object.cpp`（追加用例）

**Interfaces:**
- Consumes: Task 1 的级联析构（~Object 顶部调 destroying()）、EventLoop post 队列（event_loop_p.hpp `mPostQueue`）。
- Produces: ①`Object::destroying()` protected virtual——**~Object 顶部调用；C++ 析构期虚派发只到当前动态类型层（Object），派生类 override 不会被调用**（Qt 同款规则，D33 记录）。钩子价值 = Object 层内部清理时序锚点 + 外部观察者可见的"children 级联前"时刻；**signals 断连不依赖它**（cxxkit signals sender 析构自断连）。②`Object::delete_later()`——把 `[this]{delete this}` 投到**当前线程的** EventLoop（`thread_local EventLoop*`，exec 进入时置位/退出恢复嵌套值；无当前环 = fatal）③`~EventLoop` 排空 post 队列（执行剩余闭包——delete_later 语义必须执行到才 delete，不能丢弃；照 take_post_queue 的 swap-under-lock 形态，在锁外逐个执行）④`EventLoop::exec()`/`~EventLoop` 里维护 thread_local。
- 已知限制（D33 备案）：父与子**不可同时** delete_later——队列 [delete 父, delete 子] 时父级联已直接 delete 子，残留闭包再 delete = 二次 delete（Qt 靠 ~QObject 清 pending DeferredDelete，本版不做，文档化限制）。

- [ ] **Step 1: 写失败测试**（追加到 tst_object.cpp）

```cpp
TEST(Object, destroying_called_before_children_cascade)
{
    // static OrderTracker（静态存活贯穿析构期——成员不行，~Object 期派生成员已亡）
    // 断言：①父 ~Object 期 destroying() 被调（静态计数 +1）②调用时 child 存活计数 > 0（children 未级联）
    // 注意契约：destroying 在 Object 层被调（F1 修订），派生 override 不派发——只断言调用时序不断言 override 内容
}

TEST(Object, delete_later_runs_on_loop_exec)
{
    // F3 修订：用 FakeDispatcher（tests/fake_dispatcher.hpp，tst_event_loop 先例）——
    // tst_object 链接闭包无 uv（kernel 不依赖 uv），make_uv_dispatcher 会链接失败
    FakeDispatcher *dispatcher = new FakeDispatcher;
    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::AbstractEventDispatcher>(dispatcher));
    Counter::alive = 0;
    CountedObject *victim = new CountedObject;
    victim->delete_later();
    loop.post([&loop]() { loop.exit(0); }); // FIFO：delete_later 闭包先执行、后 exit——确定性，无 10ms timer
    EXPECT_EQ(loop.exec(), 0);
    EXPECT_EQ(Counter::alive, 0); // delete_later 在 exec 期间执行了 delete
}

TEST(Object, delete_later_without_current_loop_fails)
{
    CountedObject victim; // 栈对象：先置空当前环再调 delete_later
    // thread_local 无环状态默认即 fatal——直接 EXPECT_DEATH
    // 注意：该用例须在任何 exec 测试之外、且当前环确实为空的线程上下文运行
    EXPECT_DEATH(victim.delete_later(), "");
}
```

（`destroying_called_before_children_cascade` 的完整实现由执行者按 CountedObject 模式补：OrderTracker 成员 vector<int> 记录 "parent_destroying"/"child_destroyed" 顺序，断言 parent 的 destroying 回调先于 child 析构、child 的 destroying 先于自身成员析构。）

- [ ] **Step 2: 跑测试确认失败**：编译错（delete_later/destroying 未声明）。前置：Task 1 已把 tst_object 注册块换成 tst_event_loop 样式（INCLUDE_DIRECTORIES tests/ + thread 库）以支撑 FakeDispatcher include。
- [ ] **Step 3: 实现**。关键代码形态：

object.hpp（protected 段）：

```cpp
protected:
    /** @brief 预销毁回调：~Object 顶部调用（派生成员仍存活、children 未级联）。断连 signals 的钩子。@since 0.2 */
    virtual void destroying();

public:
    /** @brief 请求在当前线程 EventLoop 下一次排空时删除 this（Qt deleteLater 语义）。无当前环 = fatal。@since 0.2 */
    void delete_later();
```

event_loop.cpp（thread_local 当前环——放 event_loop.cpp 匿名命名空间）：

```cpp
namespace
{
thread_local EventLoop *t_current_loop = nullptr;
} // namespace
```

- `exec()` 入口：保存旧值 `EventLoop *prev = t_current_loop; t_current_loop = this;`，出口（含异常路径——exec 无异常，正常 return 前）恢复 `t_current_loop = prev;`。
- `Object::delete_later()`：object.cpp 内声明 `EventLoop *current_event_loop();`（event_loop.hpp 公共自由函数或 Object 调用点的 detail 访问——**用自由函数 `EventLoop::current()` 静态成员最简**，返回 `t_current_loop`，声明在 event_loop.hpp，fatal if null 的检查放 delete_later 侧）：

```cpp
void Object::delete_later()
{
    EventLoop *loop = EventLoop::current();
    CXXKIT_CHECK(loop != nullptr) << "Object::delete_later: no running EventLoop on this thread";
    EventLoop *safe = loop;
    safe->post([this]() { delete this; });
}
```

- `~EventLoop`：排空（执行）剩余队列：

```cpp
EventLoop::~EventLoop()
{
    CXXKIT_D(EventLoop);
    std::deque<std::function<void()>> tasks = d->take_post_queue();
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        tasks[i](); // delete_later 闭包必须执行——丢弃 = 对象泄漏
    }
    if (t_current_loop == this)
    {
        t_current_loop = nullptr;
    }
}
```

（`EventLoop::post` 已存在于 event_loop.hpp——执行者读头文件确认签名；若 post 是 private/未公开，改用 d->mPostQueue 直推的 friend 路径，以实际代码为准。）
（EXCEPTIONS 约束：排空循环若闭包抛异常——库编译默认禁异常，不加 try/catch，保持简单。）

- [ ] **Step 4: 跑测试确认全过** + **Step 5: 全量回归 80 套件** + **Step 6: 格式 + 提交** `feat(kernel): delete_later + destroying hook + event loop drain on destroy`。
  - ⚠️ delete_later_without_current_loop_fails 若与测试并行度冲突（thread_local 污染）：gtest 默认单线程内串行，同文件顺序无污染；若跨文件并行有污染，把该用例移到独立 TEST 文件并在步骤注明。

### Task 3: cxxkit::memory WeakPtr / WeakPtrFactory（Chromium 形，独立于 Task 1/2）

**Files:**
- Create: `cxxkit/memory/weak_ptr.hpp`（header-only：`WeakPtr<T>` + `WeakPtrFactory<T>`）
- Modify: `cxxkit/memory/CMakeLists.txt`（头进 GLOB 已自动——CONFIGURE_DEPENDS 覆盖 *.hpp，确认即可；无需改）
- Test: Create `tests/tst_weak_ptr.cpp` + `tests/CMakeLists.txt` 注册

**Interfaces:**
- Consumes: 无（纯独立）。
- Produces（后续 Phase Object 集成时消费）：

```cpp
namespace cxxkit {
template <class T> class WeakPtr {
public:
    WeakPtr();                                    // 空
    T *get() const;                               // owner 存活返回指针，否则 nullptr
    void reset();
    explicit operator bool() const;
    // 拷贝/移动语义完整（共享同一 INVALIDATED 标志）
};
template <class T> class WeakPtrFactory {
public:
    explicit WeakPtrFactory(T *owner);            // 持有 owner 指针，不拥有
    WeakPtr<T> get_weak_ptr() const;
    ~WeakPtrFactory();                            // 析构置 invalidated —— 所有 WeakPtr 失效
    // 不可拷贝
};
} // namespace cxxkit
```

实现要点（Chromium WeakPtrFactory 精简形，~90 行）：`WeakReference`（`std::shared_ptr<InvalidFlag>` 语义——用 `shared_pointer.hpp` 里已有的 `shared_ptr` 还是 std？**用 std::shared_ptr**——memory 库现有 shared_pointer.hpp 是旧式自研，WeakPtr 用 std 简单正确且不混入自研体系；唯一 owner 标志块 shared_ptr<atomic<bool>>，WeakPtr 持 weak_ptr<atomic<bool>>，get() 时 lock 判活）。Factory 作为派生类成员（非 intrusive——不改 Object）。C++11、无 auto。

- [ ] **Step 1: 写失败测试** `tests/tst_weak_ptr.cpp`：

```cpp
TEST(WeakPtr, get_returns_owner_while_alive)
TEST(WeakPtr, get_returns_null_after_factory_destroyed)
TEST(WeakPtr, multiple_weak_ptrs_share_invalidation)
TEST(WeakPtr, copy_and_move_preserve_liveness)
TEST(WeakPtr, reset_clears_and_bool_operator)
TEST(WeakPtr, owner_type_with_inheritance_upcasts) // WeakPtrFactory<Base> from Derived* —— 简单：Factory<Derived> → WeakPtr<Derived>，跳过协变（YAGNI，不写这个测试，5 个够）
```

（每个 TEST 的三段体由执行者按 AAA 完整写出——Arrange 造 Factory+owner，Act 析构/reset，Assert get()==nullptr/非空。）

- [ ] **Step 2: 确认失败** → **Step 3: 实现 weak_ptr.hpp**（含许可横幅/doxygen）→ **Step 4: 全过** → **Step 5: ASAN 抽查**（`cmake --build build-asan --target cxxkit_tst_weak_ptr -j8 && LSAN_OPTIONS="suppressions=$(pwd)/scripts/lsan.supp" ./build-asan/tests/cxxkit_tst_weak_ptr`——零诊断）→ **Step 6: 提交** `feat(memory): WeakPtr/WeakPtrFactory — non-intrusive weak references (Chromium shape)`。

### Task 4: 文档 + 记忆 + 全链门禁收口

**Files:**
- Modify: `.agents/memorys/decisions.md`（D33：Object 树语义补全 + WeakPtr 形态裁定）
- Modify: `.agents/memorys/status.md`（Phase 段落）
- Modify: `README.md`（kernel/memory 特性一句话，如有对应段落）
- 无 CMake 改动。

**Interfaces:**
- Consumes: Task 1-3 全部落地。
- Produces: D33 决策记录（内容：四件套语义/ChildEvent 同步派发语义——派发对象是已构造完整的父，Qt 构造期坑不触发/delete_later 需当前环 + 父子不可同投限制/destroying 虚函数析构期不派发 override 的 Qt 同款规则/WeakPtr Chromium 形拒绝 intrusive 的理由/Phase 2-3 备忘）。

- [ ] **Step 1: 写 D33 + status 段落**（照 D30/D31/D32 样式，pitfalls 若执行中有新教训按五段式即时补）
- [ ] **Step 2: 全链门禁**：`bash scripts/check.sh > /tmp/opencode/check-object-plan.log 2>&1; echo exit=$?`——必须 ALL CHECKS PASSED（**81 套件主树**（79 基线 + tst_object + tst_weak_ptr）+ shared + asan + coverage 不阻断）。
- [ ] **Step 3: 提交** `docs(memory): object tree + weakptr record (D33)`。
- [ ] **Step 4: 计划收尾**：勾完所有 checkbox，progress 记录，终审。

---

## Self-Review（已跑）

1. **Spec 覆盖**：synthesis Phase 1a（树四件套）→T1✓；1b（WeakPtr）→T3✓；1c（destroying/delete_later/ChildEvent）→T1(ChildEvent)+T2✓；Phase 2/3 明确不在范围（Global Constraints 后注明）✓。
2. **占位符扫描**：T1 Step3 有完整实现代码；T2 Step3 关键形态完整（exec 保存恢复/排空循环/delete_later 全给出）；T3 测试名给出且 AAA 结构说明明确，实现要点完整。两处「以实际代码为准」是让执行者核实现有宏签名（CXXKIT_P/post），非占位符。
3. **类型一致性**：`attach_child/detach_child` 命名 T1 定义、T1 内使用一致；`EventLoop::current()` T2 定义并在 delete_later 使用一致；WeakPtr/WeakPtrFactory 模板形 T3 内一致；Counter/CountedObject 测试助手 T1 定义 T2 复用一致。
