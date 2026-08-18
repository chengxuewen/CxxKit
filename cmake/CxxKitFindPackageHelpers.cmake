########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# FindPackage helper: cxxkit_find_package(WrapFoo PROVIDED_TARGETS CXXKitWrapFoo::WrapFoo)
# Locates a vendored wrap find module (cmake/wrap/FindWrapFoo.cmake) and includes it.
# Ported (slimmed) from OpenCTK OpenCTKFindPackageHelpers.cmake.
#
########################################################################################################################

# cxxkit_find_package(WrapName [PROVIDED_TARGETS target...])
# The wrap find modules live in cmake/wrap/ and follow FindWrap<Name>.cmake naming.
function(cxxkit_find_package wrap_name)
    cxxkit_parse_all_arguments(arg "cxxkit_find_package" "" "" "PROVIDED_TARGETS" ${ARGN})
    if("${arg_PROVIDED_TARGETS}" STREQUAL "")
        set(package_targets "")
        foreach(target ${arg_PROVIDED_TARGETS})
            list(APPEND package_targets "${target}")
        endforeach()
    else()
        set(package_targets ${arg_PROVIDED_TARGETS})
    endif()
    set(_wrap_file "${CMAKE_MODULE_PATH}/wrap/FindWrap${wrap_name}.cmake")
    if(NOT EXISTS "${_wrap_file}")
        message(FATAL_ERROR "cxxkit_find_package: FindWrap${wrap_name}.cmake not found in cmake/wrap/")
    endif()
    include("${_wrap_file}")
    # Verify the provided targets exist after inclusion
    foreach(target ${package_targets})
        if(NOT TARGET "${target}")
            message(FATAL_ERROR "cxxkit_find_package(${wrap_name}): provided target ${target} was not created by FindWrap${wrap_name}.cmake")
        endif()
    endforeach()
    set("${wrap_name}_FOUND" ON PARENT_SCOPE)
endfunction()
