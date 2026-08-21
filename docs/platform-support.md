# Platform Support Matrix

Verified status of CxxKit on supported platforms. "Verified" means the full
local gate ran (build + tests) on that configuration; partial rows list what
was validated.

Legend: ✅ fully verified · ◑ partially verified · ⬜ not verified / not a target

## OS / toolchain

| Platform | Toolchain | Build | Tests | Sanitizer | Shared | Notes |
|---|---|---|---|---|---|---|
| Linux x64 | GCC / Clang | ✅ | ✅ 40 suites | ✅ ASAN/LSAN/UBSan | ✅ | Primary dev platform; all local gates green |
| macOS (CI) | AppleClang | ✅ | ✅ (CI: format+build+test) | ⬜ | ⬜ | CI runs format/C++11 gate/build/test on ubuntu+macos |
| Linux arm64 | — | ⬜ | ⬜ | ⬜ | ⬜ | Untested; crash requires a `crash-deps-<triplet>.7z` export |
| Windows | — | ⬜ | ⬜ | ⬜ | ⬜ | Not a first-class target (verified on demand); crash needs Windows triplet archive |

## Sublibrary status

| Sublibrary | Type | Linux | macOS | Windows | Notes |
|---|---|---|---|---|---|
| base | header-only | ✅ | ✅ | ◑ | macros/compiler/config; no platform code |
| containers | header-only | ✅ | ✅ | ◑ | incl. inlined_vector (absl test helper not vendored — no test suite) |
| functional | header-only | ✅ | ✅ | ◑ | |
| numerics | header-only | ✅ | ✅ | ◑ | |
| patterns | header-only | ✅ | ✅ | ◑ | singleton |
| memory | compiled | ✅ | ✅ | ◑ | aligned_malloc/shared_memory/zero_memory |
| units | compiled | ✅ | ✅ | ◑ | |
| time | compiled | ✅ | ✅ | ◑ | date_time/elapsed_timer |
| kernel | compiled | ✅ | ✅ | ◑ | object/event/event_loop/signals/application |
| thread | compiled | ✅ | ✅ | ◑ | thread_pool/task_queue/etc. |
| text | compiled | ✅ | ✅ | ◑ | ascii/string/base64/... |
| tools | compiled | ✅ | ✅ | ◑ | logging/random/status/error/... |
| network | compiled | ✅ (with vendored cpr/curl/mbedtls) | ⬜ | ⬜ | opt-in `CXXKIT_ENABLE_LIB_NETWORK=ON`; macOS CI build only |
| crash | compiled | ✅ (x64) | ◑ (x64) | ⬜ | opt-in `CXXKIT_ENABLE_LIB_CRASH=ON`; Linux/macOS x64 archived, Windows/arm64 need `crash-deps-<triplet>.7z` first |
| profiling | header-only | ✅ (opt-in Tracy) | ⬜ | ⬜ | `CXXKIT_ENABLE_LIB_TRACY` |

## Notes & constraints

- **C4**: library and test code compile at **C++11** (`cxx_std_11`); C++14/17
  constructs are banned in library code (CI gate). Tests also C++11 (gtest 1.12.1).
- **Network 3rdparty**: building `network` pulls vendored cpr/curl/mbedtls
  compiles (10-25 min). CI keeps network off; verified locally via
  `build-shared-net`.
- **crash platform support**: the breakpad/backward-cpp dependency archive is
  triplet-specific. Linux/macOS x64 `.7z` are expected in
  `INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR`; Windows/arm64 require exporting a new
  `crash-deps-<triplet>.7z` first (vcpkg export).
- **Shared builds**: `-DCXXKIT_BUILD_SHARED_LIBS=ON` produces versioned ELF
  `.so` (soname) and exposes missing link deps / cycles hidden by static
  builds. Run tests under both forms.
- **Known flaky-area guidance**: timing-sensitive tests must use
  condition-variable / semaphore readiness barriers, never fixed short
  `sleep_for()` (fixed in 2026-08-20; see CHANGELOG).
- **verification**: `ctest --test-dir build --output-on-failure` is the
  authority; `scripts/coverage.sh build-cov` reports the coverage aggregate
  (target ≥ 80%).