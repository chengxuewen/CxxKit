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

# Ported from QExt cmake/InstallVcpkg.cmake (qext_vcpkg_install_package) and OpenCTK cmake/InstallVcpkg.cmake
# (octk_vcpkg_install_package), merged for CxxKit. General vcpkg infrastructure: install a package via vcpkg once,
# export it to a relocatable .7z archive in CXXKIT_3RDPARTY_PACKAGES_DIR, then every build consumes the archive by
# unpacking + find_package — no vcpkg toolchain needed at build time.
#
# Usage (from a FindWrap module or sublibrary CMakeLists):
#   include(InstallVcpkg)
#   cxxkit_vcpkg_install_package(breakpad TARGET CXXKitWrapBreakpad::WrapBreakpad
#       IMPORTED_TARGETS unofficial::breakpad::libbreakpad_client)
#
# Options:
#   NOT_IMPORT   - create the INTERFACE IMPORTED target without running find_package (caller imports itself)
#   TOOLS        - use the cloned vcpkg-tools copy (for tool packages, keeps the tools tree separate)
#   DYNAMIC      - use the dynamic triplet variant (default: static)
#   QUIET        - warn instead of FATAL on install failure
#   FALLBACK     - allow the slow path (git clone vcpkg + install + export + repack) when the .7z cache is
#                  missing. Default OFF: a missing archive is a FATAL_ERROR printing the exact commands to run.
#                  Keep OFF for version-pinned packages (silent version drift breaks same-source guarantees).
#
# One-value args: TARGET PREFIX OUTPUT_DIR PACK_NAME
# Multi-value args: COMPONENTS IMPORTED_TARGETS
#
# Inputs:
#   CXXKIT_3RDPARTY_PACKAGES_DIR - directory holding the .7z caches (INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR injected by
#                                  the parent project, defaults to ${PROJECT_SOURCE_DIR}/3rdparty)
#   CXXKIT_VCPKG_TRIPLET         - vcpkg triplet (default derived from CMAKE_SYSTEM_NAME + CMAKE_SYSTEM_PROCESSOR)
# Outputs (CACHE INTERNAL, consumed by later stages):
#   ${PREFIX}_FOUND, ${PREFIX}_ROOT_DIR, ${PREFIX}_INSTALL_DIR, ${PREFIX}_PACKAGE_NAME, ${PREFIX}_PACKAGE_PATH,
#   ${PREFIX}_INSTALLED


