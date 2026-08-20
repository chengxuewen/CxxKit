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

# cxxkit_find_package(WrapName [PROVIDED_TARGETS target...])
# The wrap find modules live in cmake/wrap/ and follow FindWrap<Name>.cmake naming.
# Capture the wrap dir at include time: inside a function CMAKE_CURRENT_LIST_DIR
# would resolve to the CALLING listfile's dir, not this helper's.
set(CXXKIT_WRAP_DIR "${CMAKE_CURRENT_LIST_DIR}/wrap")
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
    set(_wrap_file "${CXXKIT_WRAP_DIR}/FindWrap${wrap_name}.cmake")
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
