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

#include <ctime>
#include <chrono>
#include <sstream>
#include <iomanip>

CXXKIT_BEGIN_NAMESPACE

ClockInterface *g_clock = nullptr;
ClockInterface *ClockInterface::set_clock_for_testing(ClockInterface *clock)
{
    ClockInterface *prev = g_clock;
    g_clock = clock;
    return prev;
}

ClockInterface *ClockInterface::get_clock_for_testing()
{
    return g_clock;
}

int64_t DateTime::system_time_secs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

int64_t DateTime::system_time_m_secs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

int64_t DateTime::system_time_u_secs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

int64_t DateTime::system_time_n_secs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

int64_t DateTime::system_time_from_steady_n_secs(int64_t nsecs)
{
    std::chrono::nanoseconds nanoseconds(nsecs);
    std::chrono::steady_clock::time_point time_point(nanoseconds);

    auto steadyNow = std::chrono::steady_clock::now();
    auto offset = std::chrono::duration_cast<std::chrono::system_clock::duration>(time_point - steadyNow);
    auto systemTimePoint = std::chrono::system_clock::now() + offset;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(systemTimePoint.time_since_epoch()).count();
}

int64_t DateTime::steady_time_secs()
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    ;
}

int64_t DateTime::steady_time_m_secs()
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

int64_t DateTime::steady_time_u_secs()
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

int64_t DateTime::steady_time_n_secs()
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

int64_t DateTime::steady_time_from_system_n_secs(int64_t nsecs)
{
    auto systemNow = std::chrono::system_clock::now();
    auto steadyNow = std::chrono::steady_clock::now();
    // PIT-23 fix: nsecs is a SYSTEM-clock timestamp (Unix epoch). It must be
    // interpreted on the system_clock timeline, NOT passed to steady_clock's
    // time_point (different epoch basis — steady starts at boot, not 1970).
    auto systemTimePoint = std::chrono::system_clock::time_point(std::chrono::nanoseconds(nsecs));
    const auto offset = systemNow - systemTimePoint;
    auto steadyTimePoint = steadyNow - std::chrono::duration_cast<std::chrono::steady_clock::duration>(offset);
    return steadyTimePoint.time_since_epoch().count();
}

DateTime::LocalTime DateTime::local_time_from_system_time_secs(int64_t secs)
{
    secs = secs > 0 ? secs : system_time_secs();
    std::chrono::milliseconds milliseconds(secs);
    std::chrono::system_clock::time_point time_point(milliseconds);
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::tm *localTime = std::localtime(&time);
    const int mil = int(milliseconds.count() % 1000);
    return {mil,
            localTime->tm_sec,
            localTime->tm_min,
            localTime->tm_hour,
            localTime->tm_mday,
            localTime->tm_mon,
            localTime->tm_year + 1900,
            localTime->tm_wday,
            localTime->tm_yday,
            localTime->tm_isdst};
}

DateTime::LocalTime DateTime::local_time_from_system_time_m_secs(int64_t msecs)
{
    msecs = msecs > 0 ? msecs : system_time_m_secs();
    std::chrono::milliseconds milliseconds(msecs);
    std::chrono::system_clock::time_point time_point(milliseconds);
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::tm *localTime = std::localtime(&time);
    const int mil = int(milliseconds.count() % 1000);
    return {mil,
            localTime->tm_sec,
            localTime->tm_min,
            localTime->tm_hour,
            localTime->tm_mday,
            localTime->tm_mon,
            localTime->tm_year + 1900,
            localTime->tm_wday,
            localTime->tm_yday,
            localTime->tm_isdst};
}

std::string DateTime::local_time_string_from_system_time_secs(int64_t secs)
{
    secs = secs > 0 ? secs : DateTime::system_time_secs();
    std::chrono::seconds seconds(secs);
    std::chrono::system_clock::time_point time_point(seconds);
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::tm *localTime = std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string DateTime::local_time_string_from_system_time_m_secs(int64_t msecs)
{
    msecs = msecs > 0 ? msecs : system_time_m_secs();
    std::chrono::milliseconds milliseconds(msecs);
    std::chrono::system_clock::time_point time_point(milliseconds);
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);
    std::tm *localTime = std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
       << (milliseconds.count() % 1000);
    return ss.str();
}

int64_t DateTime::tm_to_seconds(const tm &tm)
{
    static short int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static short int cumul_mdays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon;
    int day = tm.tm_mday - 1; // Make 0-based like the rest.
    int hour = tm.tm_hour;
    int min = tm.tm_min;
    int sec = tm.tm_sec;

    bool expiry_in_leap_year = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

    if (year < 1970)
    {
        return -1;
    }
    if (month < 0 || month > 11)
    {
        return -1;
    }
    if (day < 0 || day >= mdays[month] + (expiry_in_leap_year && month == 2 - 1))
    {
        return -1;
    }
    if (hour < 0 || hour > 23)
    {
        return -1;
    }
    if (min < 0 || min > 59)
    {
        return -1;
    }
    if (sec < 0 || sec > 59)
    {
        return -1;
    }

    day += cumul_mdays[month];

    // add number of leap days between 1970 and the expiration year, inclusive.
    day += ((year / 4 - 1970 / 4) - (year / 100 - 1970 / 100) + (year / 400 - 1970 / 400));

    // We will have added one day too much above if expiration is during a leap
    // year, and expiration is in January or February.
    if (expiry_in_leap_year && month <= 2 - 1)
    { // `month` is zero based.
        day -= 1;
    }

    // Combine all variables into seconds from 1970-01-01 00:00 (except `month`
    // which was accumulated into `day` above).
    return (((static_cast<int64_t>(year - 1970) * 365 + day) * 24 + hour) * 60 + min) * 60 + sec;
}
CXXKIT_END_NAMESPACE