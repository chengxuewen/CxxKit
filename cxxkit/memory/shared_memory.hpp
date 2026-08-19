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

#pragma once

#include <cxxkit/base/global.hpp>

#include <memory>

CXXKIT_BEGIN_NAMESPACE

// SharedMemory is a base class for shared memory. It stores all required
// parameters of the buffer, but doesn't have any logic to allocate or destroy
// the actual buffer. DesktopCapturer consumers that need to use shared memory
// for video frames must extend this class with creation and destruction logic
// specific for the target platform and then call
// DesktopCapturer::SetSharedMemoryFactory().
class CXXKIT_CORE_API SharedMemory
{
public:
#if defined(CXXKIT_OS_WINDOWS)
    typedef void *Handle;
    static const Handle kInvalidHandle;
#else
    typedef int Handle;
    static const Handle kInvalidHandle;
#endif

    void *data() const { return mData; }
    size_t size() const { return mSize; }

    // Platform-specific handle of the buffer.
    Handle handle() const { return mHandle; }

    // Integer identifier that can be used used by consumers of DesktopCapturer
    // interface to identify shared memory buffers it created.
    int id() const { return mId; }

    virtual ~SharedMemory() { }

    SharedMemory(const SharedMemory &) = delete;
    SharedMemory &operator=(const SharedMemory &) = delete;

protected:
    SharedMemory(void *data, size_t size, Handle handle, int id);

    const int mId;
    void *const mData;
    const size_t mSize;
    const Handle mHandle;
};

// Interface used to create SharedMemory instances.
class SharedMemoryFactory
{
public:
    SharedMemoryFactory() { }
    virtual ~SharedMemoryFactory() { }

    SharedMemoryFactory(const SharedMemoryFactory &) = delete;
    SharedMemoryFactory &operator=(const SharedMemoryFactory &) = delete;

    virtual std::unique_ptr<SharedMemory> CreateSharedMemory(size_t size) = 0;
};
CXXKIT_END_NAMESPACE

