########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Compiler helpers: cross-compiler warning flags and runtime library option replacement.
# Ported from OpenCTK OpenCTKCompilerHelpers.cmake.
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
