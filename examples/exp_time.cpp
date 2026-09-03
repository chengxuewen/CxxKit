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

// exp_time: ElapsedTimer and DateTime — monotonic timing and wall-clock helpers.

#include <iostream>

#include <cxxkit/time/date_time.hpp>
#include <cxxkit/time/elapsed_timer.hpp>
using namespace cxxkit;

int main()
{
    // == Section 1: ElapsedTimer times a real workload (measured values are non-deterministic) ==
    ElapsedTimer timer;
    timer.start();
    uint64_t sum = 0;
    for (int i = 0; i < 2000000; ++i)
        sum += static_cast<uint64_t>(i);
    std::cout << "[1] ElapsedTimer: loop sum=" << sum << ", elapsed=" << timer.elapsed()
              << " ms (monotonic clock, non-deterministic)" << std::endl;
    // == Section 2: has_expired(timeout) polling semantics ==
    const int64_t kTimeoutMillis = 20;
    int64_t poll_count = 0;
    while (!timer.has_expired(kTimeoutMillis))
        ++poll_count;
    std::cout << "[2] has_expired(" << kTimeoutMillis << " ms) -> " << std::boolalpha
              << timer.has_expired(kTimeoutMillis) << " after " << poll_count << " polls (non-deterministic)"
              << std::endl;
    // == Section 3: DateTime wall-clock UTC epoch millis and formatted local time ==
    std::cout << "[3] DateTime::time_utc_millis() = " << DateTime::time_utc_millis()
              << " (UTC epoch ms); local_time_string() = " << DateTime::local_time_string() << std::endl;
    return 0;
}
