# cxxkit::crash 崩溃子库设计与实现计划（v2）

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增第 15 子库 `cxxkit::crash`——基于 breakpad（minidump 生成，signal-safe）+ backward-cpp（栈回溯打印，opt-in）的跨平台崩溃处理库，纯 C++11、无 Qt、无网络上传（v1 范围）。

**Architecture:** QExt 式 vcpkg 导出 .7z 缓存 + CxxKit 现有 stamp/fetch 机制消费（构建期零 vcpkg toolchain 依赖，守 C1/D4）。单 wrap（FindWrapCrashDeps）单解包合并归档 `crash-deps-<triplet>.7z`，breakpad 与 QExt::Breakpad 同源同版本（2024-02-16#1）保证链接期零符号冲突；运行期与 QExt::Breakpad 二进制级互斥（契约文档化）。backward-cpp header-only 作栈打印工具，不注册自身 SignalHandling，默认关闭、opt-in 开启。

**Tech Stack:** CMake 3.15...3.31、C++11（库）/ C++14（测试）、vcpkg（仅离线导出工具）、Google breakpad v2024-02-16#1（`unofficial::breakpad::libbreakpad_client`）、backward-cpp v2023-11-24#1（`Backward::Interface`）、elfutils 0.195 + libunwind 1.8.3（Linux runtime payload，.so 随包分发）。

**Spec:** 本计划由 2026-08-19 崩溃库分析（QExt/OpenCTK/父项目/vcpkg 生态四路调研）+ 三视角团队评审（build/api/risk）修订而成。评审决策全部内联于 Global Constraints 与各 Task，来源标注 [B]/[A]/[R]（build/api/risk-reviewer）。

---

## Global Constraints

1. **C1/D4**：唯一构建系统 CMake；三方依赖 vendored + stamp 缓存，**禁 FetchContent**；构建期不引入 vcpkg toolchain [R-H3]
2. **C4**：库代码 C++11（`cxx_std_11`）；测试 target C++14；vendored 三方不受限（PIT-8 先例）
3. **单 wrap 单解包** [R-C2]：`FindWrapCrashDeps.cmake` 一次解包合并归档，提供 breakpad + backward 两个 IMPORTED target，禁双 wrap 双解包
4. **二进制级互斥契约** [R-C1]：同进程 crash ON ⇒ QExt::Breakpad OFF；`CrashHandler::install()` 原子守卫拒绝二次安装
5. **v1 不做上传**（用户决策）：无 `set_upload_url`/`sendDumps`；**crash 不依赖 cxxkit::network**，无 cpr/curl/mbedtls 构建代价 [A-C1][B-C2]
6. **崩溃回调内**：只写 minidump + 快速返回；backward 栈打印默认 OFF，opt-in 开启后 `load_from(崩溃线程 ucontext)` [A-C2]；禁 `load_here()`（抓到 handler 线程栈）[A-C2]
7. **不反向伸进 tools**：`write_minidump()` 仅用户显式调用，禁钩 `CXXKIT_CHECK`（tools→crash 反向依赖）[A-C3]
8. **fallback = FATAL_ERROR + 精确导出命令**，禁现场自动 vcpkg install（版本漂移击穿同源）[R-H3]
9. **PIC 保障**：复用父项目已验证 .7z（QExt 消费实证）+ 配置期 `try_compile` 冒烟（PIE 链接 libbreakpad_client.a），失败 FATAL [B-H1]
10. **relocatability 门禁**：FindWrap 解析出的 -I/-L 路径断言落在解包树内 [R-Q3]
11. **版本门禁**：归档 sidecar `<name>.version` 四包版本清单，FindWrap 读取校验 [R-M1]
12. **公共头不暴露三方类型**：`CrashCallback` 自有签名；breakpad/backward 头只进 `detail/`（PRIVATE include），**无 D6 命名空间化安装** [B-C1]
13. **宏体系**：公共头 `#if CXXKIT_FEATURE_ENABLE_CRASH` 整体包裹（kernel 同构）[A-H3]；`crash_global.hpp` 三态导出宏 `CXXKIT_BUILD_SHARED_CRASH`/`CXXKIT_BUILDING_CRASH_LIB`/`CXXKIT_CRASH_API`（network 同构）[A-H4]
14. **生命周期**：「先配置后 install」；install 后 `set_*` → `CXXKIT_CHECK(false)` [A-H1]
15. **归档/路径**：合并归档 `crash-deps-<triplet>.7z`（triplet 映射表见 Task 2）[B-M1]；stamp 命名含 INPUT 路径标识 [B-M2]；`CXXKIT_3RDPARTY_PACKAGES_DIR` 顶层规范化（INPUT 注入 → 默认 `${PROJECT_SOURCE_DIR}/3rdparty` → FATAL）[B-M3]
16. **pkg-config**：映射 `unofficial::breakpad::libbreakpad_client` → `-lbreakpad_client` + pthread；禁装 vcpkg .pc（绝对 prefix 不可移动）[B-M4]
17. **zlib 不接线**：breakpad client 不需要 zlib（仅 tools 路径），现成 FindWrapZLIB 资产闲置不动 [B-附]
18. **backward Linux 运行时**：libdw 靠 `dlopen("libdw.so.1")` 解析 → elfutils/libunwind 以 .so 随包分发，crash 库 `INSTALL_RPATH $ORIGIN`；检测不到自动降级 addr2line [B-H2]
19. **平台门控**：Linux/macOS x64 优先；Windows/arm64 无 .7z → FindWrap FATAL 提示导出命令；顶层 WIN32 默认 OFF + 警告，禁静默 no-op [R-H1][B-H3]
20. **测试**：禁 gtest death test（与 breakpad 抢信号）；独立夹具可执行文件 + 轮询 .dmp + 超时 [A-M5]
21. **安装/导出链**：三方 payload 装到 `<prefix>/lib/` + `<prefix>/lib/cmake/<pkg>/`；顶层 `CXXKIT_VENDORED_FIND_DEPS` 追加 `unofficial-breakpad`、`Backward` [B-C1]
22. **TPTouch 休眠 wrap 禁复活**：grep 门禁（见 Task 6 检查命令）[R-M7]

