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

#include <functional>
#include <memory>
#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/**
 * @brief The asio (standalone, header-only) implementation of @ref DgramBackend — UDP over
 *        @c asio::ip::udp::socket (Task 3, D44).
 *
 * Mirrors the UvDgramBackend shape: bind is synchronous; send_to fires one async op per call
 * with its own heap-owned buffer copy (asio udp allows concurrent in-flight sends — no queue);
 * receive keeps ONE async_receive_from outstanding, re-arming after each delivery (level-
 * triggered, uv recv_start parity). The pump machinery mirrors AsioStreamBackend: the shared
 * per-loop io_context slot, the weak_ptr liveness bridge on every posted closure, and the
 * timer-gated 1ms cadence with PIT-55 restart() before every poll. Errors surface as errno-
 * style negative values via native_status; mapping stays in the pimpl layer.
 *
 * asio.hpp lives ONLY in dgram_backend_asio.cpp (grep gate: no asio types outside these two
 * files — the same D8 discipline stream_backend_asio applies to asio.hpp).
 */
class AsioDgramBackend : public DgramBackend
{
public:
    AsioDgramBackend();
    ~AsioDgramBackend() override;

    bool open(EventLoop &loop) override;
    bool bind(const std::string &ip, uint16_t port) override;
    uint16_t bound_port() const override;
    void send_to(const uint8_t *data,
                 size_t len,
                 const std::string &ip,
                 uint16_t port,
                 std::function<void(bool ok)> on_done) override;
    void receive_start(std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)>
                           on_datagram) override;
    void close() override;
    EventLoop &loop() const override
    {
        CXXKIT_CHECK(mLoop != nullptr) << "AsioDgramBackend::loop: backend never opened";
        return *mLoop;
    }
    int native_status() const override { return mNativeStatus; }
    void *native_handle() const override;

private:
    struct Native; // fwd: io_context reference + optional socket cell (defined in the .cpp)
    std::unique_ptr<Native> mN;

    friend struct Native;

    void ensure_pump();        /// queue one immediate pump tick (pending_tick coalescing)
    void pump_tick();          /// one poll round + timer-gated cadence decision (PIT-55 restart)
    void stop_cadence_timer(); /// cancel the 1ms repeating cadence timer (dtor/teardown)
    bool has_work() const;     /// udp socket alive? (no acceptor concept on udp)
    void arm_receive();        /// (re-)issue one async_receive_from against mRecvBuf

    EventLoop *mLoop{nullptr};
    int mNativeStatus{0};

    bool mCloseRequested{false};
    bool mRecvArmed{false};
    std::vector<uint8_t> mRecvBuf; /// one contiguous block asio fills per datagram (64KiB max UDP payload)

    std::function<void(const uint8_t *data, size_t len, const std::string &ip, uint16_t port)> mOnDatagram;
    std::string mSenderIp;   /// scratch storage for the outstanding receive's sender endpoint
    uint16_t mSenderPort{0}; /// scratch storage for the outstanding receive's sender endpoint
    uint16_t mBoundPort{0};
};

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
