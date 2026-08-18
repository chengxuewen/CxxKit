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

#pragma once

#include <cxxkit/tools/logging.hpp>

#include <cxxkit/3rdparty/spdlog/spdlog.h>
#include <cxxkit/3rdparty/spdlog/sinks/daily_file_sink.h>
#include <cxxkit/3rdparty/spdlog/sinks/stdout_color_sinks.h>
#include <cxxkit/3rdparty/spdlog/sinks/rotating_file_sink.h>

#include <atomic>

CXXKIT_BEGIN_NAMESPACE

class CXXKIT_CORE_API LoggerPrivate
{
public:
    using Context = Logger::Context;
    using MessageHandler = Logger::MessageHandler;
    struct MessageHandlerWraper
    {
        explicit MessageHandlerWraper(const MessageHandler &h) : handler(h) {}
        const MessageHandler handler;
    };

    LoggerPrivate(Logger *p, const char *name);
    virtual ~LoggerPrivate();

    bool messageHandlerOutput(const Context &context, const char *message);

    bool mNoSource;
    const int mIdNumber;
    const char * const mName;
    std::shared_ptr<spdlog::logger> mLogger;
    std::atomic_bool mLevelEnabled[LogLevelNum];
    std::atomic_bool mMessageHandleUniqueOwnership;
    std::atomic<MessageHandlerWraper *> mMessageHandlerWraper{nullptr};

protected:
    CXXKIT_DEFINE_PPTR(Logger)
    CXXKIT_DECLARE_PUBLIC(Logger)
    CXXKIT_DISABLE_COPY_MOVE(LoggerPrivate)
};

CXXKIT_END_NAMESPACE