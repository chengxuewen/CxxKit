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
    EXPECT_EQ(clock.time_nanos(), 0);

    clock.set_time(Timestamp::micros(1));
    EXPECT_EQ(clock.time_nanos(), 1000);

    clock.advance_time(TimeDelta::micros(500));
    EXPECT_EQ(clock.time_nanos(), 501000); // 1us + 500us in ns
}

TEST(FakeClock, ScopedClockInterceptsDateTime)
{
    const int64_t realBefore = DateTime::time_nanos();
    {
        ScopedFakeClock fake; // becomes the global clock for this scope
        fake.set_time(Timestamp::micros(123456));
        EXPECT_EQ(DateTime::time_nanos(), 123456000);
        fake.advance_time(TimeDelta::millis(1));
        EXPECT_EQ(DateTime::time_nanos(), 124456000); // +1ms
    }
    // After scope exit, real clock is restored.
    EXPECT_NE(DateTime::time_nanos(), 124456000);
    EXPECT_GT(DateTime::time_nanos(), 0);
    (void)realBefore;
}