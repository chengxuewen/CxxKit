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

#ifndef _CXXKIT_SOURCE_LOCATION_HPP
#define _CXXKIT_SOURCE_LOCATION_HPP

#include <cxxkit/base/global.hpp>

#include <string>

CXXKIT_BEGIN_NAMESPACE

struct SourceLocation
{
public:
    static constexpr SourceLocation current(const char *functionName = __builtin_FUNCTION(),
                                            const char *filePath = __builtin_FILE(),
                                            int lineNumber = __builtin_LINE()) noexcept
    {
        return SourceLocation(functionName, filePath, lineNumber);
    }
    constexpr SourceLocation(const char *functionName, const char *filePath, int lineNumber) noexcept
        : mFunctionName(functionName)
        , mFilePath(filePath)
        , mLineNumber(lineNumber)
    {
    }

    constexpr SourceLocation() noexcept = default;
    constexpr SourceLocation(const SourceLocation &other) noexcept = default;
    constexpr SourceLocation(SourceLocation &&other) noexcept = default;
    SourceLocation &operator=(const SourceLocation &other) noexcept = default;

    std::string fileLine() const noexcept { return std::string(this->fileName()) + ":" + std::to_string(mLineNumber); }
    std::string toString() const { return std::string(mFunctionName) + "@" + this->fileLine(); }
    constexpr const char *functionName() const noexcept { return mFunctionName; }
    const char *fileName() const noexcept { return CXXKIT_PATH_NAME(mFilePath); }
    constexpr const char *filePath() const noexcept { return mFilePath; }
    constexpr int lineNumber() const noexcept { return mLineNumber; }

private:
    const char *mFunctionName = nullptr;
    const char *mFilePath = nullptr;
    int mLineNumber = -1;
};
CXXKIT_END_NAMESPACE

// Define a macro to record the current source location.
#define CXXKIT_SOURCE_LOCATION_WITH_FUNCTION(function_name)                                                            \
    cxxkit::SourceLocation(function_name, CXXKIT_STRFILE, CXXKIT_LINE)
#define CXXKIT_SOURCE_LOCATION CXXKIT_SOURCE_LOCATION_WITH_FUNCTION(CXXKIT_STRFUNC)

#endif // _CXXKIT_SOURCE_LOCATION_HPP
