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

# implot sublibrary for cxxkit::imgui. Compiled target binding upstream implot.cpp/implot_items.cpp
# to the vendored imgui extract. Gated by CXXKIT_ENABLE_LIB_IMGUI together with the main imgui lib
# (same as sdl3/gizmo: no separate knob). Added from cxxkit/imgui/CMakeLists.txt after cxxkit_imgui.

cxxkit_find_package(Implot PROVIDED_TARGETS CxxKitWrapImplot::WrapImplot)

cxxkit_add_library(cxxkit_imgui_plot
    SOURCES ${CXXKIT_WRAP_IMPLOT_SOURCES}
    PUBLIC_LIBRARIES cxxkit::imgui
    EXPORT_NAME imgui_plot
    FOLDER "CxxKit/libs")

# Upstream implot sources include "imgui.h"/"imgui_internal.h" (quoted, resolved from the imgui
# extract root) and their own siblings — both roots must be include paths. Genexpr keeps the
# install-time export free of build-dir-prefixed paths.
target_include_directories(cxxkit_imgui_plot PUBLIC
    $<BUILD_INTERFACE:${CxxKitWrapImGui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CxxKitWrapImplot_SOURCE_DIR}>    $<BUILD_INTERFACE:${CxxKitWrapImplot_INSTALL_DIR}/include>)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_imgui_plot EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/imgui/plot/
        DESTINATION include/cxxkit/imgui/plot
        FILES_MATCHING PATTERN "*.hpp")
endif()
# Ship the implot staged headers with the imgui lib install (D6 namespaced path). Lives HERE:
# IMPORTED targets are only mutable in their creating dir scope (PIT-34).
cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImplot::WrapImplot|implot)
