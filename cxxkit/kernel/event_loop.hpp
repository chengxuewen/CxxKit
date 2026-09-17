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

#include <cxxkit/kernel/object.hpp>
#include <cxxkit/tools/enum_flags.hpp>
#include <cxxkit/base/core_config.hpp>

#include <cstdint>
#include <functional>
#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class AbstractEventDispatcher;
class EventLoopPrivate;
class CXXKIT_KERNEL_API EventLoop : public Object
{
public:
    enum class ProcessFlag
    {
        kAllEvents = 0x00,
        kWaitForMoreEvents = 0x04,
        kEventLoopExec = 0x20
    };
    CXXKIT_DECLARE_ENUM_FLAGS(ProcessFlags, ProcessFlag)

    /**
     * @brief Creates an EventLoop driven by @p dispatcher.
     *
     * The dispatcher is injected once and owned exclusively by the loop. A null dispatcher is
     * a fatal error — there is no valid dispatcher-less state.
     */
    explicit EventLoop(std::unique_ptr<AbstractEventDispatcher> dispatcher, Object *parent = nullptr);

    /**
     * @brief Creates an EventLoop driven by the default loop dispatcher (@since 0.2).
     *
     * Convenience overload — the QtCore model: @c cxxkit::EventLoop loop; runs out of the box on the
     * kernel's built-in engine. Equivalent to constructing with @ref make_default_dispatcher(); fatal
     * if @c CXXKIT_ENABLE_LOOP_BACKEND_UV is OFF and no engine was built in.
     */
    explicit EventLoop(Object *parent = nullptr);
    ~EventLoop() override;

    /**
     * @brief Processes pending events without blocking; returns what the dispatcher returned.
     *
     * Drain contract, in two parts:
     *
     * (a) Posted-task queue = snapshot semantics. The whole queue is swapped out in one shot
     * (take_post_queue) and the snapshot is drained to completion. Tasks posted during that
     * round land in the fresh queue and run on the NEXT round — they do not extend the current
     * one. Bound your own drain loops; a task that re-posts itself every round runs once per
     * round, forever (that is the intended idiom for periodic work).
     *
     * (b) Event queue = drain-until-sentinel. Entries are popped and dispatched one by one
     * until the queue reports empty. Events posted during dispatch (including events re-posted
     * by the code being dispatched) join the SAME round. Warning — amplification: a handler
     * that post_event()s to itself unconditionally therefore never lets the drain terminate;
     * guard re-posts (e.g. dirty flags) or the loop spins. When you need a bounded time slice
     * instead, use the process_events(flags, maximum_ms) overload.
     */
    bool process_events(ProcessFlags flags = ProcessFlag::kAllEvents);

    /**
     * @brief Processes events for at most @p maximum_ms.
     *
     * Shell synthesis: a one-shot interrupt timer is registered through the public timer path;
     * the loop drains posted tasks and delegates to the dispatcher with kWaitForMoreEvents until
     * the timeout fires or an event arrives. Returns true if at least one event was processed,
     * false on timeout.
     */
    bool process_events(ProcessFlags flags, uint64_t maximum_ms);

    /**
     * @brief Enqueues @p fn for execution on the loop thread. Thread-safe; callable from any thread.
     */
    void post(std::function<void()> fn);

    /**
     * @brief Registers a timer; returns its id (unique, non-zero).
     *
     * With repeat == false the shell wraps @p fn into "stop_timer(id) first, then fn" so the
     * callback fires exactly once (M3). The wrapper captures this — see the lifecycle contract
     * (event-loop design spec, appendix C): the loop must outlive pending one-shot timers.
     *
     * Zero-interval one-shot fast path: start_timer(0, fn, false) delegates to post(fn)
     * (Qt singleShotImpl precedent) — the callback runs FIFO with posted work on the next
     * drain, no engine timer registration. The returned id is a ghost id: unique and non-zero,
     * but no dispatcher entry exists for it, so stop_timer(ghostId) is a no-op (safe to call).
     * repeat == true with interval_ms == 0 still registers through the engine: a zero-period
     * repeating timer fires every loop round (Qt semantics).
     *
     * Repeating-timer contract:
     * - Ordering at interval == 0 is backend-defined: do not rely on where a zero-period
     *   repeating timer lands relative to posted tasks or other timers within a round.
     * - Overrun collapse: if a callback overruns its interval (or the loop misses deadlines),
     *   missed expirations collapse into ONE catch-up fire; the timer never emits a burst.
     *   Verified for the uv backend (libuv timer.c arms each single-shot fire); asio audit
     *   pending (tracked separately).
     * - Precision = backend-native. There is no Qt-style Coarse tolerance tier (qtimer.h only
     *   applies a >=2s Coarse heuristic upstream; we have no such layer). Qt-behavior sentences
     *   in this contract paraphrase the Qt QTimer documentation (qtimer.html), not verbatim
     *   quotes.
     */
    int start_timer(uint64_t interval_ms, std::function<void()> fn, bool repeat = true);

