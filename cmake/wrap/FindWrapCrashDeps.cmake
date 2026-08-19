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

# FindWrapCrashDeps — crash-deps merged archive consumer (breakpad + backward-cpp + elfutils + libunwind).
#
# Consumes the single merged archive crash-deps-<triplet>.7z (produced by scripts/export_crash_deps.sh) via
# cxxkit_vcpkg_install_package (cmake/InstallVcpkg.cmake, NOT_IMPORT — we wire the real targets ourselves),
# with three review-mandated gates:
#   1. version gate        — sidecar crash-deps-<triplet>.7z.version must contain the expected four versions
#   2. relocatability gate — every resolved -I/-L path must lie inside the unpack root
#   3. PIC smoke gate      — a PIE executable must link libbreakpad_client.a (re-export with the -fPIC overlay
#                            triplet if this fails)
#
# FALLBACK is OFF: a missing archive is a FATAL with export instructions (same-source guarantee vs QExt::Breakpad).
#
# Triplet notes: the archive is named by the STANDARD triplet (crash-deps-x64-linux.7z) while the installed tree
# inside carries the OVERLAY triplet (installed/x64-linux-cxxkit/, -fPIC). CXXKIT_VCPKG_TRIPLET must therefore be
# the standard triplet for the helper's archive naming; the real tree is located by marker glob and the
# CXXKitWrapCrashDeps_INSTALL_DIR cache is overridden so later configures skip re-unpacking.
#
# Provides:
#   CXXKitWrapCrashDeps::WrapBreakpad  → unofficial::breakpad::libbreakpad_client
#   CXXKitWrapCrashDeps::WrapBackward  → Backward::Interface

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CXXKitWrapCrashDeps::WrapBreakpad)
    set(CXXKitWrapCrashDeps_FOUND ON)
    return()
endif()

# ---- standard triplet map (scripts/export_crash_deps.sh OUT_NAME convention) ----------------------------------
if(WIN32)
    set(CXXKitWrapCrashDeps_TRIPLET "x64-windows")
elseif(APPLE)
    set(CXXKitWrapCrashDeps_TRIPLET "x64-osx")
else()
    set(CXXKitWrapCrashDeps_TRIPLET "x64-linux")
endif()
if(NOT DEFINED CXXKIT_VCPKG_TRIPLET)
    set(CXXKIT_VCPKG_TRIPLET "${CXXKitWrapCrashDeps_TRIPLET}")
endif()

set(_cxxkit_crashdeps_archive "${CXXKIT_3RDPARTY_PACKAGES_DIR}/crash-deps-${CXXKitWrapCrashDeps_TRIPLET}.7z")
if(NOT EXISTS "${_cxxkit_crashdeps_archive}")
    # OpenCTK-style full auto path: cxxkit_vcpkg_install() bootstraps vcpkg itself (clone + bootstrap)
    # into ${CXXKIT_TOP_LEVEL_SOURCE_DIR}/vcpkg when missing — no manual vcpkg install needed.
    if(NOT EXISTS "${CxxKitVcpkg_EXECUTABLE}")
        message(STATUS "vcpkg missing — bootstrapping via cxxkit_vcpkg_install()...")
        cxxkit_vcpkg_install()
    endif()
    # The merged archive can only be produced by scripts/export_crash_deps.sh (manifest-pinned versions,
    # -fPIC overlay triplet, .version sidecar); a bare vcpkg install would drift versions and break the
    # same-source guarantee vs QExt::Breakpad, so we run the script against the bootstrapped vcpkg.
    message(STATUS "crash-deps archive missing — auto-running scripts/export_crash_deps.sh (vcpkg install + export)...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env VCPKG_ROOT=${CxxKitVcpkg_ROOT_DIR} bash "${PROJECT_SOURCE_DIR}/scripts/export_crash_deps.sh" "" "crash-deps-${CXXKitWrapCrashDeps_TRIPLET}" "${CXXKIT_3RDPARTY_PACKAGES_DIR}"
        RESULT_VARIABLE _cxxkit_crashdeps_export_result
        COMMAND_ECHO STDOUT)
    if(NOT _cxxkit_crashdeps_export_result MATCHES 0)
        message(FATAL_ERROR "crash-deps auto-export failed (exit ${_cxxkit_crashdeps_export_result}). "
            "Check network/vcpkg bootstrap logs above, or produce the archive manually per scripts/export_crash_deps.sh.")
    endif()
endif()
# ---- consume the merged archive: single unpack, NOT_IMPORT (we create the two real targets below) ------------
# PREFIX pinned to CXXKitWrapCrashDeps so the cache vars (…_ROOT_DIR/…_INSTALL_DIR/…_PACKAGE_PATH) are stable.
# The dummy target CXXKitWrapCrashDeps_Unpack is the helper's own creation; it stays unused.
cxxkit_vcpkg_install_package(crash-deps
    TARGET CXXKitWrapCrashDeps_Unpack
    PREFIX CXXKitWrapCrashDeps
    PACK_NAME crash-deps
    NOT_IMPORT)

