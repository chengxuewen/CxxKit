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
    EXPECT_EQ(clock.current_time().us(), 1000);
    EXPECT_EQ(clock.time_in_milliseconds(), 1);
    EXPECT_EQ(clock.time_in_microseconds(), 1000);

    clock.advance_time_microseconds(500);
    EXPECT_EQ(clock.current_time().us(), 1500);

    clock.advance_time_milliseconds(2);
    EXPECT_EQ(clock.current_time().us(), 3500);

    clock.advance_time(TimeDelta::millis(1));
    EXPECT_EQ(clock.current_time().us(), 4500);
}

TEST(SimulatedClock, FromTimestampCtor)
{
    SimulatedClock clock(Timestamp::micros(42));
    EXPECT_EQ(clock.current_time().us(), 42);
}

TEST(SimulatedClock, NtpConversion)
{
    SimulatedClock clock(0); // epoch Jan 1 1970 (us)
    NtpTime ntp = clock.convert_timestamp_to_ntp_time(Timestamp::micros(0));
    EXPECT_EQ(ntp.seconds(), kNtpJan1970);
    EXPECT_EQ(ntp.fractions(), 0u);

    // Advance exactly one second.
    NtpTime ntp1 = clock.convert_timestamp_to_ntp_time(Timestamp::millis(1000));
    EXPECT_EQ(ntp1.seconds(), kNtpJan1970 + 1u);
}

TEST(Clock, ntp_to_utc)
{
    // Invalid NTP -> minus_infinity Timestamp.
    Timestamp invalid = Clock::ntp_to_utc(NtpTime(0, 0));
    EXPECT_EQ(invalid.us(), Timestamp::minus_infinity().us());

    // Jan 1 1970 in NTP seconds -> epoch 0 microseconds.
    Timestamp utc = Clock::ntp_to_utc(NtpTime(kNtpJan1970, 0));
    EXPECT_EQ(utc.us(), 0);
}

TEST(Clock, RealTimeClock)
{
    Clock *clock = Clock::get_real_time_clock();
    ASSERT_NE(clock, nullptr);
    // Same instance on repeated calls (static singleton).
    EXPECT_EQ(Clock::get_real_time_clock(), clock);

    // current_time() is monotonic (steady clock), positive and non-decreasing.
    Timestamp t1 = clock->current_time();
    Timestamp t2 = clock->current_time();
    EXPECT_GT(t1.us(), 0);
    EXPECT_GE(t2.us(), t1.us());

    // NTP conversion of a real timestamp stays valid.
    NtpTime ntp = clock->current_ntp_time();
    EXPECT_TRUE(ntp.valid());
}

TEST(Clock, ConvertToNtpMillis)
{
    SimulatedClock clock(0);
    // 1000 ms past the 1970 epoch maps to NTP second kNtpJan1970 + 1.
    NtpTime ntp = clock.convert_timestamp_to_ntp_time(Timestamp::millis(1000));
    EXPECT_EQ(ntp.to_ms(), (kNtpJan1970 + 1) * 1000LL);
    EXPECT_EQ(clock.convert_timestamp_to_ntp_time_in_milliseconds(1000), (kNtpJan1970 + 1) * 1000LL);
}

CXXKIT_END_NAMESPACE
