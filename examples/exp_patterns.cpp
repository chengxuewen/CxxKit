/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2026~Present chengxuewen.
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

// exp_patterns: Singleton / AutoSingleton lifecycle walkthrough.

#include <iostream>

#include <cxxkit/patterns/singleton.hpp>

using namespace cxxkit;

//! Process-wide configuration: inherits AutoSingleton, lazily built on first instance() call.
class AppConfig : public AutoSingleton<AppConfig>
{
public:
    int magic_number() const { return 42; }

protected:
    AppConfig() = default;

    CXXKIT_DECLARE_SINGLETON(AppConfig)
};

//! Prints on construction so the lazy build moment is visible in the output.
class LazyService : public AutoSingleton<LazyService>
{
public:
    int answer() const { return 7; }

protected:
    LazyService() { std::cout << "LazyService constructed" << std::endl; }

    CXXKIT_DECLARE_SINGLETON(LazyService)
};

int main()
{
    // [1] Basic singleton: every access hands back the same instance.
    std::cout << "[1] basic singleton" << std::endl;
    AppConfig &first = AppConfig::instance();
    AppConfig &second = AppConfig::instance();
    std::cout << "same instance: " << std::boolalpha << (&first == &second) << std::endl;
    std::cout << "magic_number: " << first.magic_number() << std::endl;

    // [2] AutoSingleton: constructed on first access, never again.
    std::cout << "[2] lazy construction" << std::endl;
    std::cout << "before first access" << std::endl;
    LazyService::instance(); //! First access builds the instance (prints once).
    std::cout << "answer: " << LazyService::instance().answer() << std::endl;
    std::cout << "after second access: " << LazyService::instance().answer() << std::endl;
    return 0;
}
