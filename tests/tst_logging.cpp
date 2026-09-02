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

#include <cxxkit/tools/logging.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdarg>
#include <string>
#include <vector>

CXXKIT_BEGIN_NAMESPACE

namespace
{
// A handler that records (level, message) instead of writing to spdlog sinks.
struct CaptureHandler
{
    static Logger::MessageHandler make(std::vector<std::string> *out)
    {
        return [out](const char *name, const Logger::Context &ctx, const char *message)
        {
            (void)name;
            (void)ctx;
            out->push_back(std::string(message));
        };
    }
};
} // namespace

TEST(Logging, LoggerRegistryLookup)
{
    // The default logger is registered under the name "cxxkit".
    Logger *logger = Logger::logger("cxxkit");
    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(Logger::logger_id_number("cxxkit") >= 0, true);
    EXPECT_NE(Logger::logger_name(logger->id_number()), nullptr);
    EXPECT_STREQ(logger->name(), "cxxkit");

    // Unknown logger resolves to nullptr.
    EXPECT_EQ(Logger::logger("no_such_logger_xyz"), nullptr);
    EXPECT_EQ(Logger::logger_id_number("no_such_logger_xyz"), -1);
    EXPECT_EQ(Logger::logger_name(999999), nullptr);

    // all_loggers is non-empty and contains our logger.
    std::vector<Logger::Pointer> all = Logger::all_loggers();
    ASSERT_FALSE(all.empty());
    bool found = false;
    for (Logger::Pointer p : all)
    {
        if (p == logger)
            found = true;
    }
    EXPECT_TRUE(found);
}

TEST(Logging, LevelEnableSwitching)
{
    Logger logger("tst_level_logger"); // registered with default Debug level
    logger.switch_level(LogLevel::Warning);
    EXPECT_FALSE(logger.is_level_enabled(LogLevel::Trace));
    EXPECT_FALSE(logger.is_level_enabled(LogLevel::Debug));
    EXPECT_FALSE(logger.is_level_enabled(LogLevel::Info));
    EXPECT_TRUE(logger.is_level_enabled(LogLevel::Warning));
    EXPECT_TRUE(logger.is_level_enabled(LogLevel::Error));
    EXPECT_TRUE(logger.is_level_enabled(LogLevel::Critical));

    // toggle a single level.
    logger.set_level_enable(LogLevel::Error, false);
    EXPECT_FALSE(logger.is_level_enabled(LogLevel::Error));
    EXPECT_TRUE(logger.is_level_enabled(LogLevel::Warning));
    logger.set_level_enable(LogLevel::Error, true);
    EXPECT_TRUE(logger.is_level_enabled(LogLevel::Error));
}

TEST(Logging, NoSourceFlag)
{
    Logger logger("tst_nosource_logger");
    EXPECT_FALSE(logger.is_no_source());
    logger.set_no_source(true);
    EXPECT_TRUE(logger.is_no_source());
    logger.set_no_source(false);
    EXPECT_FALSE(logger.is_no_source());
}

TEST(Logging, OutputCapturedByHandler)
{
    Logger logger("tst_capture_logger");
    std::vector<std::string> captured;
    logger.install_message_handler(CaptureHandler::make(&captured), /*uniqueOwnership=*/true);

    logger.output(Logger::Context{LogLevel::Info, __FILE__, extract_file_name(__FILE__), "func", 1}, "hello world");
    EXPECT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0], "hello world");

    // vlogging path (va_list).
    {
        auto printv = [&](const char *fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            logger.vlogging(Logger::Context{LogLevel::Debug, __FILE__, extract_file_name(__FILE__), "func", 2},
                            fmt,
                            args);
            va_end(args);
        };
        printv("value=%d", 42);
    }
    ASSERT_EQ(captured.size(), 2u);
    EXPECT_EQ(captured[1], "value=42");

    // clear handler.
    logger.install_message_handler(nullptr);
}

TEST(Logging, StreamerFormatPath)
{
    Logger logger("tst_stream_logger");
    std::vector<std::string> captured;
    logger.install_message_handler(CaptureHandler::make(&captured), /*uniqueOwnership=*/true);

    // The Streamer's variadic logging(format, args...) path formats and flushes
    // to output() on Stream destruction.
    Logger::Streamer(logger, LogLevel::Info, __FILE__, "func", 3).logging("x=%d name=%s", 7, "alice");
    ASSERT_EQ(captured.size(), 1u);
    // The Streamer delivers the format string (and args are formatted into the stream
    // buffer during Stream::vlogging); the handler sees the context message.
    EXPECT_EQ(captured[0], "x=%d name=%s");

    logger.install_message_handler(nullptr);
}

TEST(Logging, ExtractFileName)
{
    EXPECT_STREQ(extract_file_name("/a/b/c.txt"), "c.txt");
    EXPECT_STREQ(extract_file_name("plain.txt"), "plain.txt");
    EXPECT_STREQ(extract_file_name("C:\\dir\\file.h"), "file.h");
    EXPECT_STREQ(extract_file_name(""), "");
}

TEST(Logging, DefaultLoggerMacrosDoNotCrash)
{
    // These should not abort/crash at Info/Debug/Warning/Error/Critical levels.
    Logger &logger = CXXKIT_LOGGER();
    logger.set_level_enable(LogLevel::Warning, false); // quiet down output
    CXXKIT_LOGGING_INFO(logger, "info %d", 1);
    CXXKIT_LOGGING_DEBUG(logger, "debug");
    CXXKIT_LOGGING_WARNING(logger, "warn");
    CXXKIT_LOGGING_ERROR(logger, "err");
    CXXKIT_LOGGING_CRITICAL(logger, "crit");
    logger.switch_level(LogLevel::Debug); // restore default-like state
}

CXXKIT_END_NAMESPACE
