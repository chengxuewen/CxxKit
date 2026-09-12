# Network Backend Abstraction + Qt Alignment Implementation Plan (D41)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Zero uv leakage in cxxkit/network public API + Qt-grade error/state surface + a detail-layer backend abstraction with compile-time backend selection (uv now, standalone asio second).

**Architecture:** Plan A (user ruling) — a `StreamBackend` interface inside `cxxkit/network/detail/`; current uv-driven code moves verbatim into `stream_backend_uv`; asio (standalone, header-only) becomes `stream_backend_asio` behind the same contract. Backend selection is a CMake-time option (`CXXKIT_NETWORK_BACKEND` = `uv`|`asio`), wired per PIT-48 (`target_compile_definitions`). Public classes `TcpSocket`/`TcpServer` keep their callback signatures; the state machine stays in the pimpl (backends do I/O only). The state machine stays in the pimpl (backends do I/O only). The only breaking public change: `adopt_uv_tcp(uv_tcp_s*)` → `adopt_native(void*)`.

**Tech Stack:** C++11 (lib code), CMake `cxxkit_option`, libuv 1.49.2 (vendored), standalone asio 1.32.0 (NEW vendored, Zlib license, header-only, extract-only wrap per ImGui precedent), ctest.

**Spec:** This file is self-contained; design inputs: gap-analysis report (this session) + user rulings (Plan A; hide uv; macro/CMake-controlled backend).

## Global Constraints

- C18: commit messages + code comments English only; plan doc + AI dialogue Chinese.
- C4: lib code C++11 (no auto return / generic lambda / if constexpr / `_t` aliases in `cxxkit/`); tests follow D25 gates.
- C6: never `rm -rf build`; only `build/CMakeCache.txt build/CMakeFiles build/Testing`.
- C9/D11: new dependency = vendored tarball + FindWrap module; FetchContent forbidden.
- D8: internal includes angle-bracket `<cxxkit/...>`; **public headers contain zero uv/asio types** (core acceptance).
- D26 naming: functions snake_case; members mPascalCase; enum values kPascalCase; types PascalCase.
- Box license banner on every new file (copy from tcp_socket.hpp lines 1-13, adjust library line).
- Commit identity: `git -c user.name=chengxuewen -c user.email=1398831004@qq.com`.
- Per-task verification: `cmake --build build --parallel` → 0 errors; `ctest --test-dir build -R tcp` → all green; `pixi run clang-format -i <touched files>`; final task runs FULL `ctest --test-dir build` (85 suites) + ASAN sweep `LSAN_OPTIONS="suppressions=$(pwd)/scripts/lsan.supp" ctest --test-dir build-asan -R tcp`.
- Tests guard today: `if(CXXKIT_ENABLE_LIB_NETWORK AND CXXKIT_ENABLE_LOOP_BACKEND_UV)` in tests/CMakeLists.txt. Task 5 relaxes TCP suites to `if(CXXKIT_ENABLE_LIB_NETWORK)` (backend-agnostic).
- PIT-48: CMake option → preprocessor wiring must go through `target_compile_definitions`, never bare `#ifdef` on option names.
- PIT-41: `cxxkit_option` evaluates DEPENDS eagerly at declaration — declare `CXXKIT_NETWORK_BACKEND` before any consumer.

---

### Task 1: IPv6 dual-stack fix (bug fix, no abstraction yet)

**Files:**
- Create: `cxxkit/network/detail/address_helper.hpp` (header-only shared helper)
- Modify: `cxxkit/network/tcp_socket.cpp:~144` (connect address parse)
- Modify: `cxxkit/network/tcp_server.cpp:~97` (listen parse) + `:139` (bound_port AF_INET6 branch)
- Test: `tests/tst_tcp_socket.cpp`, `tests/tst_tcp_server.cpp` (+2 cases)

**Interfaces:**
- Produces: `namespace cxxkit::network::detail { inline bool fill_sockaddr(const std::string &ip, uint16_t port, sockaddr_storage *out); }` — IPv4/IPv6 auto-detect; false when neither parses. Shared by both backends forever.

