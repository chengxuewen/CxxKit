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
    # gtest 1.12.1 requires C++14 (M1: test targets override, libraries stay 11)
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

    if(NOT "${arg_OUTPUT_DIRECTORY}" STREQUAL "")
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${arg_OUTPUT_DIRECTORY})
    endif()

    add_test(NAME ${name} COMMAND ${name})
    set_property(TARGET ${name} PROPERTY FOLDER "CxxKit/tests")
endfunction()
