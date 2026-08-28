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

#include <cxxkit/tools/detail/logging_p.hpp>
#include <cxxkit/memory/memory.hpp>
#include <cxxkit/tools/assert.hpp>

#include <unordered_map>
#ifndef CXXKIT_OS_WIN32
#    include <stdlib.h> // abort
#else
#    include <windows.h>
#endif

CXXKIT_DEFINE_LOGGER("cxxkit", CXXKIT_LOGGER)

CXXKIT_BEGIN_NAMESPACE

namespace detail
{
static inline std::mutex &loggers_map_mutex()
{
    static std::mutex mutex;
    return mutex;
}

static inline std::unordered_map<int, Logger::Pointer> &loggers_id_map()
{
    static std::unordered_map<int, Logger::Pointer> map;
    return map;
}

static inline std::unordered_map<std::string, Logger::Pointer> &loggers_name_map()
{
    static std::unordered_map<std::string, Logger::Pointer> map;
    return map;
}

static inline std::atomic<int> &logger_id_number_counter()
{
    static std::atomic<int> counter{0};
    return counter;
}

static inline std::string current_thread_id_string()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}
}; // namespace detail

LoggerPrivate::LoggerPrivate(Logger *p, const char *name)
    : mPPtr(p)
    , mIdNumber(detail::logger_id_number_counter().fetch_add(1))
    , mName(name)
    , mNoSource(false)
{
    std::lock_guard<std::mutex> locker(detail::loggers_map_mutex());
    detail::loggers_id_map().emplace(mIdNumber, p);
    detail::loggers_name_map().emplace(name, p);
}

LoggerPrivate::~LoggerPrivate()
{
}

bool LoggerPrivate::message_handler_output(const Context &context, const char *message)
{
    const auto handlerWraper = mMessageHandlerWraper.load();
    if (handlerWraper)
    {
        handlerWraper->handler(mName, context, message);
        return mMessageHandleUniqueOwnership.load();
    }
    return false;
}

Logger::Logger(const char *name, LogLevel defaultLevel)
    : mDPtr(new LoggerPrivate(this, name))
{
    std::vector<spdlog::sink_ptr> sinks;
    const auto baseFilename = "log/" + std::string(name) + "_daily";
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    //sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(baseFilename, 0, 0, false, 7));
    mDPtr->mLogger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    mDPtr->mLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] <%t> [%s:%#] %v");
    mDPtr->mLogger->set_level(spdlog::level::trace);
    mDPtr->mLogger->flush_on(spdlog::level::debug);
    this->switch_level(defaultLevel);
}

Logger::Logger(LoggerPrivate *d)
    : mDPtr(d)
{
}

Logger::~Logger()
{
}

Logger::Pointer Logger::logger(int id_number)
{
    std::lock_guard<std::mutex> locker(detail::loggers_map_mutex());
    const auto iter = detail::loggers_id_map().find(id_number);
    return detail::loggers_id_map().end() != iter ? iter->second : nullptr;
}

Logger::Pointer Logger::logger(const char *name)
{
    std::lock_guard<std::mutex> locker(detail::loggers_map_mutex());
    const auto iter = detail::loggers_name_map().find(name);
    return detail::loggers_name_map().end() != iter ? iter->second : nullptr;
}

int Logger::logger_id_number(const char *name)
{
    auto logger = Logger::logger(name);
    return logger ? logger->id_number() : -1;
}

const char *Logger::logger_name(int id_number)
{
    auto logger = Logger::logger(id_number);
    return logger ? logger->name() : nullptr;
}

std::vector<Logger::Pointer> Logger::all_loggers()
{
    std::vector<Logger::Pointer> loggers;
    std::lock_guard<std::mutex> locker(detail::loggers_map_mutex());
    std::transform(detail::loggers_name_map().begin(),
                   detail::loggers_name_map().end(),
                   std::back_inserter(loggers),
                   [](const std::pair<std::string, Logger::Pointer> &pair) { return pair.second; });
    return loggers;
}

int Logger::id_number() const
{
    CXXKIT_D(const Logger);
    return d->mIdNumber;
}

const char *Logger::name() const
{
    CXXKIT_D(const Logger);
    return d->mName;
}

bool Logger::is_no_source() const
{
    CXXKIT_D(const Logger);
    return d->mNoSource;
}

void Logger::set_no_source(bool noSource)
{
    CXXKIT_D(Logger);
    d->mNoSource = noSource;
}

void Logger::switch_level(LogLevel level)
{
    CXXKIT_D(Logger);
    for (size_t i = 0; i < kLogLevelNum; i++)
    {
        d->mLevelEnabled[i].store(i >= (int)level);
    }
}

bool Logger::is_level_enabled(LogLevel level) const
{
    CXXKIT_D(const Logger);
    return d->mLevelEnabled[(int)level].load();
}

void Logger::set_level_enable(LogLevel level, bool enable)
{
    CXXKIT_D(Logger);
    d->mLevelEnabled[(int)level].store(enable);
}

void Logger::output(const Context &context, const char *message)
{
    CXXKIT_D(Logger);
    if (!d->message_handler_output(context, message))
    {
        if (d->mNoSource)
        {
            d->mLogger->log(static_cast<spdlog::level::level_enum>(context.level), message);
        }
        else
        {
            d->mLogger->log(spdlog::source_loc{context.file_path, context.line, context.funcName},
                            static_cast<spdlog::level::level_enum>(context.level),
                            message);
        }
    }
    if (LogLevel::Fatal == context.level)
    {
        this->fatal_abort();
    }
}

void Logger::vlogging(const Context &context, const char *format, va_list args)
{
    CXXKIT_D(Logger);
    char message[CXXKIT_LOGGING_BUFFER_SIZE_MAX] = {0};
    std::vsnprintf(message, CXXKIT_LOGGING_BUFFER_SIZE_MAX, format, args);
    if (!d->message_handler_output(context, message))
    {
        if (d->mNoSource)
        {
            d->mLogger->log(static_cast<spdlog::level::level_enum>(context.level), message);
        }
        else
        {
            d->mLogger->log(spdlog::source_loc{context.file_path, context.line, context.funcName},
                            static_cast<spdlog::level::level_enum>(context.level),
                            message);
        }
    }
    if (LogLevel::Fatal == context.level)
    {
        this->fatal_abort();
    }
}

void Logger::install_message_handler(const MessageHandler &handler, bool uniqueOwnership)
{
    CXXKIT_D(Logger);
    auto newWraper = handler ? new LoggerPrivate::MessageHandlerWraper(handler) : nullptr;
    auto oldWraper = d->mMessageHandlerWraper.exchange(newWraper);
    d->mMessageHandleUniqueOwnership.store(uniqueOwnership);
    if (oldWraper)
    {
        delete oldWraper;
    }
}

void Logger::fatal_abort()
{
#ifdef CXXKIT_OS_WIN32
    DebugBreak();
#endif
    abort();
}

CXXKIT_END_NAMESPACE

CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Trace) == SPDLOG_LEVEL_TRACE);
CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Debug) == SPDLOG_LEVEL_DEBUG);
CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Info) == SPDLOG_LEVEL_INFO);
CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Warning) == SPDLOG_LEVEL_WARN);
CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Error) == SPDLOG_LEVEL_ERROR);
CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Critical) == SPDLOG_LEVEL_CRITICAL);
CXXKIT_STATIC_ASSERT(static_cast<int>(cxxkit::LogLevel::Fatal) == SPDLOG_LEVEL_OFF);
