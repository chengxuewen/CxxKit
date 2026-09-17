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
** and to permit persons to the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/kernel/kernel_global.hpp>
#include <cxxkit/kernel/signals.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/tools/checks.hpp>

#include <atomic>
#include <functional>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * Non-deduced context wrapper (std::function parameters would otherwise block template
 * argument deduction when callers pass lambdas).
 */
template <typename T>
struct type_identity
{
    using type = T;
};

namespace detail
{
/**
 * @brief Liveness-token check for the connect_queued family (design §1.2/§2.4).
 *
 * Unified token shape std::weak_ptr<std::atomic<bool>>: lock success = alive. Natural
 * expiry = the owner's control block died (EventLoop private destroyed — the loop token
 * is never flipped by hand); a future manual store(false) flip (receiver token, Task 2)
 * composes with the same check.
 */
inline bool token_alive(const std::weak_ptr<std::atomic<bool>> &token) noexcept
{
    return token.lock() != nullptr;
}
} // namespace detail

/**
 * @brief Connects @p sig to a slot that hops onto @p loop: each emission copies the arguments
 * and enqueues @p fn to run on the loop thread via EventLoop::post.
 *
 * Value-copy semantics: arguments are copied once at emit time (std::bind decay-copies), so
 * types must be copy-constructible; mutating the emitted variables afterwards does not affect
 * a pending delivery. Because @p fn runs on the loop thread, later re-entrancy through the
 * loop is safe.
 *
 * Lifecycle (weak loop token, design §1.3): the slot captures a weak loop token captured at
 * connect time. On each emission the slot checks the token on the emitting thread — if the
 * loop has been destroyed the emission silently skips (no post, no crash), mirroring the
 * tracked-slot "dead → skip, not throw" contract. Known narrow window (documented limitation):
 * a "token check passed → post" step can still race a concurrent ~EventLoop (instruction-level
 * TOCTOU); the token shrinks the exposure from "the loop's whole remaining lifetime" to that
 * instruction-level window — the same best-effort boundary as PendingTaskSafetyFlag.
 * Closures already enqueued when the loop dies still run: ~EventLoop drains the posted-task
 * queue unconditionally and the token stays alive during that drain.
 *
 * Disconnection (spec I7): disconnect() stops future deliveries only — deliveries already
 * enqueued before the disconnect still run. To undo safely, stop the emit source first or use
 * a safety flag (PendingTaskSafetyFlag, spec M5/S8).
 *
 * Same-thread contract (W1/D10, OQ1 pre-commitment): calling a @c wait-style helper that
 * blocks the loop on work that must run on that same loop is fatal BY DESIGN — deadlock
 * prevention is deliberately stronger than Qt's runtime warning for BlockingQueuedConnection.
 * Anti-pattern warning: do not block a loop thread waiting on async work; the blocking
 * call_and_wait primitive does not exist yet, and loop-thread blocking (join, wait_for,
 * poll) stalls all queued work and timers until it returns.
 *
 * @param sig Signal to connect (thread-safe signals::Signal).
 * @param loop Target loop for delivery; null is a fatal error.
 * @param fn Slot invoked on the loop thread.
 * @return signals::Connection for the underlying signal slot (disconnect stops further delivery).
 */
template <typename... Args>
signals::Connection connect_queued(Signal<Args...> &sig,
                                   EventLoop *loop,
                                   typename type_identity<std::function<void(Args...)>>::type fn)
{
    CXXKIT_CHECK(loop != nullptr) << "connect_queued requires a loop";
    const std::weak_ptr<std::atomic<bool>> loopToken = loop->alive_token();
    return sig.connect(
        [loop, loopToken, fn](const Args &...args)
        {
            // Emit-side check on the emitting thread (PIT-40 safe: signals invoke slots outside
            // the signal lock). Dead loop → silent skip, never post into a dangling pointer.
            if (!detail::token_alive(loopToken))
            {
                return;
            }
            loop->post(std::bind(fn, args...));
        });
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
