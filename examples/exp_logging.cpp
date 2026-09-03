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

// exp_logging: tools::Logger levels, custom loggers, runtime filtering, streaming format.

#include <iostream>
#include <utility>

#include <cxxkit/tools/logging.hpp>
#include <cxxkit/tools/variant.hpp>
#include <cxxkit/text/string_utils.hpp>

CXXKIT_DEFINE_LOGGER("my_log", MY_LOGGER)

namespace expns
{
std::pair<int, double> myfunction(int a, double b)
{
    CXXKIT_WARNING("funcname:{}, extract:{}", CXXKIT_STRFUNC, CXXKIT_STRFUNC_NAME);
    return std::make_pair(a, b);
}
} // namespace expns

int main()
{
    CXXKIT_TRACE("this trace line is filtered (below default Debug)");
    CXXKIT_DEBUG("CXXKIT_DEBUG");
    CXXKIT_INFO("CXXKIT_INFO");
    CXXKIT_WARNING("CXXKIT_WARNING");
    CXXKIT_ERROR("CXXKIT_ERROR");
    CXXKIT_CRITICAL("CXXKIT_CRITICAL");

    // 2) Custom logger (default level Debug) + function/line context capture.
    std::cout << "\n--- 2. custom logger (MY_LOGGER, level Debug) ---" << std::endl;
    expns::myfunction(1, 2);
    CXXKIT_LOGGING_INFO(MY_LOGGER(), "info via custom logger, value={}", 42);
    CXXKIT_LOGGING_TRACE(MY_LOGGER(), "this trace line is filtered (below custom logger level Debug)");

    // 3) Runtime level filtering: raise global logger to Error, WARNING and below disappear.
    std::cout << "\n--- 3. runtime filtering (global logger raised to Error) ---" << std::endl;
    CXXKIT_LOGGER().switch_level(cxxkit::LogLevel::Error);
    CXXKIT_DEBUG("filtered: Debug < Error");
    CXXKIT_INFO("filtered: Info < Error");
    CXXKIT_WARNING("filtered: Warning < Error");
    CXXKIT_ERROR("CXXKIT_ERROR still passes");
    CXXKIT_CRITICAL("CXXKIT_CRITICAL still passes");

    // 4) Streaming << and fmt-style .format() on one line.
    std::cout << "\n--- 4. streaming + format mix ---" << std::endl;
    CXXKIT_LOGGER().switch_level(cxxkit::LogLevel::Trace);
    CXXKIT_INFO() << "streamed: int=" << 7 << " double=" << 2.5;
    CXXKIT_INFO("fmt:").format("{} + {} = {}", 1, 2, 3) << " (mixed)";
    CXXKIT_LOGGING_WARNING(MY_LOGGER(), "custom fmt:").format("{}-{}", "a", 1) << " end";

    std::cout << "\n--- done ---" << std::endl;
    return 0;
}
