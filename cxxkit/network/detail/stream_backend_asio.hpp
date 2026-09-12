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

#pragma once

#include <cxxkit/network/detail/stream_backend.hpp>

#include <cxxkit/tools/checks.hpp>

#include <deque>
#include <memory>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/**
 * @brief The asio (standalone, header-only) implementation of @ref StreamBackend (Task 4, D41).
 *
 * Contract mirrors UvStreamBackend exactly: on_done invoked exactly once per operation; close is
 * idempotent; pending writes/connect fan out false at close; accepted handles are handed over
 * WHOLE via adopt_native; pump_until_closed drains until the close lifecycle completed.
 *
 * EMBED ruling (Momus Q4 FINAL): the backend owns an asio::io_context and advances it ON the
 * EventLoop thread — a recurring EventLoop-posted pump task calls io_context.poll() and re-posts
 * itself while any asio work (socket/acceptor/listener) is alive. No dedicated thread, no kernel
 * dispatcher bridge; loop exits cleanly when the backend is destroyed (the pump closure checks
 * the backend's aliveness via a shared flag before re-posting).
 *
 * asio.hpp lives ONLY in stream_backend_asio.cpp (grep gate: no asio types outside these two
 * files — the same D8 discipline stream_backend_uv applies to uv.h).
 */
class AsioStreamBackend : public StreamBackend
{
public:
    AsioStreamBackend();
    ~AsioStreamBackend() override;

    // client face
    bool open(EventLoop &loop) override;
    void connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done) override;
    void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done) override;
    void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data) override;
    void read_stop() override;
    void close(std::function<void()> on_closed) override; // idempotent; synchronous teardown + drain
    bool is_open() const override;

    // server face
    bool listen(const std::string &ip, uint16_t port, int backlog) override;
    uint16_t bound_port() const override;
    void set_on_accept(std::function<void(std::unique_ptr<StreamBackend> client)> on_accept) override;

    EventLoop &loop() const override
    {
        CXXKIT_CHECK(mLoop != nullptr) << "AsioStreamBackend::loop: backend never opened";
        return *mLoop;
    }
    int native_status() const override { return mNativeStatus; }
    void *native_handle() const override;

    // native handle adopt/release
    bool adopt_native(void *native_handle, EventLoop &loop) override;
    bool adopt_fd(int fd, EventLoop &loop) override;

    // dtor discipline: close + pump the loop until the close lifecycle completed
    void pump_until_closed() override;

private:
    struct Native; // fwd: io_context + optional socket/acceptor cell (defined in the .cpp)
    std::unique_ptr<Native> mN;

    friend struct Native;

    void ensure_pump();        /// queue one immediate pump tick (pending_tick coalescing)
    void begin_close();        /// shared teardown: cancel pending work, fire mOnClosed. Idempotent.
    void pump_tick();          /// one poll round + timer-gated cadence decision (fix wave F1)
    void stop_cadence_timer(); /// cancel the 1ms repeating cadence timer (dtor/teardown/drain)
    bool has_work() const;     /// socket/acceptor alive?
    void arm_read();           /// (re-)issue one async_read_some against mReadBuf
    void arm_accept();         /// (re-)issue one async_accept (accept loop body)
    void submit_next_write();  /// lws discipline: one async_write in flight, deque feeds it

    EventLoop *mLoop{nullptr};
    int mNativeStatus{0};

    bool mCloseRequested{false};

    /// One queued transmission: the copied bytes + its completion callback (lws backpressure,
    /// uv parity — asio queues a single stream op internally but NOT across separate calls).
    struct PendingWrite
    {
        std::vector<uint8_t> mData;
        std::function<void(bool ok)> mOnWritten;
    };
    std::deque<PendingWrite> mPendingWrites; /// queued while a write is in flight (FIFO)
    std::vector<uint8_t> mInFlight;          /// storage for the in-flight async_write bytes
    std::vector<uint8_t> mReadBuf;           /// beast flat_buffer shape: one contiguous block asio fills

    std::function<void(bool ok)> mOnConnect;
    std::function<void(const uint8_t *data, ssize_t nread)> mOnData;
    std::function<void()> mOnClosed;
    std::function<void(std::unique_ptr<StreamBackend> client)> mOnAccept;
    uint16_t mBoundPort{0};
};

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
