########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2026~Present ChengXueWen.
#
# License: MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
# to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# cxxkit::uv — event-loop sublibrary over vendored libuv (opt-in).
# B1 placeholder: INTERFACE shell so the target + wiring exist; UvEventDispatcher
# implementation (sources, tests, pkg-config, install four-piece) lands in B2.

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitUv)

cxxkit_find_package(Libuv PROVIDED_TARGETS CxxKitWrapLibuv::WrapLibuv)
cxxkit_add_library(cxxkit_uv
    INTERFACE
    EXPORT_NAME uv
    FOLDER "CxxKit/libs")
target_link_libraries(cxxkit_uv INTERFACE cxxkit::base CxxKitWrapLibuv::WrapLibuv)
cxxkit_install_public_wrap_headers(cxxkit_uv WRAPS CxxKitWrapLibuv::WrapLibuv|libuv)
cxxkit_configure_library_end(CxxKitUv)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_uv EXPORT cxxkitTargets)
endif()
