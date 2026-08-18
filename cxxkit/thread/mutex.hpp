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

#pragma once

#include "cxxkit/base/global.hpp"
#include "cxxkit/tools/assert.hpp"

#include <mutex>
#include <shared_mutex>
#include <condition_variable>

CXXKIT_BEGIN_NAMESPACE

class Mutex : public std::mutex
{
public:
    using Base = std::mutex;
    using Lock = std::lock_guard<Base>;
    using UniqueLock = std::unique_lock<Base>;
    using Condition = std::condition_variable;

    using Base::Base;
    Mutex() = default;
    ~Mutex() = default;
};

class RecursiveMutex : public std::recursive_mutex
{
public:
    using Base = std::recursive_mutex;
    using Lock = std::lock_guard<Base>;
    using UniqueLock = std::unique_lock<Base>;
    using Condition = std::condition_variable_any;

    using Base::Base;
    RecursiveMutex() = default;
    ~RecursiveMutex() = default;
};

CXXKIT_END_NAMESPACE
