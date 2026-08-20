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

# FindWrapBreakpad — Google breakpad client via cxxkit_vcpkg_install_package (QExt mechanism, one wrap per package).
#
# Consumes the standard breakpad-<triplet>.7z: cache present → unpack + find_package(unofficial-breakpad);
# missing → OpenCTK-style auto fallback (bootstrap vcpkg, install, export, pack). The shared
# INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR naturally holds QExt's breakpad-x64-linux.7z (2024-02-16) — same source.
#
# Provides: CXXKitWrapBreakpad::WrapBreakpad → unofficial::breakpad::libbreakpad_client
# Gates: relocatability (resolved -I/-L inside the unpack root) + PIC smoke (PIE link of the client archive).

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CXXKitWrapBreakpad::WrapBreakpad)
    set(CXXKitWrapBreakpad_FOUND ON)
    return()
endif()

cxxkit_vcpkg_install_package(breakpad
    TARGET CXXKitWrapBreakpad::WrapBreakpad   # helper creates the empty INTERFACE IMPORTED target (QExt style)
    PREFIX CXXKitWrapBreakpad
    PACK_NAME breakpad
    NOT_IMPORT)

set(_cxxkit_wrap_breakpad_install_dir "${CXXKitWrapBreakpad_INSTALL_DIR}")

find_package(unofficial-breakpad PATHS "${_cxxkit_wrap_breakpad_install_dir}/share/unofficial-breakpad"
    NO_DEFAULT_PATH REQUIRED)

target_link_libraries(CXXKitWrapBreakpad::WrapBreakpad INTERFACE unofficial::breakpad::libbreakpad_client)
target_include_directories(CXXKitWrapBreakpad::WrapBreakpad INTERFACE "${_cxxkit_wrap_breakpad_install_dir}/include")
set_target_properties(CXXKitWrapBreakpad::WrapBreakpad PROPERTIES FOLDER "CxxKit/3rdparty")

# ---- relocatability gate -----------------------------------------------------------------------------------------
get_target_property(_cxxkit_wrap_breakpad_includes CXXKitWrapBreakpad::WrapBreakpad INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(_cxxkit_wrap_breakpad_links    CXXKitWrapBreakpad::WrapBreakpad INTERFACE_LINK_LIBRARIES)
foreach(_cxxkit_wrap_breakpad_inc IN LISTS _cxxkit_wrap_breakpad_includes)
    if(NOT _cxxkit_wrap_breakpad_inc STREQUAL "" AND NOT "${_cxxkit_wrap_breakpad_inc}" MATCHES "^${CXXKitWrapBreakpad_ROOT_DIR}")
        message(FATAL_ERROR "crash-deps relocatability gate failed: include '${_cxxkit_wrap_breakpad_inc}' "
            "lies outside the unpack root '${CXXKitWrapBreakpad_ROOT_DIR}'.")
    endif()
endforeach()
foreach(_cxxkit_wrap_breakpad_link IN LISTS _cxxkit_wrap_breakpad_links)
    if("${_cxxkit_wrap_breakpad_link}" MATCHES "^(/|\\\\)" AND NOT "${_cxxkit_wrap_breakpad_link}" MATCHES "^${CXXKitWrapBreakpad_ROOT_DIR}")
        message(FATAL_ERROR "crash-deps relocatability gate failed: absolute link path "
            "'${_cxxkit_wrap_breakpad_link}' lies outside the unpack root '${CXXKitWrapBreakpad_ROOT_DIR}'.")
    endif()
endforeach()
message(STATUS "crash-deps relocatability gate passed (breakpad)")

# ---- PIC smoke gate: a PIE executable must link the breakpad client archive --------------------------------------
get_target_property(_cxxkit_wrap_breakpad_loc unofficial::breakpad::libbreakpad_client IMPORTED_LOCATION)
if(NOT _cxxkit_wrap_breakpad_loc OR NOT EXISTS "${_cxxkit_wrap_breakpad_loc}")
    message(FATAL_ERROR "crash-deps: libbreakpad_client archive not found via unofficial-breakpad config.")
endif()
set(_cxxkit_wrap_breakpad_smoke_dir "${CXXKitWrapBreakpad_ROOT_DIR}/pic-smoke")
file(MAKE_DIRECTORY "${_cxxkit_wrap_breakpad_smoke_dir}")
file(WRITE "${_cxxkit_wrap_breakpad_smoke_dir}/main.c" "int main(void){return 0;}\n")
try_compile(CXXKitWrapBreakpad_PIC_SMOKE_PASSED
    "${_cxxkit_wrap_breakpad_smoke_dir}"
    "${_cxxkit_wrap_breakpad_smoke_dir}/main.c"
    CMAKE_FLAGS
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
    LINK_LIBRARIES "${_cxxkit_wrap_breakpad_loc}" "-pthread")
if(NOT CXXKitWrapBreakpad_PIC_SMOKE_PASSED)
    message(FATAL_ERROR "crash-deps PIC smoke failed: libbreakpad_client.a is not PIC-compatible. "
        "Re-export with a PIC triplet (set VCPKG_CMAKE_C/CXX_FLAGS '-fPIC' in the triplet).")
endif()
message(STATUS "crash-deps PIC smoke passed (${_cxxkit_wrap_breakpad_loc})")

set(CXXKitWrapBreakpad_FOUND ON)