- [ ] **Step 1.1 (failing tests)**: In `tests/tst_tcp_socket.cpp` mirror `ConnectEchoRoundTrip` exactly, but server+client use `"::1"`; name it `ConnectIpv6LoopbackRoundTrip`. Guard both new tests with a runtime probe: create a test `AF_INET6` socket; `GTEST_SKIP() << "ipv6 loopback unavailable"` when socket() fails (IPv6-disabled hosts, Momus minor note). In `tests/tst_tcp_server.cpp` add `ListenIpv6EphemeralPort`: `TcpServer s(loop); EXPECT_TRUE(s.listen("::1", 0)); EXPECT_NE(0, s.bound_port());` (+ same pump discipline as `ListenAcceptEcho`).

- [ ] **Step 1.2 (verify RED — Momus wording fix)**: `cmake --build build --target cxxkit_tst_tcp_socket cxxkit_tst_tcp_server && ctest --test-dir build -R "tcp" --output-on-failure`. EXPECT: the SOCKET test binary ABORTS via fatal CXXKIT_CHECK at tcp_socket.cpp:145 (`addr_rc == 0` is a fatal check, not a graceful false — ctest reports the suite as failed/crashed, not a clean per-case FAIL); the SERVER test fails cleanly (`listen` returns false at tcp_server.cpp:98-101). Existing 17 cases stay green.

- [ ] **Step 1.3 (implement helper)** — `cxxkit/network/detail/address_helper.hpp` full content:

```cpp
/*** Library: CxxKit ***/
#pragma once
#include <cxxkit/base/global.hpp>
#include <string>
#include <cstring>
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace cxxkit::network::detail {

/// IPv4/IPv6 auto-detecting sockaddr filler (dual-stack; shared by all backends).
/// Returns false when @p ip parses as neither family.
inline bool fill_sockaddr(const std::string &ip, uint16_t port, sockaddr_storage *out) {
    std::memset(out, 0, sizeof(*out));
    sockaddr_in *v4 = reinterpret_cast<sockaddr_in *>(out);
    if (inet_pton(AF_INET, ip.c_str(), &v4->sin_addr) == 1) {
        v4->sin_family = AF_INET;
        v4->sin_port = htons(port);
        return true;
    }
    sockaddr_in6 *v6 = reinterpret_cast<sockaddr_in6 *>(out);
    if (inet_pton(AF_INET6, ip.c_str(), &v6->sin6_addr) == 1) {
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(port);
        return true;
    }
    return false;
}

} // namespace cxxkit::network::detail
```

- [ ] **Step 1.4 (call sites)**: tcp_socket.cpp connect — replace the `uv_ip4_addr(ip.c_str(), port, &addr)` block with:
```cpp
sockaddr_storage addr{};
if (!fill_sockaddr(ip, port, &addr)) { /* existing invalid-address path: on_connected(false), state back to kIdle, return */ }
const int rc = uv_tcp_connect(req, ..., reinterpret_cast<const sockaddr *>(&addr), ...);
```
tcp_server.cpp listen — same replacement on the parse site. bound_port() — extend the existing `if (bound.ss_family == AF_INET)` chain with:
```cpp
else if (bound.ss_family == AF_INET6) {
    return ntohs(reinterpret_cast<sockaddr_in6 &>(bound).sin6_port);
}
```

- [ ] **Step 1.5 (verify GREEN)**: rebuild + `ctest --test-dir build -R tcp` → 19/19 green (17 old + 2 new).

- [ ] **Step 1.6 (commit)**: `pixi run clang-format -i` on touched files; then:
```bash
git add cxxkit/network/detail/address_helper.hpp cxxkit/network/tcp_socket.cpp cxxkit/network/tcp_server.cpp tests/tst_tcp_socket.cpp tests/tst_tcp_server.cpp
git -c user.name=chengxuewen -c user.email=1398831004@qq.com commit -m "fix(network): IPv6 dual-stack connect/listen (fill_sockaddr helper)"
```

### Task 0: Warm-up — baseline + asio tarball acquisition (Momus F1)

**Files:**
- Staging only: `/tmp/asio-asio-1-32-0.tar.gz` — committed to `3rdparty/` in Task 4 Step 4.1.

