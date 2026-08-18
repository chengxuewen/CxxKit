# cxxkit

## An open cross-platform cpp toolkit.

cxxkit is a portable cross-platform C++ development toolkit. It contains many algorithms, data structures, functional components, scripting languages and practical frameworks, which can facilitate developers to quickly develop applications and avoid the dilemma of repeated wheel building.

Organized abseil-style: **directory = sublibrary = CMake target**, pick only what you need (boost-like on-demand philosophy at abseil granularity). Minimum C++11, flat `cxxkit::` namespace, all internal includes use `<cxxkit/...>` angle-bracket paths.

## Sublibraries

| Sublibrary | Type | Contents |
|---|---|---|
| `cxxkit::base` | header-only | macros, types, compiler detection, config |
| `cxxkit::containers` | header-only | vector, array_view, inlined_vector, flat_set, concurrent_queue, vector_map |
| `cxxkit::functional` | header-only | function_view, invocable, unique_function |
| `cxxkit::numerics` | header-only | bits, divide_round, numeric, safe_compare, safe_conversions, safe_minmax |
| `cxxkit::patterns` | header-only | singleton |
| `cxxkit::memory` | compiled | aligned_malloc, shared_memory, zero_memory, smart pointers |
| `cxxkit::units` | compiled | data_size, data_rate, frequency, time_delta, timestamp |
| `cxxkit::time` | compiled | date_time, elapsed_timer |
| `cxxkit::kernel` | compiled | object, event, event_loop, signals, application |
| `cxxkit::thread` | compiled | thread_pool, task_queue, event_loop_thread, future, semaphore |
| `cxxkit::text` | compiled | string, base64, bit_buffer, ascii, string_builder, string_utils, format, string_view |
| `cxxkit::tools` | compiled | logging, random, assert, clock, status, error, metrics, filesystem, optional, expected, variant |
| `cxxkit::network` | compiled | http (cpr backend) |

## Quick start

### Build

```bash
cmake -S . -B build                 # configure (defaults: Debug, install to build/install)
cmake --build build --parallel      # build
cmake --build build --target BuildInstall   # build + install to build/install/
ctest --test-dir build              # run tests
```

Options: `-DCXXKIT_BUILD_TESTS=OFF`, `-DCXXKIT_ENABLE_LIB_NETWORK=ON`,
`-DCXXKIT_BUILD_DOCS=ON`, `-DCMAKE_INSTALL_PREFIX=/path/to/prefix`.

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

See [docs/README.md](docs/README.md) for the full documentation index.

## License

The self-owned code of this project is licensed under the permissive MIT License and can be freely applied to commercial and non-commercial projects while retaining copyright information.
However, this project also uses some scattered open source code, please replace or remove it for commercial use.
Any commercial disputes or infringement caused by using this project have nothing to do with the project and developers and shall be at your own legal risk.
When using the code of this project, the license agreement should also indicate the license of the third-party libraries that this project depends on.
