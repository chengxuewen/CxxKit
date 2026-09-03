/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
** WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
** OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
** OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
***********************************************************************************************************************/

// exp_numerics: RTC-flavored numeric algorithms — RunningStatistics / ExpFilter / sequence unwrapping.

#include <iostream>

#include <cxxkit/numerics/exp_filter.hpp>
#include <cxxkit/numerics/running_statistics.hpp>
#include <cxxkit/numerics/sequence_number_unwrapper.hpp>

using namespace cxxkit;

int main()
{
    // 1) Simulated delay control: stream latency samples through Welford's running statistics.
    const double latency_samples[] = {42.0, 38.5, 51.0, 47.5, 40.0, 55.0, 43.0, 39.5, 58.0, 45.0};
    RunningStatistics<double> delay_stats;
    for (const double sample : latency_samples)
    {
        delay_stats.add_sample(sample);
    }
    std::cout << "[measured] delay samples=" << delay_stats.size() << " mean=" << *delay_stats.get_mean()
              << " stddev=" << *delay_stats.get_standard_deviation() << std::endl;

    // 2) Simulated bandwidth estimation: smooth a noisy signal with an exponential filter.
    ExpFilter bandwidth_filter(0.9f);
    const float observed_bitrate[] = {800.0f, 500.0f, 950.0f, 700.0f, 880.0f, 620.0f, 810.0f};
    std::cout << "[measured] exp filter convergence:";
    for (const float observation : observed_bitrate)
    {
        bandwidth_filter.apply(1.0f, observation);
        std::cout << " " << bandwidth_filter.filtered();
    }
    std::cout << std::endl;

    // 3) Simulated RTP stream: unwrap 16-bit sequence numbers across the wrap-around.
    rtp_sequence_number_unwrapper unwrapper;
    const uint16_t wire_sequence[] = {65534, 65535, 0, 1, 2};
    std::cout << "[measured] unwrapped sequence:";
    for (const uint16_t wire : wire_sequence)
    {
        std::cout << " " << unwrapper.unwrap(wire);
    }
    std::cout << std::endl;
    return 0;
}
