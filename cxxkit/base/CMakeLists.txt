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

cxxkit_configure_library_begin(CxxKitBase)

# Generate core_config.hpp into the build tree
cxxkit_generate_core_config()

cxxkit_add_library(cxxkit_base
    INTERFACE
    EXPORT_NAME base
    FOLDER "CxxKit/libs")
# cxxkit/3rdparty/ 统一 include 根（三方头命名空间拷贝）
# Base 本身不消费三方头；各消费子库经 cxxkit_install_public_wrap_headers
# 把三方头拷到 build 树并烤进自己的 target。Base 保留 BUILD_INTERFACE 根，
# 供仅 include base 的 header-only 消费者可见拷贝的三方头。
target_include_directories(cxxkit_base INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

cxxkit_generate_pkg_config(cxxkit_base base)
cxxkit_configure_library_end(CxxKitBase)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_base EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/base/
        DESTINATION include/cxxkit/base
        FILES_MATCHING PATTERN "*.hpp")
    install(FILES ${PROJECT_BINARY_DIR}/cxxkit/base/core_config.hpp
        DESTINATION include/cxxkit/base)
endif()
