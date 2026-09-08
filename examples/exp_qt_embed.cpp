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
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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

/// @file exp_qt_embed.cpp
/// @brief End-to-end Qt embed walkthrough: cxxkit EventLoop driven by a host Qt loop (needs
///        CXXKIT_ENABLE_LIB_QT=ON). No GUI — QCoreApplication only; runtime gate is docs/qt-embed-smoke.md.

#include <cxxkit/kernel/connect_queued.hpp>
#include <cxxkit/kernel/event_loop.hpp>
#include <cxxkit/qt/qt_event_dispatcher.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QTimer>

// Qt rewrites `signals` to Q_SIGNALS; the kernel headers above are parsed BEFORE any Qt header
// (letter-sort: cxxkit < QtCore), so cxxkit::signals:: was declared clean. This #undef restores
// the token for the body below.
#undef signals

#include <cstdio>
#include <memory>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    cxxkit::EventLoop loop(std::unique_ptr<cxxkit::QtEventDispatcher>(new cxxkit::QtEventDispatcher()));

    // Embedded-form constraint (R-C1-5): the shell post queue drains only when someone calls
    // process_events — pump it from a host-side bridge QTimer on a short cadence.
    QTimer bridge;
    QObject::connect(&bridge, &QTimer::timeout, [&]() { loop.process_events(); });
    bridge.start(10);

    // Host-first-class timer: 100ms x 3 ticks, then end the loop.
    int ticks = 0;
    loop.start_timer(100,
                     [&]()
                     {
                         ++ticks;
                         std::printf("tick %d\n", ticks);
                         if (ticks == 3)
                         {
                             loop.exit(0);
                         }
                     });

    // Queued delivery: emit once before the run; connect_queued hops the slot onto the loop
    // thread via EventLoop::post, the bridge pump delivers it.
    cxxkit::Signal<int> sig;
    cxxkit::signals::Connection conn = cxxkit::connect_queued(sig,
                                                              &loop,
                                                              [](int v) { std::printf("queued: %d\n", v); });
    sig(42);

    // exec() is an optional API under the embedded form (the host app.exec() can own the run —
    // an equivalent host-shaped variant is `bridge.start(10); return app.exec();` with no loop.exec);
    // demonstrated here so the example exits deterministically after the third tick.
    const int rc = loop.exec();

    std::printf("rc: %d (ticks: %d)\n", rc, ticks);
    conn.disconnect();
    return rc;
}
