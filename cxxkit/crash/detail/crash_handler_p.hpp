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
    static bool onMinidump(const char *dumpPath, const char *minidumpId, void *context, bool succeeded);
#else
    static bool onMinidump(const google_breakpad::MinidumpDescriptor &descriptor, void *context, bool succeeded);
#endif

    std::unique_ptr<google_breakpad::ExceptionHandler> mHandler; // armed by install(), released by uninstall()
    std::string mDumpPath;
    CrashHandler::CrashCallback mCallback = nullptr;
    void *mCallbackContext = nullptr;
    bool mStackTraceOnCrash = false;
};

CXXKIT_END_NAMESPACE
