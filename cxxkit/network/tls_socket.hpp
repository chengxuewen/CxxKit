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

#include <cxxkit/base/global.hpp>
#include <cxxkit/kernel/event_loop.hpp> // defines the kernel guard this header gates on
#include <cxxkit/network/network_global.hpp>
#include <cxxkit/network/socket_error.hpp>
#include <cxxkit/network/socket_state.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class TcpSocket;
class TlsSocketPrivate;

/**
 * @brief TLS stream socket over a @ref TcpSocket transport (D42 T3) — Qt QSslSocket shape.
 *
 * Two entry points imply the endpoint role (F2: no TlsRole enum):
 *  - @ref connect_tls — CLIENT endpoint: composes an internal TcpSocket, connects, handshakes.
 *  - @ref start_tls   — SERVER endpoint: handshakes over an ALREADY-CONNECTED transport
 *                       injected with @ref set_transport (the accepted socket from a
 *                       TcpServer::on_connection).
 *
 * Lifecycle: kIdle → kConnecting (TCP connect + TLS handshake) → kConnected → kClosing → kClosed.
 * @ref set_verify_mode / @ref set_ca_path / @ref set_hostname MUST be called BEFORE
 * @ref connect_tls / @ref start_tls — the TLS configuration freezes when the handshake context
 * is created at entry; later setter calls have no effect on the running handshake.
 *
 * Payload API: @ref write encrypts and delivers ciphertext over the transport (partial-record
 * fragmentation and backpressure are absorbed internally); @ref read_start delivers decrypted
 * plaintext on the loop thread; a @c nread <= 0 terminal event means the TLS session ended
 * (clean close from the peer, or transport EOF).
 *
 * Threading: ALL methods are loop-thread only (fatal otherwise). Per-socket entropy + DRBG
 * (seeded in the ctor) — no TLS state is shared between sockets; every operation runs on the
 * socket's single loop thread. The class contains no third-party (TLS library) types on its
 * public surface.
 */
class CXXKIT_NETWORK_API TlsSocket
{
public:
    /**
     * @brief Creates a TLS socket bound to @p loop. CLIENT role composes its TcpSocket here.
     *
     * Loop thread only.
     */
    explicit TlsSocket(EventLoop &loop);

    /**
     * @brief Closes (if needed): best-effort close_notify to the peer, then transport close,
     *        then pumps the loop until the close callback ran. Loop thread only.
     */
    ~TlsSocket();

    /**
     * @brief CLIENT endpoint: non-blocking TCP connect to @p ip : @p port, then TLS handshake.
     *
     * @p on_connected fires once on the loop thread: @c true = handshake complete
     * (state kConnected); @c false = connect or handshake failure (the failure is also
     * reported through @ref set_on_error / @ref last_error). Legal from kIdle only.
     * Setter configuration (verify/CA/hostname) must precede this call.
     */
    void connect_tls(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_connected);

    /**
     * @brief SERVER endpoint: TLS handshake over the transport injected with @ref set_transport.
     *
     * @p on_handshake_done fires once on the loop thread with the handshake outcome.
     * Legal from kIdle only, AFTER set_transport with a connected socket; setter
     * configuration must precede this call.
     */
    void start_tls(std::function<void(bool ok)> on_handshake_done);

    /**
     * @brief Injects the ALREADY-CONNECTED transport (server accept path).
     *
     * Ownership moves into this TlsSocket; call before @ref start_tls, from kIdle. The
     * transport must be bound to the same loop. Loop thread only.
     */
    void set_transport(std::unique_ptr<TcpSocket> transport);

    /**
     * @brief Encrypts and queues @p len bytes for transmission; @p on_written fires once when
     *        the ciphertext has been handed to the transport (or with @c false on failure).
     *
     * @p data is copied before returning. Legal from kConnected. Loop thread only.
     */
    void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_written);

    /**
     * @brief Arms plaintext delivery; @p on_data fires on the loop thread with decrypted bytes.
     *
     * @c nread > 0 delivers the plaintext chunk (buffer valid only during the callback);
     * a null buffer with @c nread <= 0 is the terminal event (peer close / transport EOF) —
     * the session is finished afterwards. Loop thread only.
     */
    void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data);

    /** @brief Disarms plaintext delivery. Loop thread only. */
    void read_stop();

    /**
     * @brief Closes the TLS session: best-effort close_notify, then transport close.
     *        Idempotent. Pending writes complete with @c false. Loop thread only.
     */
    void close();

    /**
     * @brief Sets the error callback (loop thread, local-copy invocation — may close/destroy
     *        the socket). Re-setting replaces. Loop thread only.
     */
    void set_on_error(std::function<void(SocketError error, const std::string &message)> on_error);

    /** @brief Sets the state-change callback (kIdle/kConnecting/kConnected/kClosing/kClosed). */
    void set_on_state_change(std::function<void(SocketState state)> on_state_change);

    /**
     * @brief Peer certificate verification mode: 0 = none, 1 = optional (verify but continue),
     *        2 = required (default). MUST precede connect_tls/start_tls. Maps 1:1 onto the
     *        TLS library's verify modes.
     */
    void set_verify_mode(int mode);

    /**
     * @brief PEM file with the trust anchor(s) for peer verification. MUST precede
     *        connect_tls/start_tls.
     */
    void set_ca_path(const std::string &pem_path);

    /**
     * @brief PEM certificate chain this socket presents as its OWN identity (SERVER role —
     *        set_certificate + set_private_key are REQUIRED before start_tls; a TLS server
     *        without an identity cannot complete a handshake). Optional on the CLIENT role.
     *        MUST precede start_tls.
     */
    void set_certificate(const std::string &cert_pem_path);

    /**
     * @brief PEM private key matching @ref set_certificate. MUST precede start_tls when a
     *        certificate is set.
     */
    void set_private_key(const std::string &key_pem_path);

    /**
     * @brief Peer hostname for SNI + certificate verification (client role). MUST precede
     *        connect_tls/start_tls.
     */
    void set_hostname(const std::string &hostname);

    /** @brief Current machine state. Loop thread only. */
    SocketState state() const;

    /** @brief Last mapped error, kNone until the first failure (verify-optional failures land
     *         here without failing the handshake). Loop thread only. */
    SocketError last_error() const;

    /** @brief True while the session is not closing/closed (kIdle..kConnected). */
    bool is_open() const;

private:
    CXXKIT_DECLARE_PRIVATE(TlsSocket)
    CXXKIT_DEFINE_DPTR(TlsSocket)
    CXXKIT_DISABLE_COPY_MOVE(TlsSocket)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
