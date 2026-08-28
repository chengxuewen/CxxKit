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

#include <cxxkit/thread/race_checker.hpp>

CXXKIT_BEGIN_NAMESPACE

RaceChecker::Scope::Scope(const RaceChecker *raceChecker)
#if CXXKIT_DCHECK_IS_ON
    : mRaceChecker(raceChecker)
    , mRacecheckOk(raceChecker->acquire())
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
