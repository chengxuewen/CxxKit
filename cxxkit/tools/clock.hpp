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

#pragma once

#include <cxxkit/tools/tools_global.hpp>

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
class CXXKIT_TOOLS_API Clock
{
public:
    virtual ~Clock() { }

    // Return a timestamp relative to an unspecified epoch.
    virtual Timestamp current_time() = 0;
    int64_t time_in_milliseconds() { return current_time().ms(); }
    int64_t time_in_microseconds() { return current_time().us(); }

    // Retrieve an NTP absolute timestamp (with an epoch of Jan 1, 1900).
    NtpTime current_ntp_time() { return convert_timestamp_to_ntp_time(current_time()); }
    int64_t current_ntp_in_milliseconds() { return current_ntp_time().to_ms(); }

    // Converts between a relative timestamp returned by this clock, to NTP time.
    virtual NtpTime convert_timestamp_to_ntp_time(Timestamp timestamp) = 0;
    int64_t convert_timestamp_to_ntp_time_in_milliseconds(int64_t timestamp_ms)
    {
        return convert_timestamp_to_ntp_time(Timestamp::millis(timestamp_ms)).to_ms();
    }

    // Converts NtpTime to a Timestamp with UTC epoch.
    // A `Minus infinity` Timestamp is returned if the NtpTime is invalid.
    static Timestamp ntp_to_utc(NtpTime ntp_time)
    {
        if (!ntp_time.valid())
        {
            return Timestamp::minus_infinity();
        }
        // seconds since UTC epoch.
        int64_t time = ntp_time.seconds() - kNtpJan1970;
        // Microseconds since UTC epoch (not including NTP fraction)
        time = time * 1000000;
        // Fractions part of the NTP time, in microseconds.
        int64_t time_fraction = divide_round_to_nearest(int64_t{ntp_time.fractions()} * 1000000,
                                                     NtpTime::kFractionsPerSecond);
        return Timestamp::micros(time + time_fraction);
    }

    // Returns an instance of the real-time system clock implementation.
    static Clock *get_real_time_clock();
};

class CXXKIT_TOOLS_API SimulatedClock : public Clock
{
public:
    // The constructors assume an epoch of Jan 1, 1970.
    explicit SimulatedClock(int64_t initial_time_us);
    explicit SimulatedClock(Timestamp initial_time);
    ~SimulatedClock() override;

    // Return a timestamp with an epoch of Jan 1, 1970.
    Timestamp current_time() override;

    NtpTime convert_timestamp_to_ntp_time(Timestamp timestamp) override;

    // Advance the simulated clock with a given number of milliseconds or
    // microseconds.
    void advance_time_milliseconds(int64_t milliseconds);
    void advance_time_microseconds(int64_t microseconds);
    void advance_time(TimeDelta delta);

private:
    // The time is read and incremented with relaxed order. Each thread will see
    // monotonically increasing time, and when threads post tasks or messages to
    // one another, the synchronization done as part of the message passing should
    // ensure that any causual chain of events on multiple threads also
    // corresponds to monotonically increasing time.
    std::atomic<int64_t> mTimeUs;
};
CXXKIT_END_NAMESPACE

