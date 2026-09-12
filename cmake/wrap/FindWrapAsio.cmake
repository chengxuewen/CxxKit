########################################################################################################################
#
# Library: CXXKIT
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
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# Vendored asio 1.32.0 (extract-only, FindWrapImGui precedent): unpack the in-repo
# tarball, stage the header tree under install/include/cxxkit/3rdparty/asio/ (D6
# namespace path), then expose CxxKitWrapAsio::WrapAsio as an INTERFACE IMPORTED
# target. asio is header-only in standalone mode (ASIO_STANDALONE); no .a/.so is
# produced here and no build step runs.

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CxxKitWrapAsio::WrapAsio)
    set(CxxKitWrapAsio_FOUND ON)
    return()
endif()

set(CxxKitWrapAsio_NAME "asio-asio-1-32-0")
set(CxxKitWrapAsio_PKG_NAME "${CxxKitWrapAsio_NAME}.tar.gz")
set(CxxKitWrapAsio_DIR_NAME "${CxxKitWrapAsio_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapAsio_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapAsio_PKG_NAME}")
set(CxxKitWrapAsio_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapAsio_DIR_NAME}")
set(CxxKitWrapAsio_SOURCE_DIR "${CxxKitWrapAsio_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapAsio_INSTALL_DIR "${CxxKitWrapAsio_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapAsio OUTPUT_DIR "${CxxKitWrapAsio_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapAsio URL "${CxxKitWrapAsio_URL_PATH}" OUTPUT_NAME "${CxxKitWrapAsio_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapAsio_STAMP_FILE_PATH}")
    if(NOT EXISTS "${CxxKitWrapAsio_SOURCE_DIR}/asio/include/asio.hpp")
        message(FATAL_ERROR "${CxxKitWrapAsio_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapAsio_NAME} headers...")
    file(COPY "${CxxKitWrapAsio_SOURCE_DIR}/asio/include/"
        DESTINATION "${CxxKitWrapAsio_INSTALL_DIR}/include/cxxkit/3rdparty/asio")
    message(STATUS "${CxxKitWrapAsio_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapAsio_STAMP_FILE_PATH}")
endif()

# wrap lib (header-only: include dirs + standalone-mode definitions, no library to link).
# Two include dirs: the staged root (<cxxkit/3rdparty/asio/asio.hpp>, D6 namespace) AND the
# D6 asio/ dir itself — asio's internal includes are un-namespaced ("asio/detail/config.hpp"),
# so the dir that CONTAINS asio/ must be on the path for its own headers to resolve.
add_library(CxxKitWrapAsio::WrapAsio INTERFACE IMPORTED)
set_target_properties(CxxKitWrapAsio::WrapAsio PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    "${CxxKitWrapAsio_INSTALL_DIR}/include;${CxxKitWrapAsio_INSTALL_DIR}/include/cxxkit/3rdparty/asio")
target_compile_definitions(CxxKitWrapAsio::WrapAsio INTERFACE ASIO_STANDALONE)
# asio standalone needs pthreads on POSIX (posix_thread/posix_event .ipp internals):
find_package(Threads REQUIRED)
set_property(TARGET CxxKitWrapAsio::WrapAsio PROPERTY INTERFACE_LINK_LIBRARIES Threads::Threads)
set(CxxKitWrapAsio_FOUND ON)
