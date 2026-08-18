/***********************************************************************************************************************
**
** Library: cxxkit
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

#ifndef _CXXKIT_ELAPSED_TIMER_HPP
#define _CXXKIT_ELAPSED_TIMER_HPP

#include <cxxkit/base/global.hpp>

CXXKIT_BEGIN_NAMESPACE

/**
 * @addtogroup core
 * @{
 * @addtogroup ElapsedTimer
 * @brief The ElapsedTimer class provides a fast way to calculate elapsed times.
 * @{
 * @details
 * The ElapsedTimer class is usually used to quickly calculate how much time has elapsed between two events.
 * ElapsedTimer tries to use monotonic clocks if possible.
 * This means it's not possible to convert ElapsedTimer objects to a human-readable time.
 *
 * The typical use-case for the class is to determine how much time was spent in a slow operation.
 * The simplest example of such a case is for debugging purposes, as in the following example:
 */

class CXXKIT_CORE_API ElapsedTimer
{
public:
    /**
     * @enum ElapsedTimer::ClockType
     * This enum contains the different clock types that ElapsedTimer may use.
     * ElapsedTimer will always use the same clock type in a particular machine, so this value will not
     * change during the lifetime of a program.
     */
    enum class ClockType
    {
        kSystemTime,
        kTickCounter,
        kStdSteadyClock,
        kMonotonicClock,
        kMachAbsoluteTime,
        kPerformanceCounter
    };

    CXXKIT_STATIC_CONSTANT_NUMBER(kInvalidData, CXXKIT_INT64_C(0x8000000000000000))

    /**
     * Constructs an invalid ElapsedTimer. A timer becomes valid once it has been started.
     * @sa isValid(), start()
     */
    CXXKIT_CONSTEXPR ElapsedTimer() { }
    virtual ~ElapsedTimer();

    /**
     * @return Returns the clock type that this ElapsedTimer implementation uses.
     * @sa isMonotonic()
     */
    static ClockType clockType() noexcept;

    /**
    * @return Returns \c true if this is a monotonic clock, false otherwise.
    * See the information on the different clock types to understand which ones are monotonic.
     */
    static bool isMonotonic() noexcept;

    /**
     * @brief Invalidates this ElapsedTimer object.
     *
     * An invalid object can be checked with isValid(). Calculations of timer elapsed since invalid data
     * are undefined and will likely produce bizarre results.
     *
     * @sa isValid(), start(), restart()
     */
    void invalidate() noexcept { mStart = mStop = kInvalidData; }

    /**
     * @return Restarts the timer and returns the time elapsed since the previous start.
     * This function is equivalent to obtaining the elapsed time with elapsed() and then starting the timer
     * again with start(), but it does so in one single operation, avoiding the need to obtain the clock value twice.
     *
     * Calling this function on a ElapsedTimer that is invalid results in undefined behavior.
     *
     * The following example illustrates how to use this function to calibrate a parameter to a slow operation
     * (for example, an iteration count) so that this operation takes at least 250 milliseconds:
     *
     * @sa start(), invalidate(), elapsed(), isValid()
     */
    int64_t restart() noexcept;

    /**
     * Starts this timer. Once started, a timer value can be checked with elapsed() or msecsSinceReference().
     * Normally, a timer is started just before a lengthy operation.
     *
     * Also, starting a timer makes it valid again.
     *
     * @sa restart(), invalidate(), elapsed()
     */
    void start() noexcept;

    /**
     * @return Returns the number of nanoseconds since this ElapsedTimer was last started.
     * Calling this function on a ElapsedTimer that is invalid results in undefined behavior.
     *
     * On platforms that do not provide nanosecond resolution, the value returned will be the best estimate available.
     *
     * @sa start(), restart(), hasExpired(), invalidate()
     */
    int64_t nsecsElapsed() const noexcept;

    /**
     * @return Returns the number of milliseconds since this ElapsedTimer was last started.
     *
     * Calling this function on a ElapsedTimer that is invalid results in undefined behavior.
     * 
     * @sa start(), restart(), hasExpired(), isValid(), invalidate()
     */
    int64_t elapsed() const noexcept;

    /**
     * @return Returns the number of milliseconds between this ElapsedTimer and @a other.
     * If @a other was started before this object, the returned value will be negative.
     * If it was started later, the returned value will be positive.
     *
     * The return value is undefined if this object or @a other were invalidated.
     *
     * @sa secsTo(), elapsed()
     */
    int64_t msecsTo(const ElapsedTimer &other) const noexcept;

    /**
     * @return Returns the number of seconds between this ElapsedTimer and @a other.
     * If @a other was started before this object, the returned value will be negative.
     * If it was started later, the returned value will be positive.
     *
     * Calling this function on or with a ElapsedTimer that is invalid results in undefined behavior.
     *
     * @sa msecsTo(), elapsed()
     */
    int64_t secsTo(const ElapsedTimer &other) const noexcept;

    /**
     * @return Returns the number of milliseconds between last time this ElapsedTimer object was started
     * and its reference clock's start.
     * 
     * This number is usually arbitrary for all clocks except the ElapsedTimer::SystemTime clock.
     * For that clock type, this number is the number of milliseconds since January 1st, 1970 at 0:00 UTC
     * (that is, it is the Unix time expressed in milliseconds).
     *
     * On Linux, Windows and Apple platforms, this value is usually the time since the system boot, though
     * it usually does not include the time the system has spent in sleep states.
     *
     * @sa clockType(), elapsed()
     */
    int64_t msecsSinceReference() const noexcept;

    /**
     * @return Returns false if the timer has never been started or invalidated by a call to invalidate().
     */
    bool isValid() const noexcept { return mStart != kInvalidData && mStop != kInvalidData; }
    /**
     * @brief Returns true if this ElapsedTimer has already expired by @a timeout milliseconds
     * (that is, more than @a timeout milliseconds have elapsed).
     *
     * The value of @a timeout can be -1 to indicate that this timer does not expire, in which
     * case this function will always return false.
     *
     * @sa elapsed(), DeadlineTimer
     */
    bool hasExpired(int64_t timeout) const noexcept { return uint64_t(this->elapsed()) > uint64_t(timeout); }

    /**
     * @brief Returns true if this object and @a other contain the same time.
     */
    bool operator==(const ElapsedTimer &other) const noexcept { return mStart == other.mStart && mStop == other.mStop; }

    /**
     * Returns @c true if this object and @a other contain different times.
     */
    bool operator!=(const ElapsedTimer &other) const noexcept { return !(*this == other); }

    /**
     * The returned value is undefined if one of the two parameters is invalid and the other isn't.
     * However, two invalid timers are equal and thus this function will return false.
     *
     * @param v1 The first ElapsedTimer to compare.
     * @param v2 The second ElapsedTimer to compare.
     * @return Returns \c true if \a v1 was started before \a v2, false otherwise.
     */
    friend bool CXXKIT_CORE_API operator<(const ElapsedTimer &v1, const ElapsedTimer &v2) noexcept;

private:
    int64_t mStart{kInvalidData};
    int64_t mStop{kInvalidData};
};

/**
 * @}
 * @}
 */

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_ELAPSED_TIMER_HPP
