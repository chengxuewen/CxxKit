# qt embed smoke checklist (manual)

Runtime verification for `cxxkit::qt` (`QtEventDispatcher`) via `exp_qt_embed`. The CI runs a
**compile-only** gate (headless declared); **this page is the runtime gate** — same honest layering
as crash/imgui examples. No display needed: `QCoreApplication` carries no GUI dependency.

## Prerequisites

- Any Linux/macOS with Qt ≥ 6 runtime (`pkg-config --modversion Qt6Core`); Qt 5 unverified
  (≥ 5.10 is a documentation-level claim, see `cxxkit/qt/qt_event_dispatcher.hpp`)
- Build switch `CXXKIT_ENABLE_LIB_QT=ON` (coexists with `CXXKIT_ENABLE_LIB_UV=ON` — independent targets)
- Expected baseline output of `exp_qt_embed`, byte-identical across runs:

```
queued: 42
tick 1
tick 2
tick 3
rc: 0 (ticks: 3)
```

## Steps

| # | Step | Expected |
|---|------|----------|
| 1 | `cmake -S . -B build -DCXXKIT_ENABLE_LIB_UV=ON -DCXXKIT_ENABLE_LIB_QT=ON && cmake --build build --parallel` | configure + build 0 errors (uv + qt both on, no conflict) |
| 2 | `./build/examples/cxxkit_exp_qt_embed` twice, diff the outputs | rc=0 both runs; prints `queued: 42` exactly once + `tick 1..3`; two runs byte-identical |
| 3 | Cross-thread post: in a scratch variant, `std::thread` + `loop.post(...)` (tst_qt_event_dispatcher `PostDrainsViaBridgeTimer` covers it) | task runs on the main/loop thread; no crash, no lost task |
| 4 | Time the 3 ticks: `time ./build/examples/cxxkit_exp_qt_embed` | elapsed ≈ 300ms (loose ±100ms — 100ms timer × 3) |
| 5 | Nested `process_events`: call it again inside a timer/post callback (Qt allows nesting — `QCoreApplication::processEvents` re-entrancy; `NestedProcessEventsPermitted` covers it) | returns normally, no fatal (contrast: fatal on the uv engine) |
| 6 | After `tick 3` the example calls `loop.exit(0)` then `loop.exec()` returns | printed `rc: 0 (ticks: 3)` and process rc=0 (`echo $?`) |
| 7 | Disconnect before teardown (documented pattern, spec I7): `conn.disconnect()` after exec returns, then destroy the loop/signal — no late callback after disconnect (tst_event_loop `ConnectQueuedReturnsConnectionForDisconnect` covers it) | no post-exit delivery, no crash |
| 8 | Context destruction order: the normal stack order destroys the loop (dispatcher) before `QCoreApplication` | clean exit, no crash (declaration-order contract I6; `DispatcherBeforeContextAndLateWakeDropped` covers it) |

## Known boundaries

- Blocking `process_events` is an honest downgrade on the Qt engine: the pass-through never blocks
  (blocking waits are the host loop's job — spec §5, R-C1-8). `loop.exec()` under this engine is a
  bounded busy-poll while waiting for timers; the host-shaped variant (`app.exec()` without
  `loop.exec`) is the production form.
- Bell self-coalescing (R-C1-6): a wake storm posts at most one queued doorbell until the next
  `process_events` clears the flag — latency is not promised, only eventual delivery.
- Qt 5 unverified (interface touches only `QCoreApplication`/`QObject`/`QTimer`/`invokeMethod`,
  so ≥ 5.10 is theoretically compatible — documentation-level claim).
