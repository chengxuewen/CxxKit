# CxxKit 踩坑记录

> 问题 + 原因 + 解法 + 验证 + 禁止。格式：`## PIT-{n}: 标题 (日期)`。


## PIT-1: 三方头路径不得擅自简化 (2026-08-18)
- **症状**: 实现 Phase 1 时把三方 include 简化为原始路径（`fmt/...`、`tl/...`），与设计文档 §5.4 不一致
- **根因**: 实现时未对照设计文档验证三方头命名空间路径
- **解法**: 恢复为 `<cxxkit/3rdparty/<lib>/...>`（D6 决策），构建/安装双侧一致
- **验证**: `grep -rn '#include <fmt/' cxxkit/` 应为空；`grep -rn '#include <cxxkit/3rdparty/' cxxkit/` 应有结果
- **禁止**: 设计文档明确写过的机制，实现时不得擅自简化（AGENTS.md ANTI-PATTERNS 已引用）

## PIT-2: INTERFACE 库 target_sources 不能加源目录内头 (2026-08-19)
- **症状**: `target_sources(cxxkit_base INTERFACE $<BUILD_INTERFACE:${_cxxkit_headers}>)` 报 "INTERFACE_SOURCES property contains path ... prefixed in the source directory"；绝对路径版报 "Cannot find source file"
- **根因**: CMake 禁止 INTERFACE_SOURCES 引用源目录内路径（该属性面向安装消费者）
- **解法**: header-only 库用 `add_library(xxx INTERFACE ${headers})` 直接带源参数（仅供 IDE 显示，不编译）
- **验证**: `cmake -S . -B build` 通过；`build/.cmake/api/v1/reply/codemodel-v2-*.json` 对应 target jsonFile 的 sources 含 .hpp

