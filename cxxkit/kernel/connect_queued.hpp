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

#include <functional>

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

/**
 * @brief Connects @p sig to a slot that hops onto @p loop: each emission copies the arguments
 * and enqueues @p fn to run on the loop thread via EventLoop::post.
 *
 * Value-copy semantics: arguments are copied once at emit time (std::bind decay-copies), so
 * types must be copy-constructible; mutating the emitted variables afterwards does not affect
 * a pending delivery. Because @p fn runs on the loop thread, later re-entrancy through the
 * loop is safe.
 *
 * Lifecycle: @p loop must outlive the returned connection (event-loop design spec, appendix C ④).
 *
 * Disconnection (spec I7): disconnect() stops future deliveries only — deliveries already
 * enqueued before the disconnect still run. To undo safely, stop the emit source first or use
 * a safety flag (PendingTaskSafetyFlag, spec M5/S8).
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
    return sig.connect([loop, fn](const Args &...args) { loop->post(std::bind(fn, args...)); });
}

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
