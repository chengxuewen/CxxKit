#include <cxxkit/thread/race_checker.hpp>

CXXKIT_BEGIN_NAMESPACE

RaceChecker::Scope::Scope(const RaceChecker* raceChecker)
#if CXXKIT_DCHECK_IS_ON
    : mRaceChecker(raceChecker), mRacecheckOk(raceChecker->acquire())
    #endif
{
}

RaceChecker::Scope::~Scope()
{
#if CXXKIT_DCHECK_IS_ON
    mRaceChecker->release();
#endif
}

bool RaceChecker::Scope::isDetected() const
{
#if CXXKIT_DCHECK_IS_ON
    return !mRacecheckOk;
#else
    return true;
#endif
}

bool RaceChecker::acquire() const
{
    const auto currentThreadId = PlatformThread::currentThreadId();
    // Set new accessing thread if this is a new use.
    const int currentAccessCount = mAccessCount;
    mAccessCount = mAccessCount + 1;
    if (currentAccessCount == 0)
    {
        mAccessingThreadId = currentThreadId;
    }
    // If this is being used concurrently this check will fail for the second thread entering
    // since it won't set the thread.
    // Recursive use of checked methods are OK since the accessing thread remains the same.
    const auto accessingThreadId = mAccessingThreadId;
    return accessingThreadId == currentThreadId;
}

void RaceChecker::release() const
{
    mAccessCount = mAccessCount - 1;
}

CXXKIT_END_NAMESPACE
