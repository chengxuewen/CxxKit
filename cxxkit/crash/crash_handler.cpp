#include <cxxkit/crash/crash_handler.hpp>

#include <cxxkit/tools/checks.hpp>

#include <cxxkit/crash/detail/crash_handler_p.hpp>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::atomic<bool> g_installedGuard(false);

bool isDumpFile(const char *name)
{
    const size_t len = std::strlen(name);
    return len > 4 && std::strcmp(name + len - 4, ".dmp") == 0;
}

void printStackTraceFromUcontext(void *ucontext)
{
#if defined(CXXKIT_OS_LINUX)
    backward::StackTrace stackTrace;
    stackTrace.load_from(nullptr, 32, ucontext); // backward API: (void* addr, size_t depth, void* ucontext)
    backward::Printer printer;
    printer.object = true;
    printer.color_mode = backward::ColorMode::always;
    printer.address = true;
    printer.print(stackTrace, stderr);
#else
    (void)ucontext; // macOS: crashed-thread ucontext not exposed by the breakpad callback — skip (documented)
    std::fputs("[cxxkit::crash] stack trace unavailable on this platform in dump callback\n", stderr);
#endif
}

} // namespace

CXXKIT_BEGIN_NAMESPACE

CrashHandler::CrashHandler() : mDPtr(new CrashHandlerPrivate)
{
}

CrashHandler::~CrashHandler()
{
    uninstall();
}

CrashHandler &CrashHandler::instance()
{
    // C++11 function-local static: thread-safe, zero deps, lives for the process lifetime.
    // (Singleton<CrashHandler, true> was abandoned: its template had never been instantiated — C++11
    // atomic copy-init, missing CXXKIT_ASSERT include, and private-dtor-vs-unique_ptr all break.)
    static CrashHandler instance;
    return instance;
}

bool CrashHandler::isHandlerInstalled() const
{
    return g_installedGuard.load();
}

bool CrashHandler::install()
{
    if (g_installedGuard.exchange(true)) {
        // Binary-level mutual exclusion contract: a second breakpad handler would double-write
        // minidumps (QExt::Breakpad coexistence is not supported — crash ON ⇒ QExt::Breakpad OFF).
        std::fputs("[cxxkit::crash] install() rejected: a crash handler is already installed "
                   "(CrashHandler or QExt::Breakpad coexistence detected).\n",
                   stderr);
        return false;
    }
    CXXKIT_CHECK(!mDPtr->mDumpPath.empty()) << "CrashHandler::install() requires setDumpPath() first.";

#if defined(CXXKIT_OS_MACOS)
    mDPtr->mHandler.reset(new google_breakpad::ExceptionHandler(
        mDPtr->mDumpPath, nullptr, &CrashHandlerPrivate::onMinidump, this, true, NULL));
#elif defined(CXXKIT_OS_LINUX)
    mDPtr->mHandler.reset(new google_breakpad::ExceptionHandler(
        google_breakpad::MinidumpDescriptor(mDPtr->mDumpPath),
        nullptr, &CrashHandlerPrivate::onMinidump, this, true, -1));
#else
#    error "cxxkit::crash supports macOS and Linux only (Windows needs a crash-deps-x64-windows export first)."
#endif
    CXXKIT_CHECK(mDPtr->mHandler) << "CrashHandler::install() failed to create the breakpad handler.";
    return true;
}

void CrashHandler::uninstall()
{
    if (mDPtr->mHandler) {
        mDPtr->mHandler.reset(); // ExceptionHandler dtor uninstalls the signal handlers
    }
    g_installedGuard.store(false);
}

bool CrashHandler::setDumpPath(const char *path)
{
    CXXKIT_CHECK(!isHandlerInstalled()) << "CrashHandler::setDumpPath() must be called before install().";
    if (!path || !path[0]) {
        return false;
    }
    struct stat st;
    if (::stat(path, &st) != 0) {
        if (::mkdir(path, 0755) != 0) {
            return false;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        return false;
    }
    mDPtr->mDumpPath = path;
    return true;
}

bool CrashHandler::setStackTraceOnCrash(bool enable)
{
    CXXKIT_CHECK(!isHandlerInstalled()) << "CrashHandler::setStackTraceOnCrash() must be called before install().";
    mDPtr->mStackTraceOnCrash = enable;
    return true;
}

bool CrashHandler::setCallback(CrashCallback callback, void *context)
{
    CXXKIT_CHECK(!isHandlerInstalled()) << "CrashHandler::setCallback() must be called before install().";
    mDPtr->mCallback = callback;
    mDPtr->mCallbackContext = context;
    return true;
}

bool CrashHandler::writeMinidump()
{
    if (!mDPtr->mHandler) {
        return false; // not installed — manual dump without a handler is unsupported in v1
    }
    return mDPtr->mHandler->WriteMinidump();
}

std::vector<std::string> CrashHandler::dumpFileList() const
{
    std::vector<std::string> result;
    DIR *dir = ::opendir(mDPtr->mDumpPath.c_str());
    if (!dir) {
        return result;
    }
    struct dirent *entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        if (isDumpFile(entry->d_name)) {
            result.push_back(entry->d_name);
        }
    }
    ::closedir(dir);
    return result;
}

void CrashHandler::clearDumps()
{
    const std::vector<std::string> files = dumpFileList();
    for (std::vector<std::string>::const_iterator it = files.begin(); it != files.end(); ++it) {
        const std::string full = mDPtr->mDumpPath + "/" + *it;
        ::unlink(full.c_str());
    }
}

CXXKIT_END_NAMESPACE

// ---- CrashHandlerPrivate (friend of CrashHandler — reaches the private pimpl) -------------------------------
#if defined(CXXKIT_OS_MACOS)
bool cxxkit::CrashHandlerPrivate::onMinidump(const char *dumpPath, const char *minidumpId, void *context,
                                             bool succeeded)
{
    (void)minidumpId;
    CrashHandler *self = static_cast<CrashHandler *>(context);
    const char *path = dumpPath;
    void *ucontext = nullptr;
#else
bool cxxkit::CrashHandlerPrivate::onMinidump(const google_breakpad::MinidumpDescriptor &descriptor, void *context,
                                             bool succeeded)
{
    CrashHandler *self = static_cast<CrashHandler *>(context);
    const char *path = descriptor.path();
    // breakpad passes the callback's context argument = the ucontext_t* of the CRASHED thread.
    void *ucontext = context;
#endif
    CXXKIT_ASSERT(self);

    if (succeeded && self->isHandlerInstalled()) {
        CrashHandlerPrivate *d = self->dFunc();
        if (d->mStackTraceOnCrash) {
            printStackTraceFromUcontext(ucontext);
        }
        if (d->mCallback) {
            d->mCallback(path, d->mCallbackContext, succeeded);
        }
    }
    return succeeded;
}
