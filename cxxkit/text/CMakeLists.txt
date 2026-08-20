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

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitText)

cxxkit_add_library(cxxkit_text
    STATIC
    NO_ALIAS
    SOURCES string_utils.cpp ascii.cpp string.cpp base64.cpp bit_buffer.cpp string_builder.cpp string_encode.cpp
    PUBLIC_LIBRARIES cxxkit::base cxxkit::numerics
    EXPORT_NAME text
    FOLDER "CxxKit/libs")
add_library(cxxkit::text ALIAS cxxkit_text)

cxxkit_generate_pkg_config(cxxkit_text text)
cxxkit_configure_library_end(CxxKitText)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_text EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/text/
        DESTINATION include/cxxkit/text
        FILES_MATCHING PATTERN "*.hpp")
endif()
