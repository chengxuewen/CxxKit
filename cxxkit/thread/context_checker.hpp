/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present ChengXueWen.
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

#include <cxxkit/thread/thread_global.hpp>

#include <cxxkit/tools/checks.hpp>

#include <string>

CXXKIT_BEGIN_NAMESPACE

class TaskQueueBase;
class ContextCheckerPrivate;
class CXXKIT_THREAD_API ContextChecker
{
public:
    enum class InitialState : bool
    {
        kDetached = false,
        kAttached = true
    };

    explicit ContextChecker(InitialState initialState = InitialState::kAttached);
    explicit ContextChecker(TaskQueueBase *attachedTaskQueue);
    virtual ~ContextChecker();

    static std::string expectationToString(const ContextChecker *checker);

    /**
     * Returns true if sequence checker is attached to the current sequence.
     */
    bool isCurrent() const;

    /**
     * Detaches checker from sequence to which it is attached. Next attempt to do a check with this
     * checker will result in attaching this checker to the sequence on which check was performed.
     */
    void detach();

protected:
    CXXKIT_DEFINE_DPTR(ContextChecker)
    CXXKIT_DECLARE_PRIVATE(ContextChecker)
    CXXKIT_DISABLE_COPY_MOVE(ContextChecker)
};

CXXKIT_END_NAMESPACE

/**
 * Document if a function expected to be called from same thread/task queue.
 */
#define CXXKIT_RUN_ON(...) CXXKIT_ATTRIBUTE_EXCLUSIVE_LOCKS_REQUIRED(__VA_ARGS__)

/**
 * Checks current code is running on the desired sequence.
 * First statement validates it is running on the sequence `x`.
 * Second statement annotates for the thread safety analyzer the check was done.
 * Such annotation has to be attached to a function, and that function has to be called.
 * Thus current implementation creates a noop lambda and calls it.
 * @param x
 */
#define CXXKIT_DCHECK_RUN_ON(x)                                                                                        \
    CXXKIT_DCHECK((x)->isCurrent()) << "\n" << ContextChecker::expectationToString(x);                                 \
    []() CXXKIT_ATTRIBUTE_ASSERT_EXCLUSIVE_LOCK(x) {}()
