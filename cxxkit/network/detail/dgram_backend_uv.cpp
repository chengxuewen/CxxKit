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

#include <cxxkit/network/detail/dgram_backend_uv.hpp>

#include <cxxkit/kernel/uv/detail/uv_event_dispatcher.hpp>
#include <cxxkit/network/detail/address_helper.hpp>
#include <cxxkit/network/detail/dgram_backend.hpp>

#include <cxxkit/3rdparty/libuv/uv.h>

#include <cxxkit/tools/checks.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <utility>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

namespace
{
/// Largest IP datagram any host must accept (IPv4 min-reassembly requirement); uv's alloc
/// suggestion for udp is the same value. One member buffer is sized to this once.
constexpr size_t kMaxDgramSize = 65536;
} // namespace

UvDgramBackend::UvDgramBackend() = default;

UvDgramBackend::~UvDgramBackend()
{
    // Same discipline as UvStreamBackend: nothing should reach here with a live handle —
    // the pimpl's dtor pumps until the close callback ran.
    CXXKIT_CHECK(mHandle == nullptr) << "UvDgramBackend: handle outlived the drain pump (teardown bug)";
}

bool UvDgramBackend::open(EventLoop &loop)
{
    mLoop = &loop;
    // The loop's dispatcher is an UvEventDispatcher in this sublibrary's world (static_cast is the
    // inject-once contract: EventLoop owns it exclusively and cxxkit::network made it).
    mUvLoop = &static_cast<UvEventDispatcher &>(loop.dispatcher()).loop();
    return true;
}

bool UvDgramBackend::bind(const std::string &ip, uint16_t port)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvDgramBackend::bind: open() was not called";
    if (mCloseRequested)
    {
        return false; // closed backends do not re-bind (lifecycle latched, TcpSocket same shape)
    }
    sockaddr_storage addr;
    if (!fill_sockaddr(ip, port, &addr))
    {
        // Invalid address: bind failure, not a programming error — backend reports plain failure,
        // the pimpl layer maps it onto the public error surface.
        mNativeStatus = -UV_EINVAL;
        return false;
    }

    uv_udp_t *handle = new uv_udp_t;
    const int init_rc = uv_udp_init(mUvLoop, handle);
    if (init_rc != 0)
    {
        delete handle; // init never registered it with the loop
        mNativeStatus = init_rc;
        return false;
    }
    handle->data = this;
    mHandle = handle; // from here on the close callback owns the free

    const int bind_rc = uv_udp_bind(handle, reinterpret_cast<const sockaddr *>(&addr), 0);
    if (bind_rc != 0)
    {
        // The handle IS registered with the loop (init did that) — the only clean teardown is
        // uv_close + pump until the callback freed it (UvStreamBackend::listen same discipline),
        // so a retry starts from a clean slate and the loop never carries a zombie handle.
        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(mHandle)))
        {
            uv_close(reinterpret_cast<uv_handle_t *>(mHandle), &UvDgramBackend::on_closed);
        }
        for (int rounds = 0; rounds < 1000 && mHandle != nullptr; ++rounds)
        {
            mLoop->process_events(EventLoop::ProcessFlag::kAllEvents);
        }
        mNativeStatus = bind_rc;
        return false;
    }

    // Port 0 = OS-assigned; read the real port back via uv_udp_getsockname — race-free discovery
    // (R-T3-2 shape, UvStreamBackend::listen verbatim).
    mBoundPort = port;
    sockaddr_storage bound;
    int bound_len = static_cast<int>(sizeof(bound));
    if (uv_udp_getsockname(handle, reinterpret_cast<sockaddr *>(&bound), &bound_len) == 0)
    {
        if (bound.ss_family == AF_INET)
        {
            mBoundPort = ntohs(reinterpret_cast<const sockaddr_in *>(&bound)->sin_port);
        }
        else if (bound.ss_family == AF_INET6)
        {
            mBoundPort = ntohs(reinterpret_cast<const sockaddr_in6 *>(&bound)->sin6_port);
        }
    }
    return true;
}

uint16_t UvDgramBackend::bound_port() const
{
    return mBoundPort;
}