# ---- locate the real installed tree by marker (archive tree carries the overlay triplet) ---------------------
set(_cxxkit_crashdeps_install_dir "${CXXKitWrapCrashDeps_INSTALL_DIR}")
if(NOT EXISTS "${_cxxkit_crashdeps_install_dir}/share/unofficial-breakpad/unofficial-breakpadConfig.cmake"
   AND NOT EXISTS "${_cxxkit_crashdeps_install_dir}/share/unofficial-breakpad/unofficial-breakpadTargets.cmake")
    file(GLOB_RECURSE _cxxkit_crashdeps_marker
        "${CXXKitWrapCrashDeps_ROOT_DIR}/installed/*/share/unofficial-breakpad/*Config.cmake")
    if(NOT _cxxkit_crashdeps_marker)
        message(FATAL_ERROR "crash-deps archive unpacked but unofficial-breakpad config not found under "
            "${CXXKitWrapCrashDeps_ROOT_DIR}/installed/.")
    endif()
    list(LENGTH _cxxkit_crashdeps_marker _cxxkit_crashdeps_marker_count)
    if(_cxxkit_crashdeps_marker_count GREATER 1)
        message(FATAL_ERROR "crash-deps: multiple unofficial-breakpad configs found under "
            "${CXXKitWrapCrashDeps_ROOT_DIR}/installed/ — stale unpack? Clean the directory and reconfigure.")
    endif()
    get_filename_component(_cxxkit_crashdeps_marker_dir "${_cxxkit_crashdeps_marker}" DIRECTORY)
    get_filename_component(_cxxkit_crashdeps_share_dir "${_cxxkit_crashdeps_marker_dir}" DIRECTORY)
    get_filename_component(_cxxkit_crashdeps_install_dir "${_cxxkit_crashdeps_share_dir}" DIRECTORY)
    # Pin the cache so the helper's include-dir check passes on later configures (no re-unpack).
    set(CXXKitWrapCrashDeps_INSTALL_DIR "${_cxxkit_crashdeps_install_dir}" CACHE INTERNAL "" FORCE)
endif()
message(STATUS "crash-deps installed tree: ${_cxxkit_crashdeps_install_dir}")

# ---- version gate (same-source contract with QExt's breakpad 2024-02-16 export) -------------------------------
set(_cxxkit_crashdeps_expected
    "breakpad=2024-02-16"
    "backward-cpp=2023-11-24"
    "elfutils=0.195"
    "libunwind=1.8.3")
set(_cxxkit_crashdeps_version_file
    "${CXXKIT_3RDPARTY_PACKAGES_DIR}/crash-deps-${CXXKitWrapCrashDeps_TRIPLET}.7z.version")
if(NOT EXISTS "${_cxxkit_crashdeps_version_file}")
    message(FATAL_ERROR "crash-deps version sidecar not found: ${_cxxkit_crashdeps_version_file} — "
        "run scripts/export_crash_deps.sh first.")
endif()
file(STRINGS "${_cxxkit_crashdeps_version_file}" _cxxkit_crashdeps_actual)
foreach(_cxxkit_crashdeps_line IN LISTS _cxxkit_crashdeps_expected)
    list(FIND _cxxkit_crashdeps_actual "${_cxxkit_crashdeps_line}" _cxxkit_crashdeps_idx)
    if(_cxxkit_crashdeps_idx EQUAL -1)
        message(FATAL_ERROR "crash-deps version mismatch: '${_cxxkit_crashdeps_line}' not in "
            "${_cxxkit_crashdeps_version_file} (found: ${_cxxkit_crashdeps_actual}). "
            "Re-run scripts/export_crash_deps.sh — must match QExt's same-source breakpad version.")
    endif()
endforeach()
message(STATUS "crash-deps version gate passed (${_cxxkit_crashdeps_expected})")

# ---- import the two real packages ------------------------------------------------------------------------------
find_package(unofficial-breakpad PATHS "${_cxxkit_crashdeps_install_dir}/share/unofficial-breakpad"
    NO_DEFAULT_PATH REQUIRED)
find_package(Backward PATHS "${_cxxkit_crashdeps_install_dir}/share/Backward"
    NO_DEFAULT_PATH REQUIRED)

add_library(CXXKitWrapCrashDeps::WrapBreakpad INTERFACE IMPORTED GLOBAL)
target_link_libraries(CXXKitWrapCrashDeps::WrapBreakpad INTERFACE unofficial::breakpad::libbreakpad_client)
set_target_properties(CXXKitWrapCrashDeps::WrapBreakpad PROPERTIES FOLDER "CxxKit/3rdparty")

add_library(CXXKitWrapCrashDeps::WrapBackward INTERFACE IMPORTED GLOBAL)
target_link_libraries(CXXKitWrapCrashDeps::WrapBackward INTERFACE Backward::Interface)
set_target_properties(CXXKitWrapCrashDeps::WrapBackward PROPERTIES FOLDER "CxxKit/3rdparty")

# ---- relocatability gate: every resolved -I/-L must live inside the unpack root --------------------------------
set(_cxxkit_crashdeps_root "${CXXKitWrapCrashDeps_ROOT_DIR}")
foreach(_cxxkit_crashdeps_tgt IN ITEMS CXXKitWrapCrashDeps::WrapBreakpad CXXKitWrapCrashDeps::WrapBackward)
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
set(_cxxkit_crashdeps_smoke_dir "${CXXKitWrapCrashDeps_ROOT_DIR}/pic-smoke")
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
        "Re-export with the -fPIC overlay triplet (scripts/crash-deps/x64-linux-cxxkit.cmake).")
endif()
message(STATUS "crash-deps PIC smoke passed (${_cxxkit_crashdeps_breakpad_loc})")

set(CXXKitWrapCrashDeps_FOUND ON)
