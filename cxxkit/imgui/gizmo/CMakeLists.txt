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

# ImGuizmo sublibrary for cxxkit::imgui. Compiled target binding upstream ImGuizmo.cpp to the
# vendored imgui extract (cxxkit::imgui wraps the same extract). Gated by CXXKIT_ENABLE_LIB_IMGUI
# together with the main imgui lib (same as sdl3 backend: no separate knob).
# This subdirectory is added from cxxkit/imgui/CMakeLists.txt after cxxkit_imgui exists.

cxxkit_find_package(ImGuizmo PROVIDED_TARGETS CxxKitWrapImGuizmo::WrapImGuizmo)

cxxkit_add_library(cxxkit_imgui_gizmo
    SOURCES ${CXXKIT_WRAP_IMGUIZMO_SOURCES}
    PUBLIC_LIBRARIES cxxkit::imgui
    # ImGuizmo.h: `#ifndef IMGUI_API #define IMGUI_API #endif` — plain declarations under default
    # visibility, no IMGUI_API define channel needed (upstream contract honored, source untouched).
    EXPORT_NAME imgui_gizmo
    FOLDER "CxxKit/libs")

# Upstream ImGuizmo.cpp includes "imgui.h"/"imgui_internal.h" (quoted, resolved from the imgui root)
# and its own "ImGuizmo.h" sibling — the imgui extract root + gizmo source root must both be include
# paths. Genexpr keeps the install-time export free of build-dir-prefixed paths (CMake rejects those).
target_include_directories(cxxkit_imgui_gizmo PUBLIC
    $<BUILD_INTERFACE:${CxxKitWrapImGui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CxxKitWrapImGuizmo_SOURCE_DIR}>
    # Staged D6 headers (<cxxkit/3rdparty/...>) must reach consumers too — the wrap INTERFACE
    # include root is not propagated transitively unless the consumer links the wrap directly.
    $<BUILD_INTERFACE:${CxxKitWrapImGuizmo_INSTALL_DIR}/include>)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_imgui_gizmo EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/imgui/gizmo/
        DESTINATION include/cxxkit/imgui/gizmo
        FILES_MATCHING PATTERN "*.hpp")
endif()
# Ship the ImGuizmo staged header with the imgui lib install (D6 namespaced include path).
# Must live HERE, not in the parent CMakeLists: CMake forbids a parent dir from running
# target_include_directories on an IMPORTED target created in a subdirectory.
cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImGuizmo::WrapImGuizmo|imguizmo)
