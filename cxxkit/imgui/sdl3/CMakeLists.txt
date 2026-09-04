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
# The upstream impl files are the property of the dear imgui project (MIT), (c) Omar Cornut.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
########################################################################################################################

# SDL3 + OpenGL3 backend for cxxkit::imgui. Compiled target binding upstream imgui_impl_sdl3 +
# imgui_impl_opengl3 to the PlatformBackend/RendererBackend contract (cxxkit/imgui/backend.hpp).
# Gated by CXXKIT_ENABLE_LIB_IMGUI together with the main imgui lib (decision 3: no separate knob).
# This subdirectory is added from cxxkit/imgui/CMakeLists.txt after cxxkit_imgui exists.

cxxkit_find_package(SDL3 PROVIDED_TARGETS CxxKitWrapSDL3::WrapSDL3)

# No find_package(OpenGL): imgui_impl_opengl3 uses its bundled loader (imgui_impl_opengl3_loader.h,
# resolves GL procs via dlopen at runtime). At link time the backend needs no GL library on Linux;
# if a future platform does, add it here with evidence from the failing link.
cxxkit_add_library(cxxkit_imgui_sdl3
    SOURCES sdl3_backend.cpp
        ${CxxKitWrapImGui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${CxxKitWrapImGui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    PUBLIC_LIBRARIES cxxkit::imgui CxxKitWrapSDL3::WrapSDL3
    # IMGUI_API channel: upstream impl TUs never include imgui_global.hpp (same as the 5 core TUs).
    # IMGUI_IMPL_API defaults to IMGUI_API inside the impl headers, so this one define covers both.
    COMPILE_DEFINITIONS IMGUI_API=CXXKIT_IMGUI_API
    EXPORT_NAME imgui_sdl3
    FOLDER "CxxKit/libs")

# Upstream backend TUs include "imgui.h" (quoted, resolved from the imgui root) and their own
# siblings; the imgui extract root + backends dir must both be include paths. Genexpr keeps the
# install-time export free of build-dir-prefixed paths (CMake rejects those).
target_include_directories(cxxkit_imgui_sdl3 PUBLIC
    $<BUILD_INTERFACE:${CxxKitWrapImGui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CxxKitWrapImGui_SOURCE_DIR}/backends>)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_imgui_sdl3 EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/imgui/sdl3/
        DESTINATION include/cxxkit/imgui/sdl3
        FILES_MATCHING PATTERN "*.hpp")
endif()
