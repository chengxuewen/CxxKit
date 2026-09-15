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

// exp_kernel: signals walkthrough (sigslot port) + SignalR combiners + cross-thread emit.
#include <iostream>
#include <memory>
#include <vector>

#include <cxxkit/base/global.hpp>
#include <cxxkit/kernel/signals.hpp>
#include <atomic>
#include <thread>

using cxxkit::signals::Connection;
using cxxkit::signals::ScopedConnection;
using cxxkit::signals::Signal;
using cxxkit::signals::SignalR;

namespace
{

struct Alarm
{
    void on_temperature(int celsius) { std::cout << "  [member] alarm at " << celsius << "C" << std::endl; }
};

// Tracked slot owner: destroyed mid-demo to show emit skipping dead tracked slots.
struct TrackedListener
{
    void on_event(int v) { std::cout << "  [tracked] got " << v << std::endl; }
};

} // namespace

int main()
{
    // 1) Basic connect + emit: lambda slot then member-function slot, fire in connect order.
    std::cout << "--- 1. connect two slots, emit ---" << std::endl;
    Signal<int> sig;
    sig.connect([](int value) { std::cout << "  [lambda] got " << value << std::endl; });
    Alarm alarm;
    sig.connect(&Alarm::on_temperature, &alarm);
    sig(75);

    // 2) ScopedConnection RAII: the slot lives exactly as long as the scope.
    std::cout << "--- 2. ScopedConnection auto-disconnects ---" << std::endl;
    std::vector<int> sink;
    {
        ScopedConnection scoped = sig.connect_scoped([&sink](int value) { sink.push_back(value); });
        sig(1);
        std::cout << "  inside scope: " << sink.size() << " delivery" << std::endl;
    }
    sig(2);
    std::cout << "  after scope: " << sink.size() << " deliveries (still 1)" << std::endl;

    // 3) ConnectionBlocker: mutes one slot temporarily, restores on scope exit.
    std::cout << "--- 3. ConnectionBlocker mutes a slot ---" << std::endl;
    int calls = 0;
    Connection conn = sig.connect([&calls](int) { ++calls; });
    {
        cxxkit::signals::ConnectionBlocker blocker = conn.blocker();
        sig(10);
        std::cout << "  while blocked: " << calls << " calls" << std::endl;
    }
    sig(20);
    std::cout << "  after unblock: " << calls << " calls" << std::endl;

    // 4) Signal destruction: every outstanding Connection goes invalid at once.
    std::cout << "--- 4. signal dtor clears connections ---" << std::endl;
    Connection survivor;
    {
        Signal<int> temp;
        survivor = temp.connect([](int) { });
        std::cout << "  before dtor: valid=" << survivor.valid() << std::endl;
    }
    std::cout << "  after dtor:  valid=" << survivor.valid() << std::endl;

    // 5) Tracked lifetime: a slot bound to a shared_ptr-tracked object is skipped
    // safely once the object dies -- no dangling call; live slots keep receiving.
    std::cout << "--- 5. tracked slot dies mid-demo, emit skips it ---" << std::endl;
    Signal<int> tracked_sig;
    {
        auto listener = std::make_shared<TrackedListener>();
        tracked_sig.connect(&TrackedListener::on_event, listener);
        tracked_sig(1); // tracked slot alive: delivered
        std::cout << "  alive: delivered to tracked slot" << std::endl;
    } // listener destroyed here
    tracked_sig(2); // safe: dead tracked slot skipped, no dangling call
    std::cout << "  dead:  tracked slot skipped safely" << std::endl;

    // 6) Combiners: SignalR aggregates slot return values per emission.
    std::cout << "--- 6. SignalR combiners ---" << std::endl;
    SignalR<int> last_sig; // default optional_last_value: last slot's result wins
    last_sig.connect([] { return 10; });
    last_sig.connect([] { return 20; });
    auto last = last_sig();
    std::cout << "  optional_last_value: last=" << *last << std::endl;

    cxxkit::signals::SignalR<int, cxxkit::signals::maximum<int>> max_sig;
    max_sig.connect([] { return 10; });
    max_sig.connect([] { return 42; });
    max_sig.connect([] { return 30; });
    auto max_v = max_sig();
    std::cout << "  maximum: max=" << *max_v << std::endl;

    // 7) Cross-thread emit: a worker thread emits 1000 values while main is
    // connected; main joins the worker before reading the counter, so the
    // printed result is deterministic (join-before-print).
    std::cout << "--- 7. cross-thread emit ---" << std::endl;
    Signal<int> cross_sig;
    std::atomic<int> received{0};
    cross_sig.connect([&](int v) { received.fetch_add(v); });
    {
        std::thread emitter(
            [&cross_sig]
            {
                for (int i = 1; i <= 1000; ++i)
                {
                    cross_sig(i);
                }
            });
        emitter.join(); // all emissions complete before anything below runs
    }
    std::cout << "  joined: received=" << received.load() << " (expected 500500)" << std::endl;

    std::cout << "--- done ---" << std::endl;
    return 0;
}
