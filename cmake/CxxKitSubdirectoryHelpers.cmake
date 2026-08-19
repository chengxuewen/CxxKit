########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit_add_subdirectory: conditionally add a subdirectory if its CMakeLists.txt exists.
# Ported from OpenCTK OpenCTKSubdirectoryHelpers.cmake.
#
########################################################################################################################

function(cxxkit_add_subdirectory dir)
    if("${ARGN}" STREQUAL "")
        set(result ON)
    else()
        cxxkit_evaluate_expression(result ${ARGN})
    endif()
    get_filename_component(_fullPath ${dir} ABSOLUTE)
    if(EXISTS ${_fullPath}/CMakeLists.txt)
        if(${result})
            add_subdirectory(${dir})
        endif()
    endif()
endfunction()
