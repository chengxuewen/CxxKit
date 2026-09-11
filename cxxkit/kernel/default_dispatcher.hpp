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
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/kernel/kernel_global.hpp>

#include <cxxkit/kernel/abstract_event_dispatcher.hpp>

#include <memory>

#if CXXKIT_FEATURE_ENABLE_KERNEL

CXXKIT_BEGIN_NAMESPACE

/**
 * @brief Creates the default event-loop dispatcher (the engine behind @c EventLoop's default constructor).
 *
 * QtCore model (D38): the kernel carries a default loop engine (vendored libuv) but exposes no engine
 * vocabulary in its public API — the concrete @c UvEventDispatcher type stays private in
 * @c cxxkit/kernel/uv/detail/. Built when @c CXXKIT_ENABLE_LOOP_BACKEND_UV is ON (the default).
 *
 * With the backend switch OFF this is fatal by design: inject a dispatcher explicitly via
 * @c EventLoop(std::unique_ptr<AbstractEventDispatcher>) or enable the backend.
 *
 * The calling thread becomes the dispatcher's loop thread.
 * @since 0.2
 */
CXXKIT_KERNEL_API std::unique_ptr<AbstractEventDispatcher> make_default_dispatcher();

CXXKIT_END_NAMESPACE

#endif // #if CXXKIT_FEATURE_ENABLE_KERNEL
