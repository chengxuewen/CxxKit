########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# PkgConfig generation: cxxkit_generate_pkg_config(target) writes <prefix>/lib/pkgconfig/cxxkit-<name>.pc
# for sublibraries (compiled and header-only). Static-build friendly.
# Slimmed from OpenCTK OpenCTKPkgConfigHelpers.cmake (no walk-libs machinery).
#
########################################################################################################################

# cxxkit_generate_pkg_config(target pc_name [DESCRIPTION desc])
# Generates a .pc file for a cxxkit sublibrary target.
function(cxxkit_generate_pkg_config target pc_name)
    cmake_parse_arguments(arg "" "DESCRIPTION" "" ${ARGN})
    if(NOT arg_DESCRIPTION)
        set(arg_DESCRIPTION "cxxkit ${pc_name} module")
    endif()
    get_target_property(_type ${target} TYPE)
    set(_is_interface FALSE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        set(_is_interface TRUE)
    endif()
    # Collect cxxkit sublibrary dependencies (cxxkit::*) for Requires
    set(_requires "")
    get_target_property(_link_libs ${target} INTERFACE_LINK_LIBRARIES)
    foreach(_lib IN LISTS _link_libs)
        if(_lib MATCHES "^cxxkit::(.+)$")
            list(APPEND _requires "cxxkit-${CMAKE_MATCH_1}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _requires)
    string(JOIN " " _requires_str ${_requires})
    # Libs: compiled targets link -l<target>; header-only targets omit it
    if(_is_interface)
        set(_pc_libs "")
    else()
        set(_pc_libs "-L\${libdir} -l${target}")
    endif()
    # All paths are literal strings (no ${} in template); pkg-config resolves \${libdir} at query time.
    set(PC_PREFIX "\${pcfiledir}/../..")
    set(PC_EXEC_PREFIX "\${prefix}")
    set(PC_LIBDIR "\${prefix}/lib")
    set(PC_INCLUDEDIR "\${prefix}/include")
    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/cxxkit.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cxxkit-${pc_name}.pc"
        @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cxxkit-${pc_name}.pc"
        DESTINATION lib/pkgconfig)
endfunction()
