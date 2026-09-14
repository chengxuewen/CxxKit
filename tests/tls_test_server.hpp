/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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

// D42 T2 (R3): embedded mbedTLS SERVER peer for the TLS test suite. Accepts a TcpServer
// connection and drives an SSL handshake to completion over the shared TlsBioBridge
// (tls_bio_bridge.hpp — Momus F1), using the same WANT_* re-drive pattern Task 3's client
// mirrors. Each accepted connection gets a fully per-instance mbedtls_ssl_config (no globals:
// test isolation); the peer state dies after the handshake outcome is delivered.
//
// Threading: loop-thread only, end to end.

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/network/tcp_server.hpp>
#include <cxxkit/network/tcp_socket.hpp>
#include <cxxkit/network/socket_error.hpp>

#include "tls_bio_bridge.hpp"

#include <cxxkit/3rdparty/mbedtls/ssl.h>
#include <cxxkit/3rdparty/mbedtls/x509_crt.h>
#include <cxxkit/3rdparty/mbedtls/pk.h>
#include <cxxkit/3rdparty/mbedtls/entropy.h>
#include <cxxkit/3rdparty/mbedtls/ctr_drbg.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Loads a PEM certificate chain file into an initialized @c mbedtls_x509_crt (D42 T2).
 *
 * @return 0 on success, the mbedTLS error code otherwise (first error on partial parse; the
 * docs: a positive return means some certs failed to parse while the good ones still loaded).
 * Shared with Task 3's client peer.
 */
inline int tls_load_cert_chain(mbedtls_x509_crt *chain, const std::string &pem_path)
{
    return mbedtls_x509_crt_parse_file(chain, pem_path.c_str());
}

/**
 * @brief Loads a PEM private key file into an initialized @c mbedtls_pk_context (D42 T2).
 *
 * @return 0 on success, the mbedTLS error code otherwise. Shared with Task 3's client peer.
 */
inline int tls_load_private_key(mbedtls_pk_context *key, const std::string &pem_path)
{
    return mbedtls_pk_parse_keyfile(key, pem_path.c_str(), NULL, NULL, NULL);
}

/**
 * @brief Embedded mbedTLS test server: TcpServer accept -> server-side SSL handshake -> hand
 *        the (still open) socket to the test once TLS is ready (or report the failure).
 *
 * Usage: construct on the loop thread with the test CA/server PEM paths (tests run with
 * cwd = build dir — resolve absolute paths at the call site), start(), hand @c port() to a
 * client, set the ready callback before any accept. The handshake re-drive follows the D42
 * pattern: every WANT_READ/WANT_WRITE return parks the peer until the next on_data/on_written
 * trigger; return 0 fires @p on_ready(sock, true); a fatal error fires @p on_ready(sock, false).
 */
class TlsTestServer
{
public:
    /**
     * @brief Creates the listener shell. @p loop must be the loop this server lives on.
     *
     * @param ca_pem_path   trust root (conf_ca_chain) — tests/certs/ca-cert.pem
     * @param cert_pem_path own certificate chain (conf_own_cert) — tests/certs/server-cert.pem
     * @param key_pem_path  matching private key — tests/certs/server-key.pem
     */
    explicit TlsTestServer(EventLoop &loop,
                           const std::string &ca_pem_path,
                           const std::string &cert_pem_path,
                           const std::string &key_pem_path)
        : mListener(loop) // M3: mLoop member dropped — the loop flows into the listener only
        , mCaPath(ca_pem_path)
        , mCertPath(cert_pem_path)
        , mKeyPath(key_pem_path)
    {
    }

    ~TlsTestServer()
    {
        // Peer dtors free their ssl contexts (contexts must die before the certs/configs they
        // reference). The server dtor pumps the listener closed (TcpServer I6); pending peers
        // are unique_ptrs — destroyed right after here, sockets drain their own transports.
    }

    /** @brief Starts listening on 127.0.0.1:0 (ephemeral port) and arms the accept path.
     *  @return listen success. */
    bool start()
    {
        if (!mListener.listen("127.0.0.1", 0, 128))
        {
            return false;
        }
        mListener.on_connection([this](std::unique_ptr<TcpSocket> socket) { this->on_accepted(std::move(socket)); });
        return true;
    }

    /** @brief The port the listener actually bound (valid after a successful start()). */
    uint16_t port() const { return mListener.bound_port(); }

    /**
     * @brief Sets the one-shot-per-client callback: the accepted socket with @p ok = handshake
     *        outcome. On success the socket is open, TLS-established, and reads DISARMED (the
     *        test re-arms as it sees fit). The receiver takes ownership; dispose of it outside
     *        this callback (TcpServer on_connection contract).
     */
    void set_on_client_tls_ready(std::function<void(std::unique_ptr<TcpSocket>, bool ok)> on_ready)
    {
        mOnReady = std::move(on_ready);
    }

private:
    /// @brief Everything one accepted connection needs; freed as a unit after delivery/dtor.
    struct Peer
    {
        mbedtls_x509_crt mCa;
        mbedtls_x509_crt mCert;
        mbedtls_pk_context mKey;
        mbedtls_entropy_context mEntropy;
        mbedtls_ctr_drbg_context mDrbg;
        mbedtls_ssl_config mConfig;
        mbedtls_ssl_context mSsl;
        TlsBioBridge mBridge;
        std::unique_ptr<TcpSocket> mSocket;
        bool mDone = false; // handshake terminal state reached (delivered or about to be)

        Peer()
        {
            mbedtls_x509_crt_init(&mCa);
            mbedtls_x509_crt_init(&mCert);
            mbedtls_pk_init(&mKey);
            mbedtls_entropy_init(&mEntropy);
            mbedtls_ctr_drbg_init(&mDrbg);
            mbedtls_ssl_config_init(&mConfig);
            mbedtls_ssl_init(&mSsl);
        }

