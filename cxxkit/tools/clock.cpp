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

#include <cxxkit/tools/clock.hpp>
#include <cxxkit/time/date_time.hpp>

CXXKIT_BEGIN_NAMESPACE

int64_t NtpOffsetUsCalledOnce()
{
    constexpr int64_t kNtpJan1970Sec = 2208988800;
    int64_t clock_time = DateTime::TimeMicros();
    int64_t utc_time = DateTime::TimeUTCMicros();
    return utc_time - clock_time + kNtpJan1970Sec * DateTime::kUSecsPerSec;
}

NtpTime TimeMicrosToNtp(int64_t time_us)
{
    static int64_t ntp_offset_us = NtpOffsetUsCalledOnce();

    int64_t time_ntp_us = time_us + ntp_offset_us;
    CXXKIT_DCHECK_GE(time_ntp_us, 0); // Time before year 1900 is unsupported.

    // Convert seconds to uint32 through uint64 for a well-defined cast.
    // A wrap around, which will happen in 2036, is expected for NTP time.
    uint32_t ntp_seconds = static_cast<uint64_t>(time_ntp_us / DateTime::kUSecsPerSec);

    // Scale fractions of the second to NTP resolution.
    constexpr int64_t kNtpFractionsInSecond = 1LL << 32;
    int64_t us_fractions = time_ntp_us % DateTime::kUSecsPerSec;
    uint32_t ntp_fractions = us_fractions * kNtpFractionsInSecond / DateTime::kUSecsPerSec;

    return NtpTime(ntp_seconds, ntp_fractions);
}

class RealTimeClock : public Clock
{
public:
    RealTimeClock() = default;

    Timestamp CurrentTime() override { return Timestamp::Micros(DateTime::TimeMicros()); }

    NtpTime ConvertTimestampToNtpTime(Timestamp timestamp) override { return TimeMicrosToNtp(timestamp.us()); }
};

Clock *Clock::GetRealTimeClock()
{
    static Clock *const clock = new RealTimeClock();
    return clock;
}

SimulatedClock::SimulatedClock(int64_t initial_time_us)
    : mTimeUs(initial_time_us)
{
}

SimulatedClock::SimulatedClock(Timestamp initial_time)
    : SimulatedClock(initial_time.us())
{
}

SimulatedClock::~SimulatedClock()
{
}

Timestamp SimulatedClock::CurrentTime()
{
    return Timestamp::Micros(mTimeUs.load(std::memory_order_relaxed));
}

NtpTime SimulatedClock::ConvertTimestampToNtpTime(Timestamp timestamp)
{
    int64_t now_us = timestamp.us();
    uint32_t seconds = (now_us / 1000000) + kNtpJan1970;
    uint32_t fractions = static_cast<uint32_t>((now_us % 1000000) * kMagicNtpFractionalUnit / 1000000);
    return NtpTime(seconds, fractions);
}

void SimulatedClock::AdvanceTimeMilliseconds(int64_t milliseconds)
{
    AdvanceTime(TimeDelta::Millis(milliseconds));
}

void SimulatedClock::AdvanceTimeMicroseconds(int64_t microseconds)
{
    AdvanceTime(TimeDelta::Micros(microseconds));
}

// TODO(bugs.webrtc.org(12102): It's desirable to let a single thread own
// advancement of the clock. We could then replace this read-modify-write
// operation with just a thread checker. But currently, that breaks a couple of
// tests, in particular, RepeatingTaskTest.ClockIntegration and
// CallStatsTest.LastProcessedRtt.
void SimulatedClock::AdvanceTime(TimeDelta delta)
{
    mTimeUs.fetch_add(delta.us(), std::memory_order_relaxed);
}
CXXKIT_END_NAMESPACE