function(cxxkit_vcpkg_install)
    if(NOT DEFINED CXXKIT_TOP_LEVEL_SOURCE_DIR)
        set(CXXKIT_TOP_LEVEL_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
    endif()
    if(EXISTS "${CxxKitVcpkg_EXECUTABLE}" AND EXISTS "${CxxKitVcpkgTools_EXECUTABLE}")
        set(CxxKitVcpkg_FOUND ON)
        return()
    endif()

    set(CxxKitVcpkg_NAME "vcpkg")
    set(CxxKitVcpkg_ROOT_DIR "${CXXKIT_TOP_LEVEL_SOURCE_DIR}/vcpkg" CACHE INTERNAL "" FORCE)
    set(CxxKitVcpkg_INSTALL_DIR "${CxxKitVcpkg_ROOT_DIR}/installed" CACHE INTERNAL "" FORCE)
    set(CxxKitVcpkgTools_ROOT_DIR "${CXXKIT_TOP_LEVEL_SOURCE_DIR}/vcpkg-tools" CACHE INTERNAL "" FORCE)
    set(CxxKitVcpkgTools_INSTALL_DIR "${CxxKitVcpkgTools_ROOT_DIR}/installed" CACHE INTERNAL "" FORCE)
    find_package(Git REQUIRED)
    if(GIT_EXECUTABLE)
        if(WIN32)
            set(CxxKitVcpkg_EXECUTABLE_NAME "vcpkg.exe")
            set(CxxKitVcpkg_BOOTSTRAP_NAME "bootstrap-vcpkg.bat")
        else()
            set(CxxKitVcpkg_EXECUTABLE_NAME "vcpkg")
            set(CxxKitVcpkg_BOOTSTRAP_NAME "./bootstrap-vcpkg.sh")
        endif()
        if(NOT EXISTS "${CxxKitVcpkg_ROOT_DIR}/${CxxKitVcpkg_BOOTSTRAP_NAME}")
            if(EXISTS "${CxxKitVcpkg_ROOT_DIR}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CxxKitVcpkg_ROOT_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E echo "rmdir ${CxxKitVcpkg_ROOT_DIR}"
                    WORKING_DIRECTORY "${CXXKIT_TOP_LEVEL_SOURCE_DIR}"
                    RESULT_VARIABLE RMDIR_RESULT)
                if(NOT (RMDIR_RESULT MATCHES 0))
                    message(FATAL_ERROR "${CxxKitVcpkg_ROOT_DIR} dir remove failed.")
                endif()
            endif()
            message(STATUS "Start clone ${CxxKitVcpkg_NAME} in ${CXXKIT_TOP_LEVEL_SOURCE_DIR}.")
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" clone https://github.com/microsoft/vcpkg.git --depth 1
                WORKING_DIRECTORY "${CXXKIT_TOP_LEVEL_SOURCE_DIR}"
                RESULT_VARIABLE CLONE_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (CLONE_RESULT MATCHES 0))
                message(FATAL_ERROR "${CxxKitVcpkg_NAME} clone failed.")
            endif()
        endif()
        set(CxxKitVcpkg_EXECUTABLE "${CxxKitVcpkg_ROOT_DIR}/${CxxKitVcpkg_EXECUTABLE_NAME}" CACHE INTERNAL "" FORCE)
        if(NOT EXISTS "${CxxKitVcpkg_ROOT_DIR}/${CxxKitVcpkg_EXECUTABLE_NAME}")
            execute_process(
                COMMAND "${CxxKitVcpkg_BOOTSTRAP_NAME}"
                WORKING_DIRECTORY "${CxxKitVcpkg_ROOT_DIR}"
                RESULT_VARIABLE INIT_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (INIT_RESULT MATCHES 0))
                message(FATAL_ERROR "${CxxKitVcpkg_NAME} init failed.")
            endif()
        endif()
        if(NOT EXISTS "${CxxKitVcpkgTools_ROOT_DIR}/.git")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${CxxKitVcpkg_ROOT_DIR}" "${CxxKitVcpkgTools_ROOT_DIR}"
                WORKING_DIRECTORY "${CXXKIT_TOP_LEVEL_SOURCE_DIR}"
                RESULT_VARIABLE COPY_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (COPY_RESULT MATCHES 0))
                message(FATAL_ERROR "${CxxKitVcpkg_NAME} copy to vcpkg-tools failed.")
            endif()
        endif()
        set(CxxKitVcpkgTools_EXECUTABLE "${CxxKitVcpkgTools_ROOT_DIR}/${CxxKitVcpkg_EXECUTABLE_NAME}" CACHE INTERNAL "" FORCE)
    endif()
    set(CxxKitVcpkg_FOUND ON)
endfunction()


