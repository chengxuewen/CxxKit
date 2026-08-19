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

#include <cxxkit/tools/assert.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/numerics/safe_compare.hpp>

/**
 * If you for some reson need to know if DCHECKs are on, test the value of CXXKIT_DCHECK_IS_ON.
 * (Test its value, not if it's defined; it'll always be defined, to either a true or a false value.)
 */
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#    define CXXKIT_DCHECK_IS_ON 1
#else
#    define CXXKIT_DCHECK_IS_ON 0
#endif

// #define CXXKIT_CHECK(condition)                                                                                          \
//     if (!(condition))                                                                                                  \
//     cxxkit::Logger::FatalLogCall("Check "" #condition "" failed!") & CXXKIT_FATAL()

#define CXXKIT_CHECK(condition)                                                                                        \
    if (!(condition))                                                                                                  \
    CXXKIT_FATAL() << "Check "                                                                                         \
                      " #condition "                                                                                   \
                      " failed!"

#define CXXKIT_CHECK_OP(name, op, val1, val2)                                                                          \
    if (!cxxkit::Safe##name((val1), (val2)))                                                                           \
    CXXKIT_FATAL(cxxkit::StringView("Check "                                                                           \
                                    " #val1 "                                                                          \
                                    " #op "                                                                            \
                                    " #val2 "                                                                          \
                                    " failed!"))

#define CXXKIT_CHECK_EQ(val1, val2) CXXKIT_CHECK_OP(Eq, ==, val1, val2)
#define CXXKIT_CHECK_NE(val1, val2) CXXKIT_CHECK_OP(Ne, !=, val1, val2)
#define CXXKIT_CHECK_LE(val1, val2) CXXKIT_CHECK_OP(Le, <=, val1, val2)
#define CXXKIT_CHECK_LT(val1, val2) CXXKIT_CHECK_OP(Lt, <, val1, val2)
#define CXXKIT_CHECK_GE(val1, val2) CXXKIT_CHECK_OP(Ge, >=, val1, val2)
#define CXXKIT_CHECK_GT(val1, val2) CXXKIT_CHECK_OP(Gt, >, val1, val2)

/**
 * The CXXKIT_DCHECK macro is equivalent to RTC_CHECK except that it only generates code in debug builds.
 * It does reference the condition parameter in all cases, though, so callers won't risk getting warnings
 * about unused variables.
 */
#if CXXKIT_DCHECK_IS_ON
#    define CXXKIT_DCHECK(condition) CXXKIT_CHECK(condition)
#    define CXXKIT_DCHECK_EQ(v1, v2) CXXKIT_CHECK_EQ(v1, v2)
#    define CXXKIT_DCHECK_NE(v1, v2) CXXKIT_CHECK_NE(v1, v2)
#    define CXXKIT_DCHECK_LE(v1, v2) CXXKIT_CHECK_LE(v1, v2)
#    define CXXKIT_DCHECK_LT(v1, v2) CXXKIT_CHECK_LT(v1, v2)
#    define CXXKIT_DCHECK_GE(v1, v2) CXXKIT_CHECK_GE(v1, v2)
#    define CXXKIT_DCHECK_GT(v1, v2) CXXKIT_CHECK_GT(v1, v2)
#else
#    define CXXKIT_DCHECK(condition) CXXKIT_CHECK(true)
#    define CXXKIT_DCHECK_EQ(v1, v2) CXXKIT_CHECK(true)
#    define CXXKIT_DCHECK_NE(v1, v2) CXXKIT_CHECK(true)
#    define CXXKIT_DCHECK_LE(v1, v2) CXXKIT_CHECK(true)
#    define CXXKIT_DCHECK_LT(v1, v2) CXXKIT_CHECK(true)
#    define CXXKIT_DCHECK_GE(v1, v2) CXXKIT_CHECK(true)
#    define CXXKIT_DCHECK_GT(v1, v2) CXXKIT_CHECK(true)
#endif

#define CXXKIT_DCHECK_NOTREACHED() CXXKIT_DCHECK(false)

/**
 * Kills the process with an error message.
 * Never returns. Use when you wish to assert that a point in the code is never reached.
 */
#define CXXKIT_CHECK_NOTREACHED() cxxkit::Logger::FatalLogCall("Unreachable Code Reached!") & CXXKIT_FATAL()