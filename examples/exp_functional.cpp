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

// exp_functional: non-owning FunctionView and move-only UniqueFunction.

#include <iostream>
#include <memory>
#include <utility>

#include <cxxkit/functional/function_view.hpp>
#include <cxxkit/functional/unique_function.hpp>

using namespace cxxkit;

namespace
{
/// Plain function used to show FunctionView also binds function pointers.
int double_it(int i)
{
    return i * 2;
}

/// Calls back through a non-owning view -- no copy of the callable is made.
void register_callback(FunctionView<int(int)> callback, int value)
{
    std::cout << "  register_callback -> " << callback(value) << std::endl;
}
} // namespace

int main()
{
    // --- 1) FunctionView: zero-copy callback views -------------------------------
    // FunctionView owns nothing (two pointers, trivially copyable), so lambdas and
    // function pointers bind implicitly -- no allocation, no copy of the callable.
    int multiplier = 10;
    std::cout << "[1] FunctionView" << std::endl;
    register_callback([&multiplier](int i) { return multiplier * i; }, 4); // lambda
    register_callback([](int i) { return i + 1; }, 41);                    // stateless lambda
    register_callback(double_it, 20);                                      // function pointer

    // --- 2) UniqueFunction: move-only callable container --------------------------
    // A lambda capturing a unique_ptr is not copyable, so it fits UniqueFunction
    // (move-only storage) but could never be stored in std::function.
    // C++11: no init-capture -- define the pointer first, then move-capture it.
    std::unique_ptr<int> data(new int(123));
    std::unique_ptr<int> owned = std::move(data); // transfer ownership first
    UniqueFunction<int()> reader = [&owned]() { return *owned; };
    std::cout << "[2] UniqueFunction -> " << reader() << std::endl;

    // Move-only: transferring the function itself compiles; copying it would not.
    UniqueFunction<int()> moved_reader = std::move(reader);
    std::cout << "    after move -> " << moved_reader() << std::endl;

    // --- 3) FunctionView vs std::function -----------------------------------------
    // FunctionView: hot-path, non-owning parameter (callback outlives the call);
    // std::function: owning storage; UniqueFunction: owning + move-only (unique_ptr
    // captures, exclusive ownership transfer).

    return 0;
}
