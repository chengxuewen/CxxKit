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

cxxkit_configure_library_begin(CxxKitTools)

cxxkit_add_library(cxxkit_tools
    SOURCES logging.cpp random.cpp assert.cpp clock.cpp status.cpp error.cpp once_flag.cpp id_registry.cpp shared_buffer.cpp metrics.cpp ntp_time.cpp fake_clock.cpp
    PUBLIC_LIBRARIES cxxkit::base cxxkit::text cxxkit::time CxxKitWrapOptional::WrapOptional CxxKitWrapExpected::WrapExpected CxxKitWrapVariant::WrapVariant CxxKitWrapFilesystem::WrapFilesystem CxxKitWrapSpdlog::WrapSpdlog
    EXPORT_NAME tools
    FOLDER "CxxKit/libs")
cxxkit_find_package(Optional PROVIDED_TARGETS CxxKitWrapOptional::WrapOptional)
cxxkit_install_public_wrap_headers(cxxkit_tools WRAPS CxxKitWrapOptional::WrapOptional|tl)
cxxkit_find_package(Expected PROVIDED_TARGETS CxxKitWrapExpected::WrapExpected)
cxxkit_install_public_wrap_headers(cxxkit_tools WRAPS CxxKitWrapExpected::WrapExpected|tl)
cxxkit_find_package(Variant PROVIDED_TARGETS CxxKitWrapVariant::WrapVariant)
cxxkit_install_public_wrap_headers(cxxkit_tools WRAPS CxxKitWrapVariant::WrapVariant|mpark)
cxxkit_find_package(Filesystem PROVIDED_TARGETS CxxKitWrapFilesystem::WrapFilesystem)
cxxkit_install_public_wrap_headers(cxxkit_tools WRAPS CxxKitWrapFilesystem::WrapFilesystem|ghc)
cxxkit_find_package(Spdlog PROVIDED_TARGETS CxxKitWrapSpdlog::WrapSpdlog)
cxxkit_install_public_wrap_headers(cxxkit_tools WRAPS CxxKitWrapSpdlog::WrapSpdlog|spdlog)
cxxkit_generate_pkg_config(cxxkit_tools tools)
cxxkit_configure_library_end(CxxKitTools)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_tools EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/tools/
        DESTINATION include/cxxkit/tools
        FILES_MATCHING PATTERN "*.hpp")
endif()
