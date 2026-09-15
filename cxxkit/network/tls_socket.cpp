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

#include <cxxkit/network/detail/tls_error_mapping.hpp>
#include <cxxkit/network/detail/tls_socket_p.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>
#include <cxxkit/network/tls_socket.hpp>

#include <cxxkit/tools/checks.hpp>

#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

TlsSocketPrivate::TlsSocketPrivate(TlsSocket *p, EventLoop &loop)
    : mLoop(loop)
    , mLoopThreadId(std::this_thread::get_id()) // construction is loop-thread only; pin the check id
    , mP(p)
    , mAlive(new int(1))
{
    mbedtls_x509_crt_init(&mCaChain);
    mbedtls_x509_crt_init(&mOwnCert);
    mbedtls_pk_init(&mOwnKey);
    mbedtls_entropy_init(&mEntropy);
    mbedtls_ctr_drbg_init(&mDrbg);
    mbedtls_ssl_config_init(&mConfig);
    mbedtls_ssl_init(&mSsl);
    mbedtls_ctr_drbg_seed(&mDrbg,
                          mbedtls_entropy_func,
                          &mEntropy,
                          reinterpret_cast<const unsigned char *>("cxxkit-tls"),
                          11);
}

TlsSocketPrivate::~TlsSocketPrivate()
{
    // Expire the liveness token FIRST: any in-flight transport completion after here must
    // touch nothing (C1). The transport dtor closes and pumps the loop until drained —
    // mInDrain suppresses our nested-pump guard during that drain (the drain IS the pump).
    mAlive.reset();
    mInDrain = true;
    mTransport.reset(); // TcpSocket dtor: close + pump_until_closed (I6)
    mInDrain = false;
    mbedtls_ssl_free(&mSsl);
    mbedtls_ssl_config_free(&mConfig);
    mbedtls_ctr_drbg_free(&mDrbg);
    mbedtls_entropy_free(&mEntropy);
    mbedtls_pk_free(&mOwnKey);
    mbedtls_x509_crt_free(&mOwnCert);
    mbedtls_x509_crt_free(&mCaChain);
}

void TlsSocketPrivate::check_loop_thread(const char *api) const
{
    if (std::this_thread::get_id() != mLoopThreadId)
    {
        CXXKIT_FATAL() << "TlsSocket::" << api << " called from a non-loop thread";
    }
}

void TlsSocketPrivate::set_state(SocketState state)
{
    mState = state;
    if (mOnStateChange)
    {
        std::function<void(SocketState)> cb = mOnStateChange; // PIT-40: local copy
        cb(state);
    }
}

void TlsSocketPrivate::report_error(SocketError error, const std::string &message)
{
    mLastError = error;
    if (mOnError)
    {
        std::function<void(SocketError, const std::string &)> cb = mOnError; // PIT-40: local copy
        cb(error, message);
    }
}

void TlsSocketPrivate::set_transport(std::unique_ptr<TcpSocket> transport)
{
    check_loop_thread("set_transport");
    CXXKIT_CHECK(mState == SocketState::kIdle) << "TlsSocket::set_transport: state must be kIdle";
    CXXKIT_CHECK(transport != nullptr) << "TlsSocket::set_transport: null transport";
    mTransport = std::move(transport);
}

void TlsSocketPrivate::set_verify_mode(int mode)
{
    check_loop_thread("set_verify_mode");
    mVerifyMode = mode;
}

void TlsSocketPrivate::set_ca_path(const std::string &pem_path)
{
    check_loop_thread("set_ca_path");
    mCaPath = pem_path;
}

void TlsSocketPrivate::set_hostname(const std::string &hostname)
{
    check_loop_thread("set_hostname");
    mHostname = hostname;
}

void TlsSocketPrivate::set_certificate(const std::string &cert_pem_path)
{
    check_loop_thread("set_certificate");
    mOwnCertPath = cert_pem_path;
}

void TlsSocketPrivate::set_private_key(const std::string &key_pem_path)
{
    check_loop_thread("set_private_key");
    mOwnKeyPath = key_pem_path;
}

