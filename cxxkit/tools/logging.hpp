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

static constexpr int LogLevelNum = 7;

class LoggerPrivate;

// Header-only file-name basename (avoids linking tools->text for logging contexts).
// Mirror of text::extractFileName, kept local so tools stays independent of text at link time.
inline const char *extractFileName(const char *filePath)
{
    const char *slash = strrchr(filePath, '/');
    const char *wslash = strrchr(filePath, '\\');
    const char *base = filePath;
    if (slash) base = slash + 1;
    if (wslash && wslash > base - 1) base = wslash + 1;
    return base;
}

class CXXKIT_TOOLS_API Logger
{
public:
    using Pointer = Logger *;

    struct Context final
    {
        LogLevel level;
        const char *filePath;
        const char *fileName;
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

        void putUcs2(uint16_t ucs2) { }

        void putUcs4(uint32_t ucs4) { }

        void putString(const char *begin, size_t length) { }

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

        inline Stream &maybeSpace()
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
            return this->maybeSpace();
        }

        inline Stream &operator<<(char t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(signed short t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(unsigned short t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(char16_t t)
        {
            this->putUcs2(t);
            return this->maybeSpace();
        }

        inline Stream &operator<<(char32_t t)
        {
            this->putUcs4(t);
            return this->maybeSpace();
        }

        inline Stream &operator<<(signed int t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(unsigned int t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(signed long t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(signed long long t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(unsigned long t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(unsigned long long t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(float t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(double t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(const char *t)
        {
            if (t)
            {
                mStream->ss << t;
                return this->maybeSpace();
            }
            return *this;
        }

        inline Stream &operator<<(const std::string &t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(const void *t)
        {
            mStream->ss << t;
            return this->maybeSpace();
        }

        inline Stream &operator<<(std::nullptr_t)
        {
            mStream->ss << "(nullptr)";
            return this->maybeSpace();
        }

        inline Stream &operator<<(const std::stringstream &ss)
        {
            mStream->ss << ss.str();
            return this->maybeSpace();
        }

        inline Stream &operator<<(const StringView &t)
        {
            mStream->ss << t.data();
            return this->maybeSpace();
        }

        template <typename... Args>
        inline Stream &format(const char *format, const Args &...args)
        {
            mStream->ss << utils::fmt::vformat(utils::fmt::string_view(format), utils::fmt::make_format_args(args...));
            return this->maybeSpace();
        }

        template <typename... Args>
        inline Stream &printf(const char *format, const Args &...args)
        {
            mStream->ss << utils::fmt::vsprintf(utils::fmt::string_view(format), utils::fmt::make_printf_args(args...));
            return this->maybeSpace();
        }
    };

    struct Streamer final
    {
        Streamer(Logger &target, LogLevel level, const char *filePath, const char *funcName, int line)
            : logger(target)
            , context{level, filePath, extractFileName(filePath), funcName, line}
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

    static Pointer logger(int idNumber);
    static Pointer logger(const char *name);
    static int loggerIdNumber(const char *name);
    static const char *loggerName(int idNumber);
    static std::vector<Pointer> allLoggers();

    int idNumber() const;
    const char *name() const;

    bool isNoSource() const;
    void setNoSource(bool noSource);

    void switchLevel(LogLevel level);
    bool isLevelEnabled(LogLevel level) const;
    void setLevelEnable(LogLevel level, bool enable);

    void output(const Context &context, const char *message);
    void vlogging(const Context &context, const char *format, va_list args);

    using MessageHandler = std::function<void(const char *name, const Context &context, const char *message)>;
    void installMessageHandler(const MessageHandler &handler, bool uniqueOwnership = false);

protected:
    void fatalAbort();

private:
    CXXKIT_DEFINE_DPTR(Logger)
    CXXKIT_DECLARE_PRIVATE(Logger)
    CXXKIT_DISABLE_COPY_MOVE(Logger)
};

CXXKIT_END_NAMESPACE

#define CXXKIT_STRFUNC_NAME cxxkit::utils::extractFunctionName(CXXKIT_STRFUNC).c_str()

#define CXXKIT_DECLARE_LOGGER(export, logger) export cxxkit::Logger &logger();
#define CXXKIT_DEFINE_LOGGER_WITH_LEVEL(name, logger, level)                                                           \
    cxxkit::Logger &logger()                                                                                           \
    {                                                                                                                  \
        static constexpr char loggerName[] = name;                                                                     \
        static cxxkit::Logger loggerInstance(loggerName, level);                                                       \
        return loggerInstance;                                                                                         \
    }                                                                                                                  \
    static struct logger##_builder                                                                                     \
    {                                                                                                                  \
        logger##_builder()                                                                                             \
        {                                                                                                              \
            logger();                                                                                                  \
        }                                                                                                              \
    } init_##logger;

#define CXXKIT_DEFINE_LOGGER(name, logger) CXXKIT_DEFINE_LOGGER_WITH_LEVEL(name, logger, cxxkit::LogLevel::Debug)

#define CXXKIT_LOGGING(logger, level, ...)                                                                             \
    for (bool enabled = logger.isLevelEnabled(level); enabled; enabled = false)                                        \
    cxxkit::Logger::Streamer(logger, level, __FILE__, CXXKIT_STRFUNC, __LINE__).logging(__VA_ARGS__)
#define CXXKIT_LOGGING_FULL(logger, level, file, func, line, ...)                                                      \
    for (bool enabled = logger.isLevelEnabled(level); enabled; enabled = false)                                        \
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
    for (bool enabled = CXXKIT_LOGGER().isLevelEnabled(cxxkit::LogLevel::Trace); enabled; enabled = false)             \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Trace, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_DEBUG                                                                                                   \
    for (bool enabled = CXXKIT_LOGGER().isLevelEnabled(cxxkit::LogLevel::Debug); enabled; enabled = false)             \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Debug, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_INFO                                                                                                    \
    for (bool enabled = CXXKIT_LOGGER().isLevelEnabled(cxxkit::LogLevel::Info); enabled; enabled = false)              \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Info, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_WARNING                                                                                                 \
    for (bool enabled = CXXKIT_LOGGER().isLevelEnabled(cxxkit::LogLevel::Warning); enabled; enabled = false)           \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Warning, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_ERROR                                                                                                   \
    for (bool enabled = CXXKIT_LOGGER().isLevelEnabled(cxxkit::LogLevel::Error); enabled; enabled = false)             \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Error, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_CRITICAL                                                                                                \
    for (bool enabled = CXXKIT_LOGGER().isLevelEnabled(cxxkit::LogLevel::Critical); enabled; enabled = false)          \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Critical, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
#define CXXKIT_FATAL                                                                                                   \
    cxxkit::Logger::Streamer(CXXKIT_LOGGER(), cxxkit::LogLevel::Fatal, __FILE__, CXXKIT_STRFUNC, __LINE__).logging
