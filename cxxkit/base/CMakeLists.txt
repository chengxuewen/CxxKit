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
# Base 本身不消费三方头，但它是唯一永远 ON 的子库，而 fmt/nonstd 头被
# tools/text 公共头传递引用（checks.hpp → logging.hpp → text/format.hpp → fmt），
# 进而被 containers/numerics/... 的头间接引用——TEXT/TOOLS 裁剪后仍需这些头在场。
# 故 base 负责在构建树 stage fmt+nonstd（stamp 幂等；text/tools 的同款 stage 叠加无害）。
target_include_directories(cxxkit_base INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)
cxxkit_find_package(Fmt PROVIDED_TARGETS CxxKitWrapFmt::WrapFmt)
cxxkit_install_public_wrap_headers(cxxkit_base WRAPS CxxKitWrapFmt::WrapFmt|fmt)
cxxkit_find_package(StringViewLite PROVIDED_TARGETS CxxKitWrapStringViewLite::WrapStringViewLite)
cxxkit_install_public_wrap_headers(cxxkit_base WRAPS CxxKitWrapStringViewLite::WrapStringViewLite|nonstd)
cxxkit_find_package(Optional PROVIDED_TARGETS CxxKitWrapOptional::WrapOptional)
cxxkit_install_public_wrap_headers(cxxkit_base WRAPS CxxKitWrapOptional::WrapOptional|tl)
cxxkit_find_package(Expected PROVIDED_TARGETS CxxKitWrapExpected::WrapExpected)
cxxkit_install_public_wrap_headers(cxxkit_base WRAPS CxxKitWrapExpected::WrapExpected|tl)
cxxkit_find_package(Variant PROVIDED_TARGETS CxxKitWrapVariant::WrapVariant)
cxxkit_install_public_wrap_headers(cxxkit_base WRAPS CxxKitWrapVariant::WrapVariant|mpark)
cxxkit_find_package(Filesystem PROVIDED_TARGETS CxxKitWrapFilesystem::WrapFilesystem)
cxxkit_install_public_wrap_headers(cxxkit_base WRAPS CxxKitWrapFilesystem::WrapFilesystem|ghc)

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