## PIT-3: install(DIRECTORY) EXCLUDE 不匹配顶层子目录 (2026-08-19)
- **症状**: `PATTERN "*/detail/*" EXCLUDE` 后 detail/*.hpp 仍被安装
- **根因**: pattern 转 REGEX 为 `/[^/]*\/detail\/[^/]*$`，要求路径以 /detail/ 开头；但 install 相对路径是 `detail/xxx`（无前导斜杠）
- **解法**: 顶层 detail 用 `PATTERN "detail" EXCLUDE`（连目录一起排除）
- **⚠ 2026-08-19 已反转**: 用户决策 detail/ 随公共头安装（D9），EXCLUDE 已移除；PIT-3 保留作 pattern 匹配机制参考
- **验证**: `find build/install/include -type d -name detail` 应为 4（kernel/thread/tools/network）
- **验证**: `find build/install/include -type d -name detail` 应为空

## PIT-4: file(GLOB) 相对路径在 target 源列表被拒 (2026-08-19)
- **症状**: GLOB 相对路径（`*.hpp`）用于 INTERFACE_SOURCES 报 prefixed；用于 add_library 报找不到文件
- **根因**: file(GLOB) 相对当前目录展开，CMake 要求 target 源路径可解析
- **解法**: GLOB 必须 `${CMAKE_CURRENT_SOURCE_DIR}` 前缀（返回绝对路径），加 CONFIGURE_DEPENDS 保证新头自动进大纲
- **验证**: configure 通过 + codemodel 含头

## PIT-5: text ↔ tools 循环依赖（底层子库误用上层编译符号）(2026-08-19)
- **症状**: 链接报 `undefined reference to cxxkit_assert`（text 的 .cpp 用 CXXKIT_ASSERT 宏，符号在 libcxxkit_tools.a）
- **根因**: tools 依赖 text（公共头 include），text 又用 tools 的编译符号 → 依赖环；AGENTS.md 依赖方向 text 在 tools 之下
- **解法**: 底层子库廉价参数校验用 `<cassert>` 的 assert()（tools/AGENTS.md 断言选型本就推荐）；memory 保留 fatal 语义（CXXKIT_ASSERT_X）并显式链接 tools（无环：tools 不依赖 memory）
- **验证**: `cmake --build build` 链接通过；`grep -c CXXKIT_ASSERT cxxkit/text/*.cpp` 应为 0

## PIT-6: 违反 C6 执行 rm -rf build (2026-08-19)
- **症状**: 为"干净验证"执行 `rm -rf build`，用户立即中止——3rdparty stamp 缓存全清导致 5+ 分钟三方库重建
- **根因**: AI 把"验证干净配置"优先于项目硬禁令 C6；rm -rf build 无豁免场景
- **解法**: 只清 CMake 生成物 `rm -rf build/CMakeCache.txt build/CMakeFiles build/Testing`；三方库需重建时才删对应 `build/3rdparty/<lib>-<buildtype>`
- **验证**: `ls build/3rdparty/ | wc -l` 应为 21（stamp 缓存未清）
- **禁止**: 任何"图省事/求干净"的 rm -rf build（C6 硬禁令，用户已明确纠正）

## PIT-7: cxxkit_evaluate_expression 不解引用 → OR_CONDITION 恒真 (2026-08-19)
- **症状**: 未传 -D 的 CXXKIT_ENABLE_LIB_TRACY 强制 ON；排查发现所有 OR_CONDITION 选项恒真（network/tests 因显式传值或默认 ON 被掩盖）
- **根因**: `cxxkit_evaluate_expression(or_condition ${arg_OR_CONDITION})` 把变量名当字符串，`if(expression)` 对非空字符串（"CXXKIT_BUILD_ALL"）恒真——不解引用
- **解法**: `if(expression)` → `if(${expression})`（解引用变量名，CXXKIT_BUILD_ALL=OFF 时正确求假）
- **验证**: `cmake -S . -B build` 后 `grep ENABLE_LIB_TRACY build/CMakeCache.txt` 应为 OFF（未传 -D 时）

## PIT-8: TracyClient 需 C++17 编译（C++11 copy-init + atomic deleted）(2026-08-19)
- **症状**: GCC 10.5 C++11 编译 TracyClient 报 "use of deleted function std::atomic copy ctor"（TracyProfiler.cpp:936 `static std::atomic<Thread*> s_sysTraceThread = nullptr;`）
- **根因**: C++11 的 copy-initialization 要求拷贝构造可访问（atomic 拷贝构造 deleted）；C++17 guaranteed copy elision 后合法。Tracy 官方 cxx_std_11 声明与代码不符（Godot 等使用者全是 C++17+ 未踩到）
- **解法**: FindWrapTracy 加 `-DCMAKE_CXX_STANDARD=17`（vendored 三方不受 C4 C++11 约束；Tracy 头文件保持 C++11 兼容有官方背书）
- **验证**: wrap 构建通过；消费方 C++11 include Tracy.hpp 编译通过（exp_profiling）

## PIT-9: Tracy v0.13 宏语义变化 ZoneScopedS→ZoneScopedN (2026-08-19)
- **症状**: CXXKIT_PROFILE_SCOPE 展开报 "invalid conversion from const char* to int32_t"（name 传入 ScopedZone 的 depth 参数）
- **根因**: v0.13 起 ZoneScopedS 是 static-zone 变体（3 参 ZoneNamedS 语义），带名 zone 改用 ZoneScopedN(name)
- **解法**: 包装宏用 ZoneScopedN
- **验证**: 预处理展开确认 `srcloc { name, ... }`；编译+链接通过

## PIT-10: vcpkg manifest `version<` 非法字段 + 强制需要 builtin-baseline (2026-08-19)
- **症状**: `vcpkg install` 报 "unexpected field 'version<', did you mean 'version>='?"；去掉后报 "uses version>= and does not have a builtin-baseline"
- **根因**: vcpkg manifest 依赖对象只支持 `version>=`（下限）；版本上界/精确锁定靠顶层 `builtin-baseline`（vcpkg 仓库 commit）约束
- **解法**: 依赖用 `version>=` + 顶层 `"builtin-baseline": "<commit>"`（取 vcpkg HEAD，验证该 commit 的 breakpad=2024-02-16#1 与 QExt 同源）
- **验证**: vcpkg install 通过并解析到目标版本

## PIT-11: 浅克隆 vcpkg 无法 checkout builtin-baseline 版本树 (2026-08-19)
- **症状**: `read-tree <sha> failed: failed to unpack tree object` + "vcpkg was cloned as a shallow repository. Try again with a full vcpkg clone"
- **根因**: `git clone --depth 1`（QExt 原版）无 port 历史，builtin-baseline 的版本 checkout 需要完整树
- **解法**: 移植版改全量 clone（去掉 --depth 1）；已浅克隆的可 `git fetch --unshallow` 补救
- **验证**: `git cat-file -t <版本tree sha>` 命中

## PIT-12: vcpkg custom triplet 未声明系统名按 Windows 处理 (2026-08-19)
- **症状**: `error: in triplet x64-linux-cxxkit: Use of Visual Studio's Developer Prompt is unsupported on non-Windows hosts`
- **根因**: triolet 名 `<arch>-<os>` 的 os 部分未知（`linux-cxxkit`）→ vcpkg 默认按 Windows 主机处理
- **解法**: overlay triplet 内 `set(VCPKG_CMAKE_SYSTEM_NAME Linux)`
- **验证**: vcpkg install 通过

## PIT-13: `.gitignore` 的 `vcpkg*` 过宽误吞源文件 (2026-08-19)
- **症状**: `git add scripts/crash-deps/vcpkg.json` 被拒（"根据一个 .gitignore 文件而被忽略"）
- **根因**: `vcpkg*` 匹配任意以 vcpkg 开头的路径段，含 `scripts/crash-deps/vcpkg.json`
- **解法**: 锚定 `/vcpkg/` `/vcpkg-tools/`（仅忽略仓库根的自举克隆目录）
- **验证**: `git ls-files scripts/crash-deps/` 命中 vcpkg.json

## PIT-14: `Singleton<T,true>` 模板从未被实例化——三处编译错误 (2026-08-19)
- **症状**: 用 `Singleton<CrashHandler,true>` 编译 crash_handler.cpp：①`std::atomic<T*>::mInstance = nullptr` 报 deleted copy ctor（C++11/14，PIT-8 同类）；②`CXXKIT_ASSERT` 未声明（模板用它却只依赖用户先 include tools/checks.hpp）；③`virtual ~T()` private 与内部 `unique_ptr<T>` 的 default_delete 冲突
- **根因**: patterns/singleton.hpp 的 ManualLifetime 分支设计缺陷，此前零使用者（grep CXXKIT_DECLARE_SINGLETON 无命中）——我成了第一个踩坑者
- **解法**: 不修复模板（避免动现有子库），崩溃代码改用 C++11 函数局部静态 + public 析构（线程安全、进程生命周期）；**模板缺陷记待办**
- **验证**: 编译链接通过，单例句柄工作正常

## PIT-15: 多行文本注入打断代码括号配平（edit 工具） (2026-08-19)
- **症状**: tst_crash.cpp 注入 `#if defined(TEST_ELFUTILS_LIB_DIR) ... #endif` 后报 "expected '}' at end of input"，且出现重复的 `if (pid == 0) {` 残留
- **根因**: 注入行含换行符，且锚点选取在结构中间，破坏了大括号配平（edit-safety 警告的经典场景）
- **解法**: 修括号配平 + 清理重复行；**注入含换行/条件编译块时用整块范围替换，不插入行内**；edit 后立即编译验证
- **验证**: 重新编译 + 测试通过

## PIT-16: vcpkg 打包默认值偏离 QExt 语义——源码树误产 .7z (2026-08-19)
- **症状**: configure crash=ON 后 `3rdparty/breakpad-x64-linux.7z`、`backward-cpp-x64-linux.7z` 出现在源码树（未跟踪垃圾 + 与"vcpkg 产物不入库"初衷冲突）
- **根因**: 移植时给 `CXXKIT_3RDPARTY_PACKAGES_DIR` 设了默认 `${PROJECT_SOURCE_DIR}/3rdparty`——QExt 是 `INPUT_` 未注入时保持空，pack 步骤 `if(NOT "X$dir" STREQUAL "X")` 直接跳过；"改进默认值"破坏了 QExt 契约
- **解法**: 顶层仅当 `DEFINED INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR` 才设置该 CACHE（否则空）；未传入 → 打包跳过（.7z 只留 build 树 output 目录）；已误产的 .7z 删除
- **验证**: `git status` 无 3rdparty/*.7z 噪音；`cmake -D CXXKIT_ENABLE_LIB_CRASH=ON` 后 3rdparty/ 无新 .7z

## PIT-17: 无路径 `git commit` 卷入整个 staging 区——62 文件误提交 (2026-08-19)
- **症状**: `git commit`（无路径参数）提交了 62 个文件——含 worktree 中他人/用户未提交的人工修正（CMake 大小写重构、Helpers 全面改动），commit message 却只描述 memorys 更新
- **根因**: 习惯性无路径 commit + 未在 commit 前 `git status` 核对；co-editing 场景（用户并行手工改文件）下 index 里有非预期改动
- **解法**: 每次 commit 前 `git status --short` + `git diff --cached --stat` 核对；只 `git add` 明确路径；commit 带路径参数或确认 staging 区只含预期文件
- **验证**: `git show --stat <commit>` 文件列表 == 预期；本会话 88acec1 起恢复限定提交

## PIT-18: 用户并行人工修正与 AI 提交竞态——修正被无路径 commit 吞并 (2026-08-19)
- **症状**: 用户手工把 `CXXKIT_3RDPARTY_PACKAGES_DIR` 默认值改为空（QExt 语义）+ 调整 .gitignore，但这些改动未及时提交，直到一次无路径 commit 才被卷入（连同 62 文件一起）
- **根因**: 多写者工作树（AI + 用户 IDE）下，AI 的 commit 无法感知用户刚做的未提交修改；限路径提交虽安全但漏掉用户的修正
- **解法**: 提交前 `git status` 发现异常 M/?? 文件 → 暂停并询问用户归属；用户人工修正如确认主动提交（本会话 88acec1）
- **验证**: 工作树干净、所有人工修正已入库且语义正确（configure 验证 QExt 不打包行为）

## PIT-19: 变量模板 `_v` 在 C++11 下必报且不可抑制（GCC）(2026-08-20)
- **症状**: `constexpr bool is_void_v = ...` 等变量模板在 C++11 编译报 "variable templates only available with -std=c++14"，且 -Werror 下变硬错误
- **根因**: 变量模板是 C++14 特性；GCC 无论 -Wno-c++14-extensions 还是 `#pragma GCC diagnostic ignored` 都无法抑制（该诊断无公开开关，pragma 报 unknown option）
- **解法**: 变量模板物理无法 C++11 化 → 全部 `#if CXXKIT_CC_CPP14_OR_GREATER` 保护（C++11 不定义）+ 提供 C++11 结构体 `::value`（`template<T> struct is_X : std::is_X<T>{}`）+ 调用点 `_v<X>` 改 `<X>::value`
- **验证**: 全库 c++11 编译 0 error 0 variable-template warning；ctest 33/33
- **教训**: `_v` 简洁语法糖会掩盖 C++14 依赖；真 C++11 需彻底结构体化，type_traits/signals 大改

## PIT-20: 正则/脚本替换破坏嵌套模板结构（signals.hpp 大改教训）(2026-08-20)
- **症状**: 用 `std::xxx_t<[^<>]*>` 全局正则替换 C++14 别名，把含嵌套/多参的 `std::enable_if_t<cond, T>` 拆错、丢失 `::type`，制造语法破坏
- **根因**: 正则无法处理嵌套尖括号与多模板参数（如 `enable_if_t<is_callable_v<A,B>, Connection>`）；首次草率替换后 signals.hpp 无法编译
- **解法**: 用**括号匹配器**（`find_matching` 扫描 `<`/`>` 深度对齐）逐段替换结构性模板；替换后立即编译验证；宁慢勿破
- **验证**: signals.hpp C++11 编译通过
- **教训**: 大面积模板替换禁全局正则；用深度计数匹配器 + 每步编译

## PIT-21: C++11 局部类不能有成员模板（函数内 struct + template）(2026-08-20)
- **症状**: 泛型 lambda 在 TEST 内改 C++11 等价时，`struct VariadicFunc { template<typename...Args> void operator()(...); }` 定义在 TEST 函数内 → "invalid declaration of member template in local class"
- **根因**: C++11 标准：局部类（函数内定义）不能有成员模板
- **解法**: 把含成员模板的类移到**文件作用域**（include 后、TEST 前）
- **验证**: 移出后 C++11 编译通过

## PIT-22: base 强制 cxx_std_14 掩盖全库 C++14 依赖（真 C++11 化连锁）(2026-08-20)
- **症状**: 移除 base 的 `target_compile_features(cxxkit_base INTERFACE cxx_std_14)` 后，string_view/units/data_rate/bit_buffer/type_traits/signals 陆续暴露 C++14 编译错误，逐层连锁
- **根因**: base 的 cxx_std_14 通过 INTERFACE 传播给所有子库（全部链接 cxxkit::base），把全库标准抬到 C++14，掩盖了分散各处的 C++14 语法（constexpr 运算、变量模板、数字分隔符、泛型 lambda）
- **解法**: 要么全库真 C++11（逐处改），要么诚实承认 C++14 基线；本会话选前者（用户坚持 C++11）
- **验证**: 全库 c++11 编译 0 error 0 variable-template warning
- **教训**: 公共 INTERFACE 上的标准基线是"隐藏编译器"——会掩盖实际代码标准；C++11 严格性需用真实 `-std=c++11` 编译验证而非靠传播

## PIT-23: DateTime steady↔system 转换 epoch 基准不匹配（round-trip 漂移 ~1e18ns）(2026-08-20)
- **症状**: `systemTimeFromSteadyNSecs(steadyTimeFromSystemNSecs(nowSys))` round-trip 差 ~3.6e18 ns（steady 与 system clock epoch 基准不同）
- **根因**: steady_clock 的 time_point(nanoseconds) 把纳秒当相对 steady epoch，与 system 的 Unix-epoch 映射错位（date_time.cpp:71-113）
- **解法**: 已修（commit 2026-08-20）：nsecs 必须在 system_clock 时间线上解读（systemNow - systemTimePoint 得 offset，再 steadyNow - offset）；之前错误地把 system 纳秒塞进 steady_clock::time_point（epoch 基准不同）
- **验证**: tst_datetime SystemSteadyConversions 恢复精确断言（round-trip <2s）；40/40 全过

## PIT-24: text 子库两处内存缺陷（String buffer 未初始化 + ascii_string_* 泄漏）(2026-08-20)
- **症状**: a) 空 String 的 c_str()[0] 返回垃圾值（\xBE 而非 \0）；b) ascii_string_tolower/toupper 每次调用泄漏 strndup 指针（LSAN 20B/4 alloc）
- **根因**: a) StringPrivate::mBuffer[48] 无 NSDMI 初始化；b) 封装把 strlwr/strupr（strndup 新分配）结果直接赋给 std::string，op=/隐式构造只拷贝不释放
- **解法**: a) `char mBuffer[kBufferSize]{};`；b) 4 个封装改 in-place/本地转换（零分配，同时避开 strlwr NUL 截断坑）
- **验证**: 三形态全绿 40/40；ASAN/LSAN 零诊断；tst_ascii/tst_string 单测 15/10 全过

## PIT-25: 并行写作代理破坏公共头 + 提交前未编译验证（2026-08-20）
- **症状**: 9 个并行 doxygen 代理全灭（RPM 限流/stale timeout），留下损坏编辑——吞函数签名（ascii.hpp 4 个 ascii_string_* 签名行消失）、游离注释块（status.hpp @endcode 逃逸为代码）、删宏定义（source_location.hpp）、改 target 名（CMakeLists Docs→BuildDocs）、移动代码（utility.hpp）。且我 3472885 提交了损坏 ascii.hpp（当时只验了 ctest 旧 binary 没验编译）
- **根因**: a) 并行写作代理各自独立 edit 无协调，撞 API 限流后部分写入残留；b) `git add -A cxxkit/` 把未验证文件全 staged；c) 提交前用 ctest（旧 binary）冒充编译验证
- **解法**: 注释-only 改动**必须逐个文件 diff 验证纯注释**（脚本 grep 非注释行）+ **提交前全量编译 + ctest + doxygen 重跑**三重验证；代理产出默认不信任，逐文件 checkout 审查；文档编辑禁用并行代理（串行 + 每批验证）
- **验证**: 1df894a 提交前：全量 build 0 error + 40/40 ctest + doxygen 致命 warning 清零 + 28 文件逐个 diff 纯注释审核
- **禁止**: 注释批量提交前跳过编译验证；`git add -A` 对含代理改动的树直接全量 stage

## PIT-26: TaskQueueThread 空队列睡 1s + notify 无法缩短 deadline（延迟任务迟到 1s）(2026-08-24)
- **症状**: TaskQueueThreadTest.PostDelayedTask 隔离跑 15/15 必失败（1s cv 超时），全量偶发；3ms delayed 任务在 ~1s 后才执行
- **根因**: 空队列时 popNextTask sleepTime=TimeDelta 默认值 PlusInfinity → worker 睡满 wait 上限 1s；期间 postDelayedTask 的 notify 到达但**谓词 wait 不缩短 deadline**（delayed 未到期谓词 false 继续睡旧 deadline）→ 任务迟到 1s
- **解法**: 空队列/大 sleepTime（us>1000）改 1ms 短轮询；delayed 精确 sleepTime 保持精确等待（task_queue_thread.cpp popNextTask）
- **验证**: 隔离 15/15 绿（原 15/15 红）；全量 40/40 + shared/ASAN 绿；未用 IsZero（默认是 PlusInfinity 非 Zero）
- **禁止**: CV wait 用固定长 deadline 而期望 notify 缩短它——谓词版 wait_until 不会改 deadline
