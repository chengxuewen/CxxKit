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

#include <cxxkit/time/date_time.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ctime>
#include <string>

CXXKIT_BEGIN_NAMESPACE

namespace
{
// Deterministic fake clock for the ClockInterface-based APIs.
class FakeClock final : public ClockInterface
{
public:
    explicit FakeClock(int64_t nanos) : mNanos(nanos) {}
    int64_t TimeNanos() const override { return mNanos; }

private:
    int64_t mNanos;
};
} // namespace

TEST(DateTime, TimeUnitConstants)
{
    EXPECT_EQ(DateTime::kNSecsPerUSec, 1000);
    EXPECT_EQ(DateTime::kUSecsPerMSec, 1000);
    EXPECT_EQ(DateTime::kNSecsPerMSec, 1000000);
    EXPECT_EQ(DateTime::kMSecsPerSec, 1000);
    EXPECT_EQ(DateTime::kNSecsPerSec, 1000000000);
    EXPECT_EQ(DateTime::kSecsPerMin, 60);
    EXPECT_EQ(DateTime::kSecsPerHour, 3600);
    EXPECT_EQ(DateTime::kSecsPerDay, 86400);
}

TEST(DateTime, SystemTimeMonotonic)
{
    // Values are indistinguishable from each other and increasing; we just
    // validate the ordering relationships between the unit derivatives.
    const int64_t secs = DateTime::systemTimeSecs();
    const int64_t ms = DateTime::systemTimeMSecs();
    const int64_t us = DateTime::systemTimeUSecs();
    const int64_t ns = DateTime::systemTimeNSecs();

    EXPECT_GT(ms, secs * 1000);
    EXPECT_GT(us, ms * 1000);
    EXPECT_GT(ns, us * 1000);
    // Epoch sanity (year >= 2020 in seconds from 1970).
    EXPECT_GT(secs, 1577836800); // 2020-01-01
}

TEST(DateTime, SteadyTimeMonotonic)
{
    const int64_t secs = DateTime::steadyTimeSecs();
    const int64_t ms = DateTime::steadyTimeMSecs();
    const int64_t us = DateTime::steadyTimeUSecs();
    const int64_t ns = DateTime::steadyTimeNSecs();

    EXPECT_GT(ms, secs * 1000);
    EXPECT_GT(us, ms * 1000);
    EXPECT_GT(ns, us * 1000);

    // Steady time advances (a later call is >= an earlier one).
    const int64_t before = DateTime::steadyTimeNSecs();
    EXPECT_GE(DateTime::steadyTimeNSecs(), before);
}

TEST(DateTime, SystemSteadyConversions)
{
    // systemTimeFromSteadyNSecs maps the current steady clock onto the system
    // (Unix-epoch) timeline, so it should be within a couple seconds of the real
    // system time.
    const int64_t nowSys = DateTime::systemTimeNSecs();
    const int64_t steadyNow = DateTime::steadyTimeNSecs();
    // PIT-23 regression: steady<->system mapping must be symmetric now
    // (was ~1e18 ns drift from epoch-basis mismatch). Small window covers the
    // two clock-sample instants.
    EXPECT_NEAR(DateTime::systemTimeFromSteadyNSecs(steadyNow), nowSys, (int64_t)2e9);
    EXPECT_NEAR(DateTime::steadyTimeFromSystemNSecs(nowSys), steadyNow, (int64_t)2e9);
}

TEST(DateTime, LocalTimeFromSystem)
{
    // LocalTime fields must fall in their documented ranges for a recent
    // instant. (Time zone and the ms-vs-sec interpretation both vary, so we
    // only assert the structural ranges are respected.)
    DateTime::LocalTime lt = DateTime::localTimeFromSystemTimeMSecs(DateTime::systemTimeMSecs());
    EXPECT_GE(lt.sec, 0);
    EXPECT_LE(lt.sec, 60);
    EXPECT_GE(lt.min, 0);
    EXPECT_LE(lt.min, 59);
    EXPECT_GE(lt.hour, 0);
    EXPECT_LE(lt.hour, 23);
    EXPECT_GE(lt.day, 1);
    EXPECT_LE(lt.day, 31);
    EXPECT_GE(lt.mon, 0);
    EXPECT_LE(lt.mon, 11);
    EXPECT_GE(lt.year, 2020);
    EXPECT_GE(lt.mil, 0);
    EXPECT_LT(lt.mil, 1000);

    // Milliseconds variant retains the fractional millisecond.
    DateTime::LocalTime ltm = DateTime::localTimeFromSystemTimeMSecs(1577836800123LL);
    EXPECT_GE(ltm.mil, 0);
    EXPECT_LT(ltm.mil, 1000);
}