---

## 背景与决策摘要

### 调研结论（2026-08-19，四路实证）

| 来源 | 结论 |
|---|---|
| QExt | breakpad 集成 = vcpkg install → export .7z 缓存（`src/3rdparty/breakpad-x64-{linux,osx}.7z` 已是产物）→ 构建期 `cmake -E tar xzvf` 解包 → `find_package(unofficial-breakpad)` → 静态 `libbreakpad_client.a`。API 是 Qt 绑定（QObject/QNetworkAccessManager），CxxKit 需去 Qt 化。无 backward-cpp |
| OpenCTK | 无崩溃模块，无可移植代码 |
| 父项目 | 无 vcpkg toolchain（pixi + .7z）；三工程全部经 `INPUT_QEXT_3RDPARTY_PACKAGES_DIR` 委托 QExt 处理 breakpad |
| vcpkg 生态 | breakpad port v2024-02-16#1（静态，signal-safe，C++11 OK，tools 仅 Linux）；backward-cpp v2023-11-24#1（header-only，非 signal-safe，dlopen libdw）；elfutils 0.195 / libunwind 1.8.3 port 存在（linux only） |
| CxxKit | D16 已规划 crash 独立子库；profiling/Tracy 是 opt-in 模板（FindWrapTracy + INTERFACE 宏注入 + D11 分发） |

### 三视角评审关键修订（v1 → v2）

见 Global Constraints 标注。核心：上传解耦（v1 砍掉）、单 wrap 单解包、安装/导出链补全、PIC + relocatability + 版本三重门禁、fallback 改 FATAL、公共头零三方泄漏、`load_from(ucontext)` 打崩溃线程栈。

---

## 设计

### 1. 依赖体系：crash-deps 合并归档

```
┌─ 产出（开发期，vcpkg 机器）─────────────────────────────┐
│ scripts/export_crash_deps.sh（Task 1）                  │
│   vcpkg.json 锁定 4 包：breakpad 2024-02-16#1           │
│                        backward-cpp 2023-11-24#1        │
│                        elfutils 0.195（linux）          │
│                        libunwind 1.8.3（linux）         │
│   overlay triplet（x64-linux-cxxkit.cmake）：-fPIC      │
│   vcpkg export --raw → crash-deps-x64-linux.7z          │
│   + sidecar crash-deps-x64-linux.7z.version（4 包版本）  │
└──────────────────────────────────────────────────────────┘
        │ 注入路径
        ▼
┌─ 消费（构建期，零 vcpkg 依赖）──────────────────────────┐
│ FindWrapCrashDeps.cmake（Task 2）                       │
│   1) INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR 或 自身 3rdparty/ │
│   2) stamp（含输入路径标识）→ 解包 crash-deps-<triplet>.7z │
│   3) 按标记定位 installed root（glob share/unofficial-  │
│      breakpad/*Config.cmake 反推，不硬编码路径）          │
│   4) 门禁：.version 校验 + -I/-L 路径断言 + try_compile  │
│      PIC 冒烟，任一失败 FATAL（附导出命令）              │
│   5) 产出 CxxKitWrapCrashDeps::WrapBreakpad             │
│             CxxKitWrapCrashDeps::WrapBackward           │
└──────────────────────────────────────────────────────────┘
```

