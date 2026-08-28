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
#include <cxxkit/crash/crash_handler.hpp>

#include <memory>
#include <string>

// Third-party types live ONLY here (pimpl isolation): public headers never expose them.
#if defined(CXXKIT_OS_MACOS)
#    include <client/mac/handler/exception_handler.h>
#elif defined(CXXKIT_OS_LINUX)
#    include <client/linux/handler/exception_handler.h>
#endif

// backward.hpp pulls the stack-unwinding backend via BACKWARD_HAS_* definitions carried by the
// Backward::Interface target's INTERFACE_COMPILE_DEFINITIONS (vcpkg bakes them at port build time).
#include <backward.hpp>

CXXKIT_BEGIN_NAMESPACE

struct CrashHandlerPrivate
{
    // breakpad MinidumpCallback, platform-specific signature. A static member of the friend struct so it
    // can reach CrashHandler's private pimpl (free functions cannot).
#if defined(CXXKIT_OS_MACOS)
    static bool on_minidump(const char *dumpPath, const char *minidumpId, void *context, bool succeeded);
#else
    static bool on_minidump(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool succeeded);
#endif

    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler; // armed by install(), released by uninstall()
    std::string mDumpPath;
    CrashHandler::CrashCallback mCallback = nullptr;
    void *mCallbackContext = nullptr;
    bool mStackTraceOnCrash = false;
};

CXXKIT_END_NAMESPACE
