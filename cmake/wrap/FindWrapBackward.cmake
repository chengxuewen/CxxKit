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

if(TARGET CxxKitWrapBackward::WrapBackward)
    set(CxxKitWrapBackward_FOUND ON)
    return()
endif()

include(InstallVcpkg)
cxxkit_vcpkg_install_package(backward-cpp
    TARGET CxxKitWrapBackward::WrapBackward   # helper creates the empty INTERFACE IMPORTED target (QExt style)
    PREFIX CxxKitWrapBackward
    PACK_NAME backward-cpp
    NOT_IMPORT)
find_package(Backward PATHS "${CxxKitWrapBackward_INSTALL_DIR}" NO_DEFAULT_PATH REQUIRED)
target_link_libraries(CxxKitWrapBackward::WrapBackward INTERFACE Backward::Backward)
target_include_directories(CxxKitWrapBackward::WrapBackward INTERFACE "${CxxKitWrapBackward_INSTALL_DIR}/include")
set(CxxKitWrapBackward_FOUND ON)