- 解包布局：`vcpkg export --raw` → `<unpack_dir>/installed/<triplet>/`（QExt 实证）
- triplet 映射：`x64-linux`（CMAKE_SYSTEM_NAME=Linux + x86_64）、`x64-osx`（Darwin + x86_64）；其余 FATAL
- Linux 归档含 elfutils/libunwind 的 `.so`（runtime payload，backward dlopen 用）；crash 库 `INSTALL_RPATH $ORIGIN`

### 2. API 设计（v1，无上传）

```cpp
// cxxkit/crash/crash_handler.hpp —— #if CXXKIT_FEATURE_ENABLE_CRASH 整体包裹
namespace cxxkit {

class CrashHandlerPrivate;

class CXXKIT_CRASH_API CrashHandler {
public:
    using CrashCallback = bool (*)(const char* dumpPath, void* context, bool succeeded);

    static CrashHandler& instance();                 // Singleton<CrashHandler, true>（手动生命周期）
    static void destroy();                           // 显式析构（uninstall + 释放）

    bool isHandlerInstalled() const;                 // 原子探测（防双 handler 契约）
    bool install();                                  // 构造 ExceptionHandler + 原子守卫；重复 install → false + 告警
    void uninstall();

    bool setDumpPath(const char* path);              // install 前配置，install 后调用 → CXXKIT_CHECK(false)
    bool setStackTraceOnCrash(bool enable);          // 默认 false；true = 回调内 load_from(ucontext) 打栈到 stderr
    bool setCallback(CrashCallback cb, void* context); // 自有签名，三方类型不出公共头

    bool writeMinidump();                            // 手动触发（非崩溃路径，如用户 fatal 钩子）
    std::vector<std::string> dumpFileList() const;
    void clearDumps();

    CXXKIT_DECLARE_SINGLETON(CrashHandler)           // 见 cxxkit/patterns/singleton.hpp:108
private:
    CrashHandler();
    ~CrashHandler();
};

} // namespace cxxkit
```

```cpp
// cxxkit/crash/detail/crash_handler_p.hpp —— 私有实现（随公共头安装，doxygen 排除）
// 唯一允许 include 三方头的地方：
//   <breakpad/client/linux/handler/exception_handler.h>  （平台条件）
//   <backward.hpp>（仅 setStackTraceOnCrash(true) 路径实例化）
struct CrashHandlerPrivate {
    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler;  // install() 时构造
    std::string mDumpPath;
    CrashHandler::CrashCallback mCallback = nullptr;
    void* mCallbackContext = nullptr;
    bool mStackTraceOnCrash = false;
    // 平台相关：Linux MinidumpDescriptor 持有于 mHandler 构造；Windows/macOS 构造参数差异内聚于此
};
```

**回调语义**（breakpad handler 线程执行，非信号上下文）：
1. 默认：仅 breakpad 写 .dmp + 返回 `succeeded`（快速，零附加风险）
2. `setStackTraceOnCrash(true)`：`backward::StackTrace st; st.load_from(static_cast<ucontext_t*>(context));` → `backward::Printer` 打印到 stderr。handler 线程非信号上下文，风险可控；文档明示「malloc 死锁理论上限」（ponytail: 注释天花板）
3. `setCallback()` 用户自定义回调可叠加（用户回调内自行保证安全）

**单实例守卫**：`static std::atomic<bool> sInstalled;` 于 crash_handler.cpp；`install()` CAS 检查，已装 → `false` + 告警（不覆盖，守互斥契约）。

### 3. CMake 接线

