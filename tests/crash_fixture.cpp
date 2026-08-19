// crash fixture: standalone executable used by tst_crash.cpp to verify minidump generation on a real crash.
// Usage: crash_fixture <dump_path> [--stacktrace]
// Installs the CrashHandler, then raises SIGSEGV — the process dies and a .dmp must appear in dump_path.

#include <cxxkit/crash/crash_handler.hpp>

#include <csignal>
#include <cstdio>
#include <cstring>

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: crash_fixture <dump_path> [--stacktrace]\n");
        return 2;
    }
    const bool stacktrace = argc > 2 && std::strcmp(argv[2], "--stacktrace") == 0;

    cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();
    if (!handler.setDumpPath(argv[1])) {
        std::fprintf(stderr, "setDumpPath failed\n");
        return 3;
    }
    handler.setStackTraceOnCrash(stacktrace);
    if (!handler.install()) {
        std::fprintf(stderr, "install failed\n");
        return 4;
    }
    std::raise(SIGSEGV);
    // Not reached: SIGSEGV kills the process (breakpad writes the minidump first).
    return 0;
}
