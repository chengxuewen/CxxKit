# TlsSocket TLS 三期 Implementation Plan (D42)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Backend-agnostic `TlsSocket` over the D41 StreamBackend abstraction (composing `TcpSocket`), mbedTLS 3.6.2 as the TLS engine, Qt-grade error/state surface, with a self-contained in-process TLS test peer.

**Architecture:** User ruling R1: TlsSocket is a **standalone public class composing a `TcpSocket`** (not a StreamBackend decorator) — public API mirrors TcpSocket's callback shapes; StreamBackend stays untouched so uv/asio both work for free. R2: errors **extend the public `SocketError` enum** (`kTlsHandshakeFailed`, `kTlsCertificateError`, `kTlsPeerClosed`, `kTlsWantWrite` is internal-only) and reuse `set_on_error`/`set_on_state_change`. R3: tests use an **embedded mbedTLS server peer** (TcpServer accept → server-side handshake on the accepted TcpSocket). R4: test certs are **pre-generated PEMs committed under tests/certs/** (EC P-256 self-signed, 10-year).

**Tech Stack:** C++11, mbedTLS 3.6.2 (vendored wrap `CxxKitWrapMbedTLS::WrapMbedTLS`, headers at `<cxxkit/3rdparty/mbedtls/ssl.h>`), D41 StreamBackend (untouched), ctest.

**Spec:** This file is self-contained; design inputs: mbedTLS 3.6.2 research (ssl_set_bio contracts, WANT_* semantics, no timer callbacks needed for TLS stream; ssl_read can return WANT_WRITE; same-argument retry on partial writes; PEER_CLOSE_NOTIFY vs 0 distinction) + cxxkit inventory (WrapMbedTLS NOT yet in network PUBLIC_LIBRARIES; no TLS test precedent; map_transport_error table; TcpSocket PIT-40 local-copy callback discipline) + 4 user rulings above.

## Global Constraints