**FindWrapCrashDeps.cmake**（Task 2 全文骨架，模板 = `cmake/wrap/FindWrapTracy.cmake`）：
- 幂等：`if(TARGET CxxKitWrapCrashDeps::WrapBreakpad) return()`（Tracy 同款注释）
- 路径解析：`INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR`（plain set，非 CACHE）→ `CXXKIT_3RDPARTY_PACKAGES_DIR` → `${PROJECT_SOURCE_DIR}/3rdparty`
- 归档定位：`crash-deps-${CXXKIT_CRASH_TRIPLET}.7z`（triplet 映射表），缺失 → FATAL_ERROR 附 Task 1 导出命令
- stamp：`cxxkit_stamp_file_info` + 名称含输入目录 hash（防 INPUT 切换后陈旧 stamp）
- 解包：`cxxkit_fetch_3rdparty`（多顶层目录时按标记 glob 定位，不依赖 promote 行为）
- 门禁三连：`.version` 校验 → -I/-L 路径断言 → try_compile PIC 冒烟
- target：`CxxKitWrapCrashDeps::WrapBreakpad` → `find_package(unofficial-breakpad PATHS <root> NO_DEFAULT_PATH)` → `unofficial::breakpad::libbreakpad_client`；`CxxKitWrapCrashDeps::WrapBackward` → `find_package(Backward PATHS <root> NO_DEFAULT_PATH)` → `Backward::Interface`
- Windows：`FATAL_ERROR("crash-deps-x64-windows.7z 尚未导出…")`

**cxxkit/crash/CMakeLists.txt**（模板 = `cxxkit/network/CMakeLists.txt` + profiling 的 opt-in 段）：
```cmake
cxxkit_configure_library_begin(CxxKitCrash)
file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_crash ${_cxxkit_headers} crash_handler.cpp)   # 编译型
# include: BUILD_INTERFACE(源树+构建树) / INSTALL_INTERFACE(include)
target_compile_features(cxxkit_crash PUBLIC cxx_std_11)
if(CXXKIT_ENABLE_LIB_CRASH)
    cxxkit_find_package(CrashDeps PROVIDED_TARGETS CxxKitWrapCrashDeps::WrapBreakpad CxxKitWrapCrashDeps::WrapBackward)
    target_link_libraries(cxxkit_crash PUBLIC CxxKitWrapCrashDeps::WrapBreakpad CxxKitWrapCrashDeps::WrapBackward)
    set_property(TARGET cxxkit_crash PROPERTY INSTALL_RPATH "$ORIGIN")   # backward dlopen libdw.so.1 [B-H2]
    target_compile_definitions(cxxkit_crash PUBLIC CXXKIT_FEATURE_ENABLE_CRASH=1)
    cxxkit_generate_pkg_config(cxxkit_crash crash DESCRIPTION "cxxkit crash module (breakpad + backward-cpp backend)")
endif()
cxxkit_configure_library_end(CxxKitCrash)
install(TARGETS cxxkit_crash EXPORT cxxkitTargets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/crash/ DESTINATION include/cxxkit/crash FILES_MATCHING PATTERN "*.hpp")  # detail/ 随装（D9）
# 三方 payload 安装（[B-C1]，模板 = cpr/CURL 现成 D11 安装段）：
#   <unpack>/installed/<triplet>/lib/*.a → <prefix>/lib/
#   <unpack>/installed/<triplet>/share/unofficial-breakpad → <prefix>/lib/cmake/unofficial-breakpad/
#   <unpack>/installed/<triplet>/share/Backward → <prefix>/lib/cmake/Backward/
#   elfutils/libunwind 的 .so → <prefix>/lib/（linux）
# 注：公共头不暴露三方 API → 三方头不安装（消费方 pimpl 隔离）
```

**顶层 CMakeLists.txt**（修改点：option/subdir/安装块）
```cmake
cxxkit_option(CXXKIT_ENABLE_LIB_CRASH "Enable this to build crash lib" OFF)   # 无 DEPENDS network（v1 解耦）
# ...add_subdirectory 区：
cxxkit_add_subdirectory(cxxkit/crash CXXKIT_ENABLE_LIB_CRASH)
# 安装块（Tracy 同款，见现有 if(CXXKIT_ENABLE_LIB_TRACY) 段）：
if(CXXKIT_ENABLE_LIB_CRASH)
    list(APPEND CXXKIT_VENDORED_FIND_DEPS "unofficial-breakpad" "Backward")   # CxxKitConfig.cmake.in:12 find_dependency 循环
endif()
```

**cmake/CxxKitPkgConfigHelpers.cmake**（修改点：pkg-config 映射，Momus 观察 2——elseif 链实际在此文件，不在顶层 CMakeLists）
```cmake
# cxxkit_generate_pkg_config 的 elseif 链追加：
#   unofficial::breakpad::libbreakpad_client → -lbreakpad_client（+ pthread 进 Requires/Libs）
#   Backward::Interface（INTERFACE，无 lib）→ 无映射
```