function(cxxkit_vcpkg_install_package NAME)
    cxxkit_parse_all_arguments(arg
        "cxxkit_vcpkg_install_package"
        "NOT_IMPORT;TOOLS;DYNAMIC;QUIET;FALLBACK"
        "TARGET;PREFIX;OUTPUT_DIR;PACK_NAME"
        "COMPONENTS;IMPORTED_TARGETS" ${ARGN})

    if("X${arg_TARGET}" STREQUAL "X")
        set(arg_TARGET ${NAME})
    endif()
    if("X${arg_PREFIX}" STREQUAL "X")
        string(REGEX REPLACE "[^a-zA-Z0-9]" "" arg_PREFIX "${arg_TARGET}")
    endif()
    if("X${arg_OUTPUT_DIR}" STREQUAL "X")
        set(arg_OUTPUT_DIR "${PROJECT_BINARY_DIR}/3rdparty/vcpkg")
    endif()
    if("X${arg_PACK_NAME}" STREQUAL "X")
        set(arg_PACK_NAME ${NAME})
    endif()
    if(TARGET ${arg_TARGET})
        set(${arg_PREFIX}_FOUND ON)
        return()
    endif()
    if(NOT DEFINED CXXKIT_VCPKG_TRIPLET)
        # Default triplet derived from the host (override with -DCXXKIT_VCPKG_TRIPLET=...)
        if(WIN32)
            set(CXXKIT_VCPKG_TRIPLET "x64-windows")
        elseif(APPLE)
            set(CXXKIT_VCPKG_TRIPLET "x64-osx")
        else()
            set(CXXKIT_VCPKG_TRIPLET "x64-linux")
        endif()
    endif()
    if(WIN32)
        if(${arg_DYNAMIC})
            set(${arg_PREFIX}_VCPKG_TRIPLET ${CXXKIT_VCPKG_TRIPLET})
        else()
            set(${arg_PREFIX}_VCPKG_TRIPLET ${CXXKIT_VCPKG_TRIPLET}-static-md)
        endif()
    else()
        if(${arg_DYNAMIC})
            set(${arg_PREFIX}_VCPKG_TRIPLET ${CXXKIT_VCPKG_TRIPLET}-dynamic)
        else()
            set(${arg_PREFIX}_VCPKG_TRIPLET ${CXXKIT_VCPKG_TRIPLET})
        endif()
    endif()
    set(${arg_PREFIX}_INSTALLED "OFF" CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_NAME "${arg_PACK_NAME}" CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_ROOT_DIR "${arg_OUTPUT_DIR}/${arg_PACK_NAME}"  CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_PACKAGE_NAME "${arg_PACK_NAME}-${${arg_PREFIX}_VCPKG_TRIPLET}.7z"  CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_PACKAGE_PATH "${CXXKIT_3RDPARTY_PACKAGES_DIR}/${${arg_PREFIX}_PACKAGE_NAME}"  CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_INSTALL_DIR "${${arg_PREFIX}_ROOT_DIR}/installed/${${arg_PREFIX}_VCPKG_TRIPLET}" CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_VCPKG_TOOLCHAIN_FILE "${${arg_PREFIX}_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE INTERNAL "" FORCE)
    if(NOT EXISTS "${${arg_PREFIX}_INSTALL_DIR}/include")
        if(EXISTS "${${arg_PREFIX}_PACKAGE_PATH}")
            message(STATUS "${${arg_PREFIX}_PACKAGE_NAME} exist, start unpack...")
            if(NOT EXISTS "${arg_OUTPUT_DIR}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${arg_OUTPUT_DIR}"
                    WORKING_DIRECTORY "${CxxKitVcpkg_ROOT_DIR}"
                    RESULT_VARIABLE MKDIR_RESULT)
                if(NOT MKDIR_RESULT MATCHES 0)
                    message(FATAL_ERROR "${arg_OUTPUT_DIR} mkdir failed.")
                endif()
            endif()
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzvf "${${arg_PREFIX}_PACKAGE_PATH}"
                WORKING_DIRECTORY "${arg_OUTPUT_DIR}"
                RESULT_VARIABLE UNPACK_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (UNPACK_RESULT MATCHES 0))
                message(FATAL_ERROR "${${arg_PREFIX}_NAME} unpack failed.")
            endif()
        elseif(arg_FALLBACK)
            cxxkit_vcpkg_install()
            if(${arg_TOOLS})
                set(Vcpkg_EXECUTABLE ${CxxKitVcpkgTools_EXECUTABLE})
                set(Vcpkg_ROOT_DIR ${CxxKitVcpkgTools_ROOT_DIR})
            else()
                set(Vcpkg_EXECUTABLE ${CxxKitVcpkg_EXECUTABLE})
                set(Vcpkg_ROOT_DIR ${CxxKitVcpkg_ROOT_DIR})
            endif()

            unset(${arg_PREFIX}_COMPONENTS_CONFIG)
            if("X${arg_COMPONENTS}" STREQUAL "X")
                set(${arg_PREFIX}_COMPONENTS_CONFIG "")
            else()
                set(${arg_PREFIX}_COMPONENTS_CONFIG "[")
                foreach(component IN LISTS arg_COMPONENTS)
                    if(NOT "${${arg_PREFIX}_COMPONENTS_CONFIG}" STREQUAL "[")
                        set(${arg_PREFIX}_COMPONENTS_CONFIG "${${arg_PREFIX}_COMPONENTS_CONFIG},")
                    endif()
                    set(${arg_PREFIX}_COMPONENTS_CONFIG "${${arg_PREFIX}_COMPONENTS_CONFIG}${component}")
                endforeach()
                set(${arg_PREFIX}_COMPONENTS_CONFIG "${${arg_PREFIX}_COMPONENTS_CONFIG}]")
            endif()
            set(${arg_PREFIX}_VCPKG_NAME ${NAME}${${arg_PREFIX}_COMPONENTS_CONFIG}:${${arg_PREFIX}_VCPKG_TRIPLET})
            execute_process(
                COMMAND ${Vcpkg_EXECUTABLE} list ${${arg_PREFIX}_VCPKG_NAME}
                WORKING_DIRECTORY "${Vcpkg_ROOT_DIR}"
                OUTPUT_VARIABLE FIND_OUTPUT
                RESULT_VARIABLE FIND_RESULT)
            string(FIND "${FIND_OUTPUT}" "${${arg_PREFIX}_VCPKG_NAME} " FOUND_POSITION)
            if(FOUND_POSITION EQUAL -1)
                message(STATUS "${${arg_PREFIX}_NAME} not installed, start install...")
                set(${arg_PREFIX}_VCPKG_CONFIGS ${NAME}${${arg_PREFIX}_COMPONENTS_CONFIG}:${${arg_PREFIX}_VCPKG_TRIPLET})
                message(STATUS "${${arg_PREFIX}_NAME} vcpkg install configs: ${${arg_PREFIX}_VCPKG_CONFIGS}")
                if(EXISTS ${ANDROID_NDK})
                    set(ENV{ANDROID_NDK_HOME} ${ANDROID_NDK})
                endif()
                execute_process(
                    COMMAND "${Vcpkg_EXECUTABLE}" install ${${arg_PREFIX}_VCPKG_CONFIGS} --recurse
                    WORKING_DIRECTORY "${Vcpkg_ROOT_DIR}"
                    RESULT_VARIABLE INSTALL_RESULT
                    COMMAND_ECHO STDOUT)
                if(NOT (INSTALL_RESULT MATCHES 0))
                    if(${arg_QUIET})
                        message(WARNING "${${arg_PREFIX}_NAME} install failed.")
                        return()
                    else()
                        message(FATAL_ERROR "${${arg_PREFIX}_NAME} install failed.")
                    endif()
                endif()
            endif()

            message(STATUS "${${arg_PREFIX}_NAME} not exported, start export...")
            execute_process(
                COMMAND "${Vcpkg_EXECUTABLE}" export ${NAME}:${${arg_PREFIX}_VCPKG_TRIPLET}
                --raw --output=${${arg_PREFIX}_NAME} --output-dir=${arg_OUTPUT_DIR}
                WORKING_DIRECTORY "${Vcpkg_ROOT_DIR}"
                RESULT_VARIABLE EXPORT_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (EXPORT_RESULT MATCHES 0))
                message(FATAL_ERROR "${${arg_PREFIX}_NAME} export failed.")
            endif()
        else()
            # FALLBACK OFF (default): never auto-install — version drift would silently break same-source
            # guarantees (e.g. QExt::Breakpad coexistence). Print the exact commands instead.
            message(FATAL_ERROR
                "${${arg_PREFIX}_PACKAGE_NAME} not found in ${CXXKIT_3RDPARTY_PACKAGES_DIR}.\n"
                "Run these commands once on a vcpkg-capable machine to produce the cache archive:\n"
                "  <vcpkg> install ${NAME}:${${arg_PREFIX}_VCPKG_TRIPLET} --recurse\n"
                "  <vcpkg> export ${NAME}:${${arg_PREFIX}_VCPKG_TRIPLET} --raw --output=${${arg_PREFIX}_NAME} --output-dir=${arg_OUTPUT_DIR}\n"
                "  cmake -E tar cvf \"${${arg_PREFIX}_PACKAGE_PATH}\" --format=7zip \"${${arg_PREFIX}_NAME}\"\n"
                "   (WORKING_DIRECTORY: ${arg_OUTPUT_DIR})\n"
                "Or pass FALLBACK to allow cxxkit_vcpkg_install_package to do this automatically.")
        endif()
    endif()

    if(NOT "X${CXXKIT_3RDPARTY_PACKAGES_DIR}" STREQUAL "X")
        if(NOT EXISTS "${${arg_PREFIX}_PACKAGE_PATH}")
            message(STATUS "${${arg_PREFIX}_PACKAGE_NAME} not exist, start pack...")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar cvf "${${arg_PREFIX}_PACKAGE_PATH}" --format=7zip "${${arg_PREFIX}_NAME}"
                WORKING_DIRECTORY "${arg_OUTPUT_DIR}"
                RESULT_VARIABLE PACK_RESULT
                COMMAND_ECHO STDOUT)
            if(NOT (PACK_RESULT MATCHES 0))
                message(FATAL_ERROR "${${arg_PREFIX}_NAME} pack failed.")
            endif()
        endif()
    endif()

    add_library(${arg_TARGET} INTERFACE IMPORTED GLOBAL)
    set_target_properties(${arg_TARGET} PROPERTIES FOLDER "CxxKit/3rdparty")
    if (NOT arg_NOT_IMPORT)
        message(STATUS "Start find and import ${${arg_PREFIX}_NAME}...")
        if(EXISTS "${${arg_PREFIX}_INSTALL_DIR}/share/${NAME}/Find${NAME}.cmake" OR
                EXISTS "${${arg_PREFIX}_INSTALL_DIR}/share/${NAME}/${NAME}Targets.cmake")
            set(CMAKE_MODULE_PATH_CACHE ${CMAKE_MODULE_PATH})
            set(CMAKE_MODULE_PATH "${${arg_PREFIX}_INSTALL_DIR}/share/${NAME}")
            set(${NAME}_DIR "${${arg_PREFIX}_INSTALL_DIR}/share/${NAME}")
            find_package(${NAME} REQUIRED)
            if(TARGET "${NAME}::${NAME}")
                target_link_libraries(${arg_TARGET} INTERFACE ${NAME}::${NAME})
            endif()
            target_link_libraries(${arg_TARGET} INTERFACE ${arg_IMPORTED_TARGETS})
            target_include_directories(${arg_TARGET} INTERFACE "${${arg_PREFIX}_INSTALL_DIR}/include")
            set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_CACHE})
        else()
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(${arg_PREFIX}_PKGCONFIG_DIR "${${arg_PREFIX}_INSTALL_DIR}/debug/lib")
            else()
                set(${arg_PREFIX}_PKGCONFIG_DIR "${${arg_PREFIX}_INSTALL_DIR}/lib")
            endif()
            find_package(PkgConfig)
            if(PKG_CONFIG_FOUND)
                if("X${arg_COMPONENTS}" STREQUAL "X")
                    set(ENV{PKG_CONFIG_PATH} "${${arg_PREFIX}_PKGCONFIG_DIR}/pkgconfig")
                    pkg_check_modules(${NAME} REQUIRED IMPORTED_TARGET ${NAME})
                else()
                    set(ENV{PKG_CONFIG_PATH} "${${arg_PREFIX}_PKGCONFIG_DIR}/pkgconfig")
                    pkg_check_modules(${NAME} REQUIRED IMPORTED_TARGET ${arg_COMPONENTS})
                endif()
                target_link_libraries(${arg_TARGET} INTERFACE PkgConfig::${NAME})
            else()
                message(FATAL_ERROR "${${arg_PREFIX}_NAME}: pkg-config not found and no CMake config in "
                    "${${arg_PREFIX}_INSTALL_DIR}/share/${NAME}.")
            endif()
        endif()
    endif()
    set(${arg_PREFIX}_INSTALLED ON CACHE INTERNAL "" FORCE)
    set(${arg_PREFIX}_FOUND ON PARENT_SCOPE)
endfunction()
