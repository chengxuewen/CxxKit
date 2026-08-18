/***********************************************************************************************************************
**
** Library: OpenCTK
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

#include <cxxkit/text/string_view.hpp>
#include <cxxkit/tools/filesystem.hpp>
#include <cxxkit/tools/random.hpp>

#include <string>

CXXKIT_BEGIN_NAMESPACE

namespace test
{

static std::string TempFilename(const std::string &path, const std::string &name)
{
    return path + "/" +  name + "XXXXXX";
}

static std::string OutputPath()
{
    return filesystem::current_path();
}

static std::string OutputPathWithRandomDirectory()
{
    std::string path = filesystem::current_path();
    std::string rand_dir = path + utils::CreateRandomUuid();
    CXXKIT_CHECK(filesystem::create_directory(rand_dir)) << "Failed to create dir: " << rand_dir;
    return rand_dir + CXXKIT_PATH_SLASH;
}

} // namespace test

CXXKIT_END_NAMESPACE