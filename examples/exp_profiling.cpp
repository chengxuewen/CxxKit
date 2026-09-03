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

// exp_profiling: CXXKIT_PROFILE_* zone markers; observe with Tracy GUI when TRACY=ON.
#include <cxxkit/profiling/profiling.hpp>

#include <iostream>

namespace
{

// TRACY=OFF: CXXKIT_PROFILE_* expand to ((void)0) -- zero overhead, no output.
void computeChecksum()
{
    CXXKIT_PROFILE_SCOPE("computeChecksum");
    unsigned int checksum = 0;
    for (int i = 0; i < 10000; ++i)
    {
        checksum = checksum * 31u + static_cast<unsigned int>(i);
    }
    std::cout << "computeChecksum: " << checksum << std::endl;
}

void buildPayload()
{
    CXXKIT_PROFILE_SCOPE("buildPayload");
    CXXKIT_PROFILE_SCOPE("buildPayload.loop");
    unsigned long long payload = 0;
    for (int i = 0; i < 10000; ++i)
    {
        payload += static_cast<unsigned long long>(i) * 7ull;
    }
    std::cout << "buildPayload: " << payload << std::endl;
}

} // namespace

int main()
{
    {
        CXXKIT_PROFILE_SCOPE("main");
        computeChecksum();
        buildPayload();
    }
    CXXKIT_PROFILE_FRAME();
    std::cout << "This build has CXXKIT_ENABLE_LIB_TRACY=OFF: the CXXKIT_PROFILE_* markers above are" << std::endl;
    std::cout << "compiled to no-ops (zero overhead). Rebuild with -DCXXKIT_ENABLE_LIB_TRACY=ON, then connect"
              << std::endl;
    std::cout << "the Tracy GUI (https://github.com/wolfpld/tracy) to observe 4 zones:" << std::endl;
    std::cout << "  main, computeChecksum, buildPayload, buildPayload.loop + 1 frame mark." << std::endl;
    return 0;
}
