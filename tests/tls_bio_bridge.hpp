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

// D42 T2, Momus F1: the mbedTLS <-> TcpSocket bio bridge, shared verbatim by the test server peer
// (tls_test_server.hpp) and Task 3's TlsSocket pimpl. THIS SHAPE IS THE CONTRACT:
//
//  f_send: SYNCHRONOUS copy of the ciphertext into the out-box, return (int)len immediately.
//          When the out-box already holds an entry, or one transport write is in flight,
//          return MBEDTLS_ERR_SSL_WANT_WRITE — mbedTLS retries the SAME buffer later.
//          Never submit a second TcpSocket::write while one entry is in flight.
//  drain:  ONE out-box entry at a time through TcpSocket::write. on_written(true) pops the
//          entry, submits the next if any, then re-drives the owner's hook (handshake / read
//          pump). on_written(false) is the transport-error path (report + close).
//  f_recv: serves synchronously from the ciphertext in-ring; empty ring -> WANT_READ.
//          read_start stays ARMED always (level-triggered); on_data appends to the ring and
//          re-drives the hook after each push.
//  All backpressure is expressed through WANT_*; the ONLY re-drive triggers are on_written
//  and on_data. The bridge owns NOTHING about SSL state — pure transport.
//
// Threading: loop-thread only end to end (TcpSocket contract), so the deque/flag/ring need
// no synchronization.

#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/network/tcp_socket.hpp>