void TlsSocketPrivate::connect_tls(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected)
{
    check_loop_thread("connect_tls");
    CXXKIT_CHECK(on_connected != nullptr) << "TlsSocket::connect_tls requires a connected callback";
    CXXKIT_CHECK(mState == SocketState::kIdle) << "TlsSocket::connect_tls: state must be kIdle";
    CXXKIT_CHECK(!mTransport) << "TlsSocket::connect_tls: transport already injected (server path?)";
    CXXKIT_CHECK(!mCloseRequested) << "TlsSocket::connect_tls: socket is closing/closed";

    // CLIENT role (F2): role is implied by the entry point. Freeze the configuration now —
    // setters after this call have no effect on the running handshake.
    if (!mTransport)
    {
        mTransport.reset(new TcpSocket(mLoop));
    }

    mOnConnected = std::move(on_connected);
    mDone = false;
    mRoleServer = false; // connect_tls = CLIENT endpoint (F2)
    this->set_state(SocketState::kConnecting);

    TcpSocket *transport = mTransport.get();
    transport->connect(ip,
                       port,
                       [this](bool ok)
                       {
                           if (!ok)
                           {
                               // TCP connect failed: the transport reports its own error; the TLS
                               // machine just fails the entry callback and closes.
                               std::function<void(bool ok)> cb = std::move(mOnConnected);
                               mOnConnected = nullptr;
                               if (cb)
                               {
                                   cb(false);
                               }
                               this->begin_close();
                               return;
                           }
                           if (!mCloseRequested)
                           {
                               this->attach_bridge();
                               this->drive_handshake();
                           }
                       });
}

void TlsSocketPrivate::start_tls(std::function<void(bool ok)> on_handshake_done)
{
    check_loop_thread("start_tls");
    CXXKIT_CHECK(on_handshake_done != nullptr) << "TlsSocket::start_tls requires a handshake callback";
    CXXKIT_CHECK(mState == SocketState::kIdle) << "TlsSocket::start_tls: state must be kIdle";
    CXXKIT_CHECK(mTransport != nullptr) << "TlsSocket::start_tls: inject the connected transport first (set_transport)";

    // SERVER role (F2): role implied by the entry; freeze + attach + first drive.
    mOnConnected = std::move(on_handshake_done);
    mDone = false;
    mRoleServer = true; // start_tls = SERVER endpoint
    this->set_state(SocketState::kConnecting);
    this->attach_bridge();
    this->drive_handshake();
}

void TlsSocketPrivate::freeze_config()
{
    if (mConfigured)
    {
        return;
    }
    mConfigured = true;

    // Role recorded by the entry point (F2: connect_tls = CLIENT, start_tls = SERVER).
    const int rc_defaults = mbedtls_ssl_config_defaults(&mConfig,
                                                        mRoleServer ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
                                                        MBEDTLS_SSL_TRANSPORT_STREAM,
                                                        MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc_defaults != 0)
    {
        this->finish_error(network::detail::map_tls_error(rc_defaults),
                           "TlsSocket: config defaults failed (mbedtls rc " + std::to_string(rc_defaults) + ")",
                           true);
        return;
    }
    mbedtls_ssl_conf_rng(&mConfig, mbedtls_ctr_drbg_random, &mDrbg);
    mbedtls_ssl_conf_authmode(&mConfig,
                              mVerifyMode == 0   ? MBEDTLS_SSL_VERIFY_NONE
                              : mVerifyMode == 1 ? MBEDTLS_SSL_VERIFY_OPTIONAL
                                                 : MBEDTLS_SSL_VERIFY_REQUIRED);
    if (!mCaPath.empty())
    {
        if (mbedtls_x509_crt_parse_file(&mCaChain, mCaPath.c_str()) != 0)
        {
            this->finish_error(SocketError::kTlsCertificateError,
                               "TlsSocket: failed to parse CA file: " + mCaPath,
                               true);
            return;
        }
        mbedtls_ssl_conf_ca_chain(&mConfig, &mCaChain, NULL);
    }
    if (!mOwnCertPath.empty() && !mOwnKeyPath.empty())
    {
        if (mbedtls_x509_crt_parse_file(&mOwnCert, mOwnCertPath.c_str()) != 0 ||
            mbedtls_pk_parse_keyfile(&mOwnKey, mOwnKeyPath.c_str(), NULL, NULL, NULL) != 0)
        {
            this->finish_error(SocketError::kTlsCertificateError,
                               "TlsSocket: failed to parse own certificate/key",
                               true);
            return;
        }
        if (mbedtls_ssl_conf_own_cert(&mConfig, &mOwnCert, &mOwnKey) != 0)
        {
            this->finish_error(SocketError::kTlsHandshakeFailed,
                               "TlsSocket: conf_own_cert failed (key/cert mismatch?)",
                               true);
            return;
        }
    }
}

