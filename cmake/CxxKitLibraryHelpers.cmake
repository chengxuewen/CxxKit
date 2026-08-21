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
        "EXCEPTIONS;INTERFACE;STATIC;SHARED;NO_ALIAS"
        "EXPORT_NAME;PRECOMPILED_HEADER;INSTALL_RPATH"
        "HEADERS;SOURCES;LIBRARIES;PUBLIC_LIBRARIES;INCLUDE_DIRECTORIES;FOLDER;COMPILE_DEFINITIONS" ${ARGN})

    # Resolve library type: explicit option wins, otherwise CXXKIT_BUILD_SHARED_LIBS decides.
    if(arg_INTERFACE)
        set(_cxxkit_type INTERFACE)
    elseif(arg_STATIC)
        set(_cxxkit_type STATIC)
    elseif(arg_SHARED)
        set(_cxxkit_type SHARED)
    elseif(CXXKIT_BUILD_SHARED_LIBS)
        set(_cxxkit_type SHARED)
    else()
        set(_cxxkit_type STATIC)
    endif()

    # Default headers: auto-GLOB *.hpp + detail/*.hpp of the current dir (D15).
    if("${arg_HEADERS}" STREQUAL "")
        file(GLOB _cxxkit_headers CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp")
    else()
        set(_cxxkit_headers ${arg_HEADERS})
    endif()

    if(_cxxkit_type STREQUAL "INTERFACE")
        add_library(${name} INTERFACE ${_cxxkit_headers})
    else()
        if("${arg_SOURCES}" STREQUAL "")
            message(FATAL_ERROR "cxxkit_add_library(${name}): no SOURCES given and not INTERFACE.")
        endif()
        add_library(${name} ${_cxxkit_type} ${arg_SOURCES} ${_cxxkit_headers})
    endif()


    # Include trio + C++ standard baseline (both compiled and INTERFACE targets).
    if(_cxxkit_type STREQUAL "INTERFACE")
        set(_cxxkit_vis INTERFACE)
    else()
        set(_cxxkit_vis PUBLIC)
    endif()
    target_include_directories(${name} ${_cxxkit_vis}
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>)
    target_compile_features(${name} ${_cxxkit_vis} cxx_std_11)

    if(NOT arg_NO_ALIAS)
        # Namespaced short alias: cxxkit_add_library(cxxkit_time ...) -> cxxkit::time (strip cxxkit_ prefix)
        # so consumers reference cxxkit::<sub> (abseil convention), not cxxkit::cxxkit_<sub>.
        string(REGEX REPLACE "^cxxkit_" "" _cxxkit_alias_name "${name}")
        add_library(cxxkit::${_cxxkit_alias_name} ALIAS ${name})
    endif()

    if(NOT _cxxkit_type STREQUAL "INTERFACE")
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

    # FOLDER: explicit arg wins, else derive from source-relative path (octk-style).
    if(NOT "${arg_FOLDER}" STREQUAL "")
        set(_cxxkit_folder "${arg_FOLDER}")
    elseif(_cxxkit_rel_dir MATCHES "^cxxkit/(.+)$")
        set(_cxxkit_folder "CxxKit/libs/${CMAKE_MATCH_1}")
    else()
        set(_cxxkit_folder "CxxKit/libs")
    endif()
    set_target_properties(${name} PROPERTIES FOLDER "${_cxxkit_folder}")

    # COMPILE_DEFINITIONS: optional sublib-specific definitions (e.g. CXXKIT_FEATURE_ENABLE_CRASH=1).
    # Visibility matches the include trio (PUBLIC for compiled, INTERFACE for header-only).
    if(NOT "${arg_COMPILE_DEFINITIONS}" STREQUAL "")
        target_compile_definitions(${name} ${_cxxkit_vis} ${arg_COMPILE_DEFINITIONS})
    endif()

    # EXPORT_NAME: explicit arg wins, else target name minus "cxxkit_" prefix.
    if(NOT "${arg_EXPORT_NAME}" STREQUAL "")
        set(_cxxkit_export_name "${arg_EXPORT_NAME}")
    else()
        string(REGEX REPLACE "^cxxkit_" "" _cxxkit_export_name "${name}")
    endif()
    set_target_properties(${name} PROPERTIES EXPORT_NAME "${_cxxkit_export_name}")

    # Per-lib shared/export gates (octk: octk_add_library injects these):
    #   CXXKIT_BUILDING_<SUB>_LIB  - set while compiling this lib so its API macro EXPORTs
    #   CXXKIT_BUILD_SHARED_<SUB> - set when shared build so <sub>_global.hpp enters the
    #                               EXPORT/IMPORT (dynamic) branch instead of the static (empty) one.
    # INTERFACE (header-only) libs need none (no symbols to export).
    if(NOT _cxxkit_type STREQUAL "INTERFACE")
        string(TOUPPER "${_cxxkit_export_name}" _cxxkit_export_upper)
        target_compile_definitions(${name} PRIVATE CXXKIT_BUILDING_${_cxxkit_export_upper}_LIB)
        if(CXXKIT_BUILD_SHARED_LIBS)
            target_compile_definitions(${name} PRIVATE CXXKIT_BUILD_SHARED_${_cxxkit_export_upper})
        endif()
    endif()

    if(TARGET ${name})
        target_compile_definitions(${name} ${_cxxkit_vis} ${CXXKIT_GLOBAL_COMPILE_DEFINITIONS})
    endif()
endfunction()
