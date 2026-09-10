# Phase 2 事件投递基础设施设计（Event Delivery Infrastructure）

**日期**: 2026-09-09
**状态**: Approved（4 项用户裁定 + 三节设计确认）
**前置**: Phase 1（D33：对象树 + WeakPtr，`5d512ef`）
**后续**: Phase 3 线程亲和（moveToThread，需求触发另立）

## 1. 目标

把 `cxxkit::kernel` 从"post(function) 单队列 + 事件空壳"补全为 Qt 形事件投递基础设施：

1. `send_event` / `post_event` 双入口（同步/异步）
2. `install_event_filter` / `remove_event_filter` 过滤链（消除 event_filter 孤岛）
3. `kDeferredDelete` 正名化（DeferredDeleteEvent 走真 Event 队列）
4. `~Object` 清除 pending 事件（**自动解除 D33 备案的"父子不可同投 delete_later"限制**）

**不在范围**（YAGNI/Phase 3）：跨线程投递、事件压缩（compress）、sendPostedEvents 按接收者过滤、多优先级排序、moveToThread、Application 单例复活。

## 2. 已确认裁定

| # | 裁定 | 理由 |
|---|---|---|
| R1 | 队列收**裸 Event\***（Qt 同款），所有权转移给队列 | 支持 DeferredDelete 正名化与 removePostedEvents；接收方亡由 ~Object 清队列兜底 |
| R2 | DeferredDelete **迁移**到 Event 队列（function 队列版删除） | 解父子同投限制；真事件路径可被 filter 链观察（Qt 同款） |
| R3 | filter 链本期做 | 团队分析 P1 项（~40 行），测试形态已在分析中列出 |
| R4 | **双队列并存**：mPostQueue(function) 不动 + 新 mEventQueue(EventEntry) | post 热路径已验证（79+2 套件 + uv/tcp 全走），改造风险高；独立演进 |

## 3. 架构

### 3.1 数据结构

```cpp
// event_loop_p.hpp（EventLoopPrivate 新增）
struct EventEntry
{
    Object *mReceiver;
    Event *mEvent;
};
std::mutex mEventMutex;
std::deque<EventEntry> mEventQueue;
std::deque<EventEntry> take_event_queue();   // swap-under-lock，照 take_post_queue 形态
void remove_pending_events(Object *receiver); // ~Object 清理入口（锁内 remove+delete）
```

```cpp
// event.hpp（新增/修改）
class DeferredDeleteEvent : public Event   // Type::kDeferredDelete 已有（52）
{
public:
    DeferredDeleteEvent();
};

// Event::Type 补充（若需）：kUser = 1000 已有，够用；自定义事件用户继承 Event 即可
```

### 3.2 Object API（kernel/object.hpp）

```cpp
public:
    // 同步投递：直调 receiver->event(e)（filter 链前置）。调用方保留 event 所有权。
    static bool send_event(Object *receiver, Event *event);

    // 异步投递：{receiver, event} 入当前环 mEventQueue，所有权转移给队列。
    // 无运行中环 = CXXKIT_CHECK fatal（与 delete_later 同款纪律）。
    static void post_event(Object *receiver, Event *event);

    // filter 链
    void install_event_filter(Object *filter);   // 头插（后装先过滤，Qt 同款）
    void remove_event_filter(Object *filter);

protected:
    // event_filter() 虚函数已存在（Phase 0 落地），本期接通调用路径
```

### 3.3 事件路径（核心流程）

**send_event**:
```
send_event(receiver, e)
  → 若 filter 链非空：逆序遍历 receiver->mFilters，filter->event_filter(receiver, e)
    返回 true = 拦截，返回 false = 继续
  → 全未拦截：receiver->event(e)
  → 返回 e->is_accepted()（Qt 同款返回值语义）
```

**post_event → process_events 派发**:
```
post_event(receiver, e)
  → EventLoop::current() 校验（null = fatal）
  → current 的 d->mEventQueue.push_back({receiver, e})（锁内）

process_events（修改：先 post 队列后 Event 队列——嵌套投递同轮可见，解 PIT-43 族跨轮断链）
  ① take_post_queue() 排空（现有逻辑零改动）
  ② take_event_queue() 排空：
     for each {receiver, e}:
       receiver->send_event(receiver, e) 形态 = filter 链 + event()（与同步路径同一条）
       **派发用 send_event 内部逻辑（filter 链生效），event 所有权在派发后 delete**
  ③ 两队列均空 → dispatcher->process_events(flags)（现有）
```

**~Object 清理（Qt ~QObject 同款）**:
```
~Object() 顶部（destroying() 调用之后、级联之前——顺序：destroying → 清 pending → 级联 → 摘链）：
  → EventLoop::current() 非空时：current->d->remove_pending_events(this)
    （remove+delete 所有 receiver==this 的条目——含 DeferredDeleteEvent → **父子同投限制解除**）
  → current 为空：条目无法清——但投递前置已保证无环线程无 pending，此路径 = 跨线程投递存在，
    Phase 2 断言不发生（Phase 3 moveToThread 时重审）
```

