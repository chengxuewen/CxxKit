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
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
########################################################################################################################

# cxxkit::qt — event-loop driver sublibrary bridging onto a host Qt event loop (opt-in CXXKIT_ENABLE_LIB_QT).

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitQt)

# System Qt (NO_DEFAULT_PATH intentionally NOT set — contrast the vendored find_dependency chain; Qt is a host
# dependency by nature). Qt5 ≥ 5.10 is a documentation-level compatibility claim; 6 is what we build against.
find_package(Qt6 COMPONENTS Core REQUIRED)

cxxkit_add_library(cxxkit_qt
    SOURCES qt_event_dispatcher.cpp
    PUBLIC_LIBRARIES cxxkit::kernel cxxkit::tools Qt6::Core
    EXPORT_NAME qt
    FOLDER "CxxKit/libs")
target_link_libraries(cxxkit_qt PUBLIC cxxkit::base)

# M9 (PIT-36 precedent): Qt headers need C++17 — target-level standard override, helper has no such knob.
set_property(TARGET cxxkit_qt PROPERTY CXX_STANDARD 17)
# R-C1-1: no Q_OBJECT custom class (functor invokeMethod) — no moc pass. Off explicitly in case a parent
# project enables CMAKE_AUTOMOC globally.
set(CMAKE_AUTOMOC OFF)

cxxkit_configure_library_end(CxxKitQt)

# Install three-piece (uv shape minus the vendored lib pieces — Qt is a system dependency):
# no .pc (S2): CMake-only consumption; the Qt6Core.pc Requires chain is complex for little gain.
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_qt EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/qt/
        DESTINATION include/cxxkit/qt
        FILES_MATCHING PATTERN "*.hpp")
endif()
