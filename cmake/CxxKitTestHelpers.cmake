########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Test helper: cxxkit_add_test — defines a gtest executable registered with ctest.
# Test targets are compiled with C++14 (gtest 1.12.1 requirement); libraries stay C++11.
# Slimmed from OpenCTK OpenCTKTestHelpers.cmake.
#
########################################################################################################################

function(cxxkit_add_test name)
    cxxkit_parse_all_arguments(arg "cxxkit_add_test"
        ""
        ""
        "SOURCES;INCLUDE_DIRECTORIES;LIBRARIES;OUTPUT_DIRECTORY" ${ARGN})

    if("${arg_SOURCES}" STREQUAL "")
        message(FATAL_ERROR "cxxkit_add_test(${name}): no SOURCES given.")
    endif()

    add_executable(${name} ${arg_SOURCES})
    if(NOT "${arg_INCLUDE_DIRECTORIES}" STREQUAL "")
        target_include_directories(${name} PRIVATE ${arg_INCLUDE_DIRECTORIES})
    endif()
    if(NOT "${arg_LIBRARIES}" STREQUAL "")
        target_link_libraries(${name} PRIVATE ${arg_LIBRARIES})
    endif()
    # gtest 1.12.1 requires C++14 (M1: test targets override, libraries stay 11)
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if(NOT "${arg_OUTPUT_DIRECTORY}" STREQUAL "")
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY})
    endif()

    add_test(NAME ${name} COMMAND ${name})
endfunction()
