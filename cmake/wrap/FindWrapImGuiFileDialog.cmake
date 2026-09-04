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
if(TARGET CxxKitWrapImGuiFileDialog::WrapImGuiFileDialog)
    set(CxxKitWrapImGuiFileDialog_FOUND ON)
    return()
endif()

# Extract-only wrap: ImGuiFileDialog is a single .cpp/.h pair compiled by consumers directly, so
# no upstream build system is invoked and no .a/.so is produced here. Consumers compile the
# upstream source via CXXKIT_WRAP_IMGUI_FILE_DIALOG_SOURCES.
set(CxxKitWrapImGuiFileDialog_NAME "imguifiledialog-v0.6.8")
set(CxxKitWrapImGuiFileDialog_PKG_NAME "${CxxKitWrapImGuiFileDialog_NAME}.7z")
set(CxxKitWrapImGuiFileDialog_DIR_NAME "${CxxKitWrapImGuiFileDialog_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImGuiFileDialog_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImGuiFileDialog_PKG_NAME}")
set(CxxKitWrapImGuiFileDialog_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImGuiFileDialog_DIR_NAME}")
set(CxxKitWrapImGuiFileDialog_SOURCE_DIR "${CxxKitWrapImGuiFileDialog_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImGuiFileDialog_INSTALL_DIR "${CxxKitWrapImGuiFileDialog_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImGuiFileDialog OUTPUT_DIR "${CxxKitWrapImGuiFileDialog_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImGuiFileDialog URL "${CxxKitWrapImGuiFileDialog_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImGuiFileDialog_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImGuiFileDialog_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImGuiFileDialog_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImGuiFileDialog_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImGuiFileDialog_NAME} headers...")
    file(COPY
        "${CxxKitWrapImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.h"
        "${CxxKitWrapImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialogConfig.h"
        DESTINATION "${CxxKitWrapImGuiFileDialog_INSTALL_DIR}/include/cxxkit/3rdparty/imgui_file_dialog")
    message(STATUS "${CxxKitWrapImGuiFileDialog_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImGuiFileDialog_STAMP_FILE_PATH}")
endif()

# Expose upstream sources for consumers to compile (extract-only: no prebuilt library)
set(CXXKIT_WRAP_IMGUI_FILE_DIALOG_SOURCES
    "${CxxKitWrapImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.cpp"
    CACHE INTERNAL "" FORCE)

# wrap lib
add_library(CxxKitWrapImGuiFileDialog::WrapImGuiFileDialog INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImGuiFileDialog::WrapImGuiFileDialog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImGuiFileDialog_INSTALL_DIR}/include)
set(CxxKitWrapImGuiFileDialog_FOUND ON)
