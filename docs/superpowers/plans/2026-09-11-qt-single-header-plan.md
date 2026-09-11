# qt 子库解散：单头化并入 kernel（D39）

日期：2026-09-11 ｜ 依据：2026-09-11 会话裁定（用户终裁：单文件 + 移入 kernel + 命名 qt_dispatcher.hpp）
前置：D38（uv 解散——kernel 携带引擎先例 + 词汇面）

## 一、动因

D38 后 qt 子库成为孤儿：uv 引擎已入 kernel（私有 detail + 默认工厂），qt 仍是独立 compiled 库（461 行三头一 cpp + 独立开关 + 导出宏 + compiled target），身份不明（非组件非后端）。用户裁定终局形态：**整个库坍缩为单头 `cxxkit/kernel/qt_dispatcher.hpp`**，kernel 构建零 Qt 依赖，Qt 生态消费者（必然自带 Qt 工程）include 即用。

**定位语义**：kernel 两个并列引擎入口——`default_dispatcher.hpp`（uv，自主场景，kernel 携带引擎）与 `qt_dispatcher.hpp`（Qt，寄生场景，消费方自带 Qt）。后者休眠语义：不 include = 零成本零参与；include = 需要 Qt 头路径 + 消费方链接 Qt6::Core。

## 二、终版结构

```
cxxkit/kernel/qt_dispatcher.hpp    # 新增单头（~400 行）：QtEventDispatcher + Private + make_qt_dispatcher 全内联
                                   # 头顶契约注释：include 本头 = 消费方须有 Qt 头路径并链接 Qt6::Core；
                                   # kernel 自身构建/安装零 Qt 参与（纯文本分发）

消亡：cxxkit/qt/ 整树（qt_event_dispatcher.{hpp,cpp} + detail/qt_event_dispatcher_p.hpp +
      qt_global.hpp + CMakeLists.txt）、CXXKIT_QT_API、CXXKIT_ENABLE_LIB_QT、
      cxxkit::qt target、CxxKitConfig qt 组件条件、qt .pc 无（本就 CMake-only）
```

## 三、合并技术要点

| # | 项 | 明细 |
|---|---|---|
| M1 | 三文件拼接 | qt_event_dispatcher.hpp（131）+ detail/qt_event_dispatcher_p.hpp（66）+ qt_event_dispatcher.cpp（230）→ 单头。**实测无匿名命名空间/文件级 static**（纯拼接安全）；include 归并（Momus Fix-4①）：QtCore/QPointer、QTimer、QMetaObject、**QCoreApplication、QThread**、`<cxxkit/tools/checks.hpp>`、`<limits>` |
| M2 | pimpl 转内联 | `CXXKIT_DECLARE_PRIVATE/DEFINE_DPTR(QtEventDispatcher)` 删除；Private 改**值成员**（单头内完整类型可见；全库 `CXXKIT_D/d->` 重写为成员访问）；Qt 前向声明区无效——QPointer<QObject>/QTimer\* 成员需完整类型，**Qt 头必须真 include**（本头语义即如此） |
| M3 | 方法 inline 化 | 11 个方法定义（公共 9 + Private 2，Momus 核实）从 cpp 移入；`make_qt_dispatcher` 改 `inline`；零静态/全局数据核对通过 |
| M4 | 导出宏 | `CXXKIT_QT_API` 全部剥除；`qt_global.hpp` 不迁（header-only 无导出面） |
| M5 | include/注释站点 | ELT 头（event_loop_thread.hpp L40/L61）+ **kernel/application.hpp:66（Momus Fix-2）**的 make_qt_dispatcher 注释改为 make_default_dispatcher/宿主注入措辞；tst/exp 的 include `<cxxkit/qt/...>` → `<cxxkit/kernel/qt_dispatcher.hpp>` |
| M6 | CMake | 根 CMakeLists：`CXXKIT_ENABLE_LIB_QT` option + `cxxkit_add_subdirectory(cxxkit/qt)` + **L396 `set(CXXKIT_QT_ENABLED ...)` 死变量删除（Momus Fix-3①）**；CxxKitConfig.cmake.in：qt 组件 find\_dependency 条件块删除（含 `qt IN_LIST FIND_COMPONENTS` 分支）；**L245-249 CXXKIT\_QT\_LOCAL\_STDCPP 块保留不动**（Momus Fix-3②，测试/示例仍需；tests L629 过时注释同步修）；门禁 1 grep 追加 `CXXKIT_QT_ENABLED`。**kernel CMakeLists 零改动**（递归 install 自带新头；不链 Qt） |
| M7 | 测试 | tst_qt_event_dispatcher compiled 注册删除 → 单头测试 `cxxkit_tst_kernel_qt_dispatcher`：块内 `find_package(Qt6 QUIET COMPONENTS Core)` + **`if(TARGET Qt6::Core)` 守卫（Momus Fix-1，无变量案钉死）**；保留 `CXX_STANDARD 17` + `${CXXKIT_QT_LOCAL_STDCPP}` + `${CMAKE_THREAD_LIBS_INIT}`（Momus Fix-4④） |
| M8 | 示例 | exp_qt_embed：include 换新路径 + CMake 块同 M7 守卫（`find_package(Qt6 QUIET)` + `if(TARGET Qt6::Core)`）；链接面（含 stdcpp 特殊处理）**原样保留** |
| M9 | 文档/记忆 | README 库表 qt 行删除 + kernel 行加 "Qt host-bridge dispatcher (header-only, opt-in by include)"；**Quick start options 句 + Examples 表 exp_qt_embed 行的 CXXKIT_ENABLE_LIB_QT 同步清理（Momus Fix-4③）**；docs/qt-embed-smoke.md 路径同步；decisions.md 追加 D39；status.md 同步 |
| M8 | 示例 | exp_qt_embed：include 换新路径 + CMake 块守卫同 M7 新变量；链接面（Qt6::Core + 本机 g++10.5 stdcpp 特殊处理）**原样保留**（宿主工程示范身份不变） |

