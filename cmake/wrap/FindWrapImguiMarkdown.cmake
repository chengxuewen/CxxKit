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
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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
if(TARGET CxxKitWrapImguiMarkdown::WrapImguiMarkdown)
    set(CxxKitWrapImguiMarkdown_FOUND ON)
    return()
endif()

# Extract-only wrap: imgui_markdown is a single self-contained header (imgui_markdown.h,
# real includes: <stdint.h> only; imgui.h must be included BEFORE it — upstream contract).
# No sources, no library. No upstream version macro — package named by master commit date
# 2026-07-03 (4acbf80584753e15ea54eb271129995862daac8f), sha256
# 3722434437fd42e592917c8178a8a31ef36c8a01c35b6f3bcd6e6b236992edac
set(CxxKitWrapImguiMarkdown_NAME "imgui_markdown-master-20260703")
set(CxxKitWrapImguiMarkdown_PKG_NAME "${CxxKitWrapImguiMarkdown_NAME}.7z")
set(CxxKitWrapImguiMarkdown_DIR_NAME "${CxxKitWrapImguiMarkdown_NAME}-${CXXKIT_LOWER_BUILD_TYPE}")
set(CxxKitWrapImguiMarkdown_URL_PATH "${PROJECT_SOURCE_DIR}/3rdparty/${CxxKitWrapImguiMarkdown_PKG_NAME}")
set(CxxKitWrapImguiMarkdown_ROOT_DIR "${PROJECT_BINARY_DIR}/3rdparty/${CxxKitWrapImguiMarkdown_DIR_NAME}")
set(CxxKitWrapImguiMarkdown_SOURCE_DIR "${CxxKitWrapImguiMarkdown_ROOT_DIR}/source" CACHE INTERNAL "" FORCE)
set(CxxKitWrapImguiMarkdown_INSTALL_DIR "${CxxKitWrapImguiMarkdown_ROOT_DIR}/install" CACHE INTERNAL "" FORCE)
cxxkit_stamp_file_info(CxxKitWrapImguiMarkdown OUTPUT_DIR "${CxxKitWrapImguiMarkdown_ROOT_DIR}")
cxxkit_fetch_3rdparty(CxxKitWrapImguiMarkdown URL "${CxxKitWrapImguiMarkdown_URL_PATH}" OUTPUT_NAME "${CxxKitWrapImguiMarkdown_DIR_NAME}")
if(NOT EXISTS "${CxxKitWrapImguiMarkdown_STAMP_FILE_PATH}")
    if(NOT EXISTS ${CxxKitWrapImguiMarkdown_SOURCE_DIR})
        message(FATAL_ERROR "${CxxKitWrapImguiMarkdown_NAME} FetchContent failed.")
    endif()

    message(STATUS "Stage ${CxxKitWrapImguiMarkdown_NAME} headers...")
    file(COPY
        "${CxxKitWrapImguiMarkdown_SOURCE_DIR}/imgui_markdown.h"
        # Stage UN-namespaced (include/imgui_markdown/): cxxkit_install_public_wrap_headers
        # re-prefixes include/ content into <cxxkit/3rdparty>, so staging an already-namespaced
        # tree (imgui-family precedent) double-nests to cxxkit/3rdparty/cxxkit/3rdparty in the
        # build/install trees. include/imgui_markdown/ lands flat at the D6 path.
        DESTINATION "${CxxKitWrapImguiMarkdown_INSTALL_DIR}/include/imgui_markdown")
    message(STATUS "${CxxKitWrapImguiMarkdown_NAME} headers staged")
    cxxkit_make_stamp_file("${CxxKitWrapImguiMarkdown_STAMP_FILE_PATH}")
endif()

# Header-only: no CXXKIT_WRAP_IMGUI_MARKDOWN_SOURCES (nothing for consumers to compile).

# wrap lib
add_library(CxxKitWrapImguiMarkdown::WrapImguiMarkdown INTERFACE IMPORTED)
set_target_properties(CxxKitWrapImguiMarkdown::WrapImguiMarkdown PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
    ${CxxKitWrapImguiMarkdown_INSTALL_DIR}/include)
set(CxxKitWrapImguiMarkdown_FOUND ON)
