/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#include <cxxkit/kernel/object.hpp>
#include <cxxkit/tools/enum_flags.hpp>
#include <cxxkit/base/core_config.hpp>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

class EventLoopPrivate;
class CXXKIT_CORE_API EventLoop : public Object
{
public:
    enum class ProcessFlag
    {
        kAllEvents = 0x00,
        kExcludeUserInputEvents = 0x01,
        kExcludeSocketNotifiers = 0x02,
        kWaitForMoreEvents = 0x04,
        kX11ExcludeTimers = 0x08,
        kEventLoopExec = 0x20,
        kDialogExec = 0x40
    };
    CXXKIT_DECLARE_ENUM_FLAGS(ProcessFlags, ProcessFlag)

    explicit EventLoop(Object *parent = nullptr);
    ~EventLoop() override;

    bool processEvents(ProcessFlags flags = ProcessFlag::kAllEvents);
    void processEvents(ProcessFlags flags, int maximumTime);

    int exec(ProcessFlags flags = ProcessFlag::kAllEvents);
    void wakeUp();

    void exit(int retCode = 0);
    void quit();

    bool isRunning() const;

    bool event(Event *event) override;

private:
    CXXKIT_DECLARE_PRIVATE(EventLoop)
    CXXKIT_DISABLE_COPY_MOVE(EventLoop)
};

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL