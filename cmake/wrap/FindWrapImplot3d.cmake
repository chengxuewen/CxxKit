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
if(TARGET CxxKitWrapImplot3d::WrapImplot3d)
    set(CxxKitWrapImplot3d_FOUND ON)
    return()
endif()

# Extract-only wrap: implot3d has no upstream build system for a standalone library, consumers
# compile the upstream sources themselves via CXXKIT_WRAP_IMPLOT3D_SOURCES. No .a/.so is produced
# here. implot3d_demo.cpp is deliberately NOT staged as a source (contrast with imgui's own demo
# inside cxxkit_imgui): the plot demo is large and example-side opt-in.
# Vendored: brenocq/implot3d master @ 6cbefa9b976471f44deffa1571165454976fe994 (2026-07-14,
# IMPLOT3D_VERSION "0.5 WIP"), packed with .git/ stripped (demo kept, implot precedent).
# Package SHA256: 9731487c904c125c9f519a4525f429626243b5a7329b9cd4c411cc15fbb78223
set(CxxKitWrapImplot3d_NAME "implot3d-v0.5-wip")
set(CxxKitWrapImplot3d_PKG_NAME "${CxxKitWrapImplot3d_NAME}.7z")
set(CxxKitWrapImplot3d_DIR_NAME "${CxxKitWrapImplot3d_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImplot3d_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImplot3d_PKG_NAME}")
set(CxxKitWrapImplot3d_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImplot3d_DIR_NAME}")
set(CxxKitWrapImplot3d_SOURCE_DIR "${CxxKitWrapImplot3d_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImplot3d_INSTALL_DIR "${CxxKitWrapImplot3d_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImplot3d OUTPUT_DIR "${CxxKitWrapImplot3d_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImplot3d URL "${CxxKitWrapImplot3d_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImplot3d_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImplot3d_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImplot3d_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImplot3d_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImplot3d_NAME} headers...")
    file(COPY
        "${CxxKitWrapImplot3d_SOURCE_DIR}/implot3d.h"
        "${CxxKitWrapImplot3d_SOURCE_DIR}/implot3d_internal.h"
        DESTINATION "${CxxKitWrapImplot3d_INSTALL_DIR}/include/cxxkit/3rdparty/implot3d")
    message(STATUS "${CxxKitWrapImplot3d_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImplot3d_STAMP_FILE_PATH}")
endif()

# Expose upstream sources for consumers to compile (extract-only: no prebuilt library)
set(CXXKIT_WRAP_IMPLOT3D_SOURCES
    "${CxxKitWrapImplot3d_SOURCE_DIR}/implot3d.cpp"
    "${CxxKitWrapImplot3d_SOURCE_DIR}/implot3d_items.cpp"
    "${CxxKitWrapImplot3d_SOURCE_DIR}/implot3d_meshes.cpp"
    CACHE INTERNAL "" FORCE)

# wrap lib
add_library(CxxKitWrapImplot3d::WrapImplot3d INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImplot3d::WrapImplot3d PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImplot3d_INSTALL_DIR}/include)
set(CxxKitWrapImplot3d_FOUND ON)
