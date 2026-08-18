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

#include <cxxkit/tools/assert.hpp>
#include <cxxkit/tools/logging.hpp>

void cxxkit_assert_x(const char *where, const char *what, const char *file, int line) CXXKIT_NOTHROW
{
    auto loggerWraper = cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Fatal, file, where, line);
    loggerWraper.logging("%s : %s", where, what);
}

void cxxkit_assert(const char *assertion, const char *file, int line) CXXKIT_NOTHROW
{
    auto loggerWraper = cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Fatal, file, CXXKIT_STRFUNC, line);
    loggerWraper.logging("%s", assertion);
}