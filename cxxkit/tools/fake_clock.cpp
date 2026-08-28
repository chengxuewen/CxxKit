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

int64_t FakeClock::time_nanos() const
{
    Mutex::UniqueLock locker(mLock);
    return mTimeNs;
}

void FakeClock::set_time(Timestamp new_time)
{
    Mutex::UniqueLock locker(mLock);
    CXXKIT_DCHECK(new_time.us() * 1000 >= mTimeNs);
    mTimeNs = new_time.us() * 1000;
}

void FakeClock::advance_time(TimeDelta delta)
{
    Mutex::UniqueLock locker(mLock);
    mTimeNs += delta.ns();
}

void ThreadProcessingFakeClock::set_time(Timestamp time)
{
    mClock.set_time(time);
    // If message queues are waiting in a socket select() with a timeout provided
    // by the OS, they should wake up and dispatch all messages that are ready.
    // TaskThreadManager::ProcessAllMessageQueuesForTesting(); //TODO
}

void ThreadProcessingFakeClock::advance_time(TimeDelta delta)
{
    mClock.advance_time(delta);
    // TaskThreadManager::ProcessAllMessageQueuesForTesting(); //TODO
}

ScopedBaseFakeClock::ScopedBaseFakeClock()
{
    mPrevClock = set_clock_for_testing(this);
}

ScopedBaseFakeClock::~ScopedBaseFakeClock()
{
    set_clock_for_testing(mPrevClock);
}

ScopedFakeClock::ScopedFakeClock()
{
    mPrevClock = set_clock_for_testing(this);
}

ScopedFakeClock::~ScopedFakeClock()
{
    set_clock_for_testing(mPrevClock);
}
CXXKIT_END_NAMESPACE