void TlsSocketPrivate::attach_bridge()
{
    if (mSslReady || mCloseRequested || mTransport == nullptr)
    {
        return;
    }
    this->freeze_config();
    if (mCloseRequested)
    {
        return; // freeze_config hit a synchronous failure (or the user closed during connect):
                // finish_error/begin_close already ran — the bridge must NOT be attached to the
                // now-closing transport (read_start would trip the transport's closing CHECK).
                // Covers the old !mConfigured intent too: every freeze_config failure path goes
                // through finish_error -> begin_close, which latches mCloseRequested.
    }
    if (mbedtls_ssl_setup(&mSsl, &mConfig) != 0)
    {
        this->finish_error(SocketError::kTlsHandshakeFailed, "TlsSocket: ssl_setup failed", true);
        return;
    }
    mSslReady = true;
    if (!mHostname.empty())
    {
        mbedtls_ssl_set_hostname(&mSsl, mHostname.c_str()); // SNI + verify target (client role)
    }
    mbedtls_ssl_set_bio(&mSsl, this, &TlsSocketPrivate::f_send, &TlsSocketPrivate::f_recv, NULL);

    // Arm the always-on read (level-triggered into the in-ring) + error surface — the same
    // wiring as the shared test bridge, liveness-token first (C1).
    std::weak_ptr<int> alive = mAlive;
    mTransport->set_on_error(
        [this, alive](SocketError error, const std::string &message)
        {
            if (alive.expired())
            {
                return; // socket destroyed: its dtor owns the teardown now
            }
            (void)error;
            (void)message;
            if (!mDone)
            {
                this->finish_error(SocketError::kEof, "TlsSocket: transport EOF/error", true);
            }
            else if (mState == SocketState::kConnected)
            {
                // Post-handshake transport EOF: surface as peer-close through the read pump.
                mTransportEof = true;
                this->on_transport_terminal();
            }
        });
    mTransport->read_start(
        [this, alive](const uint8_t *data, ssize_t nread)
        {
            if (alive.expired())
            {
                return; // terminal event after destruction — touch nothing
            }
            if (nread > 0)
            {
                mInRing.insert(mInRing.end(), data, data + nread);
                this->on_redrive();
                // Compact the consumed ring prefix AFTER the re-drive returns (f_recv only
                // runs inside it — the erase cannot invalidate a live window; T2 I1).
                if (mInRead > 0)
                {
                    mInRing.erase(mInRing.begin(), mInRing.begin() + static_cast<std::ptrdiff_t>(mInRead));
                    mInRead = 0;
                }
            }
            // nread <= 0: transport EOF terminal — routed through set_on_error already.
        });
}

void TlsSocketPrivate::drive_handshake()
{
    // Step the handshake; WANT_* parks until the next on_written/on_data re-drive (the bio
    // bridge owns those triggers). 0 -> kConnected + callback(true); fatal -> report + close.
    while (!mDone)
    {
        const int rc = mbedtls_ssl_handshake(&mSsl);
        if (rc == 0)
        {
            mDone = true;
            // Success also ends the write pump's re-drive duties — flush any ciphertext still
            // queued so the peer never sees a truncated final flight.
            this->pump_write();
            this->set_state(SocketState::kConnected);
            // verify-optional: surface the verification outcome without failing the handshake
            if (mVerifyMode == 1)
            {
                const uint32_t flags = mbedtls_ssl_get_verify_result(&mSsl);
                if (flags != 0)
                {
                    mLastError = SocketError::kTlsCertificateError;
                }
            }
            std::function<void(bool ok)> cb = std::move(mOnConnected);
            mOnConnected = nullptr;
            if (cb)
            {
                cb(true);
            }
            this->pump_plaintext(); // any plaintext that arrived with the final flight
            return;
        }
        if (network::detail::tls_want_retry(rc))
        {
            return; // parked: WANT_READ (ring empty) or WANT_WRITE (out-box busy)
        }
        // Fatal handshake failure: map + report + close.
        this->finish_error(network::detail::map_tls_error(rc),
                           "TlsSocket: handshake failed (mbedtls rc " + std::to_string(rc) + ")",
                           true);
        return;
    }
}

