# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and
[Semantic Versioning](https://semver.org/).

Version numbering: `MAJOR.MINOR.PATCH`. MAJOR = breaking API/ABI change
(bumps `SOVERSION`), MINOR = feature, PATCH = fix.

## [Unreleased]

### Added
- Test quality hardening (2026-08-20):
  - Sanitizer builds (`CXXKIT_BUILD_SANITIZERS=ON`, ASAN/LSAN/UBSan, `build-asan/`)
  - Coverage builds (`CXXKIT_BUILD_COVERAGE=ON`, `coverage` target, `build-cov/`)
  - 19-case boundary suite (`tst_boundary`) covering safety / out-of-bounds paths
  - Coverage-expansion suites: `tst_ascii`, `tst_string`, `tst_datetime`,
    `tst_clock`, `tst_logging`, `tst_assert` — library .cpp coverage 62% → 80.5%
  - clang-tidy static analysis in CI (warn-only gate, `.clang-tidy`)
  - CI gates: namespace cleanliness (C7), C++11 strictness (C4)
- Per-sublibrary export macros (`CXXKIT_<SUB>_API`) + working shared builds
  (`CXXKIT_BUILD_SHARED_LIBS=ON`, ELF soname versioning)
- `tst_elapsed_timer` regression for never-started `restart()` overflow

### Fixed
- `DateTime::steadyTimeFromSystemNSecs` epoch-basis mismatch (PIT-23):
  round-trip with `systemTimeFromSteadyNSecs` drifted ~1e18 ns; test now
  asserts exact symmetry (<2s window)
- `ElapsedTimer::restart()` signed-overflow UB on never-started timers (F2)
- `error.cpp` FNV hash signed-overflow (F5)
- `context_checker` use-after-free (40/100 SEGFAULT → 100/100 stable)
- Flaky semaphore tests: `MultiRelease`/`MultiAcquireRelease` replaced fixed
  `sleep_for(1ms)` with a startup barrier semaphore

### Changed
- Library **and** test code unified at C++11 (`cxx_std_11`); C++14/17
  constructs banned in library code (CI gate)
- 14 sublibrary CMakeLists migrated to `cxxkit_add_library` helper; examples to
  `cxxkit_add_executable`; tests to `cxxkit_add_test` (C10)
- `cxxkitConfig.cmake` vendored `find_dependency` now follows enabled sublibs
- CI drops `-DCXXKIT_ENABLE_LIB_NETWORK=ON` (vendored cpr/curl/mbedtls build was
  the pipeline's main time cost; network verified in local `build-shared-net`)

### Removed
- `src/` directories — merged into `cxxkit/<sub>/` (D14)
- Explicit STATIC/SHARED in sublibrary CMakeLists (follows global option)

## [0.x] — 2026-08-18/19 — Phase 1 & Phase 2

### Added
- Repository skeleton: 14 sublibraries (base/containers/functional/numerics/
  patterns/memory/units/time/kernel/thread/text/tools/network/crash/profiling)
- 34 gtest suites (353 → 357 test cases with crash suite)
- vcpkg-style vendored 3rdparty (21 packages, `cmake/wrap/FindWrap*.cmake`,
  stamp-based caching, `<cxxkit/3rdparty/<lib>/...>` header namespacing)
- `cxxkitConfig.cmake` + pkg-config (13 `.pc`) install tree; default install to
  `build/install/`; `BuildAll`/`BuildInstall`/`Docs` targets
- crash sublibrary (breakpad + backward-cpp, `CXXKIT_ENABLE_LIB_CRASH=ON`)
- examples: `exp_core_version`, `exp_logging`, `exp_network_version`
- CI workflow (GitHub Actions: format + build + test on ubuntu/macos)

### Notes
- Pre-0.1 development — no release tags cut yet; API subject to change.