void UvDgramBackend::send_to(const uint8_t *data,
                             size_t len,
                             const std::string &ip,
                             uint16_t port,
                             std::function<void(bool ok)> on_done)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvDgramBackend::send_to: open() was not called";
    if (mCloseRequested || mHandle == nullptr)
    {
        mNativeStatus = -UV_ESHUTDOWN;
        if (on_done)
        {
            on_done(false);
        }
        return;
    }

    PendingSend *send = new PendingSend;
    send->mData.assign(data, data + len);
    send->mOnDone = std::move(on_done);
    if (!fill_sockaddr(ip, port, &send->mAddr))
    {
        // Invalid address: send failure, not a programming error. Inline false — no send_cb will
        // come for a request uv never took. Inert status (maps to kUnknown), UvStreamBackend::connect
        // invalid-address shape.
        delete send;
        mNativeStatus = -UV_EINVAL;
        if (on_done)
        {
            on_done(false);
        }
        return;
    }

    // Multiple uv_udp_send requests may be in flight simultaneously (libuv native support) — one
    // heap req per call, no queue: the request cell owns the buffer view, the destination address
    // and the completion callback; all three die in on_send_done (PIT-40: nothing on our side is
    // referenced after this function returns except uv-owned state).
    uv_udp_send_t *req = new uv_udp_send_t;
    req->data = send;
    uv_buf_t send_buf = uv_buf_init(reinterpret_cast<char *>(send->mData.data()), send->mData.size());
    const int rc = uv_udp_send(req,
                               reinterpret_cast<uv_udp_t *>(mHandle),
                               &send_buf,
                               1,
                               reinterpret_cast<const sockaddr *>(&send->mAddr),
                               &UvDgramBackend::on_send_done);
    if (rc != 0)
    {
        // Synchronous failure: no send_cb will come — report false inline from the callback that
        // already lives in the cell.
        std::function<void(bool ok)> cb = std::move(send->mOnDone);
        delete send;
        delete req;
        mNativeStatus = rc;
        if (cb)
        {
            cb(false);
        }
        return;
    }
    // on_done moved into the PendingSend cell before submit — the send callback owns it now.
}

void UvDgramBackend::receive_start(
    std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> on_datagram)
{
    CXXKIT_CHECK(mLoop != nullptr) << "UvDgramBackend::receive_start: open() was not called";
    if (mCloseRequested)
    {
        return;
    }
    mOnDatagram = std::move(on_datagram);
    if (mRecvArmed)
    {
        return; // already polling — callback swap is the whole update (read_start re-arm shape)
    }
    const int rc = uv_udp_recv_start(reinterpret_cast<uv_udp_t *>(mHandle),
                                     &UvDgramBackend::on_alloc,
                                     &UvDgramBackend::on_recv);
    CXXKIT_CHECK(rc == 0) << "UvDgramBackend::receive_start: uv_udp_recv_start failed (" << rc << ")";
    mRecvArmed = true;
}

void UvDgramBackend::close()
{
    begin_close();
}

void UvDgramBackend::begin_close()
{
    // F8-② idempotence: uv_close is asynchronous; a second close on the same handle is
    // use-after-free. mCloseRequested latches — every later close() is a no-op.
    if (mCloseRequested)
    {
        return;
    }
    mCloseRequested = true;

    if (mHandle == nullptr)
    {
        // Never bound (open()-only or invalid-address shape): nothing to close at the uv level.
        return;
    }
    if (mRecvArmed)
    {
        uv_udp_recv_stop(reinterpret_cast<uv_udp_t *>(mHandle));
        mRecvArmed = false;
    }
    mOnDatagram = nullptr;
    // In-flight uv_udp_send requests are NOT cancelled here: they complete through their own
    // callbacks (with UV_ECANCELED) and free their cells — no queue to drain (stream backend
    // differs: its queued writes carry OUR storage, so begin_close fans them out manually).
    if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(mHandle)))
    {
        uv_close(reinterpret_cast<uv_handle_t *>(mHandle), &UvDgramBackend::on_closed);
    }
}

// ----- uv trampolines (handle->data routes back here; Node tcp_wrap pattern) -----

