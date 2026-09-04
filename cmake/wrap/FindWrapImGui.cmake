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
if(TARGET CxxKitWrapImGui::WrapImGui)
    set(CxxKitWrapImGui_FOUND ON)
    return()
endif()

# Extract-only wrap: imgui has no upstream build system, consumers compile the
# upstream sources themselves via CXXKIT_WRAP_IMGUI_SOURCES. No .a/.so is produced here.
set(CxxKitWrapImGui_NAME "imgui-v1.92.9b")
set(CxxKitWrapImGui_PKG_NAME "${CxxKitWrapImGui_NAME}.7z")
set(CxxKitWrapImGui_DIR_NAME "${CxxKitWrapImGui_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImGui_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImGui_PKG_NAME}")
set(CxxKitWrapImGui_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImGui_DIR_NAME}")
set(CxxKitWrapImGui_SOURCE_DIR "${CxxKitWrapImGui_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImGui_INSTALL_DIR "${CxxKitWrapImGui_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImGui OUTPUT_DIR "${CxxKitWrapImGui_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImGui URL "${CxxKitWrapImGui_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImGui_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImGui_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImGui_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImGui_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImGui_NAME} headers...")
    file(COPY
        "${CxxKitWrapImGui_SOURCE_DIR}/imgui.h"
        "${CxxKitWrapImGui_SOURCE_DIR}/imconfig.h"
        "${CxxKitWrapImGui_SOURCE_DIR}/imgui_internal.h"
        "${CxxKitWrapImGui_SOURCE_DIR}/imstb_textedit.h"
        "${CxxKitWrapImGui_SOURCE_DIR}/imstb_truetype.h"
        "${CxxKitWrapImGui_SOURCE_DIR}/imstb_rectpack.h"
        "${CxxKitWrapImGui_SOURCE_DIR}/LICENSE.txt"
        DESTINATION "${CxxKitWrapImGui_INSTALL_DIR}/include/cxxkit/3rdparty/imgui")
    message(STATUS "${CxxKitWrapImGui_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImGui_STAMP_FILE_PATH}")
endif()

# Expose upstream sources for consumers to compile (extract-only: no prebuilt library)
set(CxxKitWrapImGui_SOURCE_PREFIX "${CxxKitWrapImGui_SOURCE_DIR}/imgui-v1.92.9b")
set(CXXKIT_WRAP_IMGUI_SOURCES
    "${CxxKitWrapImGui_SOURCE_DIR}/imgui.cpp"
    "${CxxKitWrapImGui_SOURCE_DIR}/imgui_demo.cpp"
    "${CxxKitWrapImGui_SOURCE_DIR}/imgui_draw.cpp"
    "${CxxKitWrapImGui_SOURCE_DIR}/imgui_tables.cpp"
    "${CxxKitWrapImGui_SOURCE_DIR}/imgui_widgets.cpp"
    CACHE INTERNAL "" FORCE)

# wrap lib
add_library(CxxKitWrapImGui::WrapImGui INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImGui::WrapImGui PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImGui_INSTALL_DIR}/include)
set(CxxKitWrapImGui_FOUND ON)