### 4. 测试设计

```
tests/crash_fixture.cpp   独立可执行（非 gtest）：argv[1]=dump 路径；setDumpPath → install →
                          raise(SIGSEGV)  → 预期进程死亡 + .dmp 落盘
tests/tst_crash.cpp       gtest 父进程：
                          TEST(手动 minidump)  writeMinidump() → dumpFileList 非空 → clearDumps()
                          TEST(双 install 守卫)  install()==true → install()==false（互斥契约）
                          TEST(崩溃产 dmp)       spawn fixture → 轮询 .dmp（超时 10s）→ 非空
                          TEST(栈打印 opt-in)    setStackTraceOnCrash(true) → 崩溃 → stderr 含回溯
tests/CMakeLists.txt     if(CXXKIT_ENABLE_LIB_CRASH) 门控注册（build-reviewer M7）
ctest 注册：dump 路径用 WORKING_DIRECTORY 下可写子目录
```

### 5. 平台矩阵

| | Linux x64 | macOS x64 | Windows / arm64 |
|---|---|---|---|
| crash-deps .7z | ✅ 脚本产出（本计划 Task 1） | ✅ 复用父项目 breakpad-x64-osx.7z 同源导出 | ❌ → FATAL 提示导出命令 |
| minidump | ✅ | ✅ | 未支持（门禁拦截） |
| 栈打印 | ✅（dlopen libdw / 降级 addr2line） | ✅（原生 backtrace） | 未支持 |
| 符号化 | ✅ 复用父项目 breakpad-tools-linux-x86_64.7z | ⚠️ Linux dump_syms 跨界解析 Mach-O 或源码构建（发布检查单） | 未支持 |

### 6. 互斥契约与发布检查单（文档化，非代码）

- 同进程 crash ON ⇒ QExt::Breakpad OFF（迁移期二选一，禁「同开」过渡态）[R-C1]
- 发布检查单新增项：注入 SIGSEGV 验证 minidump **仅产出一份**；macOS 符号步骤显式定义 [R-H2]
- TPTouch 休眠 FindWrapBreakpad（breakpad-2023.01.27 FetchContent）禁复活 [R-M7]

---

## 文件结构

```
新增：
  scripts/export_crash_deps.sh                  # Task 1 — vcpkg 导出脚本
  scripts/crash-deps/vcpkg.json                 # Task 1 — 4 包版本锁定
  scripts/crash-deps/x64-linux-cxxkit.cmake     # Task 1 — overlay triplet（-fPIC）
  cmake/wrap/FindWrapCrashDeps.cmake            # Task 2 — 单解包双 target
  cxxkit/crash/crash_global.hpp                 # Task 3 — CXXKIT_CRASH_API
  cxxkit/crash/crash_handler.hpp                # Task 3 — 公共 API（#if 包裹）
  cxxkit/crash/crash_handler.cpp                # Task 3 — 实现（原子守卫/回调/backward 集成）
  cxxkit/crash/detail/crash_handler_p.hpp       # Task 3 — pimpl（三方类型唯一居所）
  cxxkit/crash/CMakeLists.txt                   # Task 3 — target + 安装/导出 + rpath
  tests/crash_fixture.cpp                       # Task 4 — 崩溃夹具
  tests/tst_crash.cpp                           # Task 4 — gtest 套件
修改：
  CMakeLists.txt                                # Task 3 — option/subdir/安装块/find_deps
  cmake/CxxKitPkgConfigHelpers.cmake            # Task 3 — pkg-config elseif 链（-lbreakpad_client + pthread）
  tests/CMakeLists.txt                          # Task 4 — 门控注册
  tests/CMakeLists.txt                          # Task 4 — 门控注册
  README.md / docs/README.md                    # Task 5 — 子库表 + 平台矩阵
  .agents/memorys/{status,decisions,conventions}.md  # Task 6 — D17 记录
不改：现有 14 子库、现有 FindWrap*、CxxKitConfig.cmake.in（模板变量已支持）、.cmake.conf
```

---

## 实施任务分解

### Task 0: 前置环境（本机无 vcpkg）

**Files:** 无

- [ ] **Step 1: 安装 vcpkg**（仅本机开发用，CxxKit 构建不依赖它）
```bash
git clone https://github.com/microsoft/vcpkg ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh
```
- [ ] **Step 2: 验证** `~/vcpkg/vcpkg version` 输出正常

### Task 1: export_crash_deps.sh（规范产出路径）

