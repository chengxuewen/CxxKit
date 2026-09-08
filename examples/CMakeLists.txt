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

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

set(CXXKIT_EXAMPLE_LINK_LIBRARIES
    cxxkit::base cxxkit::tools cxxkit::text)

cxxkit_add_executable(cxxkit_exp_core_version
    SOURCES exp_core_version.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_logging
    SOURCES exp_logging.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_units
    SOURCES exp_units.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::units cxxkit::time
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_text
    SOURCES exp_text.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_base
    SOURCES exp_base.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_time
    SOURCES exp_time.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::time
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_patterns
    SOURCES exp_patterns.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::patterns
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_memory
    SOURCES exp_memory.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::memory
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_functional
    SOURCES exp_functional.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::functional
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")

cxxkit_add_executable(cxxkit_exp_containers
    SOURCES exp_containers.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::containers
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")


cxxkit_add_executable(cxxkit_exp_thread
    SOURCES exp_thread.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::thread
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")

cxxkit_add_executable(cxxkit_exp_numerics
    SOURCES exp_numerics.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::numerics
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_media
    SOURCES exp_media.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::media
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
cxxkit_add_executable(cxxkit_exp_kernel
    SOURCES exp_kernel.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::kernel
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
if(CXXKIT_ENABLE_LIB_NETWORK)
    cxxkit_add_executable(cxxkit_exp_network_version
        SOURCES exp_network_version.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_network_version PRIVATE cxxkit::network)
endif()

cxxkit_add_executable(cxxkit_exp_profiling
    SOURCES exp_profiling.cpp
    LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES} cxxkit::profiling
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
    FOLDER "CxxKit/examples")
# cxxkit_profiling is an unconditional no-op INTERFACE target; with CXXKIT_ENABLE_LIB_TRACY=ON its
# INTERFACE compile definitions activate the Tracy backend in this example automatically.

if(CXXKIT_ENABLE_LIB_CRASH)
    cxxkit_add_executable(cxxkit_exp_crash
        SOURCES exp_crash.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_crash PRIVATE cxxkit::crash)
endif()

if(CXXKIT_ENABLE_LIB_IMGUI)
    # WrapImplot/WrapImplot3d are IMPORTED targets created in cxxkit/imgui/plot*/CMakeLists.txt
    # (dir-scoped, PIT-34) — invisible from examples/, so re-run the wrap find here. Stamp-guarded,
    # idempotent; gives the D6 <cxxkit/3rdparty/implot/...> include root to the example target.
    cxxkit_find_package(Implot PROVIDED_TARGETS CxxKitWrapImplot::WrapImplot)
    cxxkit_find_package(Implot3d PROVIDED_TARGETS CxxKitWrapImplot3d::WrapImplot3d)
    cxxkit_add_executable(cxxkit_exp_imgui
        SOURCES imgui/exp_imgui.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui PRIVATE cxxkit::imgui cxxkit::imgui_sdl3)
    cxxkit_add_executable(cxxkit_exp_imgui_plot
        SOURCES imgui/exp_imgui_plot.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_plot PRIVATE cxxkit::imgui cxxkit::imgui_sdl3 cxxkit::imgui_plot CxxKitWrapImplot::WrapImplot)
    cxxkit_add_executable(cxxkit_exp_imgui_plot3d
        SOURCES imgui/exp_imgui_plot3d.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_plot3d PRIVATE cxxkit::imgui cxxkit::imgui_sdl3 cxxkit::imgui_plot3d CxxKitWrapImplot3d::WrapImplot3d)
    cxxkit_add_executable(cxxkit_exp_imgui_file_dialog
        SOURCES imgui/exp_imgui_file_dialog.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_file_dialog PRIVATE cxxkit::imgui cxxkit::imgui_sdl3 cxxkit::imgui_file_dialog)
    cxxkit_add_executable(cxxkit_exp_imgui_headless
        SOURCES imgui/exp_imgui_headless.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_headless PRIVATE cxxkit::imgui)
    cxxkit_add_executable(cxxkit_exp_imgui_markdown
        SOURCES imgui/exp_imgui_markdown.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_markdown PRIVATE cxxkit::imgui cxxkit::imgui_sdl3 cxxkit::imgui_markdown)
    cxxkit_add_executable(cxxkit_exp_imgui_nodes
        SOURCES imgui/exp_imgui_nodes.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_nodes PRIVATE cxxkit::imgui cxxkit::imgui_sdl3 cxxkit::imgui_nodes)
    cxxkit_add_executable(cxxkit_exp_imgui_gizmo
        SOURCES imgui/exp_imgui_gizmo.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_imgui_gizmo PRIVATE cxxkit::imgui cxxkit::imgui_sdl3 cxxkit::imgui_gizmo)
    # imgui_markdown's MarkdownHeadingFormat carries an NSDMI under IMGUI_HAS_TEXTURES
    # (imgui 1.92) — the header itself then requires C++14 (PIT-35); this TU compiles it.
    set_property(TARGET cxxkit_exp_imgui_markdown PROPERTY CXX_STANDARD 14)
endif()

if(CXXKIT_ENABLE_LIB_UV)
    cxxkit_add_executable(cxxkit_exp_event_loop
        SOURCES exp_event_loop.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_event_loop PRIVATE cxxkit::uv cxxkit::kernel)
    cxxkit_add_executable(cxxkit_exp_tcp_echo
        SOURCES exp_tcp_echo.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_tcp_echo PRIVATE cxxkit::uv cxxkit::kernel)
endif()

if(CXXKIT_ENABLE_LIB_QT)
    cxxkit_add_executable(cxxkit_exp_qt_embed
        SOURCES exp_qt_embed.cpp
        LIBRARIES ${CXXKIT_EXAMPLE_LINK_LIBRARIES}
        INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}
        FOLDER "CxxKit/examples")
    target_link_libraries(cxxkit_exp_qt_embed PRIVATE cxxkit::qt cxxkit::kernel ${CXXKIT_QT_LOCAL_STDCPP})
    # Local conda-Qt link surface (g++ 10.5 libstdc++ too old for libQt6Core; CI gcc>=11 needs none of this).
    # Target-scoped on purpose: a global EXE_LINKER_FLAGS injection poisons every other target's -L search.
    if(EXISTS "/tmp/opencode/qt-env/.pixi/envs/default/lib/stubs")
        set_property(TARGET cxxkit_exp_qt_embed PROPERTY LINK_DIRECTORIES "/tmp/opencode/qt-env/.pixi/envs/default/lib" "/tmp/opencode/qt-env/.pixi/envs/default/lib/stubs")
    endif()
    # Qt headers need C++17 (qt target property, M9 precedent) — the helper default is 11.
    set_property(TARGET cxxkit_exp_qt_embed PROPERTY CXX_STANDARD 17)
endif()