- C18: commit messages + code comments English only; plan doc + AI dialogue Chinese.
- C4: C++11 lib code (no init-capture / auto return / generic lambda); D25 gate grep must stay clean.
- D8: angle-bracket includes; public headers may include `<cxxkit/3rdparty/mbedtls/...>` ONLY inside detail/ — public TlsSocket header exposes **zero mbedTLS types** (mirrors D41's zero-uv rule).
- D26 naming: functions snake_case; members mPascalCase; enum values kPascalCase.
- PIT-40: every callback invocation through a local copy (callbacks may destroy the socket).
- PIT-45: compound CXXKIT_CHECK conditions parenthesized.
- Banner style: copy tcp_socket.hpp lines 1-13; `#pragma once`; D26 enum naming.
- Commit identity: `git -c user.name=chengxuewen -c user.email=1398831004@qq.com`; English commit messages.
- Per-task gates: `cmake --build build --parallel` 0 errors; `ctest --test-dir build -R tls` green; dual-tree: `ctest --test-dir build-asio -R tls` green; `pixi run clang-format -i` (never CMakeLists); final task: full sweep — **uv main 88, asio 87, asan-asio 86** (baselines 86/85/84 + 2 new suites: tst_tls_error_mapping + tst_tls_socket) + ASAN-asio zero-diagnostics gate.
- Tests never include backend headers (backend-agnostic acceptance).
- NEVER `rm -rf build*`.

---

### Task 0: Warm-up — cert generation + baseline

**Files:**
- Create: `tests/certs/ca-cert.pem`, `tests/certs/server-cert.pem`, `tests/certs/server-key.pem` (committed)

- [ ] **Step 0.1**: Baseline: `git status` clean; `cmake --build build --parallel` 0 err; `ctest --test-dir build | tail -1` = 86 suites; record count.
- [ ] **Step 0.2**: Generate P-256 self-signed cert chain (openssl CLI, one CA + one server leaf with SAN localhost/127.0.0.1, 10-year, no passphrase):
```bash
mkdir -p tests/certs
openssl ecparam -name prime256v1 -genkey -noout -out tests/certs/ca-key.pem
openssl req -new -x509 -key tests/certs/ca-key.pem -out tests/certs/ca-cert.pem \
  -days 3650 -subj "/CN=cxxkit-test-ca" -sha256
openssl ecparam -name prime256v1 -genkey -noout -out tests/certs/server-key.pem
openssl req -new -key tests/certs/server-key.pem -out /tmp/server.csr \
  -subj "/CN=localhost" -sha256
printf "subjectAltName=DNS:localhost,IP:127.0.0.1\nbasicConstraints=CA:FALSE\n" > /tmp/san.ext
openssl x509 -req -in /tmp/server.csr -CA tests/certs/ca-cert.pem -CAkey tests/certs/ca-key.pem \
  -CAcreateserial -out tests/certs/server-cert.pem -days 3650 -sha256 -extfile /tmp/san.ext
openssl x509 -in tests/certs/server-cert.pem -noout -text | head -5   # verify
```
Commit in Task 1 (certs land with the server-peer test file that uses them).

---

### Task 1: SocketError extension + mbedTLS error mapping (foundation)

**Files:**
- Modify: `cxxkit/network/socket_error.hpp` (+4 enumerators + to_string cases)
- Create: `cxxkit/network/detail/tls_error_mapping.hpp` (mbedTLS → SocketError inline switch + want-retry classifier)
- Modify: `cxxkit/network/CMakeLists.txt` (PUBLIC_LIBRARIES += CxxKitWrapMbedTLS::WrapMbedTLS — BEFORE any TlsSocket source exists; harmless now)
- Test: `tests/tst_tls_error_mapping.cpp` (tiny, maps + to_string coverage)

**Interfaces:**
- Produces (socket_error.hpp):
```cpp
// appended to the existing enum (values append-only — ABI-safe):
enum class SocketError {
    ...existing..., kEof,
    kTlsHandshakeFailed, kTlsCertificateError, kTlsPeerClosed, kTlsProtocolError
};
```
- Produces (detail/tls_error_mapping.hpp, namespace cxxkit::network::detail):
```cpp
inline SocketError map_tls_error(int mbedtls_ret);
// PEER_CLOSE_NOTIFY -> kTlsPeerClosed; X509_CERT_VERIFY_FAILED/CERT_* -> kTlsCertificateError;
// FATAL/HANDSHAKE_FAILURE/etc -> kTlsHandshakeFailed; other MBEDTLS_ERR_SSL_* -> kTlsProtocolError;
// anything else -> kUnknown.
inline bool tls_want_retry(int mbedtls_ret);   // WANT_READ | WANT_WRITE -> true (retryable)
```
- Consumes: existing map_transport_error stays untouched.

- [ ] **Step 1.1 (failing tests)**: tst_tls_error_mapping.cpp — assert map_tls_error(MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) == kTlsPeerClosed; (…CERT_VERIFY_FAILED) == kTlsCertificateError; (…FATAL) == kTlsHandshakeFailed; (WANT_READ) handled by tls_want_retry()==true; to_string(kTlsPeerClosed)=="tls peer closed". Include `<cxxkit/3rdparty/mbedtls/ssl.h>` for constants.
- [ ] **Step 1.2**: RED: build fails (symbols missing). Implement headers. GREEN. Register test in the `if(CXXKIT_ENABLE_LIB_NETWORK)` block (LIBRARIES + CxxKitWrapMbedTLS::WrapMbedTLS).
- [ ] **Step 1.3**: Gates: build both trees (uv + build-asio; WrapMbedTLS now PUBLIC — the .pc/Config stub chain needs a wrap row check: `CxxKitWrapMbedTLS` already has stub handling? If not, add mirror of asio stub to CxxKitConfig.in + pkg row `-lmbedtls`; verify /tmp consumer still builds). Commit `feat(network): TLS error surface + mbedTLS mapping (D42 T1)`.

### Task 2: Embedded TLS server peer (test infrastructure)

**Files:**
- Create: `tests/tls_test_server.hpp` (header-only test helper; NOT installed)
- Test: consumed by Task 3's suite

**Interfaces:**
- Produces:
```cpp
struct TlsTestServer {
    explicit TlsTestServer(EventLoop &loop, const std::string &cert_chain_pem_path,
                           const std::string &key_pem_path);
    bool start();                       // listen 127.0.0.1:0
    uint16_t port() const;
    // called on accept: wrap the TcpSocket into a server-side TLS handshake;
    // on_done(ok) fires when handshake completes (or fails)
    void set_on_client_tls_ready(std::function<void(std::unique_ptr<cxxkit::TcpSocket>, bool ok)> on_ready);
    ~TlsTestServer();                   // closes listener + pending peers
};
```
Implementation notes (mbedTLS server side): `mbedtls_ssl_config_defaults(IS_SERVER, STREAM, TLS1.2 preset)` per-instance config (NOT global — test isolation) + `conf_ca_chain` (ca-cert.pem as trust root) + `conf_own_certificate` (server chain+key) + `mbedtls_ssl_set_bio` bridging onto the accepted TcpSocket's write/read via the SAME callback pattern TlsSocket uses — **factor the bio bridges into a small shared header** `tests/tls_bio_bridge.hpp` used by BOTH server peer and Task 3's client so the test validates the same bridge shape.

- [ ] **Step 2.1**: Write tls_bio_bridge.hpp (Momus F1 — the full sync-async contract, MANDATORY): send callback (`f_send`) SYNCHRONOUSLY copies `buf` into the pimpl-owned ciphertext out-box and returns `len` immediately (mbedTLS's buffer is safe — the copy happens inside f_send); the out-box drains through `TcpSocket::write` ONE entry at a time; when the out-box or the in-flight write is busy → return `MBEDTLS_ERR_SSL_WANT_WRITE` (mbedTLS retries with the SAME buffer); **`on_written(ok)` → RE-DRIVE `drive_handshake()` (and any deferred read pump); `on_written(false)` → transport-error path**. recv side: `read_start` stays armed (level-triggered), `on_data` pushes ciphertext into the ring; `f_recv` serves from the ring synchronously, empty ring → `MBEDTLS_ERR_SSL_WANT_READ`. All backpressure is expressed via WANT_*; the ONLY re-drive triggers are on_written/on_data.
- [ ] **Step 2.2**: Write tls_test_server.hpp implementing R3 (TcpServer accept → accept a server-side ssl_context over the client TcpSocket → handshake drives via the same WANT_* eventloop re-arm pattern the client will use).
- [ ] **Step 2.3**: Gate: compile both trees (no test yet — the headers are consumed by Task 3). Commit `test(network): embedded mbedTLS test server peer (D42 T2)`.

### Task 3: TlsSocket implementation (the core task)

**Files:**
- Create: `cxxkit/network/tls_socket.hpp` (public, zero mbedTLS types)
- Create: `cxxkit/network/tls_socket.cpp`
- Create: `cxxkit/network/detail/tls_socket_p.hpp` (pimpl: mbedtls contexts + handshake state machine)
- Test: `tests/tst_tls_socket.cpp` (12-14 cases; suite `cxxkit_tst_tls_socket`) — `tst_*` convention

**Interfaces:**
- Public (`cxxkit::network::TlsSocket`), mirroring TcpSocket callback shapes:
```cpp
explicit TlsSocket(EventLoop &loop);        // composes a TcpSocket internally
~TlsSocket();                               // close_notify best-effort + close + pump

void connect_tls(const std::settings &settings, const std::string &ip, uint16_t port,
                 std::function<void(bool ok)> on_connected);   // TCP connect then handshake
void start_tls(std::function<void(bool ok)> on_handshake_done);  // SERVER endpoint (MBEDTLS_SSL_IS_SERVER set at config build). connect_tls = client endpoint. No TlsRole enum (Momus F2: role is implied by the entry; QSslSocket two-method precedent). Setters (set_verify_mode/set_ca_path/set_hostname) MUST be applied BEFORE connect_tls/start_tls — mbedTLS freezes config at ssl_setup.
void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written); // encrypts + writes; partial-write same-arg retry honored
void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data);
void read_stop();
void close();                               // close_notify best-effort, then transport close (idempotent)

void set_on_error(std::function<void(SocketError, const std::string &)>);
void set_on_state_change(std::function<void(SocketState)>);   // kConnecting(=handshake) / kConnected(=encrypted) reuse
void set_verify_mode(int mode);             // 0=none 1=optional 2=required (Qt peerVerifyMode analog)
void set_ca_path(const std::string &pem_path);
void set_hostname(const std::string &hostname);  // SNI + verify target (client role)
SocketState state() const;
SocketError last_error() const;
bool is_open() const;
// adopt path: adopt_backend → compose over an accepted backend (server accept flows)
static std::unique_ptr<TlsSocket> adopt_socket(std::unique_ptr<TcpSocket> socket);
```
(Note: `const std::settings&` is a plan shorthand — implementer should define a small `TlsSettings` struct OR flatten args; pick the laziest that reads cleanly, document choice.)
- Produces (pimpl state machine, PIT-40 everywhere):
  kIdle → (connect_tls: TCP connect → kConnecting; handshake loop) → kConnected; failures → kClosing→kClosed + report_error(map_tls_error(...)).
- Handshake driver (pimpl): `drive_handshake()` runs `mbedtls_ssl_handshake`; WANT_READ → ensure read_start armed; WANT_WRITE → out-box busy (re-drive comes from on_written per the T2 contract); 0 → kConnected + on_connected(true); fatal → map_tls_error + close path. ssl_read returning WANT_WRITE must re-arm write (renegotiation pitfall).
- write(): loop with same-argument retry on WANT_*; partial writes honored; on_written once.
- Global singletons: one `entropy+ctr_drbg` pair per EventLoop thread is FORBIDDEN (loop thread-only already) — one process-global lazily-initialized pair (single-threaded discipline already holds: every socket op is loop-thread). Document the single-thread contract in the header.

- [ ] **Step 3.1 (failing tests first)**: tst_tls_socket.cpp, suite cases:
  1. HandshakeOkClientServer: TlsTestServer + client connect_tls → on_connected(true) + state kConnected (both roles handshake over same EventLoop).
  2. EncryptedRoundTrip: post-handshake write/read roundtrip ("tls-ping" → echo → compare bytes).
  3. LargePayloadFragmentation: 256KB write → read returns it in chunks; accumulate + compare (partial-write/fragment path).
  4. VerifyRequiredAgainstSelfSignedCA: set_ca_path(ca-cert.pem) + set_hostname("localhost") → handshake ok (chain verifies).
  5. VerifyFailsWrongCA: client trusts a DIFFERENT self-signed cert → handshake fails, kTlsCertificateError.
  6. VerifyOptionalReportsButContinues: set_verify_mode(optional) + wrong CA → handshake completes; last_error()==kTlsCertificateError recorded.
  7. VerifyNoneSkips: verify none + wrong CA → handshake ok.
  8. PeerCloseNotify: server closes cleanly → client on_data EOF → kTlsPeerClosed.
  9. CloseNotifyFromClient: client close() → server peer observes clean close (server side logs PEER_CLOSE_NOTIFY).
  10. ServerRole: start_tls on an accepted server-side socket (the TlsTestServer path itself validated end-to-end from the suite).
  11. StateSequence: kConnecting → kConnected (order-checked subsequence) + kClosing/kClosed on close().
  12. WriteAfterCloseFatal (death test): write after close → CXXKIT_CHECK abort.
- [ ] **Step 3.2**: RED → implement tls_socket.hpp/.cpp/p + wiring (CMakeLists: sources += tls_socket.cpp; tests block + CxxKitWrapMbedTLS) → GREEN iteratively per case cluster (1-3, then 4-7, then 8-12).
- [ ] **Step 3.3**: Gates: build 0 errors both trees; `ctest -R tls` green BOTH trees (uv + asio) — THE backend-agnostic acceptance; format clean; 3× repeat for flake check.
- [ ] **Step 3.4**: Commit `feat(network): TlsSocket — mbedTLS over StreamBackend (D42 T3)`.

### Task 4: Consumer surface + full gates + memory

**Files:**
- Modify: `cmake/CxxKitConfig.cmake.in` (if WrapMbedTLS moves from transitive to explicit PUBLIC: stub row + libuv-gate style conditional? NO — MbedTLS is unconditional once PUBLIC; mirror the WrapLibcpr row pattern)
- Modify: `cmake/CxxKitPkgConfigHelpers.cmake` (+ `-lmbedtls` row; MbedTLS has a .pc of its own — add Requires "mbedtls" if the helper supports it — check WrapMbedTLS's own .pc)
- Modify: README (network row + examples note)
- Test: full sweep

- [ ] **Step 4.1**: Config/.pc wiring: mirror what Libcpr already does (stub row "CxxKitWrapMbedTLS::WrapMbedTLS;mbedtls.a" — check the actual lib name `libmbedcrypto.a/libmbedtls.a/libmbedx509.a` — THREE libs; the stub may need three rows or an INTERFACE_LINK_LIBRARIES chain cpr already sets; INSPECT how cpr's stub handles the curl→mbedtls chain first, then mirror). Consumer verify: /tmp find_package(cxxkit COMPONENTS network) + TlsSocket build/run under BOTH install trees (uv + asio).
- [ ] **Step 4.2**: Full gates: uv main 88/88 + asio 87/87; ASAN-asio: `build-asio-asan` reconfigure+build+ `ctest -R 'tls|tcp|http'` zero diagnostics; format dry-run clean on all touched; C++11 gate grep clean.
- [ ] **Step 4.3**: Memory: decisions.md D42 (R1-R4 + failure-handling rulings recorded during execution); pitfalls.md (TLS pitfalls from research: ssl_read WANT_WRITE renegotiation, same-arg partial-write retry, PEER_CLOSE_NOTIFY vs 0, global drbg single-loop-thread contract); status.md entry; README rows.
- [ ] **Step 4.4**: Commit `docs(memory): D42 records (TlsSocket TLS-on-StreamBackend)`.

## Self-Review (performed)

1. **Spec coverage**: error surface (T1) ✓; test infra (T2) ✓; core (T3) ✓; consumer+gates+memory (T4) ✓. Explicitly OUT: ALPN (add setter when needed — YAGNI), session resumption/tickets, renegotiation initiation (only the read-side WANT_WRITE handling), DTLS, client certs (set_local_certificate — add when needed).
2. **Placeholder scan**: `const std::settings&` flagged in-plan as implementer shorthand with a decision directive; everything else concrete.
3. **Type consistency**: map_tls_error/tls_want_retry (T1) consumed by T3 pimpl; TlsTestServer/TlsBioBridge (T2) consumed by T3 tests; SocketError extensions (T1) used across T3 tests; adopt_socket composes TcpSocket (T3) using the D41 adopt_backend chain internally.
4. **Momus verdict APPROVE-WITH-FIXES — all 3 folded**: F1 bio-bridge write-side contract explicit (f_send copies→returns len; out-box drain; WANT_WRITE when busy; on_written→re-drive trigger — was the one gap); F2 dead TlsRole enum deleted (role implied by connect_tls=client / start_tls=server) + setters-before-start ordering pinned; F3 suite arithmetic corrected (88/87/86, was hardcoded 87) + test file renamed tst_tls_socket.cpp.
5. **Known risks**: mbedTLS server-side handshake over a TcpSocket needs the SAME bio bridge the client uses — Task 2's shared header is the mitigation (test validates the bridge itself); test flake risk concentrated in handshake interleaving — mitigated by same-loop both-peers (no cross-thread) and bounded pumps.
