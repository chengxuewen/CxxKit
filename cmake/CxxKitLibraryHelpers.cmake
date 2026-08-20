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

function(cxxkit_add_library name)
    cxxkit_parse_all_arguments(arg "cxxkit_add_library"
        "EXCEPTIONS;INTERFACE"
        ""
        "SOURCES;LIBRARIES;PUBLIC_LIBRARIES;INCLUDE_DIRECTORIES;PRECOMPILED_HEADER" ${ARGN})

    if(arg_INTERFACE)
        add_library(${name} INTERFACE)
    else()
        if("${arg_SOURCES}" STREQUAL "")
            message(FATAL_ERROR "cxxkit_add_library(${name}): no SOURCES given and not INTERFACE.")
        endif()
        add_library(${name} ${arg_SOURCES})
    endif()

    # Namespaced alias
    add_library(cxxkit::${name} ALIAS ${name})

    if(NOT arg_INTERFACE)
        if(NOT "${arg_INCLUDE_DIRECTORIES}" STREQUAL "")
            target_include_directories(${name} PUBLIC ${arg_INCLUDE_DIRECTORIES})
        endif()
        if(NOT "${arg_LIBRARIES}" STREQUAL "")
            target_link_libraries(${name} PRIVATE ${arg_LIBRARIES})
        endif()
        if(NOT "${arg_PUBLIC_LIBRARIES}" STREQUAL "")
            target_link_libraries(${name} PUBLIC ${arg_PUBLIC_LIBRARIES})
        endif()
        set_target_properties(${name} PROPERTIES
            CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF)
        if(arg_EXCEPTIONS)
            if(MSVC)
                target_compile_options(${name} PRIVATE /EHsc)
            endif()
        endif()
        if(NOT "${arg_PRECOMPILED_HEADER}" STREQUAL "" AND CXXKIT_BUILD_USE_PCH)
            target_precompile_headers(${name} PRIVATE ${arg_PRECOMPILED_HEADER})
        endif()
    endif()

    if(TARGET ${name})
        target_compile_definitions(${name} PUBLIC ${CXXKIT_GLOBAL_COMPILE_DEFINITIONS})
    endif()
endfunction()
