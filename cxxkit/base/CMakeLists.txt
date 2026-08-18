########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit::base — header-only foundation sublibrary (macros, types, compiler detection).
# Also the hub for vendored 3rdparty header namespacing (cxxkit/3rdparty/<lib>/).
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitBase)

# Generate core_config.hpp into the build tree
cxxkit_generate_core_config()

add_library(cxxkit_base INTERFACE)
add_library(cxxkit::base ALIAS cxxkit_base)
set_property(TARGET cxxkit_base PROPERTY EXPORT_NAME base)

# Vendored 3rdparty wraps (headers namespaced as cxxkit/3rdparty/<lib>/)
# This is the octk mechanism to avoid conflicts with system-installed libraries.
cxxkit_find_package(Fmt PROVIDED_TARGETS CXXKitWrapFmt::WrapFmt)
cxxkit_find_package(StringViewLite PROVIDED_TARGETS CXXKitWrapStringViewLite::WrapStringViewLite)
cxxkit_find_package(Optional PROVIDED_TARGETS CXXKitWrapOptional::WrapOptional)
cxxkit_find_package(Expected PROVIDED_TARGETS CXXKitWrapExpected::WrapExpected)
cxxkit_find_package(ConcurrentQueue PROVIDED_TARGETS CXXKitWrapConcurrentQueue::WrapConcurrentQueue)
cxxkit_find_package(ReaderWriterQueue PROVIDED_TARGETS CXXKitWrapReaderWriterQueue::WrapReaderWriterQueue)
cxxkit_find_package(Function2 PROVIDED_TARGETS CXXKitWrapFunction2::WrapFunction2)
cxxkit_find_package(Filesystem PROVIDED_TARGETS CXXKitWrapFilesystem::WrapFilesystem)
cxxkit_find_package(Spdlog PROVIDED_TARGETS CXXKitWrapSpdlog::WrapSpdlog)
cxxkit_find_package(Variant PROVIDED_TARGETS CXXKitWrapVariant::WrapVariant)
cxxkit_find_package(Benchmark PROVIDED_TARGETS CXXKitWrapBenchmark::WrapBenchmark)
cxxkit_install_public_wrap_headers(cxxkit_base
    WRAPS
    CXXKitWrapFmt::WrapFmt|fmt
    CXXKitWrapStringViewLite::WrapStringViewLite|nonstd
    CXXKitWrapOptional::WrapOptional|tl
    CXXKitWrapExpected::WrapExpected|tl
    CXXKitWrapConcurrentQueue::WrapConcurrentQueue|concurrentqueue
    CXXKitWrapReaderWriterQueue::WrapReaderWriterQueue|readerwriterqueue
    CXXKitWrapFunction2::WrapFunction2|function2
    CXXKitWrapFilesystem::WrapFilesystem|ghc
    CXXKitWrapSpdlog::WrapSpdlog|spdlog
    CXXKitWrapVariant::WrapVariant|mpark
    CXXKitWrapBenchmark::WrapBenchmark|benchmark)

# cxxkit/3rdparty/ 统一 include 根（三方头命名空间拷贝）
target_include_directories(cxxkit_base INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_base INTERFACE
    CXXKitWrapFmt::WrapFmt
    CXXKitWrapStringViewLite::WrapStringViewLite
    CXXKitWrapOptional::WrapOptional
    CXXKitWrapExpected::WrapExpected
    CXXKitWrapConcurrentQueue::WrapConcurrentQueue
    CXXKitWrapReaderWriterQueue::WrapReaderWriterQueue
    CXXKitWrapFunction2::WrapFunction2
    CXXKitWrapFilesystem::WrapFilesystem
    CXXKitWrapSpdlog::WrapSpdlog
    CXXKitWrapVariant::WrapVariant
    CXXKitWrapBenchmark::WrapBenchmark)

target_compile_features(cxxkit_base INTERFACE cxx_std_11)

cxxkit_generate_pkg_config(cxxkit_base base)
cxxkit_configure_library_end(CxxKitBase)

set_property(TARGET cxxkit_base PROPERTY FOLDER "cxxkit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_base EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/base/
        DESTINATION include/cxxkit/base
        FILES_MATCHING PATTERN "*.hpp")
    install(FILES ${PROJECT_BINARY_DIR}/cxxkit/base/core_config.hpp
        DESTINATION include/cxxkit/base)
endif()
