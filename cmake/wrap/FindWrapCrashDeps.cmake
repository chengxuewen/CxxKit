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

# FindWrapCrashDeps — crash-deps consumer, pure QExt/OpenCTK mechanism.
#
# Each package is consumed by its own cxxkit_vcpkg_install_package() call (single-package .7z, exactly like
# QExt's qext_vcpkg_install_package(breakpad ...)): cache .7z present → unpack + find_package; missing →
# OpenCTK-style auto fallback (bootstrap vcpkg via cxxkit_vcpkg_install(), install, export, pack). No shell
# scripts, no merged archive — the generic InstallVcpkg function does everything.
#
# Packages:
#   breakpad       → unofficial::breakpad::libbreakpad_client  (target CXXKitWrapCrashDeps::WrapBreakpad)
#   backward-cpp   → Backward::Interface                        (target CXXKitWrapCrashDeps::WrapBackward)
#   elfutils       → runtime payload only (libdw.so.1 for backward's dlopen), NOT_IMPORT
#   libunwind      → runtime payload only, NOT_IMPORT
#
# Review gates kept: relocatability assertion (every resolved -I/-L inside the unpack root) and the PIC smoke
# (PIE link of libbreakpad_client.a). Same-source with QExt::Breakpad is achieved by caching preference:
# the shared INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR naturally contains QExt's breakpad-x64-linux.7z (2024-02-16).

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CXXKitWrapCrashDeps::WrapBreakpad)
    set(CXXKitWrapCrashDeps_FOUND ON)
    return()
endif()

# ---- consume each package (single-pack, NOT_IMPORT — we wire the real targets ourselves) ----------------------
# breakpad: archive may already exist in the shared packages dir (QExt's breakpad-x64-linux.7z, same source).
cxxkit_vcpkg_install_package(breakpad
    TARGET CXXKitWrapCrashDeps_BreakpadUnpack
    PREFIX CXXKitWrapCrashDepsBreakpad
    PACK_NAME breakpad
    NOT_IMPORT)
cxxkit_vcpkg_install_package(backward-cpp
    TARGET CXXKitWrapCrashDeps_BackwardUnpack
    PREFIX CXXKitWrapCrashDepsBackward
    PACK_NAME backward-cpp
    NOT_IMPORT)
# Runtime payloads for backward's dlopen("libdw.so.1") on Linux; no import needed. macOS/Windows skip.
if(NOT APPLE AND NOT WIN32)
    cxxkit_vcpkg_install_package(elfutils
        TARGET CXXKitWrapCrashDeps_ElfUtilsUnpack
        PREFIX CXXKitWrapCrashDepsElfUtils
        PACK_NAME elfutils
        NOT_IMPORT)
    cxxkit_vcpkg_install_package(libunwind
        TARGET CXXKitWrapCrashDeps_LibUnwindUnpack
        PREFIX CXXKitWrapCrashDepsLibUnwind
        PACK_NAME libunwind
        NOT_IMPORT)
endif()

# ---- import the two real packages ------------------------------------------------------------------------------
set(_cxxkit_crashdeps_breakpad_dir "${CXXKitWrapCrashDepsBreakpad_INSTALL_DIR}")
set(_cxxkit_crashdeps_backward_dir "${CXXKitWrapCrashDepsBackward_INSTALL_DIR}")
find_package(unofficial-breakpad PATHS "${_cxxkit_crashdeps_breakpad_dir}/share/unofficial-breakpad"
    NO_DEFAULT_PATH REQUIRED)
find_package(Backward PATHS "${_cxxkit_crashdeps_backward_dir}/share/Backward"
    NO_DEFAULT_PATH REQUIRED)

add_library(CXXKitWrapCrashDeps::WrapBreakpad INTERFACE IMPORTED GLOBAL)
target_link_libraries(CXXKitWrapCrashDeps::WrapBreakpad INTERFACE unofficial::breakpad::libbreakpad_client)
set_target_properties(CXXKitWrapCrashDeps::WrapBreakpad PROPERTIES FOLDER "CxxKit/3rdparty")

