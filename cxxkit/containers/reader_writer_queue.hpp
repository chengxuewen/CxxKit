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

#include <cxxkit/base/global.hpp>

#include <cxxkit/3rdparty/readerwriterqueue/readerwriterqueue.h>
#include <readerwritercircularbuffer.h>

CXXKIT_BEGIN_NAMESPACE;

using moodycamel::ReaderWriterQueue;

/** @brief Lock-free SPSC bounded queue (moodycamel::ReaderWriterQueue). Bounded by capacity; try_enqueue returns false when full. See vendor docs for try_dequeue/dequeue/pop/consume API. */
using moodycamel::BlockingReaderWriterQueue;
/** @brief Lock-free SPSC bounded queue with blocking read (moodycamel::BlockingReaderWriterQueue). Blocks on dequeue when empty; try_dequeue remains non-blocking. */
using moodycamel::BlockingReaderWriterCircularBuffer;
/** @brief Lock-free SPSC bounded ring buffer (moodycamel::BlockingReaderWriterCircularBuffer). Fixed-capacity circular buffer; blocking consume API for bounded producer/consumer pipelines. */

CXXKIT_END_NAMESPACE