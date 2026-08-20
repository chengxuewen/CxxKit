#pragma once

#include <cxxkit/base/global.hpp>
#include <cxxkit/crash/crash_global.hpp>

#include <string>
#include <vector>

#if CXXKIT_FEATURE_ENABLE_CRASH

CXXKIT_BEGIN_NAMESPACE

class CrashHandlerPrivate;

/**
 * @brief Process-wide crash handler (Google breakpad backend + optional backward-cpp stack printing).
 *
 * Design contract (see docs/superpowers/plans/2026-08-19-cxxkit-crash-sublib.md):
 *  - Single instance per process (C++11 function-local static, thread-safe). install() refuses a second
 *    installation (atomic guard) — coexistence with QExt::Breakpad in the same process is NOT supported:
 *    crash ON ⇒ QExt::Breakpad OFF.
 *  - Config-then-install: setDumpPath()/setStackTraceOnCrash()/setCallback() are only valid BEFORE
 *    install(); calling them after install triggers CXXKIT_CHECK.
 *  - The dump callback runs on breakpad's dedicated handler thread (NOT in signal context). Default
 *    behavior is fast-return; setStackTraceOnCrash(true) additionally prints the CRASHED thread's
 *    stack (load_from ucontext) to stderr — best-effort, backward-cpp is not async-signal-safe.
 *  - v1 has NO upload: dump files stay on disk for offline symbolication (minidump_stackwalk).
 */
class CXXKIT_CRASH_API CrashHandler
{
public:
    /** User callback invoked after a minidump is written (handler thread, keep it fast). */
    using CrashCallback = bool (*)(const char *dumpPath, void *context, bool succeeded);

    static CrashHandler &instance();

    /** Public so the function-local static can be destroyed at process exit (it is not in practice). */
    ~CrashHandler();

    /** True once install() succeeded and the handler is armed. */
    bool isHandlerInstalled() const;

    /** Arms the breakpad handler. Returns false (and keeps the process untouched) if already installed. */
    bool install();

    /** Disarms the handler (removes the signal handlers, keeps dump files). */
    void uninstall();

    /** Sets the minidump output directory. Must be called before install(). */
    bool setDumpPath(const char *path);

    /**
     * Enables printing the crashed thread's stack to stderr from the dump callback (default: off).
     * Best-effort only: backward-cpp is not async-signal-safe; on macOS the crashed-thread ucontext
     * is not available and the trace is skipped.
     */
    bool setStackTraceOnCrash(bool enable);

    /** Sets a user callback invoked after the minidump is written. Must be called before install(). */
    bool setCallback(CrashCallback callback, void *context);

    /** Manually writes a minidump (non-crash path, e.g. a user fatal hook). No handler install required. */
    bool writeMinidump();

    /** Lists *.dmp files in the dump path. */
    std::vector<std::string> dumpFileList() const;

    /** Deletes *.dmp files in the dump path. */
    void clearDumps();

    CXXKIT_DECLARE_PRIVATE(CrashHandler)
    CXXKIT_DISABLE_COPY_MOVE(CrashHandler)

private:
    CrashHandler();

    CXXKIT_DEFINE_DPTR(CrashHandler)
};

CXXKIT_END_NAMESPACE

#endif // CXXKIT_FEATURE_ENABLE_CRASH