########################################################################################################################
#
# Library: CXXKIT
#
# Copyright (C) 2025~Present ChengXueWen.
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

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CxxKitWrapImnodes::WrapImnodes)
    set(CxxKitWrapImnodes_FOUND ON)
    return()
endif()

# Extract-only wrap: imnodes has no upstream build system for a standalone library, consumers
# compile the upstream sources themselves via CXXKIT_WRAP_IMNODES_SOURCES. No .a/.so is produced here.
# Package name uses master commit date (upstream has no version macros — imnodes-master-<yyyymmdd>).
set(CxxKitWrapImnodes_NAME "imnodes-master-20260513")
set(CxxKitWrapImnodes_PKG_NAME "${CxxKitWrapImnodes_NAME}.7z")
set(CxxKitWrapImnodes_DIR_NAME "${CxxKitWrapImnodes_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImnodes_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImnodes_PKG_NAME}")
set(CxxKitWrapImnodes_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImnodes_DIR_NAME}")
set(CxxKitWrapImnodes_SOURCE_DIR "${CxxKitWrapImnodes_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImnodes_INSTALL_DIR "${CxxKitWrapImnodes_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImnodes OUTPUT_DIR "${CxxKitWrapImnodes_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImnodes URL "${CxxKitWrapImnodes_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImnodes_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImnodes_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImnodes_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImnodes_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImnodes_NAME} headers...")
    file(COPY
        "${CxxKitWrapImnodes_SOURCE_DIR}/imnodes.h"
        "${CxxKitWrapImnodes_SOURCE_DIR}/imnodes_internal.h"
        # Stage UN-namespaced (include/imnodes/): cxxkit_install_public_wrap_headers re-prefixes
        # include/ content into <cxxkit/3rdparty>, so staging an already-namespaced tree
        # (imgui-family precedent) double-nests to cxxkit/3rdparty/cxxkit/3rdparty in the
        # build/install trees. include/imnodes/ lands flat at the D6 path.
        DESTINATION "${CxxKitWrapImnodes_INSTALL_DIR}/include/imnodes")
    message(STATUS "${CxxKitWrapImnodes_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImnodes_STAMP_FILE_PATH}")
endif()

# Expose upstream sources for consumers to compile (extract-only: no prebuilt library)
set(CXXKIT_WRAP_IMNODES_SOURCES
    "${CxxKitWrapImnodes_SOURCE_DIR}/imnodes.cpp"
    CACHE INTERNAL "" FORCE)

# wrap lib
add_library(CxxKitWrapImnodes::WrapImnodes INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImnodes::WrapImnodes PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImnodes_INSTALL_DIR}/include)
set(CxxKitWrapImnodes_FOUND ON)
