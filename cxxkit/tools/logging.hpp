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

#include <cxxkit/tools/tools_global.hpp>

#include <cxxkit/text/format.hpp>
#include <cxxkit/text/string_utils.hpp>

#include <cstring>
#include <functional>
#include <ostream>
#include <sstream>
#include <vector>
#include <string>

#define CXXKIT_LOGGING_BUFFER_SIZE_MAX 102400 // 100kb

CXXKIT_BEGIN_NAMESPACE

enum class LogLevel : int
{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
    Fatal = 6,
};

static constexpr int kLogLevelNum = 7;

class LoggerPrivate;

// Header-only file-name basename (avoids linking tools->text for logging contexts).
// Mirror of text::extract_file_name, kept local so tools stays independent of text at link time.
inline const char *extract_file_name(const char *file_path)
{
    const char *slash = strrchr(file_path, '/');
    const char *wslash = strrchr(file_path, '\\');
    const char *base = file_path;
    if (slash)
        base = slash + 1;
    if (wslash && wslash > base - 1)
        base = wslash + 1;
    return base;
}

class CXXKIT_TOOLS_API Logger
{
public:
    using Pointer = Logger *;

    struct Context final
    {
        LogLevel level;
        const char *file_path;
        const char *file_name;
        const char *funcName;
        int line;
    };

    class Stream final
    {
        struct StreamData
        {
            StreamData(Logger &target, const Context &ctx, bool spa)
                : ref(1)
                , space(spa)
                , logger(target)
                , context(ctx)
            {
            }

            int ref;
            bool space;
            Logger &logger;
            std::stringstream ss;
            const Context &context;
        } *mStream;

    public:
        inline Stream(const Stream &other)
            : mStream(other.mStream)
        {
            ++mStream->ref;
        }

        inline Stream(Logger &target, const Context &ctx, bool spa = false)
            : mStream(new StreamData(target, ctx, spa))
        {
        }

        virtual ~Stream()
        {
            if (!--mStream->ref)
            {
                std::string buffer(mStream->ss.str());
                if (mStream->space && buffer.back() == ' ')
                {
                    buffer.pop_back();
                }
                mStream->logger.output(mStream->context, buffer.c_str());
                delete mStream;
            }
        }

        inline Stream &operator=(const Stream &other)
        {
            if (this != &other)
            {
                Stream copy(other);
                std::swap(mStream, copy.mStream);
            }
            return *this;
        }

        void put_ucs2(uint16_t ucs2) { }

        void put_ucs4(uint32_t ucs4) { }

        void put_string(const char *begin, size_t length) { }

        CXXKIT_FORCE_INLINE void prepend(const char *message)
        {
            std::stringstream ss;
            ss << message << mStream->ss.get();
            std::swap(mStream->ss, ss);
        }

        inline Stream &space()
        {
            mStream->space = true;
            mStream->ss << ' ';
            return *this;
        }

        inline Stream &nospace()
        {
            mStream->space = false;
            return *this;
        }

        inline Stream &maybe_space()
        {
            if (mStream->space)
            {
                mStream->ss << ' ';
            }
            return *this;
        }

        inline Stream &operator<<(bool t)
        {
            mStream->ss << (t ? "true" : "false");
            return this->maybe_space();
        }