    /**
     * @brief Cancels a timer previously registered by start_timer.
     */
    void stop_timer(int timer_id);

    /**
     * @brief Returns the EventLoop running on the calling thread, or nullptr if none.
     *
     * Set by exec() for its duration (nested exec restores the previous value on exit);
     * cleared in ~EventLoop if it still points here.
     */
    static EventLoop *current();

    /**
     * @brief Runs the loop until exit() — the blocking variant of process_events.
     *
     * Nested-loop contract is backend-split: the Qt host-bridge dispatcher allows nesting
     * (Qt itself nests event loops), while the uv engine is fatal on nested iteration (libuv
     * has no nested uv_run). Cross-backend portable code therefore MUST NOT nest exec()
     * calls — structure such flows with an inner process_events() slice instead.
     */
    int exec(ProcessFlags flags = ProcessFlag::kAllEvents);

    void wake_up();

    void exit(int retCode = 0);
    void quit();

    bool is_running() const;

    /**
     * @brief Returns the loop's injected dispatcher (phase-2 IO consumers).
     *
     * The dispatcher is owned exclusively by the loop and is never null (construction contract).
     * Phase-2 IO classes (TcpSocket/TcpServer over cxxkit::network) reach the engine through this accessor
     * instead of each carrying their own engine reference. The dispatcher's own threading contract
     * applies — engine registrations are loop-thread only.
     */
    AbstractEventDispatcher &dispatcher();

    /**
     * @brief Returns this loop's liveness token (kernel-internal: the connect_queued
     * family's dead-loop check, design §1.3).
     *
     * The token expires naturally when the loop's private is destroyed — no manual
     * invalidation point. It stays alive through the ~EventLoop posted-task drain (the
     * "delete_later closures must run" invariant), so closures already enqueued keep
     * running; after destruction, emit-side checks fail and the emission silently skips
     * instead of posting into the dangling loop.
     */
    std::weak_ptr<std::atomic<bool>> alive_token();

    bool event(Event *event) override;

private:
    CXXKIT_DECLARE_PRIVATE(EventLoop)
    CXXKIT_DISABLE_COPY_MOVE(EventLoop)

    // kernel 内部协作：Object::post_event 入队 + ~Object 清 pending（Momus F3：仅此一个 friend）
    friend class Object;

    /** @brief The single enqueue channel: the caller passes the target loop (null = fatal);
     *  pushes {receiver, event} under the lock.
     *  T2: explicit loop parameter — post_event routes by receiver->loop() affinity,
     *  decoupled from the calling thread.
     *  C2 compression: a DeferredDeleteEvent whose receiver already has one queued is
     *  deleted instead of enqueued (scan + decision both under the lock).
     *  C3 priority: stable sorted insert AFTER the compression scan — inserted before the
     *  first entry with a strictly smaller priority; otherwise push_back. With all-default-0
     *  priorities this degenerates to an exact push_back (byte-identical legacy behavior). */
    static void enqueue_event(EventLoop *loop, Object *receiver, Event *event, int priority = 0);

    /**
     * @brief ~Object 清 pending 通道：锁内 remove+delete 匹配 receiver 的条目（含 DeferredDeleteEvent）。
     *
     * Contract (D34): purge must be mutually exclusive with pop_event_entry under the lock —
     * single-threaded cascade destruction can interleave during dispatch (parent dispatch deletes
     * child → child/grandchild purge), so purge removes undelivered entries from the **queue body**
     * (never a cached snapshot); a popped entry can no longer exist — no dangling window; dispatch
     * itself runs outside the lock. Null-loop tolerance: no loop means no pending entries =
     * defensive no-op.
     * T2: explicit loop parameter — ~Object double-purges the affinity loop + the current() residual
     * (a same-loop second purge is a harmless no-op).
     */
    static void purge_pending(EventLoop *loop, Object *receiver);
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
