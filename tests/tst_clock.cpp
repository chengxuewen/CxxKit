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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

CXXKIT_BEGIN_NAMESPACE

TEST(ClockConstants, NtpEpoch)
{
    EXPECT_EQ(kNtpJan1970, 2208988800UL);
    EXPECT_GT(kMagicNtpFractionalUnit, 0.0);
}

TEST(SimulatedClock, CurrentTimeAndAdvance)
{
    SimulatedClock clock(1000); // 1000 us
    EXPECT_EQ(clock.CurrentTime().us(), 1000);
    EXPECT_EQ(clock.TimeInMilliseconds(), 1);
    EXPECT_EQ(clock.TimeInMicroseconds(), 1000);

    clock.AdvanceTimeMicroseconds(500);
    EXPECT_EQ(clock.CurrentTime().us(), 1500);

    clock.AdvanceTimeMilliseconds(2);
    EXPECT_EQ(clock.CurrentTime().us(), 3500);

    clock.AdvanceTime(TimeDelta::Millis(1));
    EXPECT_EQ(clock.CurrentTime().us(), 4500);
}

TEST(SimulatedClock, FromTimestampCtor)
{
    SimulatedClock clock(Timestamp::Micros(42));
    EXPECT_EQ(clock.CurrentTime().us(), 42);
}

TEST(SimulatedClock, NtpConversion)
{
    SimulatedClock clock(0); // epoch Jan 1 1970 (us)
    NtpTime ntp = clock.ConvertTimestampToNtpTime(Timestamp::Micros(0));
    EXPECT_EQ(ntp.seconds(), kNtpJan1970);
    EXPECT_EQ(ntp.fractions(), 0u);

    // Advance exactly one second.
    NtpTime ntp1 = clock.ConvertTimestampToNtpTime(Timestamp::Millis(1000));
    EXPECT_EQ(ntp1.seconds(), kNtpJan1970 + 1u);
}

TEST(Clock, NtpToUtc)
{
    // Invalid NTP -> MinusInfinity Timestamp.
    Timestamp invalid = Clock::NtpToUtc(NtpTime(0, 0));
    EXPECT_EQ(invalid.us(), Timestamp::MinusInfinity().us());

    // Jan 1 1970 in NTP seconds -> epoch 0 microseconds.
    Timestamp utc = Clock::NtpToUtc(NtpTime(kNtpJan1970, 0));
    EXPECT_EQ(utc.us(), 0);
}

TEST(Clock, RealTimeClock)
{
    Clock *clock = Clock::GetRealTimeClock();
    ASSERT_NE(clock, nullptr);
    // Same instance on repeated calls (static singleton).
    EXPECT_EQ(Clock::GetRealTimeClock(), clock);

    // CurrentTime() is monotonic (steady clock), positive and non-decreasing.
    Timestamp t1 = clock->CurrentTime();
    Timestamp t2 = clock->CurrentTime();
    EXPECT_GT(t1.us(), 0);
    EXPECT_GE(t2.us(), t1.us());

    // NTP conversion of a real timestamp stays valid.
    NtpTime ntp = clock->CurrentNtpTime();
    EXPECT_TRUE(ntp.Valid());
}

TEST(Clock, ConvertToNtpMillis)
{
    SimulatedClock clock(0);
    // 1000 ms past the 1970 epoch maps to NTP second kNtpJan1970 + 1.
    NtpTime ntp = clock.ConvertTimestampToNtpTime(Timestamp::Millis(1000));
    EXPECT_EQ(ntp.ToMs(), (kNtpJan1970 + 1) * 1000LL);
    EXPECT_EQ(clock.ConvertTimestampToNtpTimeInMilliseconds(1000), (kNtpJan1970 + 1) * 1000LL);
}

CXXKIT_END_NAMESPACE
