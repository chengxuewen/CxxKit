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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/kernel/kernel_global.hpp>

#include <cxxkit/kernel/event_loop.hpp>

#include <cxxkit/tools/checks.hpp>

#include <functional>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class EventLoop;

/**
 * @brief Driver interface behind an @ref EventLoop.
 *
 * EventLoop is the user-facing handle; AbstractEventDispatcher is the driver interface; concrete drivers
 * (e.g. Uv/Qt dispatchers) implement this.
 *
 * A dispatcher is injected into its EventLoop once at construction and owns the platform event source.
 * Callbacks (timer callbacks and post tasks) always run on the loop thread. Callbacks must not let
 * exceptions escape; an escaping exception is undefined behavior (it will typically terminate the
 * process, but this is not guaranteed).
 */
class CXXKIT_KERNEL_API AbstractEventDispatcher
{
public:
    AbstractEventDispatcher() = default;
    virtual ~AbstractEventDispatcher() = default;

    /**
     * @brief Process pending events.
     *
     * @param flags Event selection flags; kWaitForMoreEvents controls blocking behaviour.
     * @return true if at least one event was processed.
     */
    virtual bool process_events(EventLoop::ProcessFlags flags) = 0;

    /**
     * @brief Kick the loop thread out of a blocking process_events. Thread-safe; may coalesce (I4).
     */
    virtual void wake_up() = 0;

    /**
     * @brief Make process_events return as soon as possible (exec teardown path).
     */
    virtual void interrupt() = 0;

    /**
     * @brief Register a timer. The callback runs on the loop thread; repeat behaviour is a shell-level
     *          wrapper (not part of this interface).
     */
    virtual void start_timer(int timer_id, uint64_t interval_ms, std::function<void()> fn) = 0;

    /**
     * @brief Cancel a timer registered by start_timer.
     */
    virtual void stop_timer(int timer_id) = 0;

    /**
     * @brief Socket readiness interests for @ref register_socket_notifier. @since 0.2
     *
     * F4 combination semantics: kRead|kWrite is legal and common (TcpSocket duplex IN|OUT, curl INOUT,
     * memcached re-arms per protocol state) — enum class gets an explicit operator| below.
     */
    enum class SocketEventMask
    {
        kRead = 1,  ///< socket is readable (level-triggered, aligns uv_poll UV_READABLE)
        kWrite = 2, ///< socket is writable (level-triggered, aligns uv_poll UV_WRITABLE)
    };

    /** @brief Combines read/write interests (F4): `kRead | kWrite` is the duplex registration form. */
    friend inline SocketEventMask operator|(SocketEventMask a, SocketEventMask b)
    {
        return static_cast<SocketEventMask>(static_cast<int>(a) | static_cast<int>(b));
    }

    /**
     * @brief Register a level-triggered readiness callback for @p fd. @since 0.2
     *
     * The callback receives the mask of interests that actually fired (aligns curl's event_bitmap
     * feedback: re-arming code re-registers with a mask derived from the delivered one). Readiness is
     * level-triggered (uv_poll semantics): after a delivery the registration stays live — no re-arm
     * needed to keep receiving; a silent fd produces no callback.
     *
     * F6 deviation (documented at T4 D31 reconciliation): ① the default implementation is a fatal
     * CXXKIT_CHECK instead of D2's literal "default empty implementation" — fail-loud beats a silent
     * no-op (an fd that never becomes ready is much harder to debug than a loud fatal). Source-level
     * compatibility is unchanged: subclasses that do not override still compile; only the call is fatal.
     * ② the callback carries the fired mask (curl alignment). ③ std::function, not signals (lightweight;
     * a signals flavor can be a thin wrapper on top).
     *
     * F7 fd lifecycle rulings baked into the contract: re-registering a live fd updates its interests
     * idempotently (uv_poll_start semantics; memcached-style per-transition re-arm); closing an fd while
     * a poll is active is caller UB — unregister first, then close (libuv contract). The independent
     * RAII SocketNotifier of spec §9 is deferred (TcpSocket consumes this API directly; YAGNI).
     *
     * Loop thread only (I1). For file IO use cxxkit thread_pool instead.
     */
    virtual void register_socket_notifier(int fd, SocketEventMask mask, std::function<void(SocketEventMask)> fn)
    {
        CXXKIT_UNUSED(fd);
        CXXKIT_UNUSED(mask);
        CXXKIT_UNUSED(fn);
        CXXKIT_CHECK(false) << "not supported by this dispatcher (phase-2 feature; UvEventDispatcher implements "
                               "it). For file IO use cxxkit thread_pool instead.";
    }

    /**
     * @brief Remove the readiness registration for @p fd (no-op if not registered). @since 0.2
     *
     * Must be called before closing @p fd while a poll is active (F7-②). Loop thread only (I1).
     */
    virtual void unregister_socket_notifier(int fd)
    {
        CXXKIT_UNUSED(fd);
        // silent no-op by design: unregistering on a dispatcher that never registered anything is
        // not a bug (mirror of "unknown id is a no-op" — P2-3); the loud default lives in register.
    }

private:
    CXXKIT_DISABLE_COPY_MOVE(AbstractEventDispatcher)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
