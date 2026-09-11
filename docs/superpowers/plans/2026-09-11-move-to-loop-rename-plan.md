# API 词汇族换血计划：move_to_thread → move_to_loop（D37）

日期：2026-09-11 ｜ 依据：2026-09-11 会话四轮架构讨论收束（用户终裁方向确认）
前置：D33/D34/D35（亲和子系统已落地）、D36（Object 能力面补全）

## 一、动因（用户洞察收束）

亲和模型的真实语义早已不是"线程"：`EventLoop::current()` 是 thread_local 的**环**指针；两环可同跑一线程（嵌套 exec）；环可无线程（外部 uv_loop 自驱动，`exec()` 只是便捷壳）；`EventLoopThread` 只是宿主装饰之一。**亲和承诺 = "事件将在这个环上派发"**——API 应对驱动者不可知。现名 `move_to_thread(EventLoop*)` / `thread(): EventLoop*` 是一族假词（成员叫 thread 返回 loop）；`loop` 系才是真一致：**收 loop、返回 loop、kThreadChange 报"loop 换了"**。

外部先例佐证（librarian 调研）：Qt 的 `moveToThread(QThread*)` 名实相符**仅因** QThread 融合线程+循环宿主双重身份；cxxkit 拆开了两者，强套 Qt 词汇必名实不符。GLib 5 库合并史证明"对象树+线程设施分库+对象亲和 API"是无人区组合。

## 二、改动面（实测）

| 符号 | 改名 | 出现点 |
|---|---|---|
| `move_to_thread` | `move_to_loop` | 113 处 / 13 文件（源码 12 + 测试 51 + 文档/记忆 43） |
| `thread()` getter (Object) | `loop()` getter | 14 处调用 + 声明/定义/全 doxygen |
| `kThreadChange` | **保留**（Qt 值 22 对齐；事件语义"宿主环变了"仍诚实） | 9 处仅核注释 |
| Event 类型注释 | 措辞校准（"object has changed threads" → loop 表述） | event.hpp 1 行 |
| ELT::loop() / Application::loop() | **语义统一红利**：改名后三处 loop()（Object/ELT/Application）都返回"亲和/主环"，撞车顾虑消解 | 零改动，仅 doxygen 交叉引用 |

**不换血的**：`EventLoopThread` 类名（它就是线程宿主装饰，名实相符）；`ELT::loop()`/`Application::loop()` 方法名；`reference_counter.hpp` 反向杂音 include（独立议题，不混车）。

## 三、边界规则（防执行走样）

- **记忆/历史文档不改写**：`.agents/memorys/*` 与 `docs/superpowers/*`（plans/specs）里的 43 处 `move_to_thread` 是历史叙事（D33-D36 决策语境）——只在 decisions.md **追加** D37 记录新旧名对照，历史文档保留原名 + D37 头部注一行"历史文档中的 move_to_thread/thread() 即今之 move_to_loop/loop()"
- **qt 处的 thread() 是 Qt 自有 API，绝不可改**（Momus 修复 1）：`cxxkit/qt/qt_event_dispatcher.cpp:54` 的 `mContext->thread()` 是 `QObject::thread()`（QThread 指针比较，R-B2-5 原文），不在改名面
- **C++11**：纯改名零语法新增；`CXXKIT_CHECK` 复合条件整体括号纪律不变
- **测试函数名同步**：7 个 `move_to_thread_*` 测试名 → `move_to_loop_*`（含 DeathTest）
- **tst_event_loop_thread.cpp:105 是活测试调用非注释**（Momus 修复 2）：`victim->move_to_thread(&elt.loop())` → `victim->move_to_loop(elt)`，断言不变；同时在 ELT 头文件顺手落隐式转换符：`operator EventLoop &() const { return *mLoop; }`（**注意解引用**——mLoop 是 `std::unique_ptr<EventLoop>` plain member；doxygen："Implicit conversion to the owned loop — enables `obj.move_to_loop(elt)`. Lifetime: the ELT must outlive objects bound to its loop."）
- **验收门禁**：主树/ASAN 各 83/83 全绿 + clang-format 零违规 + `grep -rn "move_to_thread" cxxkit/ tests/ examples/` 清零 + `grep -rnE '\.thread\(\)|->thread\(\)' cxxkit/ tests/ examples/ | grep -v '^cxxkit/qt/'` 清零（记忆/历史文档与 qt 子库豁免）

