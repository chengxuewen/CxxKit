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

#include <cxxkit/numerics/safe_conversions.hpp>

#include <cstdint>
#include <limits>
#include <cmath>

CXXKIT_BEGIN_NAMESPACE

class NtpTime
{
public:
    static constexpr uint64_t kFractionsPerSecond = 0x100000000;

    NtpTime()
        : mValue(0)
    {
    }
    explicit NtpTime(uint64_t value)
        : mValue(value)
    {
    }
    NtpTime(uint32_t seconds, uint32_t fractions)
        : mValue(seconds * kFractionsPerSecond + fractions)
    {
    }

    NtpTime(const NtpTime &) = default;
    NtpTime &operator=(const NtpTime &) = default;
    explicit operator uint64_t() const { return mValue; }

    void set(uint32_t seconds, uint32_t fractions) { mValue = seconds * kFractionsPerSecond + fractions; }
    void reset() { mValue = 0; }

    int64_t to_ms() const
    {
        static constexpr double kNtpFracPerMs = 4.294967296E6; // 2^32 / 1000.
        const double frac_ms = static_cast<double>(fractions()) / kNtpFracPerMs;
        return 1000 * static_cast<int64_t>(seconds()) + static_cast<int64_t>(frac_ms + 0.5);
    }
    // NTP standard (RFC1305, section 3.1) explicitly state value 0 is invalid.
    bool valid() const { return mValue != 0; }

    uint32_t seconds() const { return utils::dchecked_cast<uint32_t>(mValue / kFractionsPerSecond); }
    uint32_t fractions() const { return utils::dchecked_cast<uint32_t>(mValue % kFractionsPerSecond); }

private:
    uint64_t mValue;
};

inline bool operator==(const NtpTime &n1, const NtpTime &n2)
{
    return static_cast<uint64_t>(n1) == static_cast<uint64_t>(n2);
}
inline bool operator!=(const NtpTime &n1, const NtpTime &n2)
{
    return !(n1 == n2);
}

namespace utils
{
// Converts `int64_t` milliseconds to Q32.32-formatted fixed-point seconds.
// Performs clamping if the result overflows or underflows.
inline int64_t Int64MsToQ32x32(int64_t milliseconds)
{
    // TODO(bugs.webrtc.org/10893): Change to use `rtc::saturated_cast` once the
    // bug has been fixed.
    double result = std::round(milliseconds * (NtpTime::kFractionsPerSecond / 1000.0));

    // Explicitly cast values to double to avoid implicit conversion warnings
    // The conversion of the std::numeric_limits<int64_t>::max() triggers
    // -Wimplicit-int-float-conversion warning in clang 10.0.0 without explicit
    // cast
    if (result <= static_cast<double>(std::numeric_limits<int64_t>::min()))
    {
        return std::numeric_limits<int64_t>::min();
    }

    if (result >= static_cast<double>(std::numeric_limits<int64_t>::max()))
    {
        return std::numeric_limits<int64_t>::max();
    }

    return utils::dchecked_cast<int64_t>(result);
}

// Converts `int64_t` milliseconds to UQ32.32-formatted fixed-point seconds.
// Performs clamping if the result overflows or underflows.
inline uint64_t Int64MsToUQ32x32(int64_t milliseconds)
{
    // TODO(bugs.webrtc.org/10893): Change to use `rtc::saturated_cast` once the
    // bug has been fixed.
    double result = std::round(milliseconds * (NtpTime::kFractionsPerSecond / 1000.0));

    // Explicitly cast values to double to avoid implicit conversion warnings
    // The conversion of the std::numeric_limits<int64_t>::max() triggers
    // -Wimplicit-int-float-conversion warning in clang 10.0.0 without explicit
    // cast
    if (result <= static_cast<double>(std::numeric_limits<uint64_t>::min()))
    {
        return std::numeric_limits<uint64_t>::min();
    }

    if (result >= static_cast<double>(std::numeric_limits<uint64_t>::max()))
    {
        return std::numeric_limits<uint64_t>::max();
    }

    return utils::dchecked_cast<uint64_t>(result);
}

// Converts Q32.32-formatted fixed-point seconds to `int64_t` milliseconds.
inline int64_t Q32x32ToInt64Ms(int64_t q32x32)
{
    return utils::dchecked_cast<int64_t>(std::round(q32x32 * (1000.0 / NtpTime::kFractionsPerSecond)));
}

// Converts UQ32.32-formatted fixed-point seconds to `int64_t` milliseconds.
inline int64_t UQ32x32ToInt64Ms(uint64_t q32x32)
{
    return utils::dchecked_cast<int64_t>(std::round(q32x32 * (1000.0 / NtpTime::kFractionsPerSecond)));
}

// Converts UQ32.32-formatted fixed-point seconds to `int64_t` microseconds.
inline int64_t UQ32x32ToInt64Us(uint64_t q32x32)
{
    return utils::dchecked_cast<int64_t>(std::round(q32x32 * (1000000.0 / NtpTime::kFractionsPerSecond)));
}

// Converts Q32.32-formatted fixed-point seconds to `int64_t` microseconds.
inline int64_t Q32x32ToInt64Us(int64_t q32x32)
{
    return utils::dchecked_cast<int64_t>(std::round(q32x32 * (1000000.0 / NtpTime::kFractionsPerSecond)));
}
} // namespace utils

CXXKIT_END_NAMESPACE

