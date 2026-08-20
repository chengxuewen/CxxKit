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

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitNetwork)

cxxkit_find_package(Libcpr PROVIDED_TARGETS CxxKitWrapLibcpr::WrapLibcpr)

cxxkit_add_library(cxxkit_network
    STATIC
    NO_ALIAS
    SOURCES http.cpp
    PUBLIC_LIBRARIES cxxkit::base cxxkit::text cxxkit::thread cxxkit::memory cxxkit::tools CxxKitWrapLibcpr::WrapLibcpr
    EXPORT_NAME network
    FOLDER "CxxKit/libs")
add_library(cxxkit::network ALIAS cxxkit_network)

cxxkit_install_public_wrap_headers(cxxkit_network
    WRAPS
    CxxKitWrapLibcpr::WrapLibcpr|cpr
    CxxKitWrapLibcurl::WrapLibcurl|curl
    CxxKitWrapMbedTLS::WrapMbedTLS|mbedtls
)

cxxkit_generate_pkg_config(cxxkit_network network)
cxxkit_configure_library_end(CxxKitNetwork)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_network EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/network/
        DESTINATION include/cxxkit/network
        FILES_MATCHING PATTERN "*.hpp")
endif()