## 四、验收门禁

1. `grep -rn "cxxkit/qt/\|CXXKIT_ENABLE_LIB_QT\|CXXKIT_QT_API\|make_qt_dispatcher" cxxkit/ tests/ examples/ CMakeLists.txt | grep -v "kernel/qt_dispatcher"` → make_qt_dispatcher 仅存在于新头 + 消费者调用点；其余三项清零
2. 主树（Qt 环境）构建 0 error + ctest 全绿（含新 tst_kernel_qt_dispatcher）
3. **无 Qt 环境 green（Momus Fix-1 钉死无变量案）**：find_package(Qt6 QUIET) 失败环境下构建+测试 green（测试/示例块自条件裁剪）——**新头休眠零参与是硬门禁**（kernel 主体编译不触碰该文件）
4. ASAN 全量零诊断（正跑法；Qt 环境下）
5. 安装树验证：`build/install/include/cxxkit/kernel/qt_dispatcher.hpp` 存在；`build/install/include/cxxkit/qt/` 不存在
6. clang-format 触碰文件干净；C18 英文

## 五、风险与备案

- **Qt 检测无变量案（Momus Fix-1 钉死）**：不做新变量；测试/示例 CMake 块直接 `find_package(Qt6 QUIET)` + `if(TARGET Qt6::Core)` 条件注册——环境有 Qt 自动跑，无则静默裁剪
- **QPointer<QObject> 成员的完整类型需求**：M2 中 Qt 头真 include 后，"公开头 Qt-free"旧卖点消亡——**这是本方案的语义本体**（用它=有 Qt），非缺陷；doxygen 顶部契约注释明示
- **signals.hpp 风格先例**：kernel 内已有 1951 行 header-only 大头（signals.hpp），400 行单头无粒度违例
- **qt_global.hpp 的 CXXKIT_BUILD_SHARED_QT 分支**：随导出宏消亡，shared 构建无 qt 面（原 qt target 在 shared 下的行为随 target 消亡，无回归面）
- **exp_qt_embed 运行验证**：守卫挂新变量后，无 Qt 环境裁剪（原逻辑不变）；有 Qt 环境仍需人工 smoke（docs/qt-embed-smoke.md 不变）

## 六、明确不做

- 不改类名 `QtEventDispatcher` / 工厂名 `make_qt_dispatcher`（D37/D38 刚稳定词汇面，不再多动）
- 不做 C 版本/GTK 桥预留（YAGNI，宿主桥家族化是将来议题）
- kernel 的 CMakeLists 不新增 Qt 构建接线（find\_package/link/option）——Momus Fix-4②：既有 L30 注释含 qt 字样不属构建接线，红线措辞按此口径
