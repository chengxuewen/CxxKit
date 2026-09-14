/**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/network/tls_socket.hpp>

#include <cxxkit/base/macros.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/tcp_socket.hpp>

#include <cxxkit/3rdparty/mbedtls/ctr_drbg.h>
#include <cxxkit/3rdparty/mbedtls/entropy.h>
#include <cxxkit/3rdparty/mbedtls/pk.h>
#include <cxxkit/3rdparty/mbedtls/ssl.h>
#include <cxxkit/3rdparty/mbedtls/x509_crt.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Private implementation of @ref TlsSocket (D42 T3).
 *
 * The mbedTLS <-> TcpSocket bio bridge here MIRRORS the shared test fixture
 * tests/tls_bio_bridge.hpp (Momus F1 contract, including the C1 liveness token —
 * `mAlive` shared_ptr + weak capture + expired-first-line in every socket callback).
 * Contract recap:
 *  - f_send: SYNCHRONOUS copy of the ciphertext into the out-box, return len immediately;
 *    busy (out-box non-empty or write in flight) -> WANT_WRITE. One TcpSocket::write at a time.
 *  - f_recv: serve from the ciphertext in-ring synchronously; empty -> WANT_READ.
 *    read_start stays ARMED always; on_data appends to the ring and re-drives.
 *  - The ONLY re-drive triggers are on_written and on_data.
 */
class TlsSocketPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(TlsSocketPrivate)

public:
    explicit TlsSocketPrivate(TlsSocket *p, EventLoop &loop);
    ~TlsSocketPrivate();

    void check_loop_thread(const char *api) const;

    // ----- public API implementation (tls_socket.cpp) -----
    void connect_tls(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected);
    void start_tls(std::function<void(bool ok)> on_handshake_done);
    void set_transport(std::unique_ptr<TcpSocket> transport);
    void attach_bridge(); // wires bio + transport callbacks after the transport connects
    void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written);
    void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data);
    void read_stop();
    void close();
    void set_on_error(std::function<void(SocketError, const std::string &)> on_error);
    void set_on_state_change(std::function<void(SocketState)> on_state_change);
    void set_verify_mode(int mode);
    void set_ca_path(const std::string &pem_path);
    void set_certificate(const std::string &cert_pem_path);
    void set_private_key(const std::string &key_pem_path);
    void set_hostname(const std::string &hostname);
    SocketState state() const;
    SocketError last_error() const;
    bool is_open() const;

    // ----- internals -----
    void drive_handshake();
    void pump_plaintext();       ///< drains decrypted bytes to mOnData
    void pump_plaintext_write(); ///< retries the pending ssl_write (WANT_* same-buffer semantics)
    void pump_write();           ///< submits the next out-box entry
    void begin_close();          ///< idempotent close path
    void finish_error(SocketError error, const std::string &message, bool fatal_handshake);
    void freeze_config();         ///< applies the recorded role + setters into the ssl config (once)
    void on_redrive();            ///< the ONLY re-drive funnel: handshake -> pending write -> reads
    void on_transport_terminal(); ///< post-handshake transport EOF surface
    /// mbedtls_ssl_set_bio callbacks (static; p_ctx routes back to this)
    static int f_send(void *p_ctx, const unsigned char *buf, size_t len);
    static int f_recv(void *p_ctx, unsigned char *buf, size_t len);
    void set_state(SocketState state);
    void report_error(SocketError error, const std::string &message);

    EventLoop &mLoop;
    std::thread::id mLoopThreadId;
    TlsSocket *mP;

    SocketState mState = SocketState::kIdle;
    SocketError mLastError = SocketError::kNone;
    bool mCloseRequested = false;

    // user callbacks (invoked through local copies, PIT-40)
    std::function<void(bool ok)> mOnConnected; // one-shot: connect_tls / start_tls outcome
    std::function<void(const uint8_t *, ssize_t)> mOnData;
    std::function<void(SocketError, const std::string &)> mOnError;
    std::function<void(SocketState)> mOnStateChange;

    // ----- bio bridge (mirror of tests/tls_bio_bridge.hpp) -----
    /// C1 liveness token: every socket-callback lambda captures a weak_ptr to this; the dtor
    /// resets it, so a stale callback expires its token and touches nothing.
    std::shared_ptr<int> mAlive;
    std::deque<std::vector<uint8_t>> mOutBox; ///< ciphertext awaiting transport write
    std::vector<uint8_t> mInRing;             ///< ciphertext from the socket
    size_t mInRead = 0;                       ///< consumed prefix of mInRing
    bool mWriteInFlight = false;              ///< one out-box entry inside TcpSocket::write

    // ----- mbedTLS state -----
    mbedtls_x509_crt mCaChain;
    mbedtls_x509_crt mOwnCert;
    mbedtls_pk_context mOwnKey;
    mbedtls_entropy_context mEntropy;
    mbedtls_ctr_drbg_context mDrbg;
    mbedtls_ssl_config mConfig;
    mbedtls_ssl_context mSsl;
    bool mConfigured = false; ///< freeze_config() ran (config_defaults + CA + authmode)
    bool mSslReady = false;   ///< ssl_setup done (bio attached, hostname set)
    bool mDone = false;       ///< handshake terminal reached — latch guards re-drive (PIT-40 family)
    bool mTransportEof =
        false;                ///< transport reported EOF/error (post-handshake) — pump_plaintext converts to peer-close
    bool mRoleServer = false; ///< recorded by connect_tls (false) / start_tls (true)

    // pending plaintext writes (FIFO, TcpSocket lws shape): the HEAD entry owns the
    // same-argument WANT_* retry state; further write() calls queue behind it.
    struct PendingWrite
    {
        std::vector<uint8_t> mData;
        std::function<void(bool ok)> mOnWritten;
    };
    std::deque<PendingWrite> mPendingWrites;

    // deferred configuration (setters, frozen into the config at entry)
    int mVerifyMode = 2; ///< 0 = none, 1 = optional, 2 = required (MBEDTLS_SSL_VERIFY_*)
    std::string mCaPath;
    std::string mOwnCertPath; ///< SERVER role: presented identity (REQUIRED for start_tls)
    std::string mOwnKeyPath;
    std::string mHostname;

    std::unique_ptr<TcpSocket> mTransport; ///< CLIENT: composed; SERVER: injected
    /// True while a transport dtor (close + loop pump) owns the dispatcher — a nested
    /// process_events from the same call stack would trip the I5 re-entrancy fatal.
    bool mInDrain = false;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