**kDeferredDelete 实装（object.cpp event()）**:
```cpp
case Event::Type::kDeferredDelete:
    delete this;        // DeleteInEventHandler 正名化——Qt ~同款
    return true;
```

**delete_later 迁移（object.cpp）**:
```cpp
void Object::delete_later()
{
    // 原 post([this]{delete this}) 删除
    post_event(this, new DeferredDeleteEvent);
}
```

### 3.4 ~EventLoop 排空扩展

现有排空（post 队列执行）之后追加：take_event_queue() 排空——**剩余 Event 一律 delete 不派发**（环已死，Qt ~QThreadData 同款语义）。current 清理语义不变（exec-only）。

### 3.5 filter 链语义细节

- **存储**：`ObjectPrivate::mFilters`（`std::vector<Object*>`），头插 install（后装先过滤——Qt 同款），remove 线性查找 erase。
- **递归防护**：Qt 允许 filter 观察自己（send_event 到自身时 filter 链也生效）——Phase 2 照 Qt，不加防重入锁；文档注明"filter 内不得 install/remove 自身所在链"（Qt 同款约束）。
- **filter 对象亡**：Qt 靠 ~QObject 清除被过滤者链表条目——**~Object 补第二清理**：从所有已 install 的 watched 对象摘除自己？**Phase 2 简化**：不反查 watched 链（需反向注册表，YAGNI）——文档契约"filter 必须比 watched 活得长或自行 remove"（比 Qt 弱，Phase 3 备忘）。**此为已知弱化，D34 备案**。
- **DeferredDelete 事件过 filter 链**：走（R2 语义一致——Qt 的 DeferredDelete 也过 eventFilter）。

## 4. 语义裁定细节（写入 D34）

1. **双队列顺序**：post 队列先于 Event 队列（每轮 process_events 内）。跨轮 FIFO 不保证（两队列间无全局序——Qt 同款：posted events 与 posted metacalls 无跨类全局序）。
2. **EventEntry 所有权链**：push 后队列拥有 Event*；take_event_queue 取出后局部 deque 拥有；派发后 delete。任何路径不得 double-delete：~Object 清理只动队列内条目；派发形态 = 锁内逐条 pop + 锁外派发（Qt 忠实，非快照）。**D34 修正不变量**：原表述“~Object 与派发不并发”在单线程级联析构下即为假（父条目派发中 → 级联析构 → 子/孙 purge）——真实不变量 = **purge 与 pop 锁内互斥、派发在锁外**：派发期间的穿插级联析构从队列本体移除未派发条目，pop 到即不存在，无快照悬垂窗口。
3. **嵌套 process_events**：允许（Qt 同款）——内层排空时拿走的条目外层不可见（swap 快照），无重入问题。
4. **send_event 的返回值**：`e->is_accepted()`（Event::accept/ignore 已有）。filter 拦截返回 false（Qt：被过滤 = 未送达）。
5. **post_event 频率**：无合并/压缩（Qt postEvent 也不合并——compress 是显式 API，YAGNI）。

## 5. 测试策略

- **tst_object.cpp 扩展**（不新建套件——事件投递是 Object API）：send_event 直达/accepted 返回值、post_event+process_events 派发顺序（post 先 event 后）、filter 链（拦截/放行/头插顺序/remove）、DeferredDelete 正名化（delete_later 真事件路径 + ~Object 清 pending 解父子同投 + 嵌套投递）。
- **父子同投解除的正名测试**（D33 限制①的解除证据）：父与子同投 delete_later → process_events 一轮 → 两者皆亡、无二次 delete（ASAN 树验证）。
- **回归**：现有 delete_later 三态测试（exec 内合法/无环 fatal/未 exec fatal）必须原语义通过——delete_later 签名与 fatal 纪律不变，只换内部通道。
- **门禁**：81 套件基线不降 + ASAN 零诊断 + check.sh 8/8。

## 6. 量级

- kernel：object.hpp/cpp（send/post/filter/~Object 清理）+ event.hpp/cpp（DeferredDeleteEvent）+ event_loop_p.hpp（EventEntry/队列）+ event_loop.cpp（process_events 双排空/~EventLoop 排空）≈ **250-300 行**
- 测试：tst_object 扩展 ~10 用例 ≈ 250 行
- 无新文件（全在既有文件内）、无 CMake 改动、无新依赖

## 7. 已知限制（D34 备案）

1. filter 生命周期弱化：filter 必须比 watched 活得长或自行 remove（Qt 有反向清理，Phase 2 不做反向注册表）
2. 跨线程 post_event = fatal（Phase 3 moveToThread 时重审）
3. 无事件压缩、无多优先级、无 sendPostedEvents 过滤（YAGNI）
4. ~Object 清 pending 仅当前线程环——跨线程 pending 在 Phase 2 是"断言不发生"态
