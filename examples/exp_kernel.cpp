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

// exp_kernel: signals-only walkthrough (sigslot port); Object/EventLoop await kernel completion.
#include <iostream>
#include <vector>

#include <cxxkit/base/global.hpp>
#include <cxxkit/kernel/signals.hpp>

using cxxkit::signals::Connection;
using cxxkit::signals::ScopedConnection;
using cxxkit::signals::Signal;

namespace
{

struct Alarm
{
    void on_temperature(int celsius) { std::cout << "  [member] alarm at " << celsius << "C" << std::endl; }
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

    std::cout << "--- done ---" << std::endl;
    return 0;
}