add_library(CXXKitWrapCrashDeps::WrapBackward INTERFACE IMPORTED GLOBAL)
target_link_libraries(CXXKitWrapCrashDeps::WrapBackward INTERFACE Backward::Interface)
set_target_properties(CXXKitWrapCrashDeps::WrapBackward PROPERTIES FOLDER "CxxKit/3rdparty")

# ---- relocatability gate: every resolved -I/-L must live inside its unpack root --------------------------------
foreach(_cxxkit_crashdeps_tgt IN ITEMS CXXKitWrapCrashDeps::WrapBreakpad CXXKitWrapCrashDeps::WrapBackward)
    if(_cxxkit_crashdeps_tgt STREQUAL "CXXKitWrapCrashDeps::WrapBreakpad")
        set(_cxxkit_crashdeps_root "${CXXKitWrapCrashDepsBreakpad_ROOT_DIR}")
    else()
        set(_cxxkit_crashdeps_root "${CXXKitWrapCrashDepsBackward_ROOT_DIR}")
    endif()
    get_target_property(_cxxkit_crashdeps_includes ${_cxxkit_crashdeps_tgt} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(_cxxkit_crashdeps_links    ${_cxxkit_crashdeps_tgt} INTERFACE_LINK_LIBRARIES)
    foreach(_cxxkit_crashdeps_inc IN LISTS _cxxkit_crashdeps_includes)
        if(NOT _cxxkit_crashdeps_inc STREQUAL "" AND NOT "${_cxxkit_crashdeps_inc}" MATCHES "^${_cxxkit_crashdeps_root}")
            message(FATAL_ERROR "crash-deps relocatability gate failed: include '${_cxxkit_crashdeps_inc}' "
                "lies outside the unpack root '${_cxxkit_crashdeps_root}'.")
        endif()
    endforeach()
    foreach(_cxxkit_crashdeps_link IN LISTS _cxxkit_crashdeps_links)
        if("${_cxxkit_crashdeps_link}" MATCHES "^(/|\\\\)" AND NOT "${_cxxkit_crashdeps_link}" MATCHES "^${_cxxkit_crashdeps_root}")
            message(FATAL_ERROR "crash-deps relocatability gate failed: absolute link path "
                "'${_cxxkit_crashdeps_link}' lies outside the unpack root '${_cxxkit_crashdeps_root}'.")
        endif()
    endforeach()
endforeach()
message(STATUS "crash-deps relocatability gate passed")

# ---- PIC smoke gate: a PIE executable must link the breakpad client archive ------------------------------------
get_target_property(_cxxkit_crashdeps_breakpad_loc unofficial::breakpad::libbreakpad_client IMPORTED_LOCATION)
if(NOT _cxxkit_crashdeps_breakpad_loc OR NOT EXISTS "${_cxxkit_crashdeps_breakpad_loc}")
    message(FATAL_ERROR "crash-deps: libbreakpad_client archive not found via unofficial-breakpad config.")
endif()
set(_cxxkit_crashdeps_smoke_dir "${CXXKitWrapCrashDepsBreakpad_ROOT_DIR}/pic-smoke")
file(MAKE_DIRECTORY "${_cxxkit_crashdeps_smoke_dir}")
file(WRITE "${_cxxkit_crashdeps_smoke_dir}/main.c" "int main(void){return 0;}\n")
try_compile(CXXKitWrapCrashDeps_PIC_SMOKE_PASSED
    "${_cxxkit_crashdeps_smoke_dir}"
    "${_cxxkit_crashdeps_smoke_dir}/main.c"
    CMAKE_FLAGS
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    LINK_LIBRARIES "${_cxxkit_crashdeps_breakpad_loc}" "-pthread")
if(NOT CXXKitWrapCrashDeps_PIC_SMOKE_PASSED)
    message(FATAL_ERROR "crash-deps PIC smoke failed: libbreakpad_client.a is not PIC-compatible. "
        "Re-export with a PIC triplet (set VCPKG_CMAKE_C/CXX_FLAGS '-fPIC' in the triplet).")
endif()
message(STATUS "crash-deps PIC smoke passed (${_cxxkit_crashdeps_breakpad_loc})")

set(CXXKitWrapCrashDeps_FOUND ON)