- [ ] **Step 0.1**: Clean tree check (`git status --short` empty) + main build tree has NETWORK=ON (`grep CXXKIT_ENABLE_LIB_NETWORK build/CMakeCache.txt`).
- [ ] **Step 0.2**: Baseline: `cmake --build build --parallel` → 0 errors; `ctest --test-dir build -R tcp` → all green; record suite total (`ctest --test-dir build | tail -1`).
- [ ] **Step 0.3**: Download + checksum-pin standalone asio (keep in /tmp until Task 4):
```bash
curl -L -o /tmp/asio-asio-1-32-0.tar.gz https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-32-0.tar.gz
sha256sum /tmp/asio-asio-1-32-0.tar.gz   # record hash in Task 4 Step 4.1 commit message
# expected layout after extract: asio-asio-1-32-0/asio/include/
```
Expect size > 2MB.

---
---

### Task 2: Public error + state surface (Qt alignment)

**Files:**
- Create: `cxxkit/network/socket_error.hpp`, `cxxkit/network/socket_state.hpp` (public headers)
- Create: `cxxkit/network/detail/error_mapping.hpp` (uv→SocketError inline switch)
- Modify: `cxxkit/network/tcp_socket.hpp` (+4 members; adopt_uv_tcp → adopt_native — the ONLY breaking public change)
- Modify: `cxxkit/network/detail/tcp_socket_p.hpp` (internal State deleted → SocketState; + mLastError/mOnError/mOnStateChange)
- Modify: `cxxkit/network/tcp_socket.cpp` (trigger points + mapping)
- Modify: `cxxkit/network/tcp_server.hpp/.cpp` (adopt_uv_tcp rename same as socket)
- Test: `tests/tst_tcp_socket.cpp` (+3 cases). NOTE (Momus): `tst_tcp_faults.cpp` has NO adopt_uv_tcp callers (only adopt_fd at :67) — nothing to adjust there.

**Interfaces:**
- Produces (public, `cxxkit::` namespace):
```cpp
enum class SocketError { kNone, kUnknown, kConnectionRefused, kConnectionReset, kTimedOut,
                         kHostUnreachable, kNetworkUnreachable, kAddrNotAvailable, kBrokenPipe, kEof };
std::string to_string(SocketError);              // full switch, all enumerators
enum class SocketState { kIdle, kConnecting, kConnected, kClosing, kClosed };
// TcpSocket additions:
void set_on_error(std::function<void(SocketError, const std::string &message)>);
void set_on_state_change(std::function<void(SocketState)>);
SocketState state() const;
SocketError last_error() const;
// breaking rename (both headers):
static std::unique_ptr<TcpSocket> adopt_native(EventLoop &loop, void *native_handle);
```
- Produces (detail): `SocketError map_uv_error(int uv_status)` in error_mapping.hpp: UV_ECONNREFUSED→kConnectionRefused, UV_ECONNRESET→kConnectionReset, UV_ETIMEDOUT→kTimedOut, UV_EHOSTUNREACH→kHostUnreachable, UV_ENETUNREACH→kNetworkUnreachable, UV_EADDRNOTAVAIL→kAddrNotAvailable, UV_EPIPE→kBrokenPipe, everything else→kUnknown.

- [ ] **Step 2.1 (failing tests)** in tst_tcp_socket.cpp:
```cpp
TEST_F(TcpSocketTest, ErrorCallbackConnectionRefused) {
    EventLoop loop;
    TcpSocket s(loop);
    SocketError got = SocketError::kNone;
    bool called = false;
    s.set_on_error([&](SocketError e, const std::string &) { got = e; called = true; });
    s.connect("127.0.0.1", 1, [](bool ok) { EXPECT_FALSE(ok); }); // port 1 refused
    /* same loop pump discipline as existing fault tests */
    EXPECT_TRUE(called);
    EXPECT_EQ(SocketError::kConnectionRefused, got);
    EXPECT_EQ(SocketError::kConnectionRefused, s.last_error());
    EXPECT_EQ(SocketState::kIdle, s.state()); // reset after failed connect
}

TEST_F(TcpSocketTest, StateChangeSequence) {
    // record states via set_on_state_change during a full echo roundtrip
    // expected subsequence: kConnecting, kConnected ... kClosed (order-checked)
    // use std::vector<SocketState> + EXPECT-equivalent comparison of the prefix
}

TEST_F(TcpSocketTest, InitialStateAndLastError) {
    EventLoop loop;
    TcpSocket s(loop);
    EXPECT_EQ(SocketState::kIdle, s.state());
    EXPECT_EQ(SocketError::kNone, s.last_error());
}
```

- [ ] **Step 2.2 (verify RED)**: build fails (symbols missing) — that is the failing state; record it.

- [ ] **Step 2.3 (implement)**: public headers per Interfaces block (banner + `#pragma once` + `<cxxkit/base/global.hpp>` + `<string>`; no uv includes). pimpl: delete `enum class State`, use `SocketState` everywhere; add `SocketState mState{SocketState::kIdle}`, `SocketError mLastError{SocketError::kNone}`, the two callbacks (invoke ONLY on loop thread, copy callback into local before invoke — PIT-40 discipline). Triggers: enter kConnecting / kConnected / kClosing / kClosed → mOnStateChange; connect failure & read error (nread < 0, EOF → kEof, else map_uv_error) → mLastError + mOnError. `state()`/`last_error()` loop-thread-only (check_loop_thread, matching class contract). Rename adopt_uv_tcp→adopt_native with `void *native_handle` param; uvTcp* casts move into the .cpp (public header keeps no uv types — only a comment `/* native uv_tcp_t* under the uv backend */`).

- [ ] **Step 2.4 (verify GREEN)**: full `ctest --test-dir build -R tcp` green (19 + 3 new; faults suite updated to adopt_native).

- [ ] **Step 2.5 (commit)**: format; `git add <touched>; git -c ... commit -m "feat(network): SocketError/SocketState surface, adopt_native rename, uv error mapping"`

---

### Task 3: StreamBackend abstraction — uv migration (zero behavior change)

**Files:**
- Create: `cxxkit/network/detail/stream_backend.hpp` (interface + factory decl)
- Create: `cxxkit/network/detail/stream_backend_uv.hpp` + `stream_backend_uv.cpp`
- Modify: `cxxkit/network/detail/tcp_socket_p.hpp` / `tcp_server_p.hpp` (members → backend instance)
- Modify: `cxxkit/network/tcp_socket.cpp` / `tcp_server.cpp` (all uv call sites → backend calls)
- Modify: `cxxkit/network/CMakeLists.txt` (+ stream_backend_uv.cpp to SOURCES)
- Test: existing suites only (19 + error/state cases) — green = acceptance

