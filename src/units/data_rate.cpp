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

#include <cxxkit/units/data_rate.hpp>
#include <cxxkit/containers/array_view.hpp>

#include <string>

CXXKIT_BEGIN_NAMESPACE

namespace utils
{
std::string toString(DataRate value)
{
    char buf[64];
    std::stringstream ss;
    if (value.IsPlusInfinity())
    {
        ss << "+inf bps";
    }
    else if (value.IsMinusInfinity())
    {
        ss << "-inf bps";
    }
    else
    {
        if (value.bps() == 0 || value.bps() % 1000 != 0)
        {
            ss << value.bps() << " bps";
        }
        else
        {
            ss << value.kbps() << " kbps";
        }
    }
    return ss.str();
}
} // namespace utils

CXXKIT_END_NAMESPACE
