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

########################################################################################################################
#
# cxxkit::crash — crash handling sublibrary (breakpad backend + backward-cpp stack printing, opt-in).
# Gated by CXXKIT_ENABLE_LIB_CRASH at the top level (cxxkit_add_subdirectory), like cxxkit::network.
# Depends on breakpad + backward-cpp via FindWrapBreakpad/FindWrapBackward (cmake/InstallVcpkg.cmake).
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitCrash)

# QExt-style one wrap per package: FindWrapBreakpad.cmake + FindWrapBackward.cmake (cmake/InstallVcpkg.cmake).
cxxkit_find_package(Breakpad PROVIDED_TARGETS CxxKitWrapBreakpad::WrapBreakpad)
cxxkit_find_package(Backward PROVIDED_TARGETS CxxKitWrapBackward::WrapBackward)

# backward.hpp dlopens libdw.so.1 at runtime: the shared lib must find the shipped .so next to itself
# (INSTALL_RPATH keeps that working for the installed shared lib).
cxxkit_add_library(cxxkit_crash
    SOURCES crash_handler.cpp
    PUBLIC_LIBRARIES cxxkit::base cxxkit::patterns cxxkit::tools CxxKitWrapBreakpad::WrapBreakpad CxxKitWrapBackward::WrapBackward
    COMPILE_DEFINITIONS CXXKIT_FEATURE_ENABLE_CRASH=1
    INSTALL_RPATH "$ORIGIN"
    EXPORT_NAME crash
    FOLDER "CxxKit/libs")

cxxkit_generate_pkg_config(cxxkit_crash crash DESCRIPTION "cxxkit crash module (breakpad + backward-cpp backend)")
cxxkit_configure_library_end(CxxKitCrash)
