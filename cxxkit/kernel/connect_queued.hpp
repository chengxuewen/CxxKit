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

#include <cxxkit/tools/optional.hpp>

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

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
 * Unified token shape std::weak_ptr<std::atomic<bool>>: lock success AND flag true = alive.
 * Two lifetime strategies compose with this single check: natural expiry (EventLoop loop
 * token — the control block dies with the loop private, flag never written) and manual
 * early flip (Object receiver token, design §2.4 — ~Object stores false BEFORE its
 * members die, closing the "derived members dead but ObjectPrivate alive" window that
 * control-block expiry alone would leave open).
 */
inline bool token_alive(const std::weak_ptr<std::atomic<bool>> &token) noexcept
{
    const std::shared_ptr<std::atomic<bool>> owner = token.lock();
    // Flag read is REQUIRED for the manual-flip strategy: a store(false) on a still-living
    // control block must read as dead (design §2.4). Natural-expiry owners never write the
    // flag, so the load stays true for them — both strategies compose.
    return (owner != nullptr) && owner->load(std::memory_order_acquire);
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

/**
 * @brief Receiver form (design §1.2 shape 2 / §2 / §3): double liveness protection for a
 * queued connection whose slot logically belongs to an Object receiver.
 *
 * ① Producer side: the returned Connection is registered into the receiver's embedded
 *    connection observer (Object::add_connection) — ~Object eagerly disconnect_all()s it
 *    (after destroying(), before purge_pending), so no NEW emit produces a NEW post into
 *    a dead receiver's loop binding.
 * ② Consumer side: the posted closure embeds the receiver liveness token — if the receiver
 *    died between enqueue and drain, the delivery silently skips (SafetyFlag precedent).
 *    The two tokens are orthogonal: the loop token gates "before enqueue" (emit side), the
 *    receiver token gates "before execution" (drain side).
 *
 * Same value-copy semantics, same silent-skip contract and same instruction-level TOCTOU
 * window as form 1. Thread-safety: all three touchpoints (connect, emit, receiver dtor)
 * may run on different threads — the std::mutex observer variant is mandatory (design §3.1).
 *
 * @param sig Signal to connect (thread-safe signals::Signal).
 * @param loop Target loop for delivery; null is a fatal error. Static binding: the loop
 *        argument is NOT redirected by receiver move_to_loop (design §3 axis-3 ruling).
 * @param receiver Owner of the slot; null is a fatal error. Must outlive every in-flight
 *        delivery or rely on the token skip (which it does automatically).
 * @param fn Slot invoked on the loop thread.
 * @return signals::Connection (also registered with @p receiver).
 */
template <typename... Args>
signals::Connection connect_queued(Signal<Args...> &sig,
                                   EventLoop *loop,
                                   Object *receiver,
                                   typename type_identity<std::function<void(Args...)>>::type fn)
{
    CXXKIT_CHECK(loop != nullptr) << "connect_queued requires a loop";
    CXXKIT_CHECK(receiver != nullptr) << "connect_queued receiver form requires a receiver";
    const std::weak_ptr<std::atomic<bool>> loopToken = loop->alive_token();
    const std::weak_ptr<std::atomic<bool>> receiverToken = receiver->alive_token();
    // Register the connection into the receiver's observer FIRST (producer-side guard): the
    // returned Connection also lands there, so ~Object disconnects it eagerly (design §3.0).
    signals::Connection conn = sig.connect(
        [loop, loopToken, receiverToken, fn](const Args &...args)
        {
            // Emit-side loop check (same as form 1): dead loop → silent skip, no post.
            if (!detail::token_alive(loopToken))
            {
                return;
            }
            loop->post(std::bind(
                [receiverToken, fn](const Args &...copiedArgs)
                {
                    // Drain-side receiver check: dead receiver → silently drop the delivery
                    // (SafetyFlag precedent). Runs on the loop thread, outside any lock (PIT-40).
                    if (!detail::token_alive(receiverToken))
                    {
                        return;
                    }
                    fn(copiedArgs...);
                },
                args...));
        });
    receiver->add_connection(conn);
    return conn;
}

// -------------------------------------------------------------------------------------------------
// call_and_wait — synchronous cross-thread call primitive (G2, OQ1 ruling: kernel-layer v1).
// -------------------------------------------------------------------------------------------------

/**
 * @brief Synchronization cell shared between the blocked caller and the loop-side runner.
 *
 * The loop side calls @c run(fn) exactly once; the caller calls @c wait() then inspects the
 * outcome. Handshake order: the result/exception is stored under the mutex BEFORE the done flag
 * is published, so everything the caller reads after @c wait() is already visible.
 */
class result_box_base
{
public:
    /// Blocks until the loop side has completed (value stored or exception stored).
    void wait()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mDoneCondition.wait(lock, [this] { return mDone; });
    }

    /// Non-null iff the loop-side invocation threw; valid after wait().
    std::exception_ptr exception()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mException;
    }

