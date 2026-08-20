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

function(cxxkit_replace_compiler_option OLD_OPTION NEW_OPTION)
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES ${OLD_OPTION})
            # the whitespace after OLD_OPTION is necessary to really match only the flag and not some sub flag
            string(REGEX REPLACE "${OLD_OPTION} " "${NEW_OPTION}" ${flag_var} "${${flag_var}}")
        else()
            set(${flag_var} "${${flag_var}} ${NEW_OPTION}")
        endif()
        set(${flag_var} ${${flag_var}} PARENT_SCOPE)
    endforeach()
endfunction()

function(cxxkit_set_compiler_warnings TARGET)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(WARNINGS "-Werror" "-Wall")
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(WARNINGS "-Werror" "-Wall")
    elseif(MSVC)
        set(WARNINGS "/WX" "/W4")
    endif()
    target_compile_options(${TARGET} PRIVATE ${WARNINGS})
endfunction()
