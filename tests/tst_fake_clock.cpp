// cxxkit::tools FakeClock + ScopedFakeClock tests — coverage for fake_clock.cpp.
#include <cxxkit/tools/fake_clock.hpp>
#include <cxxkit/time/date_time.hpp>
#include <cxxkit/units/time_delta.hpp>
#include <cxxkit/units/timestamp.hpp>

#include <gtest/gtest.h>

using namespace cxxkit;

TEST(FakeClock, SetGetAdvance)
{
    FakeClock clock;
    EXPECT_EQ(clock.TimeNanos(), 0);

    clock.SetTime(Timestamp::Micros(1));
    EXPECT_EQ(clock.TimeNanos(), 1000);

    clock.AdvanceTime(TimeDelta::Micros(500));
    EXPECT_EQ(clock.TimeNanos(), 501000); // 1us + 500us in ns
}

TEST(FakeClock, ScopedClockInterceptsDateTime)
{
    const int64_t realBefore = DateTime::TimeNanos();
    {
        ScopedFakeClock fake; // becomes the global clock for this scope
        fake.SetTime(Timestamp::Micros(123456));
        EXPECT_EQ(DateTime::TimeNanos(), 123456000);
        fake.AdvanceTime(TimeDelta::Millis(1));
        EXPECT_EQ(DateTime::TimeNanos(), 124456000); // +1ms
    }
    // After scope exit, real clock is restored.
    EXPECT_NE(DateTime::TimeNanos(), 124456000);
    EXPECT_GT(DateTime::TimeNanos(), 0);
    (void)realBefore;
}