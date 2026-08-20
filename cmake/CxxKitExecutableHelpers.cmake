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

function(cxxkit_add_executable name)
    cxxkit_parse_all_arguments(arg "cxxkit_add_executable"
        "EXCEPTIONS;WIN32;MACOSX_BUNDLE"
        ""
        "SOURCES;INCLUDE_DIRECTORIES;LIBRARIES;FOLDER" ${ARGN})

    if("${arg_SOURCES}" STREQUAL "")
        message(FATAL_ERROR "cxxkit_add_executable(${name}): no SOURCES given.")
    endif()

    set(_cxxkit_exe_flags "")
    if(arg_WIN32)
        list(APPEND _cxxkit_exe_flags WIN32)
    endif()
    if(arg_MACOSX_BUNDLE)
        list(APPEND _cxxkit_exe_flags MACOSX_BUNDLE)
    endif()
    add_executable(${name} ${_cxxkit_exe_flags} ${arg_SOURCES})

    set_target_properties(${name} PROPERTIES
        CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if(NOT "${arg_INCLUDE_DIRECTORIES}" STREQUAL "")
        target_include_directories(${name} PRIVATE ${arg_INCLUDE_DIRECTORIES})
    endif()
    if(NOT "${arg_LIBRARIES}" STREQUAL "")
        target_link_libraries(${name} PRIVATE ${arg_LIBRARIES})
    endif()
    if(arg_EXCEPTIONS)
        if(MSVC)
            target_compile_options(${name} PRIVATE /EHsc)
        endif()
    endif()

    # FOLDER: explicit arg wins, else derive from source-relative path (octk-style).
    if(NOT "${arg_FOLDER}" STREQUAL "")
        set(_cxxkit_folder "${arg_FOLDER}")
    else()
        file(RELATIVE_PATH _cxxkit_rel_dir "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        if("${_cxxkit_rel_dir}" MATCHES "^examples(/.*)?$")
            set(_cxxkit_folder "CxxKit/examples${CMAKE_MATCH_1}")
        elseif("${_cxxkit_rel_dir}" MATCHES "^tests(/.*)?$")
            set(_cxxkit_folder "CxxKit/tests${CMAKE_MATCH_1}")
        elseif("${_cxxkit_rel_dir}" STREQUAL "")
            set(_cxxkit_folder "CxxKit/apps")
        else()
            set(_cxxkit_folder "CxxKit/apps/${_cxxkit_rel_dir}")
        endif()
    endif()
    set_target_properties(${name} PROPERTIES FOLDER "${_cxxkit_folder}")
endfunction()