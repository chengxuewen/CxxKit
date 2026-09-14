# Task 3 Report: TlsSocket — mbedTLS over StreamBackend-composed TcpSocket (D42 T3)

**Commit:** `ca26433` — `feat(network): TlsSocket — mbedTLS over StreamBackend-composed TcpSocket (D42 T3)`
**Branch:** main
**Status:** DONE (all 12 cases green both trees; asio stall root-caused and fixed in-tree)

## Files

- `cxxkit/network/tls_socket.hpp` — public API, ZERO mbedTLS types
- `cxxkit/network/detail/tls_socket_p.hpp` — pimpl: mbedtls contexts + bridge mirror + state
- `cxxkit/network/tls_socket.cpp` — implementation (entropy/DRBG, pumps, close semantics)
- `cxxkit/network/CMakeLists.txt` — SOURCES += tls_socket.cpp (MbedTLS already PUBLIC)
- `tests/tst_tls_socket.cpp` — 12 cases, suite `cxxkit_tst_tls_socket`
- `tests/CMakeLists.txt` — registration + `TEST_CERTS_DIR` compile definition
- `tests/tls_test_server.hpp` — extended: park_socket / park_failed_socket / park_peer (R-T3-3 family)
- `tests/certs/wrong-ca-cert.pem` (+ key) — pre-generated second self-signed CA (EC prime256v1, CN=cxxkit-wrong-ca)

## Deviations from brief (documented per instructions)

1. **`std::settings` placeholder** — resolved per controller instruction: plain setters ONLY
   (set_verify_mode/set_ca_path/set_hostname/set_certificate/set_private_key), matching QSslSocket.
   connect_tls signature: `connect_tls(const std::string &ip, uint16_t port, std::function<void(bool ok)>)`.
