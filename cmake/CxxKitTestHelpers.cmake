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
    # tests follow the main standard (CXXKIT_FEATURE_CXX_STANDARD); C++11 floor enforced by the C++14-gate in check.sh
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if(NOT "${arg_OUTPUT_DIRECTORY}" STREQUAL "")
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY})
    endif()

    add_test(NAME ${name} COMMAND ${name})

    # Auto-derive FOLDER from directory hierarchy (octk pattern); explicit
    # pre-set FOLDER is kept as-is.
    get_target_property(_cxxkit_folder ${name} FOLDER)
    if(NOT _cxxkit_folder)
        file(RELATIVE_PATH _dir "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        if(_dir MATCHES "^tests/(.+)$")
            set_target_properties(${name} PROPERTIES FOLDER "CxxKit/tests/${CMAKE_MATCH_1}")
        elseif(_dir STREQUAL "tests")
            set_target_properties(${name} PROPERTIES FOLDER "CxxKit/tests")
        else()
            set_target_properties(${name} PROPERTIES FOLDER "CxxKit/tests/${_dir}")
        endif()
    endif()

    # <name>_check: run this single test via ctest (octk pattern).
    set(_test_config_options "")
    get_cmake_property(_is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(_is_multi_config)
        set(_test_config_options -C $<CONFIG>)
    endif()
    add_custom_target(${name}_check
        VERBATIM
        COMMENT "Running ${CMAKE_CTEST_COMMAND} -V -R \"^${name}$\" ${_test_config_options}"
        COMMAND ${CMAKE_CTEST_COMMAND} -V -R "^${name}$" ${_test_config_options})
    add_dependencies(${name}_check ${name})
    # Group with the test target in the IDE (custom targets default to the top level).
    set_property(TARGET ${name}_check PROPERTY FOLDER "CxxKit/tests")
endfunction()
