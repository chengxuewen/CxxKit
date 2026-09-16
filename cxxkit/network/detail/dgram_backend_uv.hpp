/***
Library: CxxKit
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

#pragma once

#include <cxxkit/network/detail/dgram_backend.hpp>

#include <cxxkit/tools/checks.hpp>

#include <cxxkit/network/detail/address_helper.hpp>

#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

struct uv_loop_s; // libuv loop handle (global scope: same type uv.h typedefs as uv_loop_t; D8 keeps uv out of headers)
struct uv_udp_s;  // libuv udp handle (global scope)
struct uv_udp_send_s; // libuv udp send request (global scope)
struct uv_handle_s;   // libuv base handle (global scope)
struct uv_buf_t;      // libuv buffer view (global scope: uv.h typedefs `typedef struct uv_buf_t`)


CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/**
 * @brief The uv implementation of @ref DgramBackend — connectionless UDP over @c uv_udp_t (D44).
 *
 * Mirrors the UvStreamBackend shape: the uv_udp_t heap cell lives only here (uv.h stays
 * cpp-only, D8), recv uses an alloc/recv trampoline pair on a single member buffer, send
 * copies its bytes into a per-request heap cell that the send callback frees. Unlike the
 * stream backend's lws single-flight discipline, @c uv_udp_send supports multiple in-flight
 * requests natively — one heap request per send, no queue. Errors surface as native uv
 * statuses via @c native_status; mapping to SocketError stays in the pimpl layer (Task 2).
 */
class UvDgramBackend : public DgramBackend
{
public:
    UvDgramBackend();
    ~UvDgramBackend() override;

    bool open(EventLoop &loop) override;
    bool bind(const std::string &ip, uint16_t port) override;
    uint16_t bound_port() const override;
    void send_to(const uint8_t *data,
                 size_t len,
                 const std::string &ip,
                 uint16_t port,
                 std::function<void(bool ok)> on_done) override;
    bool connect(const std::string &ip, uint16_t port) override;
    void disconnect_remote() override;
    void receive_start(std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)>
                           on_datagram) override;
    void close() override;
    EventLoop &loop() const override
    {
        CXXKIT_CHECK(mLoop != nullptr) << "UvDgramBackend::loop: backend never opened";
        return *mLoop;
    }
    int native_status() const override { return mNativeStatus; }
    void *native_handle() const override { return mHandle; }

private:
    /// One in-flight datagram: the copied bytes live here (the uv_buf view points into it),
    /// the destination address, and the completion callback. The uv send callback deletes
    /// the request cell that owns this storage.
    struct PendingSend
    {
        std::vector<uint8_t> mData;
        ::sockaddr_storage mAddr{}; /// destination address, filled at submit by fill_sockaddr
        std::function<void(bool ok)> mOnDone;
    };

    // uv C callbacks (static trampolines) — handle->data routes back here (Node tcp_wrap pattern).
    static void on_alloc(struct uv_handle_s *handle, size_t suggested_size, struct uv_buf_t *buf);
    static void on_recv(struct uv_udp_s *handle,
                        ssize_t nread,
                        const struct uv_buf_t *buf,
                        const ::sockaddr *addr,
                        unsigned flags);
    static void on_send_done(struct uv_udp_send_s *req, int status);
    static void on_closed(struct uv_handle_s *handle);

    /**
     * @brief Shared teardown: stop recv, uv_close. Idempotent via mCloseRequested.
     *
     * In-flight uv_udp_send requests are NOT cancelled here — they complete through their
     * own callbacks (with UV_ECANCELED after close) and free themselves, so no queued-callback
     * fanout is needed (unlike the stream backend's queued-write drain).
     */
    void begin_close();

    EventLoop *mLoop{nullptr};
    int mNativeStatus{0};               /// last completed op's native status (uv errno-style)
    struct uv_loop_s *mUvLoop{nullptr}; /// the dispatcher's uv loop (opaque here; uv.h is cpp-only)
    struct uv_udp_s *mHandle{nullptr};  /// heap cell; freed in the close callback

    bool mCloseRequested{false};
    bool mRecvArmed{false};
    std::vector<uint8_t> mRecvBuf; /// one contiguous block uv fills per datagram (64KiB max UDP payload)

    std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> mOnDatagram;
    uint16_t mBoundPort{0};
    bool mConnected{false}; /// default peer pinned via connect() (clears in disconnect_remote)
};

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
