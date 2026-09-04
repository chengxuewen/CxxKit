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
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
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

# imgui_markdown sublibrary for cxxkit::imgui — INTERFACE passthrough (P2a T11).
# Upstream is a single header; no compiled unit. Gated by CXXKIT_ENABLE_LIB_IMGUI
# together with the main imgui lib (same as sdl3/plot/gizmo: no separate knob).
# This subdirectory is added from cxxkit/imgui/CMakeLists.txt after cxxkit_imgui exists.
#
# NOTE: the helper silently ignores PUBLIC_LIBRARIES for INTERFACE targets — linking
# MUST be an explicit target_link_libraries(... INTERFACE ...) (profiling precedent).

cxxkit_find_package(ImguiMarkdown PROVIDED_TARGETS CxxKitWrapImguiMarkdown::WrapImguiMarkdown)

cxxkit_add_library(cxxkit_imgui_markdown
    INTERFACE
    EXPORT_NAME imgui_markdown
    FOLDER "CxxKit/libs")

# Pull in the vendored imgui extract (upstream contract: <imgui.h> before
# <imgui_markdown.h>) and the markdown header itself via the wrap's namespaced
# include tree. INTERFACE-only link: helper's PUBLIC_LIBRARIES would be ignored.
target_link_libraries(cxxkit_imgui_markdown INTERFACE
    cxxkit::imgui
    CxxKitWrapImguiMarkdown::WrapImguiMarkdown)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_imgui_markdown EXPORT cxxkitTargets)
endif()
# Ship the staged imgui_markdown.h with the imgui lib install (D6 namespaced path).
# Lives HERE: IMPORTED targets are only mutable in their creating dir scope (PIT-34).
cxxkit_install_public_wrap_headers(cxxkit_imgui WRAPS CxxKitWrapImguiMarkdown::WrapImguiMarkdown|imgui_markdown)
