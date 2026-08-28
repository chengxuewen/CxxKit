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
    if (!handler.set_dump_path(argv[1])) {
        std::fprintf(stderr, "set_dump_path failed\n");
        return 3;
    }
    handler.set_stack_trace_on_crash(stacktrace);
    if (!handler.install()) {
        std::fprintf(stderr, "install failed\n");
        return 4;
    }
    std::raise(SIGSEGV);
    // Not reached: SIGSEGV kills the process (breakpad writes the minidump first).
    return 0;
}