**Files:**
- Create: `scripts/export_crash_deps.sh`
- Create: `scripts/crash-deps/vcpkg.json`
- Create: `scripts/crash-deps/x64-linux-cxxkit.cmake`

**Interfaces:**
- Produces: `crash-deps-x64-linux.7z` + `crash-deps-x64-linux.7z.version`（供 Task 2 FindWrap 消费）

- [ ] **Step 1: 写 vcpkg.json**（4 包版本锁定，R-H3/M4）
```json
{
  "name": "cxxkit-crash-deps",
  "version-string": "2026-08-19",
  "dependencies": [
    { "name": "breakpad", "version>=": "2024-02-16" },
    { "name": "backward-cpp", "version>=": "2023-11-24" },
    { "name": "elfutils", "version>=": "0.195", "platform": "linux" },
    { "name": "libunwind", "version>=": "1.8.3", "platform": "linux" }
  ]
}
```
- [ ] **Step 2: 写 overlay triplet** `scripts/crash-deps/x64-linux-cxxkit.cmake`（PIC 硬性要求，R-Q2）
```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_C_FLAGS "${VCPKG_CMAKE_C_FLAGS} -fPIC")
set(VCPKG_CMAKE_CXX_FLAGS "${VCPKG_CMAKE_CXX_FLAGS} -fPIC")
```
- [ ] **Step 3: 写导出脚本**（核心逻辑：install → export → sidecar，失败即退出）
```bash
#!/usr/bin/env bash
set -euo pipefail
TRIPLET="${1:-x64-linux-cxxkit}"                 # overlay triplet（内部安装/导出用）
OUT_NAME="${2:-crash-deps-x64-linux}"           # 输出文件名用标准 triplet（与 FindWrap 映射表一致）
OUT_DIR="${3:-$(dirname "$0")/../3rdparty}"    # 默认 CxxKit/3rdparty
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"
"$VCPKG_ROOT/vcpkg" install --triplet "$TRIPLET" --x-manifest-root="$(dirname "$0")/crash-deps"
"$VCPKG_ROOT/vcpkg" export --raw --triplet "$TRIPLET" --x-manifest-root="$(dirname "$0")/crash-deps" \
    --output-dir="$OUT_DIR" breakpad backward-cpp elfutils libunwind
# 导出后把 export 产物改名/落位为 ${OUT_NAME}.7z（内部 overlay triplet 名不暴露给 FindWrap）
# sidecar：从 installed/vcpkg/info/*.list 提取四包版本写 ${OUT_NAME}.7z.version
# 断言 share/<pkg>/copyright 存在（license 随包，R-M5）
```
- [ ] **Step 4: 执行脚本产出归档**（本机）
```bash
bash scripts/export_crash_deps.sh
ls -la 3rdparty/crash-deps-x64-linux.7z*   # 产出 .7z + .version（标准 triplet 名）
```
- [ ] **Step 5: 验证归档结构**（`cmake -E tar tzf` 确认 `installed/` 树 + share/unofficial-breakpad + share/Backward 存在）
- [ ] **Step 6: Commit** `chore(crash): add vcpkg export script for crash deps`

### Task 2: FindWrapCrashDeps.cmake（单解包双 target）

**Files:**
- Create: `cmake/wrap/FindWrapCrashDeps.cmake`

**Interfaces:**
- Consumes: Task 1 的 `crash-deps-<triplet>.7z` + `.version`
- Produces: `CxxKitWrapCrashDeps::WrapBreakpad`、`CxxKitWrapCrashDeps::WrapBackward`（INTERFACE IMPORTED，cxxkit_find_package PROVIDED_TARGETS 契约）

- [ ] **Step 1: 写模块**（模板 = `cmake/wrap/FindWrapTracy.cmake` + QExt InstallVcpkg 导入逻辑）
  - 幂等 guard：`if(TARGET CxxKitWrapCrashDeps::WrapBreakpad) set(..._FOUND ON) return()`
  - triplet 映射：`CMAKE_SYSTEM_NAME` + `CMAKE_SYSTEM_PROCESSOR` → 文件名；未覆盖 → FATAL（含导出命令）
  - 路径解析：`CXXKIT_3RDPARTY_PACKAGES_DIR`（INPUT 注入优先，默认 `${PROJECT_SOURCE_DIR}/3rdparty`）
  - stamp：`cxxkit_stamp_file_info`（名称含输入目录路径 hash，防陈旧 stamp）
  - 解包：`cxxkit_fetch_3rdparty` → 按标记 glob 定位 installed root（`share/unofficial-breakpad/*Config.cmake` 反推）
  - 门禁三连：`.version` 四包版本比对（FATAL 附「重新运行 export_crash_deps.sh」）→ 解析出的 -I/-L 路径断言在解包树内 → `try_compile` PIE 链接 `libbreakpad_client.a` 冒烟
  - `find_package(unofficial-breakpad PATHS <root> NO_DEFAULT_PATH REQUIRED)` → `add_library(CxxKitWrapCrashDeps::WrapBreakpad INTERFACE IMPORTED)` → link `unofficial::breakpad::libbreakpad_client`
  - 同法：`find_package(Backward ...)` → `CxxKitWrapCrashDeps::WrapBackward` → `Backward::Interface`
  - Windows 分支：FATAL 提示导出命令
