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
# Provides: CxxKitWrapBreakpad::WrapBreakpad → unofficial::breakpad::libbreakpad_client
# Gates: relocatability (resolved -I/-L inside the unpack root) + PIC smoke (PIE link of the client archive).

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET CxxKitWrapBreakpad::WrapBreakpad)
    set(CxxKitWrapBreakpad_FOUND ON)
    return()
endif()

include(InstallVcpkg)
list(APPEND CxxKitWrapBreakpad_COMPONENTS core)
if(LINUX)
    list(APPEND CxxKitWrapBreakpad_COMPONENTS tools)
endif()
cxxkit_vcpkg_install_package(breakpad
    TARGET CxxKitWrapBreakpad::WrapBreakpad   # helper creates the empty INTERFACE IMPORTED target (QExt style)
    COMPONENTS ${CxxKitWrapBreakpad_COMPONENTS}
    PREFIX CxxKitWrapBreakpad
    PACK_NAME breakpad
    NOT_IMPORT)
find_package(unofficial-breakpad PATHS "${CxxKitWrapBreakpad_INSTALL_DIR}/share/unofficial-breakpad"
    NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapBreakpad::WrapBreakpad INTERFACE unofficial::breakpad::libbreakpad_client)
target_include_directories(CxxKitWrapBreakpad::WrapBreakpad INTERFACE "${CxxKitWrapBreakpad_INSTALL_DIR}/include")
set_target_properties(CxxKitWrapBreakpad::WrapBreakpad PROPERTIES FOLDER "CxxKit/3rdparty")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH_CACHE})
set(CxxKitWrapBreakpad_TOOLS_DIR "${CxxKitWrapBreakpad_INSTALL_DIR}/tools/breakpad" CACHE INTERNAL "" FORCE)
set(CxxKitWrapBreakpad_TOOLS_PACKAGE_DIR "${CXXKIT_3RDPARTY_PACKAGES_DIR}/breakpad-tools-${CXXKIT_HOST_PLATFORM_NAME}.7z" CACHE INTERNAL "" FORCE)
if(EXISTS "${CxxKitWrapBreakpad_TOOLS_DIR}")
    if(NOT "X${CXXKIT_3RDPARTY_PACKAGES_DIR}" STREQUAL "X")
        if(NOT EXISTS "${CxxKitWrapBreakpad_TOOLS_PACKAGE_DIR}")
            message(STATUS "${CxxKitWrapBreakpad_TOOLS_PACKAGE_DIR} not exist, start pack...")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar cvf "${CxxKitWrapBreakpad_TOOLS_PACKAGE_DIR}" --format=7zip "${CxxKitWrapBreakpad_TOOLS_DIR}"
                WORKING_DIRECTORY "${CxxKitWrapBreakpad_INSTALL_DIR}/tools"
                RESULT_VARIABLE PACK_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (PACK_RESULT MATCHES 0))
                message(FATAL_ERROR "${CxxKitWrapBreakpad_TOOLS_PKG_NAME} pack failed.")
            endif()
        endif()
    endif()
endif()
set(CxxKitWrapBreakpad_FOUND ON)