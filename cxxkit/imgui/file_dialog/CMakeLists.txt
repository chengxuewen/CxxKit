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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# ImGuiFileDialog sublibrary for cxxkit::imgui. Compiled target binding upstream ImGuiFileDialog.cpp
# to the vendored imgui extract (cxxkit::imgui wraps the same extract). Gated by CXXKIT_ENABLE_LIB_IMGUI
# together with the main imgui lib (same as gizmo/sdl3: no separate knob).
# This subdirectory is added from cxxkit/imgui/CMakeLists.txt after cxxkit_imgui exists.

cxxkit_find_package(ImGuiFileDialog PROVIDED_TARGETS CxxKitWrapImGuiFileDialog::WrapImGuiFileDialog)

cxxkit_add_library(cxxkit_imgui_file_dialog
    SOURCES ${CXXKIT_WRAP_IMGUI_FILE_DIALOG_SOURCES}
    PUBLIC_LIBRARIES cxxkit::imgui
    # ImGuiFileDialog.h declares `class IMGUI_API ImGuiFileDialog` — IMGUI_API resolves through the
    # imgui wrap's COMPILE_DEFINITIONS/headers channel (upstream contract honored, source untouched).
    EXPORT_NAME imgui_file_dialog
    FOLDER "CxxKit/libs")

# Upstream ImGuiFileDialog.cpp includes <imgui.h>/<imgui_internal.h> (resolved from the imgui root)
# and its own "ImGuiFileDialogConfig.h" sibling — the imgui extract root + file-dialog source root
# must both be include paths. Genexpr keeps the install-time export free of build-dir-prefixed
# paths (CMake rejects those).
target_include_directories(cxxkit_imgui_file_dialog PUBLIC
    $<BUILD_INTERFACE:${CxxKitWrapImGui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CxxKitWrapImGuiFileDialog_SOURCE_DIR}>)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_imgui_file_dialog EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/imgui/file_dialog/
        DESTINATION include/cxxkit/imgui/file_dialog
        FILES_MATCHING PATTERN "*.hpp")
endif()
# Ship the ImGuiFileDialog staged headers with the imgui lib install (D6 namespaced include path).
# Must live HERE, not in the parent CMakeLists: CMake forbids a parent dir from running
# target_include_directories on an IMPORTED target created in a subdirectory (PIT-34).
cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImGuiFileDialog::WrapImGuiFileDialog|imgui_file_dialog)