        inline Stream &operator<<(char t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(signed short t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(unsigned short t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(char16_t t)
        {
            this->put_ucs2(t);
            return this->maybe_space();
        }

        inline Stream &operator<<(char32_t t)
        {
            this->put_ucs4(t);
            return this->maybe_space();
        }

        inline Stream &operator<<(signed int t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(unsigned int t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(signed long t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(signed long long t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(unsigned long t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(unsigned long long t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(float t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(double t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(const char *t)
        {
            if (t)
            {
                mStream->ss << t;
                return this->maybe_space();
            }
            return *this;
        }

        inline Stream &operator<<(const std::string &t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(const void *t)
        {
            mStream->ss << t;
            return this->maybe_space();
        }

        inline Stream &operator<<(std::nullptr_t)
        {
            mStream->ss << "(nullptr)";
            return this->maybe_space();
        }

        inline Stream &operator<<(const std::stringstream &ss)
        {
            mStream->ss << ss.str();
            return this->maybe_space();
        }

        inline Stream &operator<<(const StringView &t)
        {
            mStream->ss << t.data();
            return this->maybe_space();
        }

        template <typename... Args>
        inline Stream &format(const char *format, const Args &...args)
        {
            mStream->ss << utils::fmt::vformat(utils::fmt::string_view(format), utils::fmt::make_format_args(args...));
            return this->maybe_space();
        }

        template <typename... Args>
        inline Stream &printf(const char *format, const Args &...args)
        {
            mStream->ss << utils::fmt::vsprintf(utils::fmt::string_view(format), utils::fmt::make_printf_args(args...));
            return this->maybe_space();
        }
    };

    struct Streamer final
    {
        Streamer(Logger &target, LogLevel level, const char *file_path, const char *funcName, int line)
            : logger(target)
            , context{level, file_path, extract_file_name(file_path), funcName, line}
        {
        }

        CXXKIT_FORCE_INLINE Stream logging(const char *string) const
        {
            // raw string
            return Stream(logger, context, false) << string;
        }
        CXXKIT_FORCE_INLINE Stream logging(const StringView &string) const
        {
            return Stream(logger, context, true) << string;
        }

        CXXKIT_FORCE_INLINE Stream logging(const std::string &string) const
        {
            return Stream(logger, context, true) << string;
        }

        CXXKIT_FORCE_INLINE Stream logging(const std::stringstream &stream) const
        {
            return Stream(logger, context, true) << stream.str();
        }

        CXXKIT_FORCE_INLINE Stream logging() const { return Stream(logger, context, false); }

        template <typename... Args>
        Stream logging(const char *format, const Args &...args) const
        {
            return Stream(logger, context, false).format(format, args...);
        }

        Logger &logger;
        Context context;
    };

    class FatalLogCall final
    {
    public:
        FatalLogCall(const char *message)
            : mMessage(message)
        {
        }

        CXXKIT_FORCE_INLINE void operator&(Stream stream) { stream.prepend(mMessage); }

    private:
        const char *mMessage;
    };

    Logger(const char *name, LogLevel defaultLevel = LogLevel::Debug);
    explicit Logger(LoggerPrivate *d);
    virtual ~Logger();

    static Pointer logger(int id_number);
    static Pointer logger(const char *name);
    static int logger_id_number(const char *name);
    static const char *logger_name(int id_number);
    static std::vector<Pointer> all_loggers();

    int id_number() const;
    const char *name() const;

    bool is_no_source() const;
    void set_no_source(bool noSource);

    void switch_level(LogLevel level);
    bool is_level_enabled(LogLevel level) const;
    void set_level_enable(LogLevel level, bool enable);

    void output(const Context &context, const char *message);
    void vlogging(const Context &context, const char *format, va_list args);

    using MessageHandler = std::function<void(const char *name, const Context &context, const char *message)>;
    void install_message_handler(const MessageHandler &handler, bool uniqueOwnership = false);

protected:
    void fatal_abort();

private:
    CXXKIT_DEFINE_DPTR(Logger)
    CXXKIT_DECLARE_PRIVATE(Logger)
    CXXKIT_DISABLE_COPY_MOVE(Logger)
};

CXXKIT_END_NAMESPACE

#define CXXKIT_STRFUNC_NAME cxxkit::utils::extract_function_name(CXXKIT_STRFUNC).c_str()

#define CXXKIT_DECLARE_LOGGER(export, logger) export cxxkit::Logger &logger();
#define CXXKIT_DEFINE_LOGGER_WITH_LEVEL(name, logger, level)                                                           \
    cxxkit::Logger &logger()                                                                                           \
    {                                                                                                                  \
        static constexpr char logger_name[] = name;                                                                    \
        static cxxkit::Logger logger_instance(logger_name, level);                                                     \
        return logger_instance;                                                                                        \
    }                                                                                                                  \
    static struct logger##_builder                                                                                     \
    {                                                                                                                  \
        logger##_builder() { logger(); }                                                                               \
    } init_##logger;

#define CXXKIT_DEFINE_LOGGER(name, logger) CXXKIT_DEFINE_LOGGER_WITH_LEVEL(name, logger, cxxkit::LogLevel::Debug)

#define CXXKIT_LOGGING(logger, level, ...)                                                                             \
    for (bool enabled = logger.is_level_enabled(level); enabled; enabled = false)                                      \
    cxxkit::Logger::Streamer(logger, level, __FILE__, CXXKIT_STRFUNC, __LINE__).logging(__VA_ARGS__)
#define CXXKIT_LOGGING_FULL(logger, level, file, func, line, ...)                                                      \
    for (bool enabled = logger.is_level_enabled(level); enabled; enabled = false)                                      \
    cxxkit::Logger::Streamer(logger, level, file, func, line).logging(__VA_ARGS__)

#define CXXKIT_LOGGING_TRACE(logger, ...)    CXXKIT_LOGGING(logger, cxxkit::LogLevel::Trace, __VA_ARGS__)
#define CXXKIT_LOGGING_DEBUG(logger, ...)    CXXKIT_LOGGING(logger, cxxkit::LogLevel::Debug, __VA_ARGS__)
#define CXXKIT_LOGGING_INFO(logger, ...)     CXXKIT_LOGGING(logger, cxxkit::LogLevel::Info, __VA_ARGS__)
#define CXXKIT_LOGGING_WARNING(logger, ...)  CXXKIT_LOGGING(logger, cxxkit::LogLevel::Warning, __VA_ARGS__)
#define CXXKIT_LOGGING_ERROR(logger, ...)    CXXKIT_LOGGING(logger, cxxkit::LogLevel::Error, __VA_ARGS__)
#define CXXKIT_LOGGING_CRITICAL(logger, ...) CXXKIT_LOGGING(logger, cxxkit::LogLevel::Critical, __VA_ARGS__)
#define CXXKIT_LOGGING_FATAL(logger, ...)    CXXKIT_LOGGING(logger, cxxkit::LogLevel::Fatal, __VA_ARGS__)

CXXKIT_DECLARE_LOGGER(CXXKIT_TOOLS_API, CXXKIT_LOGGER)
#define CXXKIT_TRACE                                                                                                   \
    for (bool enabled = CXXKIT_LOGGER().is_level_enabled(cxxkit::LogLevel::Trace); enabled; enabled = false)           \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Trace, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_DEBUG                                                                                                   \
    for (bool enabled = CXXKIT_LOGGER().is_level_enabled(cxxkit::LogLevel::Debug); enabled; enabled = false)           \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Debug, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_INFO                                                                                                    \
    for (bool enabled = CXXKIT_LOGGER().is_level_enabled(cxxkit::LogLevel::Info); enabled; enabled = false)            \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Info, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_WARNING                                                                                                 \
    for (bool enabled = CXXKIT_LOGGER().is_level_enabled(cxxkit::LogLevel::Warning); enabled; enabled = false)         \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Warning, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_ERROR                                                                                                   \
    for (bool enabled = CXXKIT_LOGGER().is_level_enabled(cxxkit::LogLevel::Error); enabled; enabled = false)           \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Error, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_CRITICAL                                                                                                \
    for (bool enabled = CXXKIT_LOGGER().is_level_enabled(cxxkit::LogLevel::Critical); enabled; enabled = false)        \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Critical, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_FATAL                                                                                                   \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Fatal, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
