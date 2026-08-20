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

function(cxxkit_parse_all_arguments prefix type options one_value_args multi_value_args)
    # Mirrors octk macro: (result type options oneValueArgs multiValueArgs)
    cmake_parse_arguments(${prefix} "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    set(${prefix}_UNPARSED_ARGUMENTS "${${prefix}_UNPARSED_ARGUMENTS}" PARENT_SCOPE)
    foreach(option ${options})
        set(${prefix}_${option} "${${prefix}_${option}}" PARENT_SCOPE)
    endforeach()
    foreach(multi_option ${multi_value_args})
        set(${prefix}_${multi_option} "${${prefix}_${multi_option}}" PARENT_SCOPE)
    endforeach()
    foreach(single_option ${one_value_args})
        set(${prefix}_${single_option} "${${prefix}_${single_option}}" PARENT_SCOPE)
    endforeach()
endfunction()

# Evaluate a CMake boolean expression, supporting variables, NOT/AND/OR.
function(cxxkit_evaluate_expression result)
    if(NOT "${ARGN}" STREQUAL "")
        set(expression "${ARGN}")
    else()
        set(expression "${result}")
    endif()
    if(${expression})
        set(${result} ON PARENT_SCOPE)
    else()
        set(${result} OFF PARENT_SCOPE)
    endif()
endfunction()

function(cxxkit_option variable description value)
    cxxkit_parse_all_arguments(arg
        "cxxkit_option"
        ""
        ""
        "DEPENDS;EMIT_IF;SET;SET_NEGATE;OR_CONDITION;VERIFY" ${ARGN})
    cxxkit_evaluate_expression(result ${value})
    if("${arg_EMIT_IF}" STREQUAL "")
        set(emit_if ON)
    else()
        cxxkit_evaluate_expression(emit_if ${arg_EMIT_IF})
    endif()
    if("${arg_OR_CONDITION}" STREQUAL "")
        set(or_condition OFF)
    else()
        cxxkit_evaluate_expression(or_condition ${arg_OR_CONDITION})
    endif()
    set(input OFF)
    # If INPUT_ is defined trying to use INPUT_ variable to enable/disable option.
    if((DEFINED "INPUT_${variable}")
            AND (NOT "${INPUT_${variable}}" STREQUAL "undefined")
            AND (NOT "${INPUT_${variable}}" STREQUAL ""))
        set(input ON)
        if(INPUT_${variable})
            set(input_result ON)
        else()
            set(input_result OFF)
        endif()
    elseif(or_condition)
        set(input ON)
        set(input_result ON)
    endif()
    # Warn about an option which is not emitted, but the user explicitly provided a value for it.
    if(input)
        if(emit_if)
            set(result ${input_result})
        else()
            message(WARNING "Option ${variable} is not emitted, but the user explicitly provided a value for it.")
        endif()
    endif()
    # Evaluate depends result
    if("${arg_DEPENDS}" STREQUAL "")
        set(depends_result ON)
    else()
        cxxkit_evaluate_expression(depends_result ${arg_DEPENDS})
    endif()
    if(${depends_result})
        if(input AND emit_if)
            # Forced state: INPUT_ injection / OR_CONDITION in effect — GUI greyed out (intentional).
            unset(${variable} CACHE)
            set(${variable} "${result}" CACHE STRING "${description}" FORCE)
            set(_option_string_type_if_cache_${variable} ON CACHE INTERNAL "${description}" FORCE)
        else()
            # Normal state: user-editable BOOL checkbox (no FORCE). Clear any forced STRING residue first.
            if(${_option_string_type_if_cache_${variable}})
                unset(${variable} CACHE)
            endif()
            set(${variable} ${result} CACHE BOOL "${description}")
            set(_option_string_type_if_cache_${variable} OFF CACHE INTERNAL "${description}" FORCE)
            set(result ${${variable}})
        endif()
    else()
        # Depends failed: greyed out with an explanatory value (OpenCTK behavior).
        if(${result})
            message(WARNING "Option ${variable} is depends on ${arg_DEPENDS}.")
        endif()
        set(${variable} "OFF" CACHE STRING "${description} depends on ${arg_DEPENDS}!" FORCE)
        set(_option_string_type_if_cache_${variable} ON CACHE INTERNAL "${description}" FORCE)
        set(result OFF)
    endif()
    if(${variable})
        message(STATUS "${variable}: ON")
    else()
        message(STATUS "${variable}: OFF")
    endif()
    set(${variable} ${result} PARENT_SCOPE)
endfunction()