- [ ] **Step 2: 独立验证模块**（临时最小 CMake 工程链接两 target 编译通过）
- [ ] **Step 3: Commit** `feat(crash): add FindWrapCrashDeps wrap module`

### Task 3: cxxkit::crash 子库 + 顶层接线

**Files:**
- Create: `cxxkit/crash/crash_global.hpp`、`cxxkit/crash/crash_handler.hpp`、`cxxkit/crash/crash_handler.cpp`、`cxxkit/crash/detail/crash_handler_p.hpp`、`cxxkit/crash/CMakeLists.txt`
- Modify: `CMakeLists.txt`（option + `cxxkit_add_subdirectory(cxxkit/crash CXXKIT_ENABLE_LIB_CRASH)` + 安装块 VENDORED_FIND_DEPS 追加）；`cmake/CxxKitPkgConfigHelpers.cmake`（elseif 链追加 breakpad→-lbreakpad_client + pthread，Momus 观察 2）

**Interfaces:**
- Consumes: Task 2 两 target；`cxxkit::base`（宏/断言）；`cxxkit::tools`（CXXKIT_CHECK，仅库内）——**依赖方向 base→…→tools→crash，无 network**
- Produces: `cxxkit::crash` target + 公共 API（第 2 节草案，实现时以 singleton.hpp 实际宏为准微调）

- [ ] **Step 1: 写 crash_global.hpp**（network 同构三态导出宏，照抄 `cxxkit/network/network_global.hpp` 改名）
- [ ] **Step 2: 写 crash_handler.hpp**（第 2 节 API 草案；`#if CXXKIT_FEATURE_ENABLE_CRASH` 包裹；`CXXKIT_DECLARE_SINGLETON`；先读 `cxxkit/patterns/singleton.hpp:35-108` 确认宏配对）
- [ ] **Step 3: 写 detail/crash_handler_p.hpp**（唯一 include 三方头处；平台条件 include；持有 `unique_ptr<ExceptionHandler>`）
- [ ] **Step 4: 写 crash_handler.cpp**（实现：install 原子守卫 → 平台分支构造 ExceptionHandler（Linux MinidumpDescriptor / mac 路径参数，参考 QExt 三平台分支但去 Qt）；回调转发：默认快速返回 → opt-in 时 `backward::StackTrace::load_from(ucontext_t*)` + Printer 打 stderr → 用户回调叠加；writeMinidump/dumpFileList/clearDumps；install 后 set_* → `CXXKIT_CHECK(false)`）
- [ ] **Step 5: 写 crash/CMakeLists.txt**（第 3 节骨架 + `INSTALL_RPATH $ORIGIN` + 三方 payload 安装段照 cpr/CURL 先例）
- [ ] **Step 6: 改顶层 CMakeLists.txt + cmake/CxxKitPkgConfigHelpers.cmake**（option/subdir/安装块/find_deps；pkg-config elseif 链在 helper 文件而非顶层）
- [ ] **Step 7: 构建验证**
```bash
cmake -S . -B build -DCXXKIT_ENABLE_LIB_CRASH=ON   # configure 通过（含 FindWrap 门禁）
cmake --build build --parallel                      # 编译通过
```
- [ ] **Step 8: Commit** `feat(crash): add crash sublibrary (breakpad + backward-cpp)`

### Task 4: 测试

**Files:**
- Create: `tests/crash_fixture.cpp`、`tests/tst_crash.cpp`
- Modify: `tests/CMakeLists.txt`（`if(CXXKIT_ENABLE_LIB_CRASH)` 门控注册，build-reviewer M7）

**Interfaces:**
- Consumes: Task 3 的 `cxxkit::crash` 公共 API（fixture 用 install/setDumpPath；tst 用全部）

