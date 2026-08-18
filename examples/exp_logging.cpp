#include <iostream>
#include <utility>

#include "cxxkit/tools/logging.hpp"
#include "cxxkit/tools/variant.hpp"
#include "cxxkit/text/string_utils.hpp"

CXXKIT_DEFINE_LOGGER("my_log", MY_LOGGER)

namespace expns
{
std::pair<int, double> myfunction(int a, double b)
{
    CXXKIT_WARNING("funcname:%s, extract:%s", CXXKIT_STRFUNC, CXXKIT_STRFUNC_NAME);
    return std::make_pair(a, b);
}
} // namespace expns

int main()
{
    std::cout << "cxxkit_logging exp!\n" << std::endl;
    expns::myfunction(1, 2);
    std::cout << "\ncxxkit_logging c!" << std::endl;

    std::cout << "\ncxxkit_logging cxx!" << std::endl;
    CXXKIT_LOGGING_WARNING(MY_LOGGER(), "CXXKIT_LOGGING_WARN");
    CXXKIT_LOGGING_WARNING(MY_LOGGER(), "tst-") << "ss";
    CXXKIT_LOGGING_WARNING(MY_LOGGER()).format("{}-{}-", "tst", 1) << "ss";
    CXXKIT_LOGGING_WARNING(MY_LOGGER(), "{}-{}-", "tst", 1);
    CXXKIT_LOGGING_WARNING(MY_LOGGER()).format("{}-", cxxkit::utils::fmt::ptr(expns::myfunction)) << "ss";
    CXXKIT_LOGGING_WARNING(MY_LOGGER());

    CXXKIT_TRACE("CXXKIT_TRACE");
    CXXKIT_DEBUG("CXXKIT_DEBUG");
    CXXKIT_INFO("CXXKIT_INFO");
    CXXKIT_WARNING("CXXKIT_WARN");
    CXXKIT_ERROR("CXXKIT_ERROR");
    CXXKIT_CRITICAL("CXXKIT_CRITICAL");
    // CXXKIT_FATAL("CXXKIT_FATAL");

    CXXKIT_WARNING() << cxxkit::StringView("CXXKIT_WARNING StringView");

    std::cout << "log all!" << std::endl;
    CXXKIT_LOGGER().switchLevel(cxxkit::LogLevel::Trace);
    CXXKIT_TRACE("CXXKIT_TRACE");
    CXXKIT_TRACE() << "stream CXXKIT_TRACE";
    CXXKIT_DEBUG("CXXKIT_DEBUG");
    CXXKIT_DEBUG() << "stream CXXKIT_DEBUG";
    CXXKIT_INFO("CXXKIT_INFO");
    CXXKIT_INFO() << "stream CXXKIT_INFO";
    CXXKIT_WARNING("CXXKIT_WARN");
    CXXKIT_WARNING() << "stream CXXKIT_WARN";
    CXXKIT_ERROR("CXXKIT_ERROR");
    CXXKIT_ERROR() << "stream CXXKIT_ERROR";
    CXXKIT_CRITICAL("CXXKIT_CRITICAL");
    CXXKIT_CRITICAL() << "stream CXXKIT_CRITICAL";
    CXXKIT_FATAL("CXXKIT_FATAL");
    CXXKIT_FATAL() << "stream CXXKIT_FATAL";

    return 0;
}