**Interfaces:**
- Consumes: Task 2's SocketState/SocketError (mapping stays in pimpl layer, NOT backend).
- Produces:
```cpp
namespace cxxkit::network::detail {

class StreamBackend {
public:
    virtual ~StreamBackend() = default;
    // client face
    virtual bool open(EventLoop &loop) = 0;
    virtual void connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done) = 0;
    virtual void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done) = 0;
    virtual void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data) = 0;
    virtual void read_stop() = 0;
    virtual void close(std::function<void()> on_closed) = 0;   // idempotent; async under uv
    virtual bool is_open() const = 0;
    // server face
    virtual bool listen(const std::string &ip, uint16_t port, int backlog) = 0;
    virtual uint16_t bound_port() const = 0;
    virtual void set_on_accept(std::function<void(std::unique_ptr<StreamBackend> client)> on_accept) = 0;
    // native handle adopt/release (void* — backend-owned type)
    virtual bool adopt_native(void *native_handle, EventLoop &loop) = 0;
    // adopt entries (Momus F2: fd adopt REQUIRED — public adopt_fd keeps tests green; grep gate depends on it)
    virtual bool adopt_native(void *native_handle, EventLoop &loop) = 0;
    virtual bool adopt_fd(int fd, EventLoop &loop) = 0;   // uv: init+uv_tcp_open; asio: socket.assign
    // dtor discipline: pump the loop until the native close callback ran
    // (covers ALL THREE existing pump loops: ~TcpSocket, ~TcpServer, listen-failure teardown)
    virtual void pump_until_closed() = 0;
};

std::unique_ptr<StreamBackend> make_stream_backend();   // compiled per backend selection

} // namespace cxxkit::network::detail
```
Contract notes (verbatim from today's uv code): single in-flight write with FIFO queue is a BACKEND concern under uv (uv_write) — implement inside stream_backend_uv; close idempotence + pump-until-closed are backend responsibilities; pimpl keeps state machine, callbacks, thread-affinity checks.

- [ ] **Step 3.1**: write stream_backend.hpp (content above + banner). Factory declared here, defined in the backend TU.
- [ ] **Step 3.2**: create stream_backend_uv.{hpp,cpp} by MOVING code verbatim from tcp_socket.cpp/tcp_server.cpp into `UvStreamBackend : public StreamBackend` methods: connect (uv_tcp_connect + fill_sockaddr), write (lws single-flight + PendingWrite deque + uv_write — **PendingWrite deque OWNERSHIP moves into the backend; begin_close's synchronous false-fanout over queued writes (tcp_socket.cpp:263-284) becomes backend close()'s responsibility**), read_start/read_stop (uv_read_start/stop + alloc/read callbacks), close (idempotent guard + uv_close + on_closed), listen/bind/getsockname + AF_INET6 branch, accept drain loop (produces UvStreamBackend via adopt_native), pump_until_closed (**ALL THREE pump loops move here**: ~TcpSocket `for rounds < 1000` at tcp_socket.cpp:79, ~TcpServer at tcp_server.cpp:80 (condition `mHandle != nullptr`, not state), listen-failure teardown at tcp_server.cpp:127), adopt_fd (fresh uv_tcp_init cell + uv_tcp_open), adopt_native (attach semantics as `attach_connected_handle`). **Move, don't redesign.** No uv types outside stream_backend_uv.{hpp,cpp} — uv.hpp include lives ONLY in those two files.
- [ ] **Step 3.3**: rewrire pimpls: `UvEventDispatcher *mDispatcher` + `uv_tcp_t *mHandle` → `std::unique_ptr<StreamBackend> mBackend{make_stream_backend()}`; pimpl callbacks now forward into backend calls; state machine / check_loop_thread / error mapping stay in pimpl. tcp_socket.cpp/tcp_server.cpp lose ALL direct uv references (grep `uv_` in tcp_socket.cpp/tcp_server.cpp must return 0 after this task).
- [ ] **Step 3.4 (verify)**: build 0 errors; `grep -n "uv_\|uv\.h\|libuv" cxxkit/network/tcp_socket.hpp cxxkit/network/tcp_server.hpp cxxkit/network/tcp_socket.cpp cxxkit/network/tcp_server.cpp` → ZERO matches; full `ctest --test-dir build -R tcp` green (same case count as Task 2 end).
- [ ] **Step 3.5 (commit)**: format; `git add -A cxxkit/network; git -c ... commit -m "refactor(network): StreamBackend abstraction, uv backend migration (no behavior change)"`

---

### Task 4: asio second backend (vendored wrap + implementation)

**Files:**
- Add: `3rdparty/asio-asio-1-32-0.tar.gz` (downloaded in Task 0 Step 0.3, committed here)
- Create: `cmake/wrap/FindWrapAsio.cmake` (extract-only, FindWrapImGui precedent)
- Create: `cxxkit/network/detail/stream_backend_asio.hpp` + `.cpp`
- Modify: `cxxkit/network/CMakeLists.txt` (backend option + conditional sources/libs/compile definitions)
- Test: run existing TCP suites under BOTH backends (same binaries, different builds)

**RESOLVED DESIGN (Momus Q4 — EMBED FINAL, gate closed):** asio integration shape —
- ~~BRIDGE~~ REJECTED: `register_socket_notifier` lives on `UvEventDispatcher`; under a uv-disabled kernel (the exact configuration an asio backend targets) the notifier machinery does not exist — BRIDGE impossible there.
- **EMBED (FINAL)**: `asio::io_context` owned by the backend, pumped on the SAME EventLoop thread via EventLoop post/timer hooks (`io_context.poll()` interleaved between loop rounds). Native asio completion semantics, natural error_code mapping; no kernel changes required.

**Interfaces:**
- Consumes: Task 3 `StreamBackend` contract verbatim.
- Produces: `make_stream_backend()` second definition guarded by `CXXKIT_NETWORK_BACKEND_ASIO`; `UvEventDispatcher` include REMAINS legal only inside stream_backend_uv files; `kernel` uv coupling drops to zero under asio builds (network no longer links WrapLibuv then).

- [ ] **Step 4.1 (wrap)**: commit tarball; FindWrapAsio.cmake — copy FindWrapImGui.cmake extract-only branch; target `CxxKitWrapAsio::WrapAsio` INTERFACE with include dir `asio-asio-1-32-0/asio/include`; stamp-guarded extract; NO build. `ASIO_STANDALONE` defined on the target (`target_compile_definitions`), `ASIO_HAS_STD_CHRONO` etc. per asio standalone defaults. Register wrap in `cmake/CxxKitFindPackageHelpers.cmake` mapping if such registry exists (mirror WrapAsio naming from Libcpr).
- [ ] **Step 4.2 (CMake option)**: in network/CMakeLists.txt BEFORE cxxkit_add_library:
```cmake
set(CXXKIT_NETWORK_BACKEND "uv" CACHE STRING "cxxkit network backend (uv|asio)")
set_property(CACHE CXXKIT_NETWORK_BACKEND PROPERTY STRINGS uv asio)
if(CXXKIT_NETWORK_BACKEND STREQUAL "asio")
    cxxkit_find_package(Asio PROVIDED_TARGETS CxxKitWrapAsio::WrapAsio)
    list(APPEND _backend_sources detail/stream_backend_asio.cpp)
    set(_backend_libs CxxKitWrapAsio::WrapAsio)
else()
    cxxkit_find_package(Libuv PROVIDED_TARGETS CxxKitWrapLibuv::WrapLibuv)
    list(APPEND _backend_sources detail/stream_backend_uv.cpp)
    set(_backend_libs CxxKitWrapLibuv::WrapLibuv)
endif()
```
...then SOURCES/PUBLIC_LIBRARIES use the expanded vars; wire preprocessor via `target_compile_definitions(cxxkit_network PUBLIC CXXKIT_NETWORK_BACKEND_ASIO=1)` in the asio branch (PIT-48).
- [ ] **Step 4.2b (consumer surface wiring — Momus F3)**: three pieces, all gated on the asio branch:
  (a) install tree: cxxkit/network/CMakeLists.txt:49-51 `install(DIRECTORY ... FILES_MATCHING "*.hpp")` installs ALL headers unconditionally — under asio, `detail/stream_backend_uv.hpp` (includes uv.h) must be excluded: add `PATTERN "detail/stream_backend_uv.hpp" EXCLUDE` (D38/D39 install-EXCLUDE precedent); symmetric: under uv exclude `detail/stream_backend_asio.hpp`.
  (b) CxxKitConfig.cmake.in: add WrapAsio stub row + `find_dependency` equivalent (media cf9e1e9 stub-chain precedent: stub INTERFACE_LINK_LIBRARIES → installed asio is header-only so include-dir-only stub suffices); gate the existing libuv stub block (`if(CXXKIT_LOOP_BACKEND_UV OR CXXKIT_NETWORK)`) to account for backend: under asio network no longer implies libuv.
  (c) CxxKitPkgConfigHelpers.cmake:73 hardcodes `CxxKitWrapLibuv::WrapLibuv` mapping — add `CxxKitWrapAsio::WrapAsio` row so asio-built .pc Requires chain is correct.
- [ ] **Step 4.3 (asio backend)**: implement `AsioStreamBackend : public StreamBackend` honoring the SAME contract: single in-flight write is NOT needed (asio queues internally) — but keep the callback discipline: on_done invoked exactly once; close idempotent; pump_until_closed becomes wait-until-`close` handler ran (asio close is synchronous — drain pending handler queue); adopt_fd = `asio::ip::tcp::socket::assign(acceptor protocol, fd)`; set_on_accept from an `asio::basic_socket_acceptor` async_accept loop. Error mapping: asio error_code → SocketError (refused/reset/timedout/host_unreachable/network_unreachable/addr_not_available/broken_pipe; eof → kEof via read handler) — extend `error_mapping.hpp` with `map_asio_error(const std::error_code&)` next to the uv one. ipv6 via fill_sockaddr (single parse path).
- [ ] **Step 4.4 (dual-backend verify)**: build TWO trees: `build` (uv) + `build-asio` (configure `-DCXXKIT_NETWORK_BACKEND=asio`); `ctest --test-dir build -R tcp` and `ctest --test-dir build-asio -R tcp` both green (same suite set; tests never include backend headers). Confirm asio tree: `grep "libuv" build-asio/cxxkit/network/CMakeFiles/cxxkit_network.dir/link.txt` → empty.
- [ ] **Step 4.5 (commit)**: format; two commits: `chore(3rdparty): vendor asio 1.32.0 + extract-only wrap` (attach sha256 from Task 0 Step 0.3) and `feat(network): asio second backend behind CXXKIT_NETWORK_BACKEND (consumer wiring incl. Config/pc/install EXCLUDE)`.

---

### Task 5: Test decoupling + guard relaxation

**Files:**
- Modify: `tests/CMakeLists.txt` — TCP test blocks: `if(CXXKIT_ENABLE_LIB_NETWORK AND CXXKIT_ENABLE_LOOP_BACKEND_UV)` → `if(CXXKIT_ENABLE_LIB_NETWORK)`.
- Modify: `examples/CMakeLists.txt:231` — same guard relaxation for exp_tcp_echo (Momus F4).
- Test: ctest under both trees.

- [ ] **Step 5.1**: relax both guards (exact block edits; keep `${CXXKIT_TEST_LINK_LIBRARIES} cxxkit::network ${CMAKE_THREAD_LIBS_INIT}` lines untouched).
- [ ] **Step 5.2 (verify)**: uv tree ctest unchanged count; asio tree: TCP suites + exp_tcp_echo REGISTER and pass/build (in Task 4 they ran because LOOP_BACKEND_UV was still ON by default — this step proves registration works without the uv kernel backend flag; build exp_tcp_echo in the asio tree and run it → rc=0).
- [ ] **Step 5.3 (commit)**: `test(network): decouple tcp suites + tcp echo example from loop backend guard`

---

### Task 6: Full gates + memory (controller-run, no subagent)

- [ ] **Step 6.1**: main tree FULL build + `ctest --test-dir build` all green (85+ suites); ASAN: `LSAN_OPTIONS="suppressions=$(pwd)/scripts/lsan.supp" ctest --test-dir build-asan -R tcp` green; clang-format dry-run clean on all new/modified files; C++11 gate grep on new lib headers clean.
- [ ] **Step 6.2**: memory writes — decisions.md `## D41: Network 后端抽象 + Qt 对齐`（R1 方案 A detail 层抽象/R2 adopt_native 破坏性收敛/R3 编译期 CMake 选后端/R4 asio EMBED 桥接裁定/R5 测试后端无关）; pitfalls.md PIT-50（uv_ip4_addr 硬编码 IPv4——默认参数化全族地址族）; status.md 新条目; conventions.md 如需（后端选择变量命名规约）。
- [ ] **Step 6.3**: README network row + AGENTS.md（如 STATUS 表存在则同步）; docs note: backend matrix（uv=默认，asio=opt-in）。
- [ ] **Step 6.4 (commit)**: `docs(memory): D41 records (network backend abstraction)`

## Self-Review (performed)

1. **Spec coverage**: IPv6 fix (Task 1) ✓; error/state surface (Task 2) ✓; uv hiding (Tasks 2-3: adopt_native + zero uv in public/upper cpps) ✓; abstraction (Task 3) ✓; asio second backend (Task 4) ✓; backend-agnostic tests (Task 5) ✓; memory (Task 6) ✓. HTTP P1 verbs/multipart/proxy explicitly OUT of scope this round (user focus = socket abstraction); recorded as follow-up.
2. **Placeholder scan**: no TBD/TODO; every task has concrete file paths, full code or exact edit instructions, verification commands, commit messages.
3. **Type consistency**: `fill_sockaddr` (Task 1) consumed by uv backend (Task 3) and asio backend (Task 4); `SocketError/SocketState` (Task 2) referenced unchanged in Tasks 3-5; `make_stream_backend()` factory (Task 3) extended by Task 4; `adopt_native(void*)` (Task 2) matches `StreamBackend::adopt_native(void*, EventLoop&)` (Task 3).
4. **Momus verdict APPROVE-WITH-FIXES — all 4 fixes folded**: F1 Task 0 added (tarball acquisition + checksum, was dangling reference); F2 StreamBackend.adopt_fd added (grep gate was unsatisfiable without it) + PendingWrite ownership + all-three-pump-loops named; F3 Step 4.2b consumer wiring (install EXCLUDE symmetric / CxxKitConfig stub + libuv-stub backend condition / pkg-config WrapAsio row); F4 examples guard relaxation + RED wording (fatal-CHECK abort, not clean FAIL) + IPv6 GTEST_SKIP guard + stale tst_tcp_faults claim removed (file has NO adopt_uv_tcp callers — only adopt_fd at :67).
5. Design gates closed: EMBED ruling final (Momus concur — BRIDGE impossible under uv-disabled kernel: register_socket_notifier lives on UvEventDispatcher).
