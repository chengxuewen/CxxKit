# CxxKit

## An open cross-platform cpp toolkit.

CxxKit is a portable cross-platform C++ development toolkit. It contains many algorithms, data structures, functional components, scripting languages and practical frameworks, which can facilitate developers to quickly develop applications and avoid the dilemma of repeated wheel building.

Organized abseil-style: **directory = sublibrary = CMake target**, pick only what you need (boost-like on-demand philosophy at abseil granularity). Minimum C++11, flat `cxxkit::` namespace, all internal includes use `<cxxkit/...>` angle-bracket paths.

## Sublibraries

| Sublibrary | Type | Contents |
|---|---|---|
| `cxxkit::base` | header-only | macros, types, compiler detection, config |
| `cxxkit::containers` | header-only | vector, array_view, fixed_array, inlined_vector, flat_set, flat_hash_map, flat_hash_set, concurrent_queue, vector_map |
| `cxxkit::functional` | header-only | function_view, invocable, unique_function |
| `cxxkit::numerics` | compiled | bits, divide_round, numeric, safe_compare, safe_conversions, safe_minmax, running_statistics, sequence_number_util, sequence_number_unwrapper, percentile_filter, exp_filter, byte_order |
| `cxxkit::patterns` | header-only | singleton |
| `cxxkit::memory` | compiled | aligned_malloc, shared_memory, zero_memory, smart pointers |
| `cxxkit::units` | compiled | data_size, data_rate, frequency, time_delta, timestamp |
| `cxxkit::time` | compiled | date_time, elapsed_timer |
| `cxxkit::kernel` | compiled | object, event, event_loop, signals, application |
| `cxxkit::thread` | compiled | thread_pool, task_queue, event_loop_thread, future, semaphore |
| `cxxkit::text` | compiled | string, base64, bit_buffer, ascii, string_builder, string_utils, str_split, crc32, format, string_view |
| `cxxkit::tools` | compiled | logging, random, assert, clock, status, error, metrics, filesystem, optional, expected, variant |
| `cxxkit::network` | compiled | http (cpr backend) |
| `cxxkit::crash` | compiled | crash handler, minidump (breakpad), stack trace (backward-cpp) — opt-in `CXXKIT_ENABLE_LIB_CRASH` |
| `cxxkit::imgui` | compiled | headless ImGui context (`ImGuiHost`) over host-injected `PlatformBackend`/`RendererBackend` — opt-in `CXXKIT_ENABLE_LIB_IMGUI` |
| `cxxkit::uv` | compiled | event loop dispatcher (socket notifier: `register/unregister_socket_notifier` uv_poll backend) + TcpSocket/TcpServer (memcached-style state machines) over vendored libuv — opt-in `CXXKIT_ENABLE_LIB_UV` |
| `cxxkit::qt` | compiled | event-loop bridge onto a host Qt event loop (`QtEventDispatcher`) — opt-in `CXXKIT_ENABLE_LIB_QT` (CMake-only consumption, no .pc) |

## Quick start

### Build

```bash
cmake -S . -B build                 # configure (defaults: Debug, install to build/install)
cmake --build build --parallel      # build
cmake --build build --target BuildInstall   # build + install to build/install/
ctest --test-dir build              # run tests
```

Options: `-DCXXKIT_BUILD_TESTS=OFF`, `-DCXXKIT_ENABLE_LIB_NETWORK=ON`, `-DCXXKIT_ENABLE_LIB_CRASH=ON`, `-DCXXKIT_BUILD_DOCS=ON`, `-DCMAKE_INSTALL_PREFIX=/path/to/prefix`.
Sanitizer / coverage（`scripts/check.sh` 步骤 4/7 与 7/7 会自动接入，若对应 build 目录存在）:
`-DCXXKIT_BUILD_SANITIZERS=ON`（用 -B build-asan 生成）、`-DCXXKIT_BUILD_COVERAGE=ON`（用 -B build-cov 生成，`coverage` target 产出 `build-cov/coverage/summary.txt`）。

**On-demand sublibraries**: each core sublibrary has a `-DCXXKIT_ENABLE_LIB_<SUB>=ON/OFF` switch (text, containers, functional, numerics, patterns, units, memory, kernel, thread, media) plus the opt-in gates network/crash/imgui/uv/qt/tracy. `base`, `tools`, `time` and `profiling` are always built (dependency-graph root / symbol hub / time↔tools cycle). Dependencies are clamped, not auto-enabled: requesting a sublibrary whose dependency is OFF emits a WARNING and stays OFF.

Minimal build (header-only core only):

```bash
cmake -S . -B build \
  -DCXXKIT_ENABLE_LIB_TEXT=OFF -DCXXKIT_ENABLE_LIB_CONTAINERS=ON -DCXXKIT_ENABLE_LIB_FUNCTIONAL=ON \
  -DCXXKIT_ENABLE_LIB_NUMERICS=ON -DCXXKIT_ENABLE_LIB_PATTERNS=ON -DCXXKIT_ENABLE_LIB_MEMORY=OFF \
  -DCXXKIT_ENABLE_LIB_KERNEL=OFF -DCXXKIT_ENABLE_LIB_THREAD=OFF -DCXXKIT_ENABLE_LIB_MEDIA=OFF
cmake --build build --parallel
```

### crash sublibrary (breakpad + backward-cpp)

```cpp
#include <cxxkit/crash/crash_handler.hpp>

cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();
handler.setDumpPath("/tmp/dumps");
handler.setStackTraceOnCrash(true);   // optional: print the crashed thread's stack to stderr
handler.install();
```

**Platform support**: Linux/macOS x64 (Windows/arm64 need a `crash-deps-<triplet>.7z` export first).

**Dependency cache**: the first `-DCXXKIT_ENABLE_LIB_CRASH=ON` configure auto-fetches vcpkg (if the
`crash-deps-x64-linux.7z` cache is missing) and builds breakpad/backward-cpp/elfutils/libunwind
(10-25 min, one time). Later builds consume the .7z cache — no vcpkg needed at build time.
Set `INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR` to share the parent project's cache directory.

**Coexistence contract**: crash ON ⇒ QExt::Breakpad OFF in the same process — two breakpad handlers
would double-write minidumps. `CrashHandler::install()` refuses a second installation.

**Symbolication** (Linux): `dump_syms <binary> > <module>.sym`, then
`minidump_stackwalk <dump>.dmp symbols/`.

### Use with CMake (installed)

```cmake
find_package(cxxkit REQUIRED COMPONENTS base text tools)
target_link_libraries(app cxxkit::text cxxkit::tools)
```

### Use with pkg-config

```bash
pkg-config --cflags --libs cxxkit-text
pkg-config --cflags --libs cxxkit-network   # pulls fmt/spdlog/libcurl/mbedtls chain
```

### Use vendored 3rdparty headers

Third-party headers are namespaced to avoid conflicts with system-installed libraries:

```cpp
#include <cxxkit/3rdparty/fmt/format.h>
#include <cxxkit/3rdparty/tl/optional.hpp>
```

## Documentation

```bash
cmake -S . -B build -DCXXKIT_BUILD_DOCS=ON
cmake --build build --target Docs   # outputs build/doc/html/
```

See the file `docs/README.md` for the full documentation index.
## Examples

`examples/` ships one runnable walkthrough per sublibrary (20 total, `imgui` opt-in).
Build them with the main build (`cmake --build build`); binaries land in `build/examples/`.

| Example | Sublibrary | Highlights |
|---|---|---|
| `exp_base` | base | version/compiler-feature report, pimpl (`CXXKIT_DEFINE_DPTR`), `DISABLE_COPY_MOVE` |
| `exp_containers` | containers | flat_hash_map/set, ArrayView, InlinedVector, FixedArray |
| `exp_functional` | functional | FunctionView (non-owning), UniqueFunction (move-only) |
| `exp_kernel` | kernel | signals-only walkthrough; Object/EventLoop await kernel completion |
| `exp_media` | media | FrameGenerator pattern -> I420Buffer rotate/scale -> PSNR |
| `exp_memory` | memory | SharedRefPtr intrusive counting, aligned_malloc, zero_memory |
| `exp_numerics` | numerics | RunningStatistics, ExpFilter, sequence unwrapping |
| `exp_patterns` | patterns | Singleton / AutoSingleton lifecycle |
| `exp_profiling` | profiling | CXXKIT_PROFILE_* zones (Tracy GUI when `CXXKIT_ENABLE_LIB_TRACY=ON`) |
| `exp_text` | text | StringBuilder, str_split, base64, crc32 |
| `exp_thread` | thread | ThreadPool, TaskQueueThread FIFO, Barrier |
| `exp_time` | time | ElapsedTimer, DateTime |
| `exp_logging` | tools | level macros, custom loggers, runtime filtering |
| `exp_units` | units | DataSize/DataRate/TimeDelta/Frequency typed arithmetic |
| `exp_crash` | crash | safe path: config, manual minidump (needs `CXXKIT_ENABLE_LIB_CRASH=ON`) |
| `exp_network_version` | network | HTTP failure-path demo (needs `CXXKIT_ENABLE_LIB_NETWORK=ON`) |
| `exp_imgui` | imgui | SDL3+GL3 windowed core; plus `examples/imgui/` family: headless, plot, plot3d, gizmo, file_dialog, markdown, nodes |
| `exp_event_loop` | uv | deterministic 3-tick timer loop, rc=0 (needs `CXXKIT_ENABLE_LIB_UV=ON`) |
| `exp_tcp_echo` | uv | TcpServer+TcpSocket loopback echo over an ephemeral port, fixed stdout, rc=0 (needs `CXXKIT_ENABLE_LIB_UV=ON`) |
| `exp_qt_embed` | qt | cxxkit EventLoop embedded in a host Qt loop: bridge QTimer pump, queued signal delivery, rc=0 (needs `CXXKIT_ENABLE_LIB_QT=ON`; runtime gate `docs/qt-embed-smoke.md`) |

All examples print deterministic output except where a value is genuinely runtime-dependent (timers, clocks — annotated in-line). No example performs a real network request or a deliberate crash.

## License

The self-owned code of this project is licensed under the permissive MIT License and can be freely applied to commercial and non-commercial projects while retaining copyright information.
However, this project also uses some scattered open source code, please replace or remove it for commercial use.
Any commercial disputes or infringement caused by using this project have nothing to do with the project and developers and shall be at your own legal risk.
When using the code of this project, the license agreement should also indicate the license of the third-party libraries that this project depends on.
