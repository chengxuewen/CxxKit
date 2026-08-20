// cxxkit::crash tests — minidump generation, install guard, manual dump, dump listing/cleanup, stack trace opt-in.
// The crash test spawns the standalone crash_fixture executable (never gtest death tests — breakpad intercepts
// the very signals death tests rely on) and polls for the .dmp file with a timeout.

#include <cxxkit/crash/crash_handler.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

std::string makeTempDir()
{
    const std::string tmpl = "/tmp/cxxkit_crash_test_XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    char *created = ::mkdtemp(&buf[0]);
    if (!created) {
        return "";
    }
    return std::string(created);
}

bool fileExists(const std::string &path)
{
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

// Spawns the fixture; returns the waitpid status (WIFSIGNALED true when SIGSEGV killed it).
int runFixture(const std::string &dumpDir, const std::string &fixturePath, bool stacktrace)
{
    const pid_t pid = ::fork();
    if (pid == 0) {
#if defined(TEST_ELFUTILS_LIB_DIR)
        ::setenv("LD_LIBRARY_PATH", TEST_ELFUTILS_LIB_DIR, 1); // backward dlopen("libdw.so.1")
#endif
        if (stacktrace) {
            ::execl(fixturePath.c_str(), "crash_fixture", dumpDir.c_str(), "--stacktrace", nullptr);
        } else {
            ::execl(fixturePath.c_str(), "crash_fixture", dumpDir.c_str(), nullptr);
        }
        _exit(127); // execl failed
    }
    if (pid < 0) {
        return -1;
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    return status;
}

// Polls dir for a *.dmp file (breakpad writes asynchronously); returns the filename or "".
std::string waitForDump(const std::string &dir, int timeoutSeconds)
{
    const std::time_t deadline = std::time(nullptr) + timeoutSeconds;
    while (std::time(nullptr) < deadline) {
        DIR *d = ::opendir(dir.c_str());
        if (d) {
            struct dirent *e = nullptr;
            while ((e = ::readdir(d)) != nullptr) {
                const size_t len = std::strlen(e->d_name);
                if (len > 4 && std::strcmp(e->d_name + len - 4, ".dmp") == 0) {
                    std::string found(e->d_name);
                    ::closedir(d);
                    return found;
                }
            }
            ::closedir(d);
        }
        ::usleep(100 * 1000);
    }
    return "";
}

} // namespace

TEST(CrashHandler, InstallGuard)
{
    cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();
    handler.uninstall(); // clean state

    const std::string dir = makeTempDir();
    ASSERT_FALSE(dir.empty());
    ASSERT_TRUE(handler.setDumpPath(dir.c_str()));
    ASSERT_TRUE(handler.install());
    EXPECT_TRUE(handler.isHandlerInstalled());
    EXPECT_FALSE(handler.install()); // mutual exclusion contract: second install refused
    handler.uninstall();
    EXPECT_FALSE(handler.isHandlerInstalled());
    ::rmdir(dir.c_str());
}

TEST(CrashHandler, ManualMinidump)
{
    cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();
    handler.uninstall();

    const std::string dir = makeTempDir();
    ASSERT_FALSE(dir.empty());
    ASSERT_TRUE(handler.setDumpPath(dir.c_str()));
    ASSERT_TRUE(handler.install());
    EXPECT_TRUE(handler.writeMinidump());

    const std::vector<std::string> dumps = handler.dumpFileList();
    EXPECT_FALSE(dumps.empty());
    handler.clearDumps();
    EXPECT_TRUE(handler.dumpFileList().empty());
    handler.uninstall();
    ::rmdir(dir.c_str());
}

TEST(CrashHandler, CrashProducesMinidump)
{
    cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();
    handler.uninstall();

    // The fixture installs its own handler in a separate process — the parent must NOT install one,
    // otherwise both would write dumps into the same directory and the test would be ambiguous.
    const std::string dir = makeTempDir();
    ASSERT_FALSE(dir.empty());

    const std::string fixturePath = std::string(TEST_BINARY_DIR) + "/crash_fixture";
    const int status = runFixture(dir, fixturePath, false);
    ASSERT_TRUE(WIFSIGNALED(status)); // SIGSEGV killed it

    const std::string dumpFile = waitForDump(dir, 10);
    EXPECT_FALSE(dumpFile.empty()) << "no .dmp appeared in " << dir;
    if (!dumpFile.empty()) {
        const std::string full = dir + "/" + dumpFile;
        EXPECT_TRUE(fileExists(full));
        struct stat st;
        EXPECT_TRUE(::stat(full.c_str(), &st) == 0);
        EXPECT_GT(static_cast<long>(st.st_size), 0);
        ::unlink(full.c_str());
    }
    ::rmdir(dir.c_str());
}

TEST(CrashHandler, StackTraceOptIn)
{
    cxxkit::CrashHandler &handler = cxxkit::CrashHandler::instance();
    handler.uninstall();

    const std::string dir = makeTempDir();
    ASSERT_FALSE(dir.empty());
    const std::string fixturePath = std::string(TEST_BINARY_DIR) + "/crash_fixture";

    // Capture the fixture's stderr through a pipe: with --stacktrace the dump callback prints the
    // crashed thread's trace via backward-cpp (best-effort, addresses present when a backend resolves).
    int pipefd[2];
    ASSERT_EQ(::pipe(pipefd), 0);
    const pid_t pid = ::fork();
    ASSERT_GE(pid, 0);
    if (pid == 0) {
#if defined(TEST_ELFUTILS_LIB_DIR)
        ::setenv("LD_LIBRARY_PATH", TEST_ELFUTILS_LIB_DIR, 1); // backward dlopen("libdw.so.1")
#endif
        ::dup2(pipefd[1], STDERR_FILENO);
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        ::execl(fixturePath.c_str(), "crash_fixture", dir.c_str(), "--stacktrace", nullptr);
        _exit(127);
    }
    ::close(pipefd[1]);
    std::string captured;
    char buf[512];
    ssize_t n = 0;
    while ((n = ::read(pipefd[0], buf, sizeof(buf))) > 0) {
        captured.append(buf, static_cast<size_t>(n));
    }
    ::close(pipefd[0]);
    int status = 0;
    ::waitpid(pid, &status, 0);

    EXPECT_TRUE(WIFSIGNALED(status));
    // Best-effort bar: opt-in trace printing must produce SOME output on the crash path.
    EXPECT_FALSE(captured.empty()) << "no stack trace output on stderr";
    ::rmdir(dir.c_str());
}