## 四、任务分解（SDD，单任务足够——纯机械替换 + 语义校准）

### T1 — API 换血（单实现者 + Momus 复核）

1. **kernel 签名层**（object.hpp/.cpp + object_p.hpp 注释 + event.hpp 注释）：
   - `move_to_thread(EventLoop *target)` → `move_to_loop(EventLoop *target)`（声明 + 定义 + 内部 CHECK 消息 "move_to_loop: ..."）
   - `EventLoop *thread() const` → `EventLoop *loop() const`（getter；doxygen 重写："Returns the loop dispatching this object's events (null = no affinity). Lifetime: the affinity loop must outlive objects bound to it."）
   - object.cpp 内部 `d_func()->mThread` **成员名不动**（ObjectPrivate::mThread 是私有实现细节，改名无 API 价值——备案）
   - doxygen "thread affinity" 措辞逐行裁定（Momus 修复 3）：grep -niE 'thread' object.hpp/cpp 预期仅 ~15 行需改；**start_timer/kill_timer/delete_later 三处 CHECK 消息保留 thread 措辞**（约束的是调用线程，EventLoop::current() 是 thread_local——改成 loop 反而失实）；其余 affinity 语义行才换 loop 措辞
   - event.hpp `kThreadChange` 注释 → "the object's dispatch loop has changed"
2. **测试层**（tst_object 43 处 + tst_application 6 处 + tst_event_loop_thread 1 处）：
   - `move_to_thread(` → `move_to_loop(`、`.thread()`/`->thread()` → `.loop()`/`->loop()`
   - 7 个测试函数名 `move_to_thread_*` → `move_to_loop_*`
3. **ELT 转换符**（event_loop_thread.hpp/.cpp + 活测试行迁移）：
   - `operator EventLoop &() const { return *mLoop; }`（解引用 unique_ptr，Momus 修复 2；doxygen 见 §三）
   - event_loop_thread.hpp:46 头部文档示例 `worker.move_to_thread(&elt.loop())` → `worker.move_to_loop(elt)`
   - 新测试：`obj.move_to_loop(elt)` 编译通过 + 亲和生效（复用跨线程 delete_later 用例形态）
4. **README/AGENTS**：`cxxkit/thread` 行 ELT 描述同步（若提及 move_to_thread）
5. **记忆**：decisions.md 追加 D37（动因/新旧对照/先例引用/kThreadChange 保留理由/ELT 转换符裁定）；pitfalls 无新增
6. **提交**：`refactor(kernel)!: rename move_to_thread/thread() to move_to_loop/loop() (D37)`—— breaking-change 标记（0.2 未发版，免费窗口）

### 验收（实现者自查 + controller 复验）

- 全量替换后 `grep -rn "move_to_thread" cxxkit/ tests/ examples/` 清零
- `grep -rnE '\.thread\(\)|->thread\(\)' cxxkit/ tests/ examples/` 清零
- 主树 83/83 + ASAN 83/83（LSAN_OPTIONS 正跑法）+ clang-format 干净
- 三处 `loop()` 语义核对：Object::loop()=亲和环 / ELT::loop()=自持环 / Application::loop()=主环——doxygen 互相引用闭环

## 五、风险与备案

- **`loop()` getter 与 ELT/Application 的 loop() 三义性**：接受并视为特性（三处都返回"该上下文的环"），doxygen 交叉注明
- **qt_event_dispatcher.{hpp,cpp} 明确不改**（Momus 修复 1）：其 thread() 与注释描述的是 Qt QThread 语义，非 cxxkit 亲和 API；验收门禁已豁免 qt 子库
- **历史文档断层**：D37 头部加新旧名对照说明，消除后续考古成本
- **ELT 转换符重载决议**：引用转换（1 次用户转换）+ 取址标准转换优于其他路径，无歧义（上轮 Metis 已推演）；`&elt` 取 ELT 地址场景（如 parent 传参）不受影响（转换不参与）
EOF