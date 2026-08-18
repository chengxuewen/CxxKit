########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Configure helpers: compile-time macro generation via configure_file.
# Slimmed from OpenCTK OpenCTKConfigureHelpers.cmake (configure_definition only).
#
########################################################################################################################

# cxxkit_configure_definition(NAME [VALUE value])
# Registers a compile definition on the cxxkit_global_compile_definitions list (consumed by
# cxxkit_configure_library_end to attach to every library target).
set(CXXKIT_GLOBAL_COMPILE_DEFINITIONS "")

function(cxxkit_configure_definition name)
    cxxkit_parse_all_arguments(arg "cxxkit_configure_definition" "PUBLIC;PRIVATE" "VALUE" "" ${ARGN})
    if(NOT "${arg_VALUE}" STREQUAL "")
        set(def "${name}=${arg_VALUE}")
    else()
        set(def "${name}")
    endif()
    set(CXXKIT_GLOBAL_COMPILE_DEFINITIONS ${CXXKIT_GLOBAL_COMPILE_DEFINITIONS} "${def}" CACHE INTERNAL "" FORCE)
endfunction()

function(cxxkit_configure_library_begin name)
    set(CXXKIT_CURRENT_LIBRARY "${name}" CACHE INTERNAL "" FORCE)
endfunction()

function(cxxkit_configure_library_end name)
    # Attach global compile definitions to the library target and its alias
    if(TARGET "${name}")
        target_compile_definitions(${name} PUBLIC ${CXXKIT_GLOBAL_COMPILE_DEFINITIONS})
    endif()
endfunction()
