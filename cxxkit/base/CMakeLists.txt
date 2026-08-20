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
# Vendored 3rdparty wraps (headers namespaced as cxxkit/3rdparty/<lib>/)
# This is the octk mechanism to avoid conflicts with system-installed libraries.
cxxkit_find_package(Fmt PROVIDED_TARGETS CxxKitWrapFmt::WrapFmt)
cxxkit_find_package(StringViewLite PROVIDED_TARGETS CxxKitWrapStringViewLite::WrapStringViewLite)
cxxkit_find_package(Optional PROVIDED_TARGETS CxxKitWrapOptional::WrapOptional)
cxxkit_find_package(Expected PROVIDED_TARGETS CxxKitWrapExpected::WrapExpected)
cxxkit_find_package(ConcurrentQueue PROVIDED_TARGETS CxxKitWrapConcurrentQueue::WrapConcurrentQueue)
cxxkit_find_package(ReaderWriterQueue PROVIDED_TARGETS CxxKitWrapReaderWriterQueue::WrapReaderWriterQueue)
cxxkit_find_package(Function2 PROVIDED_TARGETS CxxKitWrapFunction2::WrapFunction2)
cxxkit_find_package(Filesystem PROVIDED_TARGETS CxxKitWrapFilesystem::WrapFilesystem)
cxxkit_find_package(Spdlog PROVIDED_TARGETS CxxKitWrapSpdlog::WrapSpdlog)
cxxkit_find_package(Variant PROVIDED_TARGETS CxxKitWrapVariant::WrapVariant)
cxxkit_find_package(Benchmark PROVIDED_TARGETS CxxKitWrapBenchmark::WrapBenchmark)
cxxkit_install_public_wrap_headers(cxxkit_base
    WRAPS
    CxxKitWrapFmt::WrapFmt|fmt
    CxxKitWrapStringViewLite::WrapStringViewLite|nonstd
    CxxKitWrapOptional::WrapOptional|tl
    CxxKitWrapExpected::WrapExpected|tl
    CxxKitWrapConcurrentQueue::WrapConcurrentQueue|concurrentqueue
    CxxKitWrapReaderWriterQueue::WrapReaderWriterQueue|readerwriterqueue
    CxxKitWrapFunction2::WrapFunction2|function2
    CxxKitWrapFilesystem::WrapFilesystem|ghc
    CxxKitWrapSpdlog::WrapSpdlog|spdlog
    CxxKitWrapVariant::WrapVariant|mpark
    CxxKitWrapBenchmark::WrapBenchmark|benchmark)

# cxxkit/3rdparty/ 统一 include 根（三方头命名空间拷贝）
# Helper adds the source/build/install include trio; base additionally exposes the
# build tree's 3rdparty header copy root.
target_include_directories(cxxkit_base INTERFACE
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)
target_link_libraries(cxxkit_base INTERFACE
    CxxKitWrapFmt::WrapFmt
    CxxKitWrapStringViewLite::WrapStringViewLite
    CxxKitWrapOptional::WrapOptional
    CxxKitWrapExpected::WrapExpected
    CxxKitWrapConcurrentQueue::WrapConcurrentQueue
    CxxKitWrapReaderWriterQueue::WrapReaderWriterQueue
    CxxKitWrapFunction2::WrapFunction2
    CxxKitWrapFilesystem::WrapFilesystem
    CxxKitWrapSpdlog::WrapSpdlog
    CxxKitWrapVariant::WrapVariant
    CxxKitWrapBenchmark::WrapBenchmark)

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
