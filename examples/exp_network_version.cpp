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

// exp_network: cpr-backed HTTP — failure-path demo (connection refused); no real endpoints (CI-safe).

#include <iostream>

#include <cxxkit/network/http.hpp>

using namespace cxxkit;

int main()
{
    std::cout << "exp_network_version start!" << std::endl;

    // 1. Request construction (no send): Session stages url, query parameters,
    //    headers and timeouts; a send happens only when get()/post() is called.
    http::Session session;
    session.set_url(http::Url{"http://127.0.0.1:1/"});
    http::Parameters parameters{http::Parameter("source", "cxxkit-example")};
    session.set_parameters(parameters);
    session.set_header(http::Header{{"Accept", "application/json"}});
    session.set_timeout(http::Timeout(1500));
    session.set_connect_timeout(http::ConnectTimeout(1000));
    std::cout << "session staged (not sent): url=http://127.0.0.1:1/ params=1"
                 " headers=1 timeout=1500ms connect_timeout=1000ms"
              << std::endl;

    // 2. Failure-path demo: port 1 has no listener, so the connection is refused
    //    deterministically on localhost (no external network, CI-safe). A failed
    //    transfer yields status_code() == 0 with empty text()/reason(); branch on
    //    that before treating a Response as a real HTTP reply.
    http::Response::SharedPtr response = http::get(http::Url{"http://127.0.0.1:1/"}, http::Timeout(1500));
    if (0 == response->status_code())
    {
        std::cout << "expected failure: status_code=0 (transport error / connection refused),"
                     " text=\""
                  << response->text() << "\", reason=\"" << response->reason()
                  << "\" — handle this path before reading the body" << std::endl;
    }
    else
    {
        std::cout << "unexpected success: status_code=" << response->status_code() << " reason=" << response->reason()
                  << std::endl;
    }

    std::cout << "exp_network_version end!" << std::endl;
    return 0;
}