2. **`adopt_socket` static dropped** — `set_transport(std::unique_ptr<TcpSocket>)` covers the
   accepted-socket injection (controller's stated preference); no second entry point.
3. **Server identity API added** — `set_certificate`/`set_private_key`: a TLS SERVER endpoint
   cannot complete a handshake without an identity (root-caused via NO_CLIENT_CERTIFICATE /
   CA_CHAIN_REQUIRED probes). Certificates load lazily in freeze_config(); failure is mapped to
   kTlsCertificateError.
4. **TEST_SOURCE_DIR** — no precedent existed; added `TEST_CERTS_DIR` only on the new target.

## Test cases (12, all green)

| # | Case | Shape |
|---|------|-------|
| 1 | HandshakeOkClientServer | TlsTestServer oracle, verify required |
| 2 | StateSequence | kConnecting→kConnected→kClosing→kClosed order-checked |
| 12 | WriteAfterCloseFatal | death test, CXXKIT_CHECK, `fast` style (socketpair path) |
| 3 | EncryptedRoundTrip | "tls-ping" over server-role TlsSocket echo peer (Q5) |
| 4 | LargePayloadFragmentation | 256KB → 16×16KB records → reassemble+memcmp |
| 5 | VerifyRequiredOk | right CA + hostname → ok, last_error kNone |
| 6 | VerifyFailsWrongCa | wrong CA → fatal alert → kTlsCertificateError |
| 7 | VerifyOptionalReports | mode 1 + wrong CA → connected, last_error kTlsCertificateError |
| 8 | VerifyNoneSkips | mode 0 → ok, kNone |
| 9 | PeerCloseNotify | server close → client on_data(0) + kTlsPeerClosed |
| 10 | CloseNotifyFromClient | client close → server EOF + kTlsPeerClosed |
| 11 | ServerRoleStartTls | accepted transport + start_tls + roundtrip |

RED evidence: first build failed with `fatal error: cxxkit/network/tls_socket.hpp: No such file`.

## Gate results

| Gate | Result |
|---|---|
| uv tree ctest -R tls ×3 | 2/2 suites green ×3 (13 cases incl. mapping) |
| asio tree ctest -R tls ×3 | 2/2 suites green ×3 |
| full uv ctest | **88/88** (87 + new suite) |
| ASAN (build-asan, lsan.supp) | **88/88** incl. tls suite — zero sanitizer diagnostics |
| clang-format 23.1.0 | clean on all 4 files |
| C++11 gate (`auto x =`/`if constexpr`) | 0 hits |
| octk residue | 0 |

## Root causes found during GREEN (all fixed at root)

1. **Process-global entropy+DRBG poisoned handshakes** — both endpoints on one loop thread
   interleaved draws from the SAME ctr_drbg; the server's ECDHE key material diverged from what
   the client derived → INVALID_MAC (-29056) mid-handshake under asio (and flaky elsewhere).
   Fixed: per-socket entropy+ctr_drbg seeded in the pimpl ctor (matches the T2 oracle shape).
2. **Tiny-write overwrite** — a single `mPendingWrite` slot: a second `write()` (server echo)
   overwrote the in-flight pending record and its callback (dropped data + stalled pipeline).
   Fixed: pending-write FIFO (`std::deque<PendingWrite>`), HEAD entry owns same-buffer retry.
3. **4KB read buffer vs 16KB records** — `ssl_read` returns MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL for
   a buffer smaller than the pending record; we mapped it to kTlsProtocolError and tore the
   session down. Fixed: record-sized stack buffer + explicit BUFFER_TOO_SMALL guard.
4. **close_notify raced the transport close** — the notify queued via f_send was either canceled
   by the immediate transport close (peer saw raw FIN) or double-sent (INVALID_MAC at the peer).
   Fixed: notify captured out of the out-box and sent DIRECTLY (bypassing the bridge); transport
   closes on its completion; deferred kClosed reported from that path.
5. **Transport EOF under TLS** — asio delivers EOF via the read callback (not set_on_error);
   without a marker, ssl_read parked WANT_READ forever. Fixed: `mTransportEof` latch →
   pump_plaintext converts to on_data(0) + kTlsPeerClosed.
6. **asio scheduler auto-stop (THE asio stall)** — `io_context::poll()` marks the scheduler
   STOPPED when a round finds nothing ready; every op registered after that sat un-run forever
   (both sides frozen post-handshake until a teardown pump reaped them). Fixed:
   `io->restart()` before every `poll()` in `pump_tick` — no-op when not stopped, documented
   asio pattern. Symptom chain in traces: `handlers=0` ticks forever, then everything completes
   at once during teardown pumping.
7. **Test-harness time accounting** — `wait_for`'s `waited += 250` burned the whole 4s budget in
   ~12ms of wall time (NOWAIT rounds return instantly), starving the 1ms-cadence-driven pump.
   Fixed: steady_clock wall deadline. (Harness-level; benefits both trees.)
8. **I5/UAF in fixtures** — sockets AND the bridge-owning Peer must die OUTSIDE dispatcher
   callbacks: added `park_socket` / `park_failed_socket` / `park_peer` handoffs to
   TlsTestServer + ownership-parking order in TlsEchoPeer (ASAN-proven: a residual read event
   into the freed Peer's bridge = UAF; a transport dtor drain inside a callback = I5 fatal).

## Case-count reconciliation

Controller predicted 87+1=88 uv / 86 asio. Actual: uv 87→**88** ✓; asio 86→**87** (qt
auto-detect delta of 1 as documented in D41.5/D39).

## Known limitations / concerns

- `LargePayloadFragmentation` ASAN shadowing: none observed — full ASAN 88/88 green.
- asio `io->restart()` is now load-bearing for the whole asio backend; flagged in a comment
  with the D42 reference. If a future asio bump changes restart semantics, the pump is the
  place to look.
- Server-role TlsSocket in tests presents the test server cert with verify_mode 0 (client-auth
  off). authmode REQUIRED without a CA chain aborts with CA_CHAIN_REQUIRED (mbedTLS consistency
  check) — documented in the test fixture comment.
- Client-role `set_certificate` is accepted but unnecessary; server role REQUIRES it (documented
  in the public header).
- GLFW-style third backend, renegotiation mid-stream, session resumption: out of scope (needs-trigger).
