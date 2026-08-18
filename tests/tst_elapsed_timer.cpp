/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright 2016 The WebRTC Project Authors.
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

#include "cxxkit/time/elapsed_timer.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>
#include <limits>
#include <thread>

CXXKIT_BEGIN_NAMESPACE

namespace
{
static const int minResolution = 100; // the minimum resolution for the tests

static std::string toString(const ElapsedTimer &t)
{
    std::stringstream ss;
    ss << "(" << t.msecsSinceReference() << ")";
    return ss.str();
}

} // namespace

TEST(ElapsedTimerTest, Statics)
{
    std::cout << "Clock type is " << (int)ElapsedTimer::clockType() << std::endl;
    std::cout << "Said clock is" << (ElapsedTimer::isMonotonic() ? "monotonic" : "not monotonic") << std::endl;
    ElapsedTimer t;
    t.start();
    std::cout << "Current time is " << t.msecsSinceReference() << std::endl;
}

TEST(ElapsedTimerTest, Validity)
{
    ElapsedTimer t;

    EXPECT_TRUE(!t.isValid()); // non-POD now, it should always start invalid

    t.start();
    EXPECT_TRUE(t.isValid());

    t.invalidate();
    EXPECT_TRUE(!t.isValid());
}

TEST(ElapsedTimerTest, Basics)
{
    ElapsedTimer t1;
    t1.start();

    EXPECT_TRUE(t1.msecsSinceReference() != 0);

    EXPECT_EQ(t1, t1);
    EXPECT_TRUE(!(t1 != t1));
    EXPECT_TRUE(!(t1 < t1));
    EXPECT_EQ(t1.msecsTo(t1), int64_t(0));
    EXPECT_EQ(t1.secsTo(t1), int64_t(0));

    uint64_t value1 = t1.msecsSinceReference();
    std::cout << "value1:" << value1 << " t1:" << toString(t1) << std::endl;
    int64_t nsecs = t1.nsecsElapsed();
    int64_t elapsed = t1.restart();
    std::cout << "nsecs:" << nsecs << " elapsed:" << elapsed << std::endl;
    EXPECT_TRUE(elapsed < minResolution);
    EXPECT_TRUE(nsecs / 1000000 < minResolution);

    uint64_t value2 = t1.msecsSinceReference();
    std::cout << "value2:" << value2 << " t1:" << toString(t1) << " elapsed:" << elapsed << " nsecs:" << nsecs
              << std::endl;
    // in theory, elapsed == value2 - value1

    // However, since ElapsedTimer keeps internally the full resolution,
    // we have here a rounding error due to integer division
    EXPECT_TRUE(std::abs(elapsed - int64_t(value2 - value1)) <= 1);
}

TEST(ElapsedTimerTest, Elapsed)
{
    ElapsedTimer t1;
    t1.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(2 * minResolution));

    auto nsecs = t1.nsecsElapsed();
    auto msecs = t1.elapsed();
    EXPECT_TRUE(nsecs > 0);
    EXPECT_TRUE(msecs > 0);
    // the number of elapsed nanoseconds and milliseconds should match
    EXPECT_TRUE(nsecs - msecs * 1000000 < 1000000);

    if (msecs > 8 * minResolution)
    {
        GTEST_SKIP() << "Sampling timer took too long, aborting test";
    }

    EXPECT_TRUE(t1.hasExpired(minResolution));
    EXPECT_TRUE(!t1.hasExpired(8 * minResolution));
    EXPECT_TRUE(!t1.hasExpired(-1));

    int64_t elapsed = t1.restart();
    EXPECT_TRUE(elapsed >= msecs);
    EXPECT_TRUE(elapsed < msecs + 3 * minResolution);
}

TEST(ElapsedTimerTest, MsecsTo)
{
    ElapsedTimer t1;
    t1.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(minResolution));
    ElapsedTimer t2;
    t2.start();

    EXPECT_TRUE(t1 != t2);
    EXPECT_TRUE(!(t1 == t2));
    EXPECT_TRUE(t1 < t2);

    auto diff = t1.msecsTo(t2);
    EXPECT_TRUE(diff > 0);
    diff = t2.msecsTo(t1);
    EXPECT_TRUE(diff < 0);
}

CXXKIT_END_NAMESPACE
