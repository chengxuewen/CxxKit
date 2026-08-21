# Contributing to CxxKit

Thanks for considering contributing! This project is organized abseil-style:
**directory = sublibrary = CMake target**, C++11 baseline, vendored 3rdparty
dependencies only.

## Development setup

```bash
# Dependencies (Linux): cmake, ninja, clang-format, g++/clang++
cmake -S . -B build                              # configure (default Debug)
cmake --build build --parallel                   # build
ctest --test-dir build --output-on-failure       # run all tests (40 suites)
```

Optional verification builds (in separate dirs — see C6):

```bash
# Shared-library form (exposes missing link deps / cycles)
cmake -S . -B build-shared -DCXXKIT_BUILD_SHARED_LIBS=ON -DCXXKIT_BUILD_TESTS=ON
cmake --build build-shared --parallel && ctest --test-dir build-shared

# Sanitizers (ASAN/LSAN/UBSan) — use scripts/lsan.supp for LSAN suppressions
cmake -S . -B build-asan -DCXXKIT_BUILD_SANITIZERS=ON -DCXXKIT_BUILD_TESTS=ON
cmake --build build-asan --parallel
LSAN_OPTIONS=suppressions=$PWD/scripts/lsan.supp ctest --test-dir build-asan

# Coverage
cmake -S . -B build-cov -DCXXKIT_BUILD_COVERAGE=ON -DCXXKIT_BUILD_TESTS=ON
cmake --build build-cov --parallel && ctest --test-dir build-cov
bash scripts/coverage.sh build-cov   # aggregate should be >= 80%
```

## The one-command local gate

```bash
bash scripts/check.sh
```

Runs 8 steps: format → namespace (C7) → C++11 strictness (C4) → sanitizer
(if build-asan exists) → shared build (if build-shared exists) → build →
test → coverage report (if build-cov exists).

## Code conventions (summary)

| Rule | Constraint |
|---|---|
| **C1** | CMake is the only build system. Vendored 3rdparty + stamp. No FetchContent. |
| **C4** | Library code C++11 (`cxx_std_11`). Tests also C++11. No `if constexpr`, `_t` aliases, generic lambdas. |
| **C6** | **Never `rm -rf build`** — clobbers the 3rdparty stamp cache (5+ min rebuild). Clean only `build/CMakeCache.txt build/CMakeFiles build/Testing`. |
| **C7** | Brand `CxxKit` display-layer; code identifiers lowercase `cxxkit` (dirs/namespace/targets/includes/pkg-config). |
| **C8** | Sublibrary dir = headers + sources + CMakeLists (abseil-style). No `src/` subdir. |
| **D8** | Internal includes use angle brackets: `#include <cxxkit/...>`. |
| **D9** | Private headers `xxx_p.hpp` live in `<sub>/detail/`. |

Full details: [.agents/memorys/conventions.md](.agents/memorys/conventions.md)
and [.agents/memorys/decisions.md](.agents/memorys/decisions.md).

## Adding a sublibrary

1. Create `cxxkit/<sub>/` with public headers + `*.cpp` (compiled) + `CMakeLists.txt`
   using `cxxkit_add_library` (see `cmake/CxxKitLibraryHelpers.cmake`).
2. Register tests via `cxxkit_add_test` in `tests/CMakeLists.txt`.
3. Run `bash scripts/check.sh`; keep coverage above 80%.

## Testing

- GoogleTest suites in `tests/tst_*.cpp` (gtest 1.12.1, vendored).
- New tests must compile at C++11 and pass in both normal and ASAN builds.
- Timing-sensitive tests must be deterministic: prefer condition-variable /
  semaphore readiness barriers over fixed `sleep_for()` (see PIT: flaky tests).
- Full suite: `ctest --test-dir build --output-on-failure`.

## Formatting

clang-format is authoritative (`.clang-format`). Format before committing:

```bash
clang-format -i <changed-files>
```

## Static analysis

clang-tidy (`.clang-tidy`) scans library `.cpp` files in CI (warn-only gate).
Run locally:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build $(find cxxkit -name '*.cpp' -not -path '*3rdparty*')
```

## Commit guidelines

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
type(scope): description
```

- Types: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `perf`, `ci`.
- One logical change per commit; tests must pass before committing.
- Do not commit generated artifacts (`build*`, `3rdparty` caches, `coverage/`).

## Pull requests

1. Target `main`.
2. PR description: summary, test results (counts), any trade-offs.
3. CI must be green (format, namespace, C++11 gate, build, test, clang-tidy warn-only).
4. Keep the diff focused; review your own diff before requesting review.