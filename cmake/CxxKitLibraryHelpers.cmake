########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Library helper: cxxkit_add_library — thin wrapper over native add_library that:
#   - creates the target with sources/libraries/include dirs
#   - creates a namespaced alias (cxxkit::<name>)
#   - attaches C++ standard (default CXXKIT_FEATURE_CXX_STANDARD)
#   - handles EXCEPTIONS flag on MSVC (/EHsc)
#   - attaches global compile definitions
# Slimmed from OpenCTK OpenCTKLibraryHelpers.cmake.
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
