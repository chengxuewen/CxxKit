# cxxkit Documentation Index

## API Reference

- **Doxygen** (generated): `cmake -S . -B build -DCXXKIT_BUILD_DOCS=ON && cmake --build build --target Docs`
  - Output: `build/doc/html/index.html`
  - Covers all 13 sublibraries; excludes `detail/` and `*_p.hpp` private headers.

## Design & Architecture

| Document | Path | Content |
|---|---|---|
| Refactor design spec | [superpowers/specs/2026-08-18-cxxkit-refactor-design.md](superpowers/specs/2026-08-18-cxxkit-refactor-design.md) | abseil-style layout, namespace, 3rdparty wrap system, cmake helpers migration list, review findings (H1/H2/M1-M4) |
| Phase 1 plan | [superpowers/plans/2026-08-18-cxxkit-phase1-skeleton-sublibs.md](superpowers/plans/2026-08-18-cxxkit-phase1-skeleton-sublibs.md) | skeleton, helpers, header-only sublibraries, tests, install |
| Project memory | [.agents/memorys/](../.agents/memorys/) | status (phases), conventions (C1-C6), decisions (D1-D10), pitfalls |
| Dev rules | [.agents/rules/](../.agents/rules/) | common + cpp coding/review/testing/security rules |

## Examples

| Example | Demonstrates |
|---|---|
| [exp_core_version](../examples/exp_core_version.cpp) | minimal entry |
| [exp_logging](../examples/exp_logging.cpp) | logging API (CXXKIT_DEFINE_LOGGER, LOGGING_WARNING, TRACE/DEBUG/INFO), variant, string_utils |
| [exp_network_version](../examples/exp_network_version.cpp) | network sublibrary (gated by CXXKIT_ENABLE_LIB_NETWORK) |

## Key Decisions (summary)

- **D1** abseil-style sublibrary organization (directory = target = namespace path)
- **D2** flat `cxxkit::` namespace
- **D3** C++11 baseline (tests C++14, gtest 1.12.1)
- **D4** vendored 3rdparty (no FetchContent), `<cxxkit/3rdparty/<lib>/...>` header namespacing
- **D5** media/imgui stay in OpenCTK (continuation path deferred)
- **D7** cmake helpers migration list
- **D8** angle-bracket includes everywhere
- **D9** private headers in `<sub>/detail/` (not installed)
- **D10** original-project residue issues (dead includes, disabled tests)

Full details: [.agents/memorys/decisions.md](../.agents/memorys/decisions.md)
