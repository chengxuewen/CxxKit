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

#include <cxxkit/time/time_global.hpp>

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/checks.hpp>
#include <cxxkit/text/string_view.hpp>

CXXKIT_BEGIN_NAMESPACE

struct CXXKIT_TIME_API ClockInterface
{
    virtual ~ClockInterface() { }
    virtual int64_t time_nanos() const = 0;

    static ClockInterface *set_clock_for_testing(ClockInterface *clock);
    // Returns previously set clock, or nullptr if no custom clock is being used.
    static ClockInterface *get_clock_for_testing();
};

class CXXKIT_TIME_API DateTime
{
public:
    struct LocalTime
    {
        int mil;  /* milliseconds after the minute [0-1000] */
        int sec;  /* seconds after the minute [0-60] */
        int min;  /* minutes after the hour [0-59] */
        int hour; /* hours since midnight [0-23] */
        int day;  /* day of the month [1-31] */
        int mon;  /* months since January [0-11] */
        int year; /* years since 1900 */

        int days_since_sunday;  /* days since Sunday [0-6] */
        int days_since_january; /* days since January 1 [0-365] */
        int isdst;              /* Daylight Savings Time flag */
    };

    CXXKIT_STATIC_CONSTANT_NUMBER(kNSecsPerUSec, int64_t(1000))

    CXXKIT_STATIC_CONSTANT_NUMBER(kUSecsPerMSec, int64_t(1000))
    CXXKIT_STATIC_CONSTANT_NUMBER(kNSecsPerMSec, int64_t(kNSecsPerUSec *kUSecsPerMSec))

    CXXKIT_STATIC_CONSTANT_NUMBER(kMSecsPerSec, int64_t(1000))
    CXXKIT_STATIC_CONSTANT_NUMBER(kUSecsPerSec, int64_t(kUSecsPerMSec *kMSecsPerSec))
    CXXKIT_STATIC_CONSTANT_NUMBER(kNSecsPerSec, int64_t(kNSecsPerMSec *kMSecsPerSec))

    CXXKIT_STATIC_CONSTANT_NUMBER(kSecsPerMin, int64_t(60))
    CXXKIT_STATIC_CONSTANT_NUMBER(kMSecsPerMin, int64_t(kMSecsPerSec *kSecsPerMin))
    CXXKIT_STATIC_CONSTANT_NUMBER(kUSecsPerMin, int64_t(kUSecsPerSec *kSecsPerMin))
    CXXKIT_STATIC_CONSTANT_NUMBER(kNSecsPerMin, int64_t(kNSecsPerSec *kSecsPerMin))

    CXXKIT_STATIC_CONSTANT_NUMBER(kMinsPerHour, int64_t(60))
    CXXKIT_STATIC_CONSTANT_NUMBER(kSecsPerHour, int64_t(kSecsPerMin *kMinsPerHour))
    CXXKIT_STATIC_CONSTANT_NUMBER(kMSecsPerHour, int64_t(kMSecsPerMin *kMinsPerHour))
    CXXKIT_STATIC_CONSTANT_NUMBER(kUSecsPerHour, int64_t(kUSecsPerMin *kMinsPerHour))
    CXXKIT_STATIC_CONSTANT_NUMBER(kNSecsPerHour, int64_t(kNSecsPerMin *kMinsPerHour))

    CXXKIT_STATIC_CONSTANT_NUMBER(kHoursPerDay, int64_t(24))
    CXXKIT_STATIC_CONSTANT_NUMBER(kMinsPerDay, int64_t(kMinsPerHour *kHoursPerDay))
    CXXKIT_STATIC_CONSTANT_NUMBER(kSecsPerDay, int64_t(kSecsPerHour *kHoursPerDay))
    CXXKIT_STATIC_CONSTANT_NUMBER(kMSecsPerDay, int64_t(kMSecsPerHour *kHoursPerDay))
    CXXKIT_STATIC_CONSTANT_NUMBER(kUSecsPerDay, int64_t(kUSecsPerHour *kHoursPerDay))
    CXXKIT_STATIC_CONSTANT_NUMBER(kNSecsPerDay, int64_t(kNSecsPerHour *kHoursPerDay))

    /* system_clock CLOCK_REALTIME for log/datetime */
    static int64_t system_time_secs();
    static int64_t system_time_m_secs();
    static int64_t system_time_u_secs();
    static int64_t system_time_n_secs();
    static int64_t system_time_from_steady_n_secs(int64_t nsecs);

