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
if(TARGET CxxKitWrapImplot::WrapImplot)
    set(CxxKitWrapImplot_FOUND ON)
    return()
endif()

# Extract-only wrap: implot has no upstream build system for a standalone library, consumers
# compile the upstream sources themselves via CXXKIT_WRAP_IMPLOT_SOURCES. No .a/.so is produced
# here. implot_demo.cpp is deliberately NOT staged as a source (contrast with imgui's own demo
# inside cxxkit_imgui): the plot demo is large and example-side opt-in.
set(CxxKitWrapImplot_NAME "implot-v1.1-wip")
set(CxxKitWrapImplot_PKG_NAME "${CxxKitWrapImplot_NAME}.7z")
set(CxxKitWrapImplot_DIR_NAME "${CxxKitWrapImplot_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImplot_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImplot_PKG_NAME}")
set(CxxKitWrapImplot_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImplot_DIR_NAME}")
set(CxxKitWrapImplot_SOURCE_DIR "${CxxKitWrapImplot_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImplot_INSTALL_DIR "${CxxKitWrapImplot_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImplot OUTPUT_DIR "${CxxKitWrapImplot_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImplot URL "${CxxKitWrapImplot_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImplot_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImplot_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImplot_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImplot_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImplot_NAME} headers...")
    file(COPY
        "${CxxKitWrapImplot_SOURCE_DIR}/implot.h"
        "${CxxKitWrapImplot_SOURCE_DIR}/implot_internal.h"
        DESTINATION "${CxxKitWrapImplot_INSTALL_DIR}/include/cxxkit/3rdparty/implot")
    message(STATUS "${CxxKitWrapImplot_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImplot_STAMP_FILE_PATH}")
endif()

# Expose upstream sources for consumers to compile (extract-only: no prebuilt library)
set(CXXKIT_WRAP_IMPLOT_SOURCES
    "${CxxKitWrapImplot_SOURCE_DIR}/implot.cpp"
    "${CxxKitWrapImplot_SOURCE_DIR}/implot_items.cpp"
    CACHE INTERNAL "" FORCE)

# wrap lib
add_library(CxxKitWrapImplot::WrapImplot INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImplot::WrapImplot PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImplot_INSTALL_DIR}/include)
set(CxxKitWrapImplot_FOUND ON)
