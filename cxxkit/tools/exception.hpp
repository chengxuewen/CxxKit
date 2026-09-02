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

#include <cxxkit/base/global.hpp>
#include <cxxkit/tools/logging.hpp>
#include <cxxkit/tools/expected.hpp>
#include <cxxkit/text/string_view.hpp>

#include <string>
#include <exception>

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
static inline void noop(void)
{
}

struct ExceptionWhat final
{
    ExceptionWhat(const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        char string[CXXKIT_LINE_MAX] = {0};
        std::vsnprintf(string, CXXKIT_LINE_MAX, format, args);
        va_end(args);
        what = string;
    }
    ExceptionWhat(StringView string)
        : what(string)
    {
    }
    ExceptionWhat(const std::string &string)
        : what(string)
    {
    }
    ExceptionWhat(const std::stringstream &stream)
        : what(stream.str())
    {
    }
    std::string what;
};

static inline const char *get_c_str_helper(const char *string)
{
    return string;
}
static inline const char *get_c_str_helper(const StringView &string)
{
    return string.data();
}
static inline const char *get_c_str_helper(const std::string &string)
{
    return string.data();
}
static inline const char *get_c_str_helper(const ExceptionWhat &exceptionWhat)
{
    return exceptionWhat.what.c_str();
}
}; // namespace detail

namespace utils
{
template <typename R>
Expected<R, std::string> try_catch_call(const std::function<R()> func)
{
#if CXXKIT_HAS_EXCEPTIONS
    try
    {
        return func();
    }
    catch (std::exception &e)
    {
        return make_unexpected(e.what());
    }
#else
    return func();
#endif
}
} // namespace utils

CXXKIT_END_NAMESPACE

#if CXXKIT_HAS_EXCEPTIONS
#    define CXXKIT_TRY                              try
#    define CXXKIT_CATCH(A)                         catch (A)
#    define CXXKIT_RETHROW                          throw
#    define CXXKIT_THROW_DELEGATE(exception, what)  throw exception(cxxkit::detail::get_c_str_helper(what))
#    define CXXKIT_THROW_NO_MSG_DELEGATE(exception) throw exception()
#else
#    define CXXKIT_TRY                              if (true)
#    define CXXKIT_CATCH(A)                         else
#    define CXXKIT_RETHROW                          cxxkit::detail::noop()
#    define CXXKIT_THROW_DELEGATE(exception, what)  CXXKIT_FATAL("%s", detail::get_c_str_helper(what))
#    define CXXKIT_THROW_NO_MSG_DELEGATE(exception) CXXKIT_FATAL("%s", #exception)
#endif
#define CXXKIT_THROW(exception, ...)   CXXKIT_THROW_DELEGATE(exception, cxxkit::detail::ExceptionWhat(__VA_ARGS__))
#define CXXKIT_THROW_NO_MSG(exception) CXXKIT_THROW_NO_MSG_DELEGATE(exception)

#define CXXKIT_THROW_STD_LOGIC_ERROR(...)      CXXKIT_THROW(std::logic_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_INVALID_ARGUMENT(...) CXXKIT_THROW(std::invalid_argument, __VA_ARGS__)
#define CXXKIT_THROW_STD_DOMAIN_ERROR(...)     CXXKIT_THROW(std::domain_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_LENGTH_ERROR(...)     CXXKIT_THROW(std::length_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_OUT_OF_RANGE(...)     CXXKIT_THROW(std::out_of_range, __VA_ARGS__)
#define CXXKIT_THROW_STD_RUNTIME_ERROR(...)    CXXKIT_THROW(std::runtime_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_RANGE_ERROR(...)      CXXKIT_THROW(std::range_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_OVERFLOW_ERROR(...)   CXXKIT_THROW(std::overflow_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_UNDERFLOW_ERROR(...)  CXXKIT_THROW(std::underflow_error, __VA_ARGS__)
#define CXXKIT_THROW_STD_BAD_FUNCTION_CALL()   CXXKIT_THROW_NO_MSG(std::bad_function_call)
#define CXXKIT_THROW_STD_BAD_ALLOC()           CXXKIT_THROW_NO_MSG(std::bad_alloc)