    /* steady_clock CLOCK_MONOTONIC for wait/hrtime */
    static int64_t steady_time_secs();
    static int64_t steady_time_m_secs();
    static int64_t steady_time_u_secs();
    static int64_t steady_time_n_secs();
    static int64_t steady_time_from_system_n_secs(int64_t nsecs);

    static LocalTime local_time_from_system_time_secs(int64_t secs = -1);
    static LocalTime local_time_from_system_time_m_secs(int64_t msecs = -1);
    static std::string local_time_string_from_system_time_secs(int64_t secs = -1);
    static std::string local_time_string_from_system_time_m_secs(int64_t msecs = -1);

    static LocalTime local_time_from_steady_time_secs(int64_t secs = -1)
    {
        secs = secs > 0 ? secs : steady_time_secs();
        return local_time_from_system_time_secs(system_time_from_steady_n_secs(secs * kNSecsPerSec) / kNSecsPerSec);
    }
    static LocalTime local_time_from_steady_time_m_secs(int64_t msecs = -1)
    {
        msecs = msecs > 0 ? msecs : steady_time_m_secs();
        return local_time_from_system_time_m_secs(system_time_from_steady_n_secs(msecs * kNSecsPerMSec) / kNSecsPerMSec);
    }
    static CXXKIT_FORCE_INLINE std::string local_time_string_from_steady_time_secs(int64_t secs = -1)
    {
        secs = secs > 0 ? secs : steady_time_secs();
        return local_time_string_from_system_time_secs(system_time_from_steady_n_secs(secs * kNSecsPerSec) / kNSecsPerSec);
    }
    static CXXKIT_FORCE_INLINE std::string local_time_string_from_steady_time_m_secs(int64_t msecs = -1)
    {
        msecs = msecs > 0 ? msecs : steady_time_m_secs();
        return local_time_string_from_system_time_m_secs(system_time_from_steady_n_secs(msecs * kNSecsPerMSec) / kNSecsPerMSec);
    }
    static CXXKIT_FORCE_INLINE std::string local_time_string() { return local_time_string_from_steady_time_m_secs(); }

    static CXXKIT_FORCE_INLINE int64_t time_utc_nanos()
    {
        auto clock = ClockInterface::get_clock_for_testing();
        return clock ? clock->time_nanos() : system_time_n_secs();
    }
    static CXXKIT_FORCE_INLINE int64_t time_utc_micros() { return time_utc_nanos() / kNSecsPerUSec; }
    static CXXKIT_FORCE_INLINE int64_t time_utc_millis() { return time_utc_nanos() / kNSecsPerMSec; }

    static CXXKIT_FORCE_INLINE int64_t time_nanos()
    {
        auto clock = ClockInterface::get_clock_for_testing();
        return clock ? clock->time_nanos() : steady_time_n_secs();
    }
    static CXXKIT_FORCE_INLINE int64_t time_micros() { return time_nanos() / kNSecsPerUSec; }
    static CXXKIT_FORCE_INLINE int64_t time_millis() { return time_nanos() / kNSecsPerMSec; }
    static CXXKIT_FORCE_INLINE int64_t time_after(int64_t elapsed)
    {
        CXXKIT_DCHECK_GE(elapsed, 0);
        return time_millis() + elapsed;
    }
    static CXXKIT_FORCE_INLINE int64_t time_since(int64_t earlier) { return DateTime::time_millis() - earlier; }
    static CXXKIT_FORCE_INLINE int32_t time_diff32(uint32_t later, uint32_t earlier) { return later - earlier; }
    static CXXKIT_FORCE_INLINE int64_t time_diff(int64_t later, int64_t earlier) { return later - earlier; }
    static CXXKIT_FORCE_INLINE int64_t time_until(int64_t later) { return later - time_millis(); }
    static CXXKIT_FORCE_INLINE uint32_t Time32() { return static_cast<uint32_t>(time_nanos() / kNSecsPerMSec); }

    static CXXKIT_FORCE_INLINE int64_t time_after_m_secs(int64_t elapsed)
    {
        CXXKIT_DCHECK_GE(elapsed, 0);
        return DateTime::steady_time_m_secs() + elapsed;
    }
    static CXXKIT_FORCE_INLINE int64_t time_since_m_secs(int64_t earlier)
    {
        CXXKIT_DCHECK_GE(earlier, 0);
        return DateTime::steady_time_m_secs() - earlier;
    }
    static CXXKIT_FORCE_INLINE int64_t time_until_m_secs(int64_t later)
    {
        CXXKIT_DCHECK_GE(later, 0);
        return later - DateTime::steady_time_m_secs();
    }

    static int64_t tm_to_seconds(const tm &tm);
};
CXXKIT_END_NAMESPACE

