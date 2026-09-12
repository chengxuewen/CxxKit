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

#include <deque>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_KERNEL

struct uv_loop_s; // libuv loop handle (global scope: same type uv.h typedefs as uv_loop_t; D8 keeps uv out of headers)
struct uv_tcp_s;  // libuv tcp handle (global scope)
struct uv_connect_s; // libuv connect request (global scope)
struct uv_write_s;   // libuv write request (global scope)
struct uv_handle_s;  // libuv base handle (global scope)
struct uv_stream_s;  // libuv stream base (global scope)
struct uv_buf_t;     // libuv buffer view (global scope: uv.h typedefs `typedef struct uv_buf_t`)


CXXKIT_BEGIN_NAMESPACE

namespace network
{
namespace detail
{

/**
 * @brief The uv implementation of @ref StreamBackend — code moved verbatim from
 *        tcp_socket.cpp / tcp_server.cpp (Task 3, zero behavior change).
 *
 * Owns the uv_tcp_t heap cell, the lws single-flight write state (one uv_write in flight,
 * FIFO PendingWrite deque) and the drain-pump loop. uv.hpp lives ONLY in stream_backend_uv.cpp
 * (grep gate: no uv types outside these two files).
 */
class UvStreamBackend : public StreamBackend
{
public:
    UvStreamBackend();
    ~UvStreamBackend() override;

    // client face
    bool open(EventLoop &loop) override;
    void connect(const std::string &ip, uint16_t port, std::function<void(bool ok)> on_done) override;
    void write(const uint8_t *data, size_t len, std::function<void(bool ok)> on_done) override;
    void read_start(std::function<void(const uint8_t *data, ssize_t nread)> on_data) override;
    void read_stop() override;
    void close(std::function<void()> on_closed) override; // idempotent; async under uv
    bool is_open() const override;

    // server face
    bool listen(const std::string &ip, uint16_t port, int backlog) override;
    uint16_t bound_port() const override;
    void set_on_accept(std::function<void(std::unique_ptr<StreamBackend> client)> on_accept) override;

    // native handle adopt/release
    bool adopt_native(void *native_handle, EventLoop &loop) override;
    bool adopt_fd(int fd, EventLoop &loop) override;
    void *native_handle() const override { return mHandle; }
    EventLoop &loop() const override { return *mLoop; }
    int native_status() const override { return mNativeStatus; }

    // dtor discipline: pump the loop until the native close callback ran
    void pump_until_closed() override;

private:
    /// One queued transmission: the copied bytes + its completion callback (lws backpressure).
    struct PendingWrite
    {
        std::vector<uint8_t> mData;
        std::function<void(bool ok)> mOnWritten;
    };

    // uv C callbacks (static trampolines) — handle->data routes back here (Node tcp_wrap pattern).
    static void on_connect_done(struct uv_connect_s *req, int status);
    static void on_alloc(struct uv_handle_s *handle, size_t suggested_size, struct uv_buf_t *buf);
    static void on_read(struct uv_stream_s *stream, ssize_t nread, const struct uv_buf_t *buf);
    static void on_write_done(struct uv_write_s *req, int status);
    static void on_closed(struct uv_handle_s *handle);
    static void on_connection_cb(struct uv_stream_s *server, int status);
    static void on_discard_closed(struct uv_handle_s *handle);

    void submit_next_write();

    /** @brief Shared teardown: flush pending callbacks, stop reading, uv_close. Idempotent via mCloseRequested. */
    void begin_close();

    EventLoop *mLoop{nullptr};
    int mNativeStatus{0};                      /// last completed op's native status (uv errno-style)
    struct uv_loop_s *mUvLoop{nullptr};        /// the dispatcher's uv loop (opaque here; uv.h is cpp-only)
    struct uv_tcp_s *mHandle{nullptr};         /// heap cell; freed in the close callback
    struct uv_connect_s *mConnectReq{nullptr}; /// heap cell; freed in the connect callback
    struct uv_write_s *mWriteReq{nullptr};     /// in-flight write's req; freed in the write callback
    /// In-flight write's buffer view (storage owned by mInFlight). Opaque void-pair here: uv_buf_t
    /// (base/len) cannot be forward-declared opaquely as a value member; the .cpp reconstructs it
    /// from mInFlight each submit — single source of truth, zero uv types leaked.
    void *mWriteBufBase{nullptr};
    size_t mWriteBufLen{0};

    bool mCloseRequested{false};
    bool mReadArmed{false};
    std::deque<PendingWrite> mPendingWrites; /// queued while a write is in flight (FIFO, lws)
    std::vector<uint8_t> mInFlight;          /// storage for the in-flight uv_write (must outlive the req)
    std::vector<uint8_t> mReadBuf;           /// beast flat_buffer shape: one contiguous block uv fills from

    std::function<void(bool ok)> mOnConnect;
    std::function<void(bool ok)> mOnWrittenCurrent; /// in-flight write's callback; handed back in write done
    std::function<void(const uint8_t *data, ssize_t nread)> mOnData;
    std::function<void()> mOnClosed;
    std::function<void(std::unique_ptr<StreamBackend> client)> mOnAccept;
    uint16_t mBoundPort{0};
};

/** @brief Compiled per backend selection: the uv definition (the only one today). */
std::unique_ptr<StreamBackend> make_stream_backend();

} // namespace detail
} // namespace network

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