void TlsSocketPrivate::pump_plaintext()
{
    // Drains decrypted bytes from the SSL context to mOnData. Runs on every re-drive trigger.
    while (mOnData && mState == SocketState::kConnected && !mCloseRequested)
    {
        // One full TLS record: MBEDTLS_SSL_OUT_CONTENT_LEN (16KB) + overhead. ssl_read
        // returns MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL for a smaller buffer — the record stays
        // buffered and the next call re-reads it.
        if (mTransportEof)
        {
            // Transport EOF under TLS: without new ciphertext ssl_read would park forever —
            // surface the peer-close terminal directly.
            std::function<void(const uint8_t *, ssize_t)> cb = mOnData;
            cb(nullptr, 0);
            mLastError = SocketError::kTlsPeerClosed;
            if (mOnError)
            {
                std::function<void(SocketError, const std::string &)> ecb = mOnError;
                ecb(SocketError::kTlsPeerClosed, "TlsSocket: transport EOF under TLS");
            }
            this->begin_close();
            return;
        }
        uint8_t buffer[MBEDTLS_SSL_OUT_CONTENT_LEN + 256];
        const int rc = mbedtls_ssl_read(&mSsl, buffer, sizeof(buffer));
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            return; // no more plaintext now (WANT_WRITE = renegotiation — re-drive on on_written)
        }
        if (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || rc == 0)
        {
            // Clean TLS shutdown from the peer: terminal on_data + kTlsPeerClosed, then close.
            std::function<void(const uint8_t *, ssize_t)> cb = mOnData;
            cb(nullptr, 0);
            mLastError = SocketError::kTlsPeerClosed;
            if (mOnError)
            {
                std::function<void(SocketError, const std::string &)> ecb = mOnError;
                ecb(SocketError::kTlsPeerClosed, "TlsSocket: peer closed the TLS session");
            }
            this->begin_close();
            return;
        }
        if (rc == MBEDTLS_ERR_SSL_CONN_EOF)
        {
            // Transport EOF under TLS without close_notify: treat as clean-close-adjacent —
            // report kTlsPeerClosed (the session cannot continue either way).
            std::function<void(const uint8_t *, ssize_t)> cb = mOnData;
            cb(nullptr, 0);
            mLastError = SocketError::kTlsPeerClosed;
            if (mOnError)
            {
                std::function<void(SocketError, const std::string &)> ecb = mOnError;
                ecb(SocketError::kTlsPeerClosed, "TlsSocket: transport EOF under TLS");
            }
            this->begin_close();
            return;
        }
        if (rc == MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL)
        {
            return; // caller buffer smaller than the pending record — cannot happen with the
                    // record-sized buffer above, but never treat it as fatal
        }
        if (rc < 0)
        {
            this->finish_error(network::detail::map_tls_error(rc),
                               "TlsSocket: read failed (mbedtls rc " + std::to_string(rc) + ")",
                               false);
            return;
        }
        // rc > 0: deliver the plaintext chunk. The callback may close() — re-check the state.
        std::function<void(const uint8_t *, ssize_t)> cb = mOnData;
        cb(buffer, rc);
    }
}

void TlsSocketPrivate::pump_write()
{
    if (!mSslReady || mTransport == nullptr || mWriteInFlight || mOutBox.empty())
    {
        return;
    }
    mWriteInFlight = true;
    const std::vector<uint8_t> &front = mOutBox.front();
    std::weak_ptr<int> alive = mAlive; // C1: token survives bridge destruction
    mTransport->write(front.data(),
                      front.size(),
                      [this, alive](bool ok)
                      {
                          if (alive.expired())
                          {
                              return; // socket died mid-flight: teardown already handled the rest
                          }
                          if (mCloseRequested)
                          {
                              // Our own close() is mid-teardown (this very callback stack came
                              // from its fanout or from the transport drain). Touch NOTHING —
                              // especially not mOutBox: begin_close() clears/destroys it right
                              // after mTransport->close() returns, and this lambda frame sits
                              // between the two.
                              return;
                          }
                          mWriteInFlight = false;
                          if (!ok)
                          {
                              this->finish_error(SocketError::kEof, "TlsSocket: transport write failed", false);
                              return;
                          }
                          mOutBox.pop_front();
                          this->pump_write(); // submit the next queued entry
                          this->on_redrive();
                      });
}

