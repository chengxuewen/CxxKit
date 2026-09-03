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

// exp_units: strongly-typed unit arithmetic — DataSize/DataRate/TimeDelta/Frequency walkthrough.

#include <iostream>

#include <cxxkit/time/elapsed_timer.hpp>
#include <cxxkit/units/data_rate.hpp>
#include <cxxkit/units/data_size.hpp>
#include <cxxkit/units/frequency.hpp>
#include <cxxkit/units/time_delta.hpp>

using namespace cxxkit;

// Scene: a 1.5 MB video chunk is shipped over a link; we derive the transfer
// time, frame pacing and observed frame frequency from strongly typed units.
// Every step keeps its unit type, so mixing bits and bytes is a compile error.

static void scene_chunk_transfer()
{
    std::cout << "[1] chunk transfer: DataSize / DataRate -> TimeDelta" << std::endl;

    const DataSize chunk = DataSize::bytes(1500000);       // 1.5 MB video chunk
    const DataRate link = DataRate::kilobits_per_sec(800); // 800 kbps link
    std::cout << "chunk: " << chunk.bytes<int64_t>() << " bytes" << std::endl;
    std::cout << "link : " << link.kbps<int64_t>() << " kbps = " << link.bytes_per_sec<double>() << " B/s" << std::endl;

    // Cross-unit division: DataSize / DataRate -> TimeDelta (compile-checked).
    const TimeDelta transfer_time = chunk / link;
    std::cout << "transfer time = chunk / link = " << transfer_time.ms<int64_t>() << " ms" << std::endl;

    // Typed comparison instead of raw int compare.
    std::cout << "over 10 s budget? " << (transfer_time > TimeDelta::seconds(10) ? "yes" : "no") << std::endl;
}

static void scene_frame_pacing()
{
    std::cout << "[2] frame pacing: TimeDelta arithmetic + Rate*Time -> DataSize" << std::endl;

    // TimeDelta + TimeDelta stays TimeDelta.
    const TimeDelta frame_interval = TimeDelta::millis(20);
    const TimeDelta two_frames = frame_interval + frame_interval;
    std::cout << "two frames at 50 fps need " << two_frames.ms<int64_t>() << " ms" << std::endl;

    // 1 / interval -> Frequency.
    const Frequency fps = 1 / frame_interval;
    std::cout << "frame frequency = 1 / interval = " << fps.hertz<double>() << " Hz" << std::endl;

    // DataRate * TimeDelta -> DataSize: how much an 800 kbps link moves per frame.
    const DataRate link = DataRate::kilobits_per_sec(800);
    const DataSize per_frame = link * frame_interval;
    std::cout << "bytes per frame = link * interval = " << per_frame.bytes<int64_t>() << " bytes" << std::endl;
}

static void scene_elapsed_timer()
{
    std::cout << "[3] ElapsedTimer: measured wall time is runtime-dependent" << std::endl;

    ElapsedTimer timer;
    timer.start();

    DataSize moved = DataSize::Zero();
    const int kFrames = 50;
    for (int i = 0; i < kFrames; ++i)
    {
        moved += DataRate::kilobits_per_sec(800) * TimeDelta::millis(20);
    }
    std::cout << "accounted payload: " << moved.bytes<int64_t>() << " bytes in " << kFrames << " frames" << std::endl;

    const int64_t elapsed_ms = timer.elapsed(); // nondeterministic, wall clock
    std::cout << "measured loop time = " << elapsed_ms << " ms (nondeterministic value)" << std::endl;
}

int main()
{
    scene_chunk_transfer();
    scene_frame_pacing();
    scene_elapsed_timer();
    return 0;
}
