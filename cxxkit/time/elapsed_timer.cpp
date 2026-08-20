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

#include <cxxkit/time/elapsed_timer.hpp>
#include <cxxkit/base/core_config.hpp>

#include <chrono>

CXXKIT_BEGIN_NAMESPACE

ElapsedTimer::~ElapsedTimer()
{
}

#if CXXKIT_FEATURE_USE_STD_STEADY_CLOCK

ElapsedTimer::ClockType ElapsedTimer::clockType() noexcept
{
    return ClockType::kStdSteadyClock;
}

bool ElapsedTimer::isMonotonic() noexcept
{
    return true;
}

int64_t ElapsedTimer::restart() noexcept
{
    const auto old = mStart;
    const auto now = std::chrono::steady_clock::now();
    mStart = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    mStop = 0;
    // mStart is int64_t; if it was never started it holds kInvalidData (INT64_MIN).
    // Subtracting the sentinel from the current ns counter overflows int64_t.
    if (old == kInvalidData)
    {
        return 0;
    }
    return (mStart - old) / 1000000;
}

void ElapsedTimer::start() noexcept
{
    this->restart();
}

int64_t ElapsedTimer::nsecsElapsed() const noexcept
{
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() - mStart;
}

int64_t ElapsedTimer::elapsed() const noexcept
{
    return this->nsecsElapsed() / 1000000;
}

int64_t ElapsedTimer::msecsTo(const ElapsedTimer &other) const noexcept
{
    return (other.mStart - mStart) / 1000000;
}

int64_t ElapsedTimer::secsTo(const ElapsedTimer &other) const noexcept
{
    return this->msecsTo(other) / 1000;
}

int64_t ElapsedTimer::msecsSinceReference() const noexcept
{
    return mStart / 1000000;
}

#else
#    error "unknown elapsed timer backend"
#endif

bool operator<(const ElapsedTimer &v1, const ElapsedTimer &v2) noexcept
{
    return v1.mStart < v2.mStart;
}

CXXKIT_END_NAMESPACE