/// @brief The ONLY re-drive entry (F1 contract): every progress trigger funnels here.
/// Order: handshake step, pending plaintext write, decrypted-data drain.
void TlsSocketPrivate::on_redrive()
{
    if (mCloseRequested)
    {
        return;
    }
    if (!mDone)
    {
        this->drive_handshake();
        if (mCloseRequested)
        {
            return; // handshake failure closed inside drive_handshake
        }
    }
    if (mDone && mState == SocketState::kConnected)
    {
        if (!mPendingWrites.empty())
        {
            this->pump_plaintext_write();
        }
        this->pump_plaintext();
    }
}

/// @brief Post-handshake transport EOF: mbedTLS reads it back as CONN_EOF/PEER_CLOSE_NOTIFY
/// from the ring, so just re-drive — pump_plaintext converts it to the terminal event.
void TlsSocketPrivate::on_transport_terminal()
{
    this->on_redrive();
}

// ----- mbedtls_ssl_set_bio callbacks (static; p_ctx routes back to this) -----

int TlsSocketPrivate::f_send(void *p_ctx, const unsigned char *buf, size_t len)
{
    TlsSocketPrivate *self = static_cast<TlsSocketPrivate *>(p_ctx);
    if (self->mWriteInFlight || !self->mOutBox.empty())
    {
        return MBEDTLS_ERR_SSL_WANT_WRITE; // mbedTLS retries the SAME buffer later
    }
    // mbedTLS fragments to MBEDTLS_SSL_OUT_CONTENT_LEN (~16KB) — the int cast is safe (M1).
    // The buffer is only valid inside f_send — the copy IS the contract.
    self->mOutBox.push_back(std::vector<uint8_t>(buf, buf + len));
    self->pump_write(); // submit ONE entry immediately; completion pops + re-drives
    return static_cast<int>(len);
}

int TlsSocketPrivate::f_recv(void *p_ctx, unsigned char *buf, size_t len)
{
    TlsSocketPrivate *self = static_cast<TlsSocketPrivate *>(p_ctx);
    if (self->mInRead >= self->mInRing.size())
    {
        return MBEDTLS_ERR_SSL_WANT_READ; // empty ring: wait for the next on_data
    }
    const size_t available = self->mInRing.size() - self->mInRead;
    const size_t n = (available < len) ? available : len;
    for (size_t i = 0; i < n; ++i)
    {
        buf[i] = self->mInRing[self->mInRead + i];
    }
    self->mInRead += n;
    return static_cast<int>(n);
}

void TlsSocketPrivate::pump_plaintext_write()
{
    // Drains the pending-write FIFO one record at a time. The HEAD entry owns the
    // same-argument WANT_* retry state (mbedTLS: after a partial flush, retry with the SAME
    // remainder; a completed rc>0 advances the remainder). Only re-drive triggers call this.
    while (!mPendingWrites.empty() && mState == SocketState::kConnected && !mCloseRequested)
    {
        PendingWrite &head = mPendingWrites.front();
        if (head.mData.empty())
        {
            std::function<void(bool ok)> cb = std::move(head.mOnWritten);
            mPendingWrites.pop_front();
            if (cb)
            {
                cb(true); // zero-length write: trivially done
            }
            continue;
        }
        const int rc = mbedtls_ssl_write(&mSsl, head.mData.data(), head.mData.size());
        if (network::detail::tls_want_retry(rc))
        {
            return; // stay pending; the next on_written/on_data re-drives
        }
        if (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || rc == 0)
        {
            std::function<void(bool ok)> cb = std::move(head.mOnWritten);
            mPendingWrites.clear();
            if (cb)
            {
                cb(false);
            }
            mLastError = SocketError::kTlsPeerClosed;
            this->begin_close();
            return;
        }
        if (rc < 0)
        {
            std::function<void(bool ok)> cb = std::move(head.mOnWritten);
            mPendingWrites.clear();
            if (cb)
            {
                cb(false);
            }
            this->finish_error(network::detail::map_tls_error(rc),
                               "TlsSocket: write failed (mbedtls rc " + std::to_string(rc) + ")",
                               false);
            return;
        }
        // rc > 0: partial or full flush of the head entry; advance the remainder.
        if (static_cast<size_t>(rc) >= head.mData.size())
        {
            std::function<void(bool ok)> cb = std::move(head.mOnWritten);
            mPendingWrites.pop_front();
            if (cb)
            {
                cb(true);
            }
            continue;
        }
        head.mData.erase(head.mData.begin(), head.mData.begin() + rc);
        return; // remainder written on the next re-drive (same-buffer semantics)
    }
}

