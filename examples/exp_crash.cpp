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

// exp_crash: safe-path crash handler setup — config, manual minidump; real-crash demos live in tst_crash.

#include <iostream>
#include <string>
#include <vector>

#include <cxxkit/crash/crash_handler.hpp>

int main()
{
    cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();

    // 1) Config-then-install: set_dump_path()/set_stack_trace_on_crash() are only valid BEFORE
    //    install(); install() returns false if a handler is already armed (mutual exclusion).
    const char *dump_dir = "/tmp/cxxkit_exp_dumps";
    std::cout << "set_dump_path(\"" << dump_dir << "\"): " << handler.set_dump_path(dump_dir) << std::endl;
    std::cout << "set_stack_trace_on_crash(true): " << handler.set_stack_trace_on_crash(true) << std::endl;
    std::cout << "install(): " << handler.install() << std::endl;
    std::cout << "is_handler_installed(): " << handler.is_handler_installed() << std::endl;
    std::cout << "install() again (refused by guard): " << handler.install() << std::endl;

    // 2) Manual minidump: safe, non-crash API — writes a .dmp and returns without crashing.
    handler.write_minidump();
    const std::vector<std::string> dumps = handler.dump_file_list();
    std::cout << "write_minidump(): ok, " << dumps.size() << " dump(s) in " << dump_dir << std::endl;
    for (size_t i = 0; i < dumps.size(); ++i)
    {
        std::cout << "  " << dumps[i] << std::endl;
    }

    // 3) Cleanup: uninstall() disarms the handler (dump files are kept on disk).
    handler.clear_dumps();
    handler.uninstall();
    std::cout << "uninstall(): " << handler.is_handler_installed() << " (false = disarmed)" << std::endl;
    std::cout << "note: real-crash demos live in tests/tst_crash.cpp (crash_fixture) — this example"
                 " never crashes on purpose."
              << std::endl;
    return 0;
}
