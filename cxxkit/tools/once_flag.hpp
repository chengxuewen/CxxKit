/***********************************************************************************************************************
**
** Library: cxxkit
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

#ifndef _CXXKIT_ONCE_FLAG_HPP
#define _CXXKIT_ONCE_FLAG_HPP

#include <cxxkit/base/global.hpp>

#include <atomic>
#include <thread>
#include <functional>

CXXKIT_BEGIN_NAMESPACE

class OnceFlag
{
public:
    enum class State
    {
        NeverCalled = 0,
        InProcess,
        Done
    };

    OnceFlag() { }
    virtual ~OnceFlag() { }

    template <typename Func>
    bool call(Func func)
    {
        if (this->enter())
        {
            func();
            this->leave();
            return true;
        }
        return false;
    }

    bool enter()
    {
        auto state = State::NeverCalled;
        if (mState.compare_exchange_strong(state, State::InProcess))
        {
            return true;
        }
        while (State::Done != mState.load())
        {
            std::this_thread::yield();
        }
        return false;
    }

    void leave() { mState.exchange(State::Done); }

    bool isDone() const { return State::Done == mState.load(); }
    bool isInProcess() const { return State::InProcess == mState.load(); }
    bool isNeverCalled() const { return State::NeverCalled == mState.load(); }

    static OnceFlag *localOnceFlag();

protected:
    std::atomic<State> mState{State::NeverCalled};
    CXXKIT_DISABLE_COPY_MOVE(OnceFlag)
};

// TODO::del
class MutableOnceFlag final : public OnceFlag
{
public:
    MutableOnceFlag() = default;
    ~MutableOnceFlag() = default;

    bool reset()
    {
        auto expected = State::Done;
        return mState.compare_exchange_strong(expected, State::NeverCalled);
    }
};


namespace utils
{
template <typename Func>
bool callOnce(OnceFlag &flag, Func func)
{
    return flag.call(std::move(func));
}
template <typename Func>
void callOncePerThread(Func func)
{
    callOnce(*OnceFlag::localOnceFlag(), func);
}
} // namespace utils

CXXKIT_END_NAMESPACE

#endif // _CXXKIT_ONCE_FLAG_HPP