void TlsSocketPrivate::write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written)
{
    check_loop_thread("write");
    CXXKIT_CHECK(on_written != nullptr) << "TlsSocket::write requires a written callback";
    CXXKIT_CHECK(data != nullptr || len == 0) << "TlsSocket::write: null data with non-zero length";
    CXXKIT_CHECK(mState == SocketState::kConnected && !mCloseRequested)
        << "TlsSocket::write: state must be kConnected (got " << static_cast<int>(mState) << ")";

    PendingWrite pending;
    pending.mData.assign(data, data + len);
    pending.mOnWritten = std::move(on_written);
    mPendingWrites.push_back(std::move(pending));
    this->pump_plaintext_write();
    this->pump_plaintext(); // writes can surface peer data (renegotiation) — drain
}

void TlsSocketPrivate::read_start(std::function<void(const uint8_t *, ssize_t)> on_data)
{
    check_loop_thread("read_start");
    CXXKIT_CHECK(on_data != nullptr) << "TlsSocket::read_start requires a data callback";
    mOnData = std::move(on_data);
    this->pump_plaintext(); // drain whatever decrypted payload is already buffered
}

void TlsSocketPrivate::read_stop()
{
    check_loop_thread("read_stop");
    mOnData = nullptr;
}

void TlsSocketPrivate::finish_error(SocketError error, const std::string &message, bool fatal_handshake)
{
    (void)fatal_handshake;
    report_error(error, message);
    // Deliver the entry callback (false) if the handshake was still in flight.
    if (!mDone)
    {
        mDone = true;
        std::function<void(bool ok)> cb = std::move(mOnConnected);
        mOnConnected = nullptr;
        if (cb)
        {
            cb(false);
        }
    }
    this->begin_close();
}

void TlsSocketPrivate::begin_close()
{
    if (mCloseRequested)
    {
        return; // idempotent
    }
    mCloseRequested = true;
    if (mState != SocketState::kClosed)
    {
        this->set_state(SocketState::kClosing);
    }
    // Best-effort close_notify: queue the alert through the live bridge — but ONLY while
    // the transport has not started its own close (its write path is dead after close() and
    // would bounce us right back into this teardown).
    if (mSslReady && mTransport != nullptr && mTransport->is_open())
    {
        (void)mbedtls_ssl_close_notify(&mSsl); // queues onto mOutBox via f_send
    }
    // close_notify handling: if close_notify was accepted by the bridge (box entry exists
    // and NOT yet in flight), forward that one record DIRECTLY below. If it is already
    // in flight (submitted through pump_write inside close_notify), the pump_write
    // completion path performs the deferred transport close instead.
    std::vector<uint8_t> notify;
    if (!mOutBox.empty() && mSslReady && !mWriteInFlight)
    {
        notify = std::move(mOutBox.front()); // the close_notify record (queued above)
    }
    mOutBox.clear();
    // Fan out remaining pending writes with false.
    while (!mPendingWrites.empty())
    {
        std::function<void(bool ok)> cb = std::move(mPendingWrites.front().mOnWritten);
        mPendingWrites.pop_front();
        if (cb)
        {
            cb(false);
        }
    }
    if (mTransport)
    {
        // Drop the bridge-capturing callbacks FIRST: our own close() may be running from
        // inside a transport callback; a backend fanout onto those lambdas mid-close must
        // not re-enter.
        mTransport->set_on_error(std::function<void(SocketError, const std::string &)>());
        mTransport->read_stop();
        if (!notify.empty())
        {
            // close_notify captured: send it DIRECTLY (bypassing the dead bridge) and close
            // the transport only after its completion — the peer observes a clean TLS close.
            std::weak_ptr<int> alive = mAlive;
            mTransport->write(notify.data(),
                              notify.size(),
                              [this, alive](bool)
                              {
                                  if (alive.expired() || !mCloseRequested || mTransport == nullptr)
                                  {
                                      return;
                                  }
                                  mTransport->close();
                                  this->set_state(SocketState::kClosed); // the deferred path reports closure here
                              });
            return; // kClosed lands via the notify-completion path (or our dtor)
        }
        mTransport->close(); // no notify: latched + asynchronous; NO loop pump here (the pump
                             // from inside a callback is the I5 nested-iteration fatal). The
                             // handle dies at our dtor's transport teardown (I6).
    }
    this->set_state(SocketState::kClosed);
}

