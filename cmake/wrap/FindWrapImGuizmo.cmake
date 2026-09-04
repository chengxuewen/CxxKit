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
if(TARGET CxxKitWrapImGuizmo::WrapImGuizmo)
    set(CxxKitWrapImGuizmo_FOUND ON)
    return()
endif()

# Extract-only wrap: ImGuizmo has no upstream build system for a standalone library, consumers
# compile the upstream sources themselves via CXXKIT_WRAP_IMGUIZMO_SOURCES. No .a/.so is produced here.
set(CxxKitWrapImGuizmo_NAME "imguizmo-v1.92.5")
set(CxxKitWrapImGuizmo_PKG_NAME "${CxxKitWrapImGuizmo_NAME}.7z")
set(CxxKitWrapImGuizmo_DIR_NAME "${CxxKitWrapImGuizmo_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImGuizmo_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImGuizmo_PKG_NAME}")
set(CxxKitWrapImGuizmo_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImGuizmo_DIR_NAME}")
set(CxxKitWrapImGuizmo_SOURCE_DIR "${CxxKitWrapImGuizmo_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImGuizmo_INSTALL_DIR "${CxxKitWrapImGuizmo_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImGuizmo OUTPUT_DIR "${CxxKitWrapImGuizmo_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImGuizmo URL "${CxxKitWrapImGuizmo_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImGuizmo_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImGuizmo_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImGuizmo_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImGuizmo_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImGuizmo_NAME} headers...")
    file(COPY
        "${CxxKitWrapImGuizmo_SOURCE_DIR}/ImGuizmo.h"
        DESTINATION "${CxxKitWrapImGuizmo_INSTALL_DIR}/include/cxxkit/3rdparty/imguizmo")
    message(STATUS "${CxxKitWrapImGuizmo_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImGuizmo_STAMP_FILE_PATH}")
endif()

# Expose upstream sources for consumers to compile (extract-only: no prebuilt library)
set(CXXKIT_WRAP_IMGUIZMO_SOURCES
    "${CxxKitWrapImGuizmo_SOURCE_DIR}/ImGuizmo.cpp"
    CACHE INTERNAL "" FORCE)

# wrap lib
add_library(CxxKitWrapImGuizmo::WrapImGuizmo INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImGuizmo::WrapImGuizmo PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImGuizmo_INSTALL_DIR}/include)
set(CxxKitWrapImGuizmo_FOUND ON)
