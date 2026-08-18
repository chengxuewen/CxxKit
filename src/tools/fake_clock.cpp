/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
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

#include <cxxkit/tools/fake_clock.hpp>

CXXKIT_BEGIN_NAMESPACE

int64_t FakeClock::TimeNanos() const
{
    Mutex::UniqueLock locker(lock_);
    return time_ns_;
}

void FakeClock::SetTime(Timestamp new_time)
{
    Mutex::UniqueLock locker(lock_);
    CXXKIT_DCHECK(new_time.us() * 1000 >= time_ns_);
    time_ns_ = new_time.us() * 1000;
}

void FakeClock::AdvanceTime(TimeDelta delta)
{
    Mutex::UniqueLock locker(lock_);
    time_ns_ += delta.ns();
}

void ThreadProcessingFakeClock::SetTime(Timestamp time)
{
    clock_.SetTime(time);
    // If message queues are waiting in a socket select() with a timeout provided
    // by the OS, they should wake up and dispatch all messages that are ready.
    // TaskThreadManager::ProcessAllMessageQueuesForTesting(); //TODO
}

void ThreadProcessingFakeClock::AdvanceTime(TimeDelta delta)
{
    clock_.AdvanceTime(delta);
    // TaskThreadManager::ProcessAllMessageQueuesForTesting(); //TODO
}

ScopedBaseFakeClock::ScopedBaseFakeClock()
{
    prev_clock_ = SetClockForTesting(this);
}

ScopedBaseFakeClock::~ScopedBaseFakeClock()
{
    SetClockForTesting(prev_clock_);
}

ScopedFakeClock::ScopedFakeClock()
{
    prev_clock_ = SetClockForTesting(this);
}

ScopedFakeClock::~ScopedFakeClock()
{
    SetClockForTesting(prev_clock_);
}
CXXKIT_END_NAMESPACE