        ~Peer()
        {
            mbedtls_ssl_free(&mSsl);
            mbedtls_ssl_config_free(&mConfig);
            mbedtls_ctr_drbg_free(&mDrbg);
            mbedtls_entropy_free(&mEntropy);
            mbedtls_pk_free(&mKey);
            mbedtls_x509_crt_free(&mCert);
            mbedtls_x509_crt_free(&mCa);
        }
    };

    void on_accepted(std::unique_ptr<TcpSocket> socket)
    {
        std::unique_ptr<Peer> peer(new Peer);
        Peer *p = peer.get();

        int rc = mbedtls_ctr_drbg_seed(&p->mDrbg,
                                       mbedtls_entropy_func,
                                       &p->mEntropy,
                                       reinterpret_cast<const unsigned char *>("cxxkit-tls-test"),
                                       15); // strlen("cxxkit-tls-test")
        if (rc != 0)
        {
            this->fail(p, std::move(socket));
            return;
        }
        if (tls_load_cert_chain(&p->mCa, mCaPath) != 0 || tls_load_cert_chain(&p->mCert, mCertPath) != 0 ||
            tls_load_private_key(&p->mKey, mKeyPath) != 0)
        {
            this->fail(p, std::move(socket));
            return;
        }
        rc = mbedtls_ssl_config_defaults(&p->mConfig,
                                         MBEDTLS_SSL_IS_SERVER,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT);
        if (rc != 0)
        {
            this->fail(p, std::move(socket));
            return;
        }
        mbedtls_ssl_conf_rng(&p->mConfig, mbedtls_ctr_drbg_random, &p->mDrbg);
        mbedtls_ssl_conf_ca_chain(&p->mConfig, &p->mCa, NULL);
        rc = mbedtls_ssl_conf_own_cert(&p->mConfig, &p->mCert, &p->mKey);
        if (rc != 0)
        {
            this->fail(p, std::move(socket));
            return;
        }
        rc = mbedtls_ssl_setup(&p->mSsl, &p->mConfig);
        if (rc != 0)
        {
            this->fail(p, std::move(socket));
            return;
        }

        TlsBioBridge::set_bio(&p->mSsl, &p->mBridge);
        p->mSocket = std::move(socket);
        p->mBridge.attach(
            *p->mSocket,
            [this, p]() { this->drive_handshake(p); },
            [this, p]() { this->on_transport_error(p); });

        // Push the peer BEFORE the first drive (I2): a synchronous terminal (immediate fatal) or
        // even a same-call success inside drive_handshake erases from mPeers — the entry must
        // exist for erase_peer to find, or the Peer would leak and any later state would dangle.
        mPeers.push_back(std::move(peer));
        this->drive_handshake(p); // first kick (usually parks on WANT_READ for the ClientHello)
    }

    void drive_handshake(Peer *p)
    {
        while (true)
        {
            const int rc = mbedtls_ssl_handshake(&p->mSsl);
            if (rc == 0)
            {
                // Handshake complete: disarm reads (the test re-arms per its scenario), clear
                // the bridge-capturing error lambda (the Peer dies here — the socket outlives
                // it in test ownership), then deliver.
                p->mSocket->read_stop();
                p->mSocket->set_on_error(std::function<void(SocketError, const std::string &)>());
                p->mDone = true;
                std::unique_ptr<TcpSocket> socket = std::move(p->mSocket);
                this->erase_peer(p);
                if (mOnReady)
                {
                    mOnReady(std::move(socket), true);
                }
                return;
            }
            if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                return; // parked: the next on_data / on_written re-drives
            }
            // Fatal (alert / cert mismatch / protocol garbage): deliver failure once.
            this->fail_handshake(p, rc);
            return;
        }
    }

    void on_transport_error(Peer *p)
    {
        // Socket EOF/transport failure or a failed bridge write: the handshake can never
        // complete — report failure (unless already delivered) and drop the peer.
        if (p->mDone)
        {
            return;
        }
        this->fail_handshake(p, MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY);
    }

    void fail_handshake(Peer *p, int mbedtls_rc)
    {
        p->mDone = true;
        std::unique_ptr<TcpSocket> socket = std::move(p->mSocket);
        if (socket)
        {
            socket->close(); // dtor would pump the loop mid-callback — close() only (I5)
            // Clear the bridge-capturing error lambda: the Peer dies below, the socket does not.
            socket->set_on_error(std::function<void(SocketError, const std::string &)>());
        }
        this->erase_peer(p);
        if (mOnReady)
        {
            mOnReady(std::move(socket), false);
        }
    }

    void fail(Peer *p, std::unique_ptr<TcpSocket> socket)
    {
        // Synchronous setup failure (seed/cert/config error): close the transport, deliver the
        // failure, drop the peer (erase_peer finds it because on_accepted pushes every peer
        // before any terminal can run — I2).
        socket->close();
        this->erase_peer(p);
        if (mOnReady)
        {
            mOnReady(std::move(socket), false);
        }
    }

    void erase_peer(Peer *p)
    {
        for (std::vector<std::unique_ptr<Peer>>::iterator it = mPeers.begin(); it != mPeers.end(); ++it)
        {
            if (it->get() == p)
            {
                mPeers.erase(it); // destroys the Peer: ssl context first, then configs/certs
                return;
            }
        }
    }

private:
    TcpServer mListener;
    std::string mCaPath;
    std::string mCertPath;
    std::string mKeyPath;
    std::function<void(std::unique_ptr<TcpSocket>, bool ok)> mOnReady;
    std::vector<std::unique_ptr<Peer>> mPeers;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