TEST(DateTime, LocalTimeString)
{
    const int64_t secs = 1577836800;
    std::string s = DateTime::localTimeStringFromSystemTimeSecs(secs);
    // Format "%Y-%m-%d %H:%M:%S" of a known UTC instant; timezone-dependent,
    // so we only check the structural shape.
    EXPECT_EQ(s.size(), 19u);
    EXPECT_EQ(s[4], '-');
    EXPECT_EQ(s[7], '-');
    EXPECT_EQ(s[13], ':');

    // MSecs variant appends ".mmm".
    std::string sm = DateTime::localTimeStringFromSystemTimeMSecs(1577836800123LL);
    EXPECT_EQ(sm.size(), 23u);
    EXPECT_EQ(sm[19], '.');
}

TEST(DateTime, TmToSeconds)
{
    std::tm tm = {};
    tm.tm_year = 70;  // 1970
    tm.tm_mon = 0;    // January
    tm.tm_mday = 1;
    EXPECT_EQ(DateTime::TmToSeconds(tm), 0);

    // One day later.
    tm.tm_mday = 2;
    EXPECT_EQ(DateTime::TmToSeconds(tm), 86400);

    // 2000-01-01 (epoch offset already known).
    std::tm y2k = {};
    y2k.tm_year = 100; // 2000
    y2k.tm_mon = 0;
    y2k.tm_mday = 1;
    EXPECT_EQ(DateTime::TmToSeconds(y2k), 946684800);

    // Leap day in a leap year 2000-02-29 -> valid.
    std::tm leap = {};
    leap.tm_year = 100; // 2000
    leap.tm_mon = 1;    // Feb
    leap.tm_mday = 29;
    EXPECT_GT(DateTime::TmToSeconds(leap), 946684800);

    // Invalid month.
    std::tm badMonth = {};
    badMonth.tm_year = 100;
    badMonth.tm_mon = 12;
    badMonth.tm_mday = 1;
    EXPECT_EQ(DateTime::TmToSeconds(badMonth), -1);

    // Invalid day (Feb 30 in non-leap 2001).
    std::tm badDay = {};
    badDay.tm_year = 101; // 2001
    badDay.tm_mon = 1;    // Feb
    badDay.tm_mday = 30;
    EXPECT_EQ(DateTime::TmToSeconds(badDay), -1);

    // Invalid hour.
    std::tm badHour = {};
    badHour.tm_year = 70;
    badHour.tm_mon = 0;
    badHour.tm_mday = 1;
    badHour.tm_hour = 24;
    EXPECT_EQ(DateTime::TmToSeconds(badHour), -1);

    // Year before 1970.
    std::tm preEpoch = {};
    preEpoch.tm_year = 69;
    preEpoch.tm_mon = 0;
    preEpoch.tm_mday = 1;
    EXPECT_EQ(DateTime::TmToSeconds(preEpoch), -1);
}

TEST(DateTime, ClockInterfaceOverride)
{
    // Set a fake clock, verify TimeNanos path uses it, then restore.
    FakeClock fake(123456789000LL); // some fixed nanos
    ClockInterface *prev = ClockInterface::SetClockForTesting(&fake);
    EXPECT_EQ(ClockInterface::GetClockForTesting(), &fake);

    // TimeNanos reflects the fake clock.
    EXPECT_EQ(DateTime::TimeNanos(), fake.TimeNanos());
    EXPECT_EQ(DateTime::TimeUTCNanos(), fake.TimeNanos());

    // Derived functions are consistent.
    EXPECT_EQ(DateTime::TimeMicros(), fake.TimeNanos() / 1000);
    EXPECT_EQ(DateTime::TimeMillis(), fake.TimeNanos() / 1000000);

    // TimeAfter/TimeSince/TimeUntil arithmetic.
    const int64_t now = DateTime::TimeMillis();
    EXPECT_EQ(DateTime::TimeAfter(50), now + 50);
    EXPECT_EQ(DateTime::TimeSince(now), 0);
    EXPECT_EQ(DateTime::TimeUntil(now + 60), 60);

    // Restore the previous clock so we don't leak state to other tests.
    ClockInterface *restored = ClockInterface::SetClockForTesting(prev);
    EXPECT_EQ(restored, &fake);
    EXPECT_EQ(ClockInterface::GetClockForTesting(), prev);
}

CXXKIT_END_NAMESPACE
