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
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/qt/qt_event_dispatcher.hpp>

#include <cxxkit/base/macros.hpp>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <atomic>
#include <functional>
#include <map>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/** @brief Private implementation of @ref QtEventDispatcher (CXXKIT_DEFINE_DPTR pimpl partner; flat namespace
 * per project convention — same layout as uv's detail/uv_event_dispatcher_p.hpp).
 */
class QtEventDispatcherPrivate
{
    CXXKIT_DISABLE_COPY_MOVE(QtEventDispatcherPrivate)

public:
    explicit QtEventDispatcherPrivate(QtEventDispatcher *p, QObject *context);

    void check_loop_thread(const char *api) const;

    QPointer<QObject> mContext;      /// host context; goes null after its destruction (F8-Qt doorbell guard)
    std::map<int, QTimer *> mTimers; /// timer_id -> live QTimer (heap-allocated, deleteLater'd)
    std::atomic<bool> mBellPending{
        false}; /// doorbell coalescing: set by wake_up, cleared at process_events entry (R-C1-6)
    std::atomic<bool> mInterrupted{false}; /// set by interrupt(); near no-op under the pass-through shape (R-C1-8)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
