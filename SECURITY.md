# Security Policy

## Supported versions

Security fixes are applied to the current `main` branch. There are no
long-term-support releases at this stage of the project; please upgrade to
the latest commit or release when a fix lands.

## Reporting a vulnerability

**Do NOT open a public GitHub issue for security vulnerabilities.**

Report privately to the maintainer:

- **Email**: 1398831004@qq.com (ChengXueWen, project maintainer)

Please include:

1. Affected sublibrary(s) and version/commit.
2. A minimal reproduction (code snippet or test).
3. Impact description (crash, memory unsafety, data exposure, ...).
4. Any suggested fix, if you have one.

You can expect an acknowledgement within 5 business days, and a fix plan /
fixed commit as soon as the issue is confirmed.

## Security-relevant areas in this project

| Area | Risk notes |
|---|---|
| `cxxkit/text/ascii.cpp` | numeric parsing (`ascii_strtod`/`strtoll`/`strtoull`) — boundary inputs validated in `tst_ascii`; treat untrusted strings carefully |
| `cxxkit/text/string_encode` / `base64` | encoder/decoder bounds tested in `tst_boundary` (19 cases, ASAN-clean) |
| `cxxkit/network/http` | network sublibrary (optional, `CXXKIT_ENABLE_LIB_NETWORK=ON`) wraps cpr/curl; TLS via mbedtls vendored build |
| `cxxkit/crash` | breakpad minidump handler writes dump files; keep dump paths untrusted-input-free |
| `cxxkit/thread` | thread_pool/task_queue — ownership/lifetime discipline; verified under ASAN/LSAN |
| 3rdparty vendored code | all vendored under `3rdparty/`; check upstream advisories for the pinned versions |

## Security checks before merging

- No hardcoded secrets (API keys, passwords, tokens) — CI greps for `sk-`,
  `api_key`, `password=` patterns.
- No unsafe C functions in new code (`strcpy`, `strcat`, `sprintf`, `gets`).
- Prefer `std::string`/`std::vector` and bounds-checked access at trust boundaries.
- Sanitizer builds (ASAN/LSAN/UBSan) pass for changed code:
  `LSAN_OPTIONS=suppressions=scripts/lsan.supp ctest --test-dir build-asan`