void UvDgramBackend::on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    UvDgramBackend *backend = static_cast<UvDgramBackend *>(handle->data);
    (void)handle;
    // One contiguous member block, capped at the max datagram size — a udp recv never needs more
    // (uv's udp suggestion equals it); handing uv the whole buffer avoids per-dgram churn.
    const size_t size = (suggested_size > kMaxDgramSize) ? kMaxDgramSize : suggested_size;
    if (backend->mRecvBuf.size() < size)
    {
        backend->mRecvBuf.resize(size);
    }
    buf->base = reinterpret_cast<char *>(backend->mRecvBuf.data());
    buf->len = static_cast<unsigned>(backend->mRecvBuf.size());
}

void UvDgramBackend::on_recv(uv_udp_t *handle,
                             ssize_t nread,
                             const uv_buf_t *buf,
                             const ::sockaddr *addr,
                             unsigned flags)
{
    UvDgramBackend *backend = static_cast<UvDgramBackend *>(handle->data);
    (void)buf;
    (void)flags;
    if (nread == 0)
    {
        return; // EAGAIN/no-data (addr == nullptr too): keep-alive probe, keep the interest armed
    }
    if (nread < 0)
    {
        // Recv error (UV_ECANCELED after close, EMSGSIZE... ): stop receiving (uv docs: the
        // caller MUST stop on error), surface the status; the pimpl maps it (Task 2) and the
        // user's error/state path decides re-arm. No auto-delivery of a datagram.
        backend->mNativeStatus = static_cast<int>(nread);
        uv_udp_recv_stop(handle);
        backend->mRecvArmed = false;
        backend->mOnDatagram = nullptr; // dropping the callback stops deliveries; handle stays open
        return;
    }
    if (addr == nullptr || !backend->mOnDatagram)
    {
        return; // no source address (truncated-keep-alive shape) or no consumer installed
    }
    // PIT-40 copy discipline: the callback may close()/destroy the backend mid-execution —
    // invoke through a local copy (F8-① shape), and hand out copies of the source address
    // strings built before the call (the sockaddr lives only for this callback).
    std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> cb = backend
                                                                                                        ->mOnDatagram;
    char ip_str[INET6_ADDRSTRLEN] = {0};
    uint16_t port_value = 0;
    if (addr->sa_family == AF_INET)
    {
        const sockaddr_in *v4 = reinterpret_cast<const sockaddr_in *>(addr);
        uv_inet_ntop(AF_INET, &v4->sin_addr, ip_str, sizeof(ip_str));
        port_value = ntohs(v4->sin_port);
    }
    else if (addr->sa_family == AF_INET6)
    {
        const sockaddr_in6 *v6 = reinterpret_cast<const sockaddr_in6 *>(addr);
        uv_inet_ntop(AF_INET6, &v6->sin6_addr, ip_str, sizeof(ip_str));
        port_value = ntohs(v6->sin6_port);
    }
    else
    {
        return; // unknown family: not deliverable through the (ip, port) surface
    }
    const std::string ip_value(ip_str);
    cb(reinterpret_cast<const uint8_t *>(buf->base), static_cast<size_t>(nread), ip_value, port_value);
}

void UvDgramBackend::on_send_done(uv_udp_send_t *req, int status)
{
    PendingSend *send = static_cast<PendingSend *>(req->data);
    UvDgramBackend *backend = reinterpret_cast<uv_udp_t *>(req->handle)->data != nullptr
                                  ? static_cast<UvDgramBackend *>(reinterpret_cast<uv_udp_t *>(req->handle)->data)
                                  : nullptr;
    if (backend != nullptr)
    {
        backend->mNativeStatus = status; // the req->handle->data back-route needs no member on the cell
    }
    delete req;
    if (send->mOnDone)
    {
        // PIT-40: invoke through a local copy — the callback may close()/destroy the backend.
        std::function<void(bool ok)> cb = std::move(send->mOnDone);
        cb(status == 0);
    }
    delete send;
}

void UvDgramBackend::on_closed(uv_handle_t *handle)
{
    UvDgramBackend *backend = static_cast<UvDgramBackend *>(handle->data);
    backend->mHandle = nullptr;
    // The allocation is uv_udp_t; the callback parameter type is uv_handle_t. Deleting through the
    // base-typed pointer is a new-delete-type-mismatch (ASAN) — cast back to the real allocation
    // type (UvStreamBackend::on_closed same comment).
    delete reinterpret_cast<uv_udp_t *>(handle);
}

std::unique_ptr<DgramBackend> make_dgram_backend()
{
    return std::unique_ptr<DgramBackend>(new UvDgramBackend);
}

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
