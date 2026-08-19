########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# cxxkit_option: option with DEPENDS/EMIT_IF/OR_CONDITION and INPUT_ variable injection.
# Ported (slimmed) from OpenCTK OpenCTKOptionHelpers.cmake.
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
            unset(${variable} CACHE)
            set(${variable} "${result}" CACHE STRING "${description}" FORCE)
        endif()
        if(NOT input)
            if(NOT DEFINED ${variable})
                set(${variable} "${value}" CACHE STRING "${description}" FORCE)
            endif()
            set(result ${${variable}})
        endif()
    else()
        set(${variable} OFF CACHE STRING "${description}" FORCE)
        set(result OFF)
    endif()
    if(${variable})
        message(STATUS "${variable}: ON")
    else()
        message(STATUS "${variable}: OFF")
    endif()
    set(${variable} ${result} PARENT_SCOPE)
endfunction()
