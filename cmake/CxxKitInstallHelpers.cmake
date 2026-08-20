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

function(cxxkit_install)
    if(CXXKIT_BUILD_INSTALL)
        install(${ARGV})
    endif()
endfunction()

# cxxkit_install_public_wrap_headers(target WRAPS wrap_target|shortname ...)
# Copies vendored 3rdparty headers into the cxxkit namespace (<cxxkit/3rdparty/<shortname>/...>)
# for both the build tree and install prefix. This is the octk mechanism to avoid
# conflicts with system-installed libraries (e.g. <cxxkit/3rdparty/fmt/format.h> vs <fmt/format.h>).
# Ported from OpenCTK OpenCTKPublicWrapHelpers.cmake.
function(cxxkit_install_public_wrap_headers target)
    cmake_parse_arguments(arg "" "" "WRAPS" ${ARGN})
    foreach(pair ${arg_WRAPS})
        # Parse wrap_target|shortname (using | as separator to avoid CMake list splitting)
        string(REPLACE "|" ";" pair_list ${pair})
        list(GET pair_list 0 wrap_target)
        list(GET pair_list 1 shortname)
        # Extract namespace prefix for _INSTALL_DIR variable
        string(REGEX REPLACE "::.*$" "" wrap_prefix ${wrap_target})
        set(install_dir_var "${wrap_prefix}_INSTALL_DIR")
        if(NOT ${install_dir_var})
            message(WARNING "cxxkit_install_public_wrap_headers: ${install_dir_var} not set for ${wrap_target}")
            continue()
        endif()
        set(wrap_install_dir "${${install_dir_var}}")
        set(wrap_include_dir "${wrap_install_dir}/include")
        if(EXISTS "${wrap_include_dir}")
            # Build tree copy: <build>/include/cxxkit/3rdparty/<shortname>/...
            file(COPY "${wrap_include_dir}/" DESTINATION "${PROJECT_BINARY_DIR}/include/cxxkit/3rdparty")
        endif()
        # Install tree copy: <prefix>/include/cxxkit/3rdparty/<shortname>/...
        install(DIRECTORY "${wrap_include_dir}/"
            DESTINATION "include/cxxkit/3rdparty"
            FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.h"
            PATTERN "*.hh")
        # Include path points to the ROOT, so <cxxkit/3rdparty/fmt/os.h> resolves correctly
        target_include_directories(${wrap_target} INTERFACE
            "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
            "$<INSTALL_INTERFACE:include>")
        # Bake into main target for find_package export
        target_include_directories(${target} INTERFACE
            "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
            "$<INSTALL_INTERFACE:include>")
        # Install static libraries alongside headers so installed consumers can link
        # against the vendored 3rdparty libs (M3: static libs ship with the package).
        if(EXISTS "${wrap_install_dir}/lib")
            file(GLOB _wrap_libs "${wrap_install_dir}/lib/*.a" "${wrap_install_dir}/lib/*.lib")
            if(_wrap_libs)
                install(FILES ${_wrap_libs}
                    DESTINATION lib)
            endif()
        endif()
        # vcpkg-style: install the 3rdparty's own CMake config + pkgconfig files so
        # consumers can find_dependency(<lib>) with full transitive chains
        # (e.g. spdlogConfig -> find_dependency(fmt), cprConfig -> find_dependency(CURL)).
        if(EXISTS "${wrap_install_dir}/lib/cmake")
            file(GLOB _wrap_cmake_dirs "${wrap_install_dir}/lib/cmake/*")
            foreach(_cmake_dir IN LISTS _wrap_cmake_dirs)
                get_filename_component(_cmake_name "${_cmake_dir}" NAME)
                install(DIRECTORY "${_cmake_dir}/"
                    DESTINATION "lib/cmake/${_cmake_name}")
            endforeach()
        endif()
        if(EXISTS "${wrap_install_dir}/lib/pkgconfig")
            install(DIRECTORY "${wrap_install_dir}/lib/pkgconfig/"
                DESTINATION "lib/pkgconfig")
        endif()
        # Executables shipped by a 3rdparty (e.g. curl CLI referenced by CURLConfig)
        if(EXISTS "${wrap_install_dir}/bin")
            install(DIRECTORY "${wrap_install_dir}/bin/"
                DESTINATION "bin")
        endif()
    endforeach()
endfunction()