protected:
    void finish()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mDone = true;
        mDoneCondition.notify_one();
    }

    void finish_exception()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mException = std::current_exception();
        mDone = true;
        mDoneCondition.notify_one();
    }

    std::mutex mMutex;
    std::condition_variable mDoneCondition;
    bool mDone{false};
    std::exception_ptr mException;
};

template <typename R>
class result_box CXXKIT_FINAL : public result_box_base
{
public:
    template <typename F>
    void run(F &&fn)
    {
        try
        {
            R value(fn());
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mValue.emplace(std::move(value));
            }
            this->finish();
        }
        catch (...)
        {
            this->finish_exception();
        }
    }

    /// Valid after wait() when no exception was stored; moves the result out.
    R take()
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return std::move(*mValue);
    }

private:
    Optional<R> mValue;
};

template <>
class result_box<void> CXXKIT_FINAL : public result_box_base
{
public:
    template <typename F>
    void run(F &&fn)
    {
        try
        {
            fn();
            this->finish();
        }
        catch (...)
        {
            this->finish_exception();
        }
    }

    void take() { }
};

/**
 * @brief Calls @p fn on @p loop 's thread and blocks the calling thread until it returns.
 *
 * The synchronous counterpart of @ref connect_queued (Qt BlockingQueuedConnection shape).
 * v1 caps (by ruling): no timeout, no cancellation, single fire.
 *
 * Contract:
 * - Same-loop call is fatal by design: blocking the loop's own thread on the loop would
 *   deadlock. Stronger than Qt, which only warns at runtime.
 * - A destroyed loop is fatal: a synchronous caller would otherwise block forever. Note
 *   @ref connect_queued 's silent-skip contract does NOT carry over — the token guard here is
 *   best-effort (the raw-loop pointer is dereferenced to fetch it), so a loop that died long
 *   before the call is a caller contract violation turned fail-fast, not a guaranteed-detectable
 *   state. The loop must outlive the call.
 * - Exceptions propagate: if @p fn throws, the original exception is rethrown on the caller side.
 * - Blocking anti-pattern warning: do not hold locks or resources while calling; if the result
 *   is not needed, prefer @ref connect_queued (fire-and-forget) or EventLoop::post.
 */
template <typename F>
auto call_and_wait(EventLoop *loop, F &&fn) -> decltype(fn())
{
    CXXKIT_CHECK(loop != nullptr) << "call_and_wait requires a loop";
    if (EventLoop::current() == loop)
    {
        CXXKIT_FATAL() << "call_and_wait on the loop's own thread would deadlock";
    }
    if (!detail::token_alive(loop->alive_token()))
    {
        CXXKIT_FATAL() << "call_and_wait on a dead loop";
    }

    typedef typename std::decay<decltype(fn())>::type result_type;
    result_box<result_type> box;
    loop->post([&box, fn] { box.run(fn); });
    box.wait();
    const std::exception_ptr failure = box.exception();
    if (failure)
    {
        std::rethrow_exception(failure);
    }
    return box.take();
}


CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
