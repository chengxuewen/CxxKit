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

#ifndef _CXXKIT_CLOCK_HPP
#define _CXXKIT_CLOCK_HPP

#include <cxxkit/units/timestamp.hpp>
#include <cxxkit/tools/ntp_time.hpp>

#include <cstdint>
#include <atomic>
#include <memory>

CXXKIT_BEGIN_NAMESPACE

// January 1970, in NTP seconds.
const uint32_t kNtpJan1970 = 2208988800UL;

// Magic NTP fractional unit.
const double kMagicNtpFractionalUnit = 4.294967296E+9;

// A clock interface that allows reading of absolute and relative timestamps.
class CXXKIT_CORE_API Clock
{
public:
    virtual ~Clock() {}

    // Return a timestamp relative to an unspecified epoch.
    virtual Timestamp CurrentTime() = 0;
    int64_t TimeInMilliseconds() { return CurrentTime().ms(); }
    int64_t TimeInMicroseconds() { return CurrentTime().us(); }

    // Retrieve an NTP absolute timestamp (with an epoch of Jan 1, 1900).
    NtpTime CurrentNtpTime() { return ConvertTimestampToNtpTime(CurrentTime()); }
    int64_t CurrentNtpInMilliseconds() { return CurrentNtpTime().ToMs(); }

    // Converts between a relative timestamp returned by this clock, to NTP time.
    virtual NtpTime ConvertTimestampToNtpTime(Timestamp timestamp) = 0;
    int64_t ConvertTimestampToNtpTimeInMilliseconds(int64_t timestamp_ms)
    {
        return ConvertTimestampToNtpTime(Timestamp::Millis(timestamp_ms)).ToMs();
    }

    // Converts NtpTime to a Timestamp with UTC epoch.
    // A `Minus Infinity` Timestamp is returned if the NtpTime is invalid.
    static Timestamp NtpToUtc(NtpTime ntp_time)
    {
        if (!ntp_time.Valid())
        {
            return Timestamp::MinusInfinity();
        }
        // Seconds since UTC epoch.
        int64_t time = ntp_time.seconds() - kNtpJan1970;
        // Microseconds since UTC epoch (not including NTP fraction)
        time = time * 1000000;
        // Fractions part of the NTP time, in microseconds.
        int64_t time_fraction = DivideRoundToNearest(int64_t{ntp_time.fractions()} * 1000000,
                                                     NtpTime::kFractionsPerSecond);
        return Timestamp::Micros(time + time_fraction);
    }

    // Returns an instance of the real-time system clock implementation.
    static Clock *GetRealTimeClock();
};

class CXXKIT_CORE_API SimulatedClock : public Clock
{
public:
    // The constructors assume an epoch of Jan 1, 1970.
    explicit SimulatedClock(int64_t initial_time_us);
    explicit SimulatedClock(Timestamp initial_time);
    ~SimulatedClock() override;

    // Return a timestamp with an epoch of Jan 1, 1970.
    Timestamp CurrentTime() override;

    NtpTime ConvertTimestampToNtpTime(Timestamp timestamp) override;

    // Advance the simulated clock with a given number of milliseconds or
    // microseconds.
    void AdvanceTimeMilliseconds(int64_t milliseconds);
    void AdvanceTimeMicroseconds(int64_t microseconds);
    void AdvanceTime(TimeDelta delta);

private:
    // The time is read and incremented with relaxed order. Each thread will see
    // monotonically increasing time, and when threads post tasks or messages to
    // one another, the synchronization done as part of the message passing should
    // ensure that any causual chain of events on multiple threads also
    // corresponds to monotonically increasing time.
    std::atomic<int64_t> time_us_;
};
CXXKIT_END_NAMESPACE

#endif // _CXXKIT_CLOCK_HPP