void TlsSocketPrivate::close()
{
    check_loop_thread("close");
    if (mState == SocketState::kClosed || mCloseRequested)
    {
        return;
    }
    this->begin_close();
}

SocketState TlsSocketPrivate::state() const
{
    return mState;
}

SocketError TlsSocketPrivate::last_error() const
{
    return mLastError;
}

bool TlsSocketPrivate::is_open() const
{
    return !mCloseRequested && mState != SocketState::kClosed;
}

// ----- public API forwarding -----

TlsSocket::TlsSocket(EventLoop &loop)
{
    mDPtr.reset(new TlsSocketPrivate(this, loop));
}

TlsSocket::~TlsSocket()
{
    // TlsSocketPrivate dtor: close_notify best-effort + transport close + pump (I6 shape).
}

void TlsSocket::connect_tls(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected)
{
    CXXKIT_D(TlsSocket);
    d->connect_tls(ip, port, std::move(on_connected));
}

void TlsSocket::start_tls(std::function<void(bool ok)> on_handshake_done)
{
    CXXKIT_D(TlsSocket);
    d->start_tls(std::move(on_handshake_done));
}

void TlsSocket::set_transport(std::unique_ptr<TcpSocket> transport)
{
    CXXKIT_D(TlsSocket);
    d->set_transport(std::move(transport));
}

void TlsSocket::write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written)
{
    CXXKIT_D(TlsSocket);
    d->write(data, len, std::move(on_written));
}

void TlsSocket::read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data)
{
    CXXKIT_D(TlsSocket);
    d->read_start(std::move(on_data));
}

void TlsSocket::read_stop()
{
    CXXKIT_D(TlsSocket);
    d->read_stop();
}

void TlsSocket::close()
{
    CXXKIT_D(TlsSocket);
    d->close();
}

void TlsSocket::set_on_error(std::function<void(SocketError, const std::string &)> on_error)
{
    CXXKIT_D(TlsSocket);
    d->check_loop_thread("set_on_error");
    d->mOnError = std::move(on_error);
}

void TlsSocket::set_on_state_change(std::function<void(SocketState)> on_state_change)
{
    CXXKIT_D(TlsSocket);
    d->check_loop_thread("set_on_state_change");
    d->mOnStateChange = std::move(on_state_change);
}

void TlsSocket::set_verify_mode(int mode)
{
    CXXKIT_D(TlsSocket);
    d->set_verify_mode(mode);
}

void TlsSocket::set_ca_path(const std::string &pem_path)
{
    CXXKIT_D(TlsSocket);
    d->set_ca_path(pem_path);
}

void TlsSocket::set_hostname(const std::string &hostname)
{
    CXXKIT_D(TlsSocket);
    d->set_hostname(hostname);
}

void TlsSocket::set_certificate(const std::string &cert_pem_path)
{
    CXXKIT_D(TlsSocket);
    d->set_certificate(cert_pem_path);
}

void TlsSocket::set_private_key(const std::string &key_pem_path)
{
    CXXKIT_D(TlsSocket);
    d->set_private_key(key_pem_path);
}

SocketState TlsSocket::state() const
{
    CXXKIT_D(const TlsSocket);
    return d->state();
}

SocketError TlsSocket::last_error() const
{
    CXXKIT_D(const TlsSocket);
    return d->last_error();
}

bool TlsSocket::is_open() const
{
    CXXKIT_D(const TlsSocket);
    return d->is_open();
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