#include <cxxkit/3rdparty/mbedtls/ssl.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief mbedTLS BIO adapter over a @ref TcpSocket for event-loop-driven SSL (D42 T2).
 *
 * Attach after the transport connects, before the first @c mbedtls_ssl_handshake() call. The
 * owner (TlsTestServer peer / Task 3's TlsSocket) keeps the SSL context and re-drives it from
 * @p on_redrive whenever the bridge surfaces a progress trigger (bytes written or bytes read).
 * Detach/destroy the bridge before the socket.
 */
struct TlsBioBridge
{
    TlsBioBridge()
        : mAlive(new int(1))
    {
    }
    ~TlsBioBridge() { mAlive.reset(); } // expires every outstanding weak token (C1 fix)


    /// @brief Liveness token (C1 fix): every socket-callback lambda that captures this also
    /// captures a weak_ptr to mAlive; the bridge dtor resets mAlive, so a callback that
    /// outlives the bridge (in-flight write completion, level-triggered read event) expires
    /// its token and returns without touching a single member. TcpSocket has no
    /// flush/cancel API for a queued write — the token is the only safe answer.
    std::shared_ptr<int> mAlive;

    /// @brief Ciphertext out-box entry: bytes mbedTLS handed f_send, waiting for transport write.
    std::deque<std::vector<uint8_t>> mOutBox;
    /// @brief Ciphertext in-ring: bytes from the socket, read by f_recv up to mInRead offset.
    std::vector<uint8_t> mInRing;
    /// @brief Bytes of mInRing already consumed by f_recv (everything before is dead weight).
    size_t mInRead = 0;
    /// @brief True while one out-box entry is inside TcpSocket::write awaiting on_written.
    bool mWriteInFlight = false;
    /// @brief Owner re-drive hook: handshake step / deferred read pump (set at attach).
    std::function<void()> mOnRedrive;
    /// @brief Transport failure path (on_written(false)): report + close, owner-supplied.
    std::function<void()> mOnTransportError;
    /**
     * @brief Arms the socket: error reporting + always-armed level-triggered read into the ring.
     *
     * @p on_redrive runs after every ring append (and after each out-box write completes);
     * @p on_transport_error runs once on socket error or write failure. Idempotent: a later
     * attach replaces the socket and callbacks (the old socket must be closed by then).
     */
    void attach(TcpSocket &socket, std::function<void()> on_redrive, std::function<void()> on_transport_error)
    {
        mSocket = &socket;
        mOnRedrive = std::move(on_redrive);
        mOnTransportError = std::move(on_transport_error);
        std::weak_ptr<int> alive = mAlive;
        socket.set_on_error(
            [this, alive](SocketError error, const std::string &message)
            {
                if (alive.expired())
                {
                    return; // bridge destroyed — pending write was cancelled with false; owner already knows
                }
                (void)error;
                (void)message;
                // EOF / transport error: surface once to the owner, which closes the peer.
                if (mOnTransportError)
                {
                    std::function<void()> cb = mOnTransportError;
                    cb();
                }
            });
        // Always armed (level-triggered): the ring is the read buffer; empty ring is WANT_READ.
        socket.read_start(
            [this, alive](const uint8_t *data, ssize_t nread)
            {
                if (alive.expired())
                {
                    // M4: terminal invocation may arrive after Peer/bridge destruction — the
                    // token check above already returned; this branch must not touch members
                    // regardless.
                    return;
                }
                if (nread > 0)
                {
                    mInRing.insert(mInRing.end(), data, data + nread);
                    if (mOnRedrive)
                    {
                        std::function<void()> cb = mOnRedrive;
                        cb();
                    }
                    // I1: compact the consumed ring prefix HERE, after the re-drive returns —
                    // f_recv only ever runs inside mOnRedrive, so this erase cannot invalidate
                    // a live f_recv window. Keeps the ring bounded without owner cooperation.
                    if (mInRead > 0)
                    {
                        mInRing.erase(mInRing.begin(), mInRing.begin() + static_cast<std::ptrdiff_t>(mInRead));
                        mInRead = 0;
                    }
                }
                // nread <= 0 (EOF/error terminal event) routes through set_on_error already.
            });
    }

    /// @brief The attached transport socket (nullptr until attach).
    TcpSocket *socket() const { return mSocket; }

    /// @brief Submits the next out-box entry (no-op when empty or a write is in flight).
    void pump()
    {
        if (mSocket == nullptr || mWriteInFlight || mOutBox.empty())
        {
            return;
        }
        mWriteInFlight = true;
        const std::vector<uint8_t> &front = mOutBox.front();
        std::weak_ptr<int> alive = mAlive; // token survives bridge destruction (C1)
        mSocket->write(front.data(),
                       front.size(),
                       [this, alive](bool ok)
                       {
                           if (alive.expired())
                           {
                               return; // bridge died mid-flight (C1): the owner already delivered or
                                       // tore down the peer — pop/pump/redrive must not run.
                           }
                           mWriteInFlight = false;
                           if (!ok)
                           {
                               // Transport-error path: report + close is the owner's business.
                               if (mOnTransportError)
                               {
                                   std::function<void()> cb = mOnTransportError;
                                   cb();
                               }
                               return;
                           }
                           mOutBox.pop_front(); // entry fully drained; slot reusable next send
                           // Submit the next queued entry, THEN re-drive the owner: a queued
                           // entry's completion is itself a progress trigger for the handshake.
                           this->pump();
                           if (mOnRedrive)
                           {
                               std::function<void()> cb = mOnRedrive;
                               cb();
                           }
                       });
    }

    /// @brief Compacts the consumed ring prefix (kept public: Task 3's client reuses the
    /// bridge for post-handshake data pump, where a manual compaction between reads may be
    /// preferable to waiting for the next on_data tail).
    void compact_ring()
    {
        mInRing.erase(mInRing.begin(), mInRing.begin() + static_cast<std::ptrdiff_t>(mInRead));
        mInRead = 0;
    }

    // ----- mbedtls_ssl_set_bio callbacks (static; p_ctx routes back to this) -----

    static int f_send(void *p_ctx, const unsigned char *buf, size_t len)
    {
        TlsBioBridge *self = static_cast<TlsBioBridge *>(p_ctx);
        if (self->mWriteInFlight || !self->mOutBox.empty())
        {
            return MBEDTLS_ERR_SSL_WANT_WRITE; // mbedTLS retries the SAME buffer later
        }
        // M1: mbedTLS fragments to MBEDTLS_SSL_OUT_CONTENT_LEN (~16KB), so len never
        // approaches INT_MAX — the static_cast<int> below is safe.
        // mbedTLS's buffer is only valid inside f_send — the copy IS the contract.
        self->mOutBox.push_back(std::vector<uint8_t>(buf, buf + len));
        self->pump(); // submit ONE entry immediately; completion pops + re-drives
        return static_cast<int>(len);
    }

    static int f_recv(void *p_ctx, unsigned char *buf, size_t len)
    {
        TlsBioBridge *self = static_cast<TlsBioBridge *>(p_ctx);
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

    /// @brief Wires the callbacks onto an SSL context (f_recv_timeout unused — pure non-blocking).
    static void set_bio(mbedtls_ssl_context *ssl, TlsBioBridge *bridge)
    {
        mbedtls_ssl_set_bio(ssl, bridge, &TlsBioBridge::f_send, &TlsBioBridge::f_recv, NULL);
    }

private:
    TcpSocket *mSocket = nullptr;
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
