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

#ifndef _CXXKIT_FAKE_CLOCK_HPP
#define _CXXKIT_FAKE_CLOCK_HPP

#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/time/date_time.hpp>
#include <cxxkit/units/timestamp.hpp>
#include <cxxkit/thread/mutex.hpp>

CXXKIT_BEGIN_NAMESPACE

// Fake clock for use with unit tests, which does not tick on its own.
// Starts at time 0.
//
// TODO(deadbeef): Unify with SimulatedClock.
class CXXKIT_CORE_API FakeClock : public ClockInterface
{
public:
    FakeClock() = default;
    FakeClock(const FakeClock &) = delete;
    FakeClock &operator=(const FakeClock &) = delete;
    ~FakeClock() override = default;

    // ClockInterface implementation.
    int64_t TimeNanos() const override;

    // Methods that can be used by the test to control the time.

    // Should only be used to set a time in the future.
    void SetTime(Timestamp new_time);

    void AdvanceTime(TimeDelta delta);

private:
    mutable Mutex lock_;
    int64_t time_ns_ CXXKIT_ATTRIBUTE_GUARDED_BY(lock_) = 0;
};

class CXXKIT_CORE_API ThreadProcessingFakeClock : public ClockInterface
{
public:
    int64_t TimeNanos() const override { return clock_.TimeNanos(); }
    void SetTime(Timestamp time);
    void AdvanceTime(TimeDelta delta);

private:
    FakeClock clock_;
};

// Helper class that sets itself as the global clock in its constructor and
// unsets it in its destructor.
class CXXKIT_CORE_API ScopedBaseFakeClock : public FakeClock
{
public:
    ScopedBaseFakeClock();
    ~ScopedBaseFakeClock() override;

private:
    ClockInterface *prev_clock_;
};

// TODO(srte): Rename this to reflect that it also does thread processing.
class CXXKIT_CORE_API ScopedFakeClock : public ThreadProcessingFakeClock
{
public:
    ScopedFakeClock();
    ~ScopedFakeClock() override;

private:
    ClockInterface *prev_clock_;
};
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_FAKE_CLOCK_HPP
