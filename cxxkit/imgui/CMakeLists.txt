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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitImGui)

cxxkit_find_package(ImGui PROVIDED_TARGETS CxxKitWrapImGui::WrapImGui)
cxxkit_add_library(cxxkit_imgui
    SOURCES context.cpp fake_backend.cpp ${CXXKIT_WRAP_IMGUI_SOURCES}
    PUBLIC_LIBRARIES cxxkit::base cxxkit::tools CxxKitWrapImGui::WrapImGui
    # Upstream 5 TUs never include imgui_global.hpp, so they get IMGUI_API from these target
    # definitions only. CXXKIT_IMGUI_API= empty mirrors imgui_global.hpp's static-lib branch
    # (default visibility) — the standard model for building upstream imgui into a lib.
    COMPILE_DEFINITIONS IMGUI_API=CXXKIT_IMGUI_API CXXKIT_IMGUI_API=
    EXPORT_NAME imgui
    FOLDER "CxxKit/libs")
cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImGui::WrapImGui|imgui)
cxxkit_configure_library_end(CxxKitImGui)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_imgui EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/imgui/
        DESTINATION include/cxxkit/imgui
        FILES_MATCHING PATTERN "*.hpp")
endif()