- [ ] **Step 1: 写 crash_fixture.cpp**（argv[1]=dump 路径 → setDumpPath → install → `raise(SIGSEGV)`）
- [ ] **Step 2: 写 tst_crash.cpp**（第 4 节四个用例：手动 minidump / 双 install 守卫 / 崩溃产 dmp（spawn + 轮询 10s 超时）/ 栈打印 opt-in 断言 stderr 含回溯）
- [ ] **Step 3: 改 tests/CMakeLists.txt** 门控注册（fixture 链接 cxxkit_crash；tst 链接 gtest + cxxkit_crash；ctest WORKING_DIRECTORY 可写 dump 目录）
- [ ] **Step 4: 运行验证**
```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure -R crash   # 4 用例全过
```
- [ ] **Step 5: 手动 E2E**（互斥契约验证：同一进程先 QExt 式装 handler 再 crash install → 第二次 install 返回 false）
- [ ] **Step 6: Commit** `test(crash): add crash sublibrary tests`

### Task 5: 文档

**Files:**
- Modify: `README.md`（子库表 + crash 行）、`docs/README.md`（索引）、`docs/superpowers/plans/`（本计划留档）

- [ ] **Step 1: 更新子库表**（`cxxkit::crash` | 编译 | crash handler、minidump（breakpad）、栈回溯（backward-cpp）| opt-in `CXXKIT_ENABLE_LIB_CRASH`）
- [ ] **Step 2: 写互斥契约文档**（docs/README 或 README 段：crash ON ⇒ QExt::Breakpad OFF；TPTouch 休眠 wrap 禁复活）
- [ ] **Step 3: 平台矩阵入 README**
- [ ] **Step 4: Commit** `docs(crash): document crash sublibrary`

### Task 6: 记忆更新 + 门禁

**Files:**
- Modify: `.agents/memorys/status.md`（第 15 子库 + 测试数）、`.agents/memorys/decisions.md`（新增 D17）、`.agents/memorys/conventions.md`（如需）

- [ ] **Step 1: decisions.md 记 D17**（crash 子库设计定案：QExt 式 .7z + 单 wrap 单解包 + v1 无上传 + 互斥契约 + fallback FATAL）
- [ ] **Step 2: status.md 更新**（子库表 + 待办划除 D16 遗留）
- [ ] **Step 3: 门禁验证**
```bash
bash scripts/check.sh                      # format + namespace + build + test 全绿
grep -rn "crash-deps" cmake/ CMakeLists.txt   # 无硬编码路径残留
grep -rn "breakpad-2023.01.27\|FindWrapBreakpad" cmake/  # TPTouch 休眠 wrap 未混入（应为空）
```
- [ ] **Step 4: Commit** `chore(crash): update project memory`

---

## 风险登记与缓解

| 风险 | 等级 | 缓解 |
|---|---|---|
| 运行期双 handler（与 QExt::Breakpad 同开） | CRITICAL | 互斥契约文档 + install() 原子守卫 + 发布检查单 E2E 项 |
| 归档跨机不可用（relocatability 未实证） | HIGH | FindWrap -I/-L 路径断言 + CI 异路径解包冒烟（Task 2 门禁） |
| 新 vcpkg 导出版本漂移（非 2024-02-16） | HIGH | vcpkg.json 版本锁定 + .version sidecar 门禁 |
| 静态库非 PIC 链接失败 | HIGH | overlay triplet -fPIC + try_compile 冒烟（配置期暴露） |
| backward dlopen libdw.so.1 运行时找不到 | MEDIUM | .so 随包分发 + INSTALL_RPATH $ORIGIN + addr2line 自动降级 |
| macOS 符号化链路缺失 | MEDIUM | 发布检查单：Linux dump_syms 跨界或源码构建 |
| 双 wrap 双解包回归 | MEDIUM | 单 FindWrapCrashDeps 模块（评审已定，代码评审时再查） |

## 回滚

- 全部为新增文件 + 顶层 CMakeLists 增量修改：`git rm` 新增文件 + `git checkout -- CMakeLists.txt tests/CMakeLists.txt README.md` 即还原
- 现有 14 子库零改动；`CXXKIT_ENABLE_LIB_CRASH` 默认 OFF，不影响默认构建
- 归档文件（3rdparty/crash-deps-*.7z）不在 git（3rdparty 只放源 tarball，.7z 由脚本产出到构建缓存或 INPUT 注入目录；脚本本身入库）
