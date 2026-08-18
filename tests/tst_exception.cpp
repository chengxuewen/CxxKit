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

#include <cxxkit/tools/exception.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

CXXKIT_BEGIN_NAMESPACE

namespace
{
} // namespace

#if CXXKIT_HAS_EXCEPTIONS
#    define EXPECT_THROW_OR_DEATH(statement, expected_exception) EXPECT_THROW(statement, expected_exception)
#else
#    define EXPECT_THROW_OR_DEATH(statement, expected_exception) EXPECT_DEATH(statement, #expected_exception)
#endif

TEST(ExceptionTest, StdLogicError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::logic_error, ""), std::logic_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::logic_error, "std::logic_error:d", 1), std::logic_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::logic_error, StringView("std::logic_error")), std::logic_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::logic_error, std::string("std::logic_error")), std::logic_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LOGIC_ERROR(""), std::logic_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LOGIC_ERROR("std::logic_error:d", 1), std::logic_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LOGIC_ERROR(StringView("std::logic_error")), std::logic_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LOGIC_ERROR(std::string("std::logic_error")), std::logic_error);
}

TEST(ExceptionTest, StdInvalidArgument)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::invalid_argument, ""), std::invalid_argument);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::invalid_argument, "std::invalid_argument:d", 1), std::invalid_argument);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::invalid_argument, StringView("std::invalid_argument")),
                          std::invalid_argument);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::invalid_argument, std::string("std::invalid_argument")),
                          std::invalid_argument);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_INVALID_ARGUMENT(""), std::invalid_argument);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_INVALID_ARGUMENT("std::invalid_argument:d", 1), std::invalid_argument);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_INVALID_ARGUMENT(StringView("std::invalid_argument")), std::invalid_argument);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_INVALID_ARGUMENT(std::string("std::invalid_argument")), std::invalid_argument);
}

TEST(ExceptionTest, StdDomainError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::domain_error, ""), std::domain_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::domain_error, "std::domain_error:d", 1), std::domain_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::domain_error, StringView("std::domain_error")), std::domain_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::domain_error, std::string("std::domain_error")), std::domain_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_DOMAIN_ERROR(""), std::domain_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_DOMAIN_ERROR("std::domain_error:d", 1), std::domain_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_DOMAIN_ERROR(StringView("std::domain_error")), std::domain_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_DOMAIN_ERROR(std::string("std::domain_error")), std::domain_error);
}

TEST(ExceptionTest, StdLengthError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::length_error, ""), std::length_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::length_error, "std::length_error:d", 1), std::length_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::length_error, StringView("std::length_error")), std::length_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::length_error, std::string("std::length_error")), std::length_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LENGTH_ERROR(""), std::length_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LENGTH_ERROR("std::length_error:d", 1), std::length_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LENGTH_ERROR(StringView("std::length_error")), std::length_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_LENGTH_ERROR(std::string("std::length_error")), std::length_error);
}

TEST(ExceptionTest, StdOutOfRange)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::out_of_range, ""), std::out_of_range);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::out_of_range, "std::out_of_range:d", 1), std::out_of_range);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::out_of_range, StringView("std::out_of_range")), std::out_of_range);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::out_of_range, std::string("std::out_of_range")), std::out_of_range);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OUT_OF_RANGE(""), std::out_of_range);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OUT_OF_RANGE("std::out_of_range:d", 1), std::out_of_range);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OUT_OF_RANGE(StringView("std::out_of_range")), std::out_of_range);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OUT_OF_RANGE(std::string("std::out_of_range")), std::out_of_range);
}

TEST(ExceptionTest, StdRuntimeError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::runtime_error, ""), std::runtime_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::runtime_error, "std::runtime_error:d", 1), std::runtime_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::runtime_error, StringView("std::runtime_error")), std::runtime_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::runtime_error, std::string("std::runtime_error")), std::runtime_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RUNTIME_ERROR(""), std::runtime_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RUNTIME_ERROR("std::runtime_error:d", 1), std::runtime_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RUNTIME_ERROR(StringView("std::runtime_error")), std::runtime_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RUNTIME_ERROR(std::string("std::runtime_error")), std::runtime_error);
}

TEST(ExceptionTest, StdRangeError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::range_error, ""), std::range_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::range_error, "std::range_error:d", 1), std::range_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::range_error, StringView("std::range_error")), std::range_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::range_error, std::string("std::range_error")), std::range_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RANGE_ERROR(""), std::range_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RANGE_ERROR("std::range_error:d", 1), std::range_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RANGE_ERROR(StringView("std::range_error")), std::range_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_RANGE_ERROR(std::string("std::range_error")), std::range_error);
}

TEST(ExceptionTest, StdOverflowError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::overflow_error, ""), std::overflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::overflow_error, "std::overflow_error:d", 1), std::overflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::overflow_error, StringView("std::overflow_error")), std::overflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::overflow_error, std::string("std::overflow_error")), std::overflow_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OVERFLOW_ERROR(""), std::overflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OVERFLOW_ERROR("std::overflow_error:d", 1), std::overflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OVERFLOW_ERROR(StringView("std::overflow_error")), std::overflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_OVERFLOW_ERROR(std::string("std::overflow_error")), std::overflow_error);
}

TEST(ExceptionTest, StdUnderflowError)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::underflow_error, ""), std::underflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::underflow_error, "std::underflow_error:d", 1), std::underflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::underflow_error, StringView("std::underflow_error")), std::underflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW(std::underflow_error, std::string("std::underflow_error")), std::underflow_error);

    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_UNDERFLOW_ERROR(""), std::underflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_UNDERFLOW_ERROR("std::underflow_error:d", 1), std::underflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_UNDERFLOW_ERROR(StringView("std::underflow_error")), std::underflow_error);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_UNDERFLOW_ERROR(std::string("std::underflow_error")), std::underflow_error);
}

TEST(ExceptionTest, StdBadFunctionCall)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_NO_MSG(std::bad_function_call), std::bad_function_call);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_BAD_FUNCTION_CALL(), std::bad_function_call);
}

TEST(ExceptionTest, StdBadAlloc)
{
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_NO_MSG(std::bad_alloc), std::bad_alloc);
    EXPECT_THROW_OR_DEATH(CXXKIT_THROW_STD_BAD_ALLOC(), std::bad_alloc);
}

CXXKIT_END_NAMESPACE