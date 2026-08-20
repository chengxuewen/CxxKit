########################################################################################################################
#
# Library: CxxKit
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
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################
#
# cxxkit::crash — crash handling sublibrary (breakpad backend + backward-cpp stack printing, opt-in).
# Gated by CXXKIT_ENABLE_LIB_CRASH at the top level (cxxkit_add_subdirectory), like cxxkit::network.
# Depends on the crash-deps merged archive via FindWrapCrashDeps (see cmake/InstallVcpkg.cmake).
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitCrash)

cxxkit_find_package(CrashDeps PROVIDED_TARGETS CXXKitWrapCrashDeps::WrapBreakpad CXXKitWrapCrashDeps::WrapBackward)

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_crash
    ${_cxxkit_headers}
    crash_handler.cpp)
add_library(cxxkit::crash ALIAS cxxkit_crash)

target_include_directories(cxxkit_crash PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_crash PUBLIC
    cxxkit::base
    cxxkit::patterns
    cxxkit::tools
    CXXKitWrapCrashDeps::WrapBreakpad
    CXXKitWrapCrashDeps::WrapBackward)
target_compile_features(cxxkit_crash PUBLIC cxx_std_11)
target_compile_definitions(cxxkit_crash PUBLIC CXXKIT_FEATURE_ENABLE_CRASH=1)
# backward.hpp dlopens libdw.so.1 at runtime (Linux): the shared lib must find the shipped .so next to itself.
set_target_properties(cxxkit_crash PROPERTIES
    EXPORT_NAME crash
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    INSTALL_RPATH "$ORIGIN")

cxxkit_generate_pkg_config(cxxkit_crash crash DESCRIPTION "cxxkit crash module (breakpad + backward-cpp backend)")
cxxkit_configure_library_end(CxxKitCrash)

set_property(TARGET cxxkit_crash PROPERTY FOLDER "CxxKit/libs")
    # vcpkg payload (D11): static archive + configs + runtime .so so installed consumers resolve
    # find_dependency(unofficial-breakpad / Backward) from cxxkitConfig.cmake and dlopen libdw.so.1.
    # Single-package unpack roots (QExt mechanism): one INSTALL_DIR per package.
    install(FILES "${CXXKitWrapCrashDepsBreakpad_INSTALL_DIR}/lib/libbreakpad_client.a"
        DESTINATION lib)
    install(DIRECTORY "${CXXKitWrapCrashDepsBreakpad_INSTALL_DIR}/share/unofficial-breakpad"
        DESTINATION lib/cmake)
    install(DIRECTORY "${CXXKitWrapCrashDepsBackward_INSTALL_DIR}/share/Backward"
        DESTINATION lib/cmake)
    if(NOT APPLE AND NOT WIN32)
        # Linux runtime payload for backward-cpp's dlopen("libdw.so.1") (elfutils/libunwind chain).
        file(GLOB _cxxkit_crash_so_payload
            "${CXXKitWrapCrashDepsElfUtils_INSTALL_DIR}/lib/libdw.so*"
            "${CXXKitWrapCrashDepsElfUtils_INSTALL_DIR}/lib/libelf.so*"
            "${CXXKitWrapCrashDepsLibUnwind_INSTALL_DIR}/lib/libunwind.so*"
            "${CXXKitWrapCrashDepsElfUtils_INSTALL_DIR}/lib/liblzma.so*")
        install(FILES ${_cxxkit_crash_so_payload}
            DESTINATION lib)
    endif()
endif()

# Build-tree runtime payload: copy the dlopen'ed .so next to the built crash lib so local tests and
# shared builds resolve libdw.so.1 at runtime (backward dlopens by name, RPATH \$ORIGIN applies).
if(NOT APPLE AND NOT WIN32)
    file(GLOB _cxxkit_crash_so_payload_build
        "${CXXKitWrapCrashDepsElfUtils_INSTALL_DIR}/lib/libdw.so*"
        "${CXXKitWrapCrashDepsElfUtils_INSTALL_DIR}/lib/libelf.so*"
        "${CXXKitWrapCrashDepsLibUnwind_INSTALL_DIR}/lib/libunwind.so*"
        "${CXXKitWrapCrashDepsElfUtils_INSTALL_DIR}/lib/liblzma.so*")
    add_custom_command(TARGET cxxkit_crash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_cxxkit_crash_so_payload_build}
        "$<TARGET_FILE_DIR:cxxkit_crash>"
        VERBATIM)
endif()
