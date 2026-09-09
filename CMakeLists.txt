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

cmake_minimum_required(VERSION 3.15...3.31)
include(.cmake.conf)

project(CxxKit VERSION 0.1.0 LANGUAGES CXX C)

set(CXXKIT_VERSION ${PROJECT_VERSION})
set(CXXKIT_COPYRIGHT "Copyright (C) 2025~Present ChengXueWen.")
set(CXXKIT_LICENSE "MIT License")
set(CXXKIT_PRODUCT_NAME "CxxKit")

#-----------------------------------------------------------------------------------------------------------------------
# Pre-calculate the developer_build feature if it's set by the user via INPUT_CXXKIT_FEATURE_DEV_BUILD
#-----------------------------------------------------------------------------------------------------------------------
if(NOT DEFINED CXXKIT_FEATURE_DEV_BUILD)
    if(NOT DEFINED INPUT_CXXKIT_FEATURE_DEV_BUILD)
        set(CXXKIT_FEATURE_DEV_BUILD ON)
    else()
        set(CXXKIT_FEATURE_DEV_BUILD ${INPUT_CXXKIT_FEATURE_DEV_BUILD})
    endif()
endif()
message(STATUS "Project feature dev build ${CXXKIT_FEATURE_DEV_BUILD}")

#-----------------------------------------------------------------------------------------------------------------------
# C++ standard (default 11, per design decision)
#-----------------------------------------------------------------------------------------------------------------------
if(INPUT_CXXKIT_FEATURE_CXX_STANDARD)
    set(CXXKIT_FEATURE_CXX_STANDARD ${INPUT_CXXKIT_FEATURE_CXX_STANDARD} CACHE STRING "C++ standard" FORCE)
    set(CXXKIT_CXX_STANDARD_SOURCE "input")
elseif(CMAKE_CXX_STANDARD)
    set(CXXKIT_FEATURE_CXX_STANDARD ${CMAKE_CXX_STANDARD} CACHE STRING "C++ standard" FORCE)
    set(CXXKIT_CXX_STANDARD_SOURCE "cmake")
else()
    set(CXXKIT_FEATURE_CXX_STANDARD 11 CACHE STRING "C++ standard")
    set(CXXKIT_CXX_STANDARD_SOURCE "default")
endif()
set_property(CACHE CXXKIT_FEATURE_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23 26)

set(CXXKIT_CXX_STANDARD_VALID OFF)
set(CXXKIT_CXX_STANDARD_VALUES 11 14 17 20 23 26)
foreach(cxx_standard ${CXXKIT_CXX_STANDARD_VALUES})
    if("${CXXKIT_FEATURE_CXX_STANDARD}" VERSION_GREATER_EQUAL "${cxx_standard}")
        set(CXXKIT_CXX_STANDARD_${cxx_standard} 1)
    else()
        set(CXXKIT_CXX_STANDARD_${cxx_standard} 0)
    endif()
    if("${cxx_standard}" STREQUAL "${CXXKIT_FEATURE_CXX_STANDARD}")
        set(CXXKIT_CXX_STANDARD_VALID ON)
    endif()
endforeach()
if(NOT CXXKIT_CXX_STANDARD_VALID)
    message(FATAL_ERROR " Unsupport C++ standard: ${CXXKIT_FEATURE_CXX_STANDARD}. Support: ${CXXKIT_CXX_STANDARD_VALUES}")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Using C++: ${CXXKIT_FEATURE_CXX_STANDARD} (${CXXKIT_CXX_STANDARD_SOURCE})")

#-----------------------------------------------------------------------------------------------------------------------
# Install prefix handling (octk-style WILL_INSTALL logic)
# Default: install into <build>/install for build-install testing;
# explicit -DCMAKE_INSTALL_PREFIX or -DINPUT_CXXKIT_FEATURE_INSTALL_PREFIX overrides.
#-----------------------------------------------------------------------------------------------------------------------
if(NOT DEFINED INPUT_CXXKIT_FEATURE_INSTALL_PREFIX)
    if(CMAKE_INSTALL_PREFIX AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
        # User explicitly passed -DCMAKE_INSTALL_PREFIX
        set(CXXKIT_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    else()
        # Default: build tree install dir (octk dev-build style, for install testing).
        # Write back to CMAKE_INSTALL_PREFIX so the install target uses it too.
        set(CXXKIT_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install")
        set(CMAKE_INSTALL_PREFIX "${CXXKIT_INSTALL_PREFIX}" CACHE PATH "Install prefix" FORCE)
    endif()
else()
    set(CXXKIT_INSTALL_PREFIX ${INPUT_CXXKIT_FEATURE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "${CXXKIT_INSTALL_PREFIX}" CACHE PATH "Install prefix" FORCE)
endif()
message(STATUS "Project install prefix set as '${CXXKIT_INSTALL_PREFIX}'.")

#-----------------------------------------------------------------------------------------------------------------------
# Detect 3rdparty packages directories
#-----------------------------------------------------------------------------------------------------------------------
if(DEFINED INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR)
    set(CXXKIT_3RDPARTY_PACKAGES_DIR "${INPUT_CXXKIT_3RDPARTY_PACKAGES_DIR}" CACHE PATH "3rdparty packages directories")
else()
    set(CXXKIT_3RDPARTY_PACKAGES_DIR "" CACHE PATH "3rdparty packages directories")
endif()
message(STATUS "Project 3rdparty packages directories: ${CXXKIT_3RDPARTY_PACKAGES_DIR}")

if(CXXKIT_INSTALL_PREFIX STREQUAL CMAKE_CURRENT_BINARY_DIR)
    set(CXXKIT_WILL_INSTALL_VALUE OFF)
else()
    set(CXXKIT_WILL_INSTALL_VALUE ON)
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Build type default
#-----------------------------------------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    if(CXXKIT_FEATURE_DEV_BUILD)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    else()
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    endif()
    message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Processor count & build type helpers (used by FindWrap modules)
#-----------------------------------------------------------------------------------------------------------------------
include(ProcessorCount)
ProcessorCount(CXXKIT_PROCESSOR_COUNT)
set(CXXKIT_NUMBER_OF_ASYNC_JOBS ${CXXKIT_PROCESSOR_COUNT})
string(TOLOWER "${CMAKE_BUILD_TYPE}" CXXKIT_LOWER_BUILD_TYPE)

#-----------------------------------------------------------------------------------------------------------------------
# CMake module path & helpers
#-----------------------------------------------------------------------------------------------------------------------
set(CXXKIT_CMAKE_DIR "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CXXKIT_CMAKE_DIR} ${CMAKE_MODULE_PATH})

include(CxxKitOptionHelpers)
include(CxxKitSubdirectoryHelpers)
include(CxxKitCompilerHelpers)
include(CxxKitFindPackageHelpers)
include(CxxKitFetchHelpers)
include(CxxKitConfigureHelpers)
include(CxxKitLibraryHelpers)
include(CxxKitExecutableHelpers)
include(CxxKitInstallHelpers)
include(CxxKitTestHelpers)
include(CxxKitPkgConfigHelpers)
include(CxxKitPlatformSupport)
include(InstallVcpkg)

#-----------------------------------------------------------------------------------------------------------------------
# Build options
#-----------------------------------------------------------------------------------------------------------------------
cxxkit_option(CXXKIT_BUILD_ALL "Enable this to build all artifacts" OFF)
cxxkit_option(CXXKIT_BUILD_SHARED_LIBS "Enable this to build as dynamically" OFF
    SET BUILD_SHARED_LIBS)
cxxkit_option(CXXKIT_BUILD_USE_PCH "Enable this to build use precompiled header files for compilation" ON)
cxxkit_option(CXXKIT_BUILD_COMPILER_WARNING "Enable this to build target with compiler warnings" OFF)
cxxkit_option(CXXKIT_BUILD_WARNINGS_ARE_ERRORS "Enable this to build with warnings as errors" ON)
cxxkit_option(CXXKIT_BUILD_SANITIZERS "Enable ASAN/LSAN/UBSan for all targets" OFF
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_COVERAGE "Enable gcov coverage instrumentation" OFF
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_BENCHMARKS "Enable this to build the benchmarks" OFF)
cxxkit_option(CXXKIT_BUILD_DOCS "Enable this to build the documentation" OFF)
cxxkit_option(CXXKIT_BUILD_LIBS "Enable this to build libs" ON
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_APPS "Enable this to build apps" ON
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_TESTS "Enable this to build tests" ON
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_EXAMPLES "Enable this to build examples" ON
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_BUILD_INSTALL "Enable this to build the installer" ${CXXKIT_WILL_INSTALL_VALUE}
    OR_CONDITION CXXKIT_BUILD_ALL)
# base/profiling stay unconditional (R-M0-1): base is the dependency-graph root
# (turning it off = everything off — the option would be pure noise), profiling has
# zero dependencies and zero beneficiaries. The anchor variable still exists so the
# DEPENDS expressions below stay uniform.
set(CXXKIT_ENABLE_LIB_BASE ON)
set(CXXKIT_ENABLE_LIB_PROFILING ON)
# 12 core sublibrary opt-in switches (Momus F1: topological order — cxxkit_option
# evaluates DEPENDS immediately, so upstream dependencies must be declared first:
# text → containers → functional → numerics → patterns → units → memory → kernel → thread →
# media (10 switches; base/time/tools/profiling unconditional — symbol-cycle rulings).
# time is unconditional like tools (Ruling v2, M0): tools/clock.cpp references
# DateTime symbols (time), while time headers reference tools' CXXKIT_LOGGER — a
# symbol-level cycle (nm-verified both directions). Neither is separable.
if(DEFINED CXXKIT_ENABLE_LIB_TIME AND NOT CXXKIT_ENABLE_LIB_TIME)
    message(WARNING "CXXKIT_ENABLE_LIB_TIME=OFF ignored: time is unconditional (time↔tools symbol cycle, M0 Ruling v2).")
endif()
set(CXXKIT_ENABLE_LIB_TIME ON)
cxxkit_option(CXXKIT_ENABLE_LIB_TEXT "Enable this to build text lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE" OR_CONDITION CXXKIT_BUILD_ALL)
# tools is unconditional like base (Ruling, M0): its CHECK/logging symbols (CXXKIT_LOGGER,
# Logger::output) are referenced by the headers of time/units/containers/numerics/functional/
# patterns — every path back into tools' .cpp creates a cycle (tools needed time; time's
# headers need tools symbols). An opt-in switch for it would be pure noise.
if(DEFINED CXXKIT_ENABLE_LIB_TOOLS AND NOT CXXKIT_ENABLE_LIB_TOOLS)
    message(WARNING "CXXKIT_ENABLE_LIB_TOOLS=OFF ignored: tools is unconditional (symbol hub, M0 Ruling).")
endif()
set(CXXKIT_ENABLE_LIB_TOOLS ON)
cxxkit_option(CXXKIT_ENABLE_LIB_CONTAINERS "Enable this to build containers lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TOOLS" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_FUNCTIONAL "Enable this to build functional lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TOOLS" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_NUMERICS "Enable this to build numerics lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TOOLS" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_PATTERNS "Enable this to build patterns lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TOOLS" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_UNITS "Enable this to build units lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_CONTAINERS AND CXXKIT_ENABLE_LIB_NUMERICS AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_MEMORY "Enable this to build memory lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TOOLS" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_KERNEL "Enable this to build kernel lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_MEMORY AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_THREAD "Enable this to build thread lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_FUNCTIONAL AND CXXKIT_ENABLE_LIB_KERNEL AND CXXKIT_ENABLE_LIB_MEMORY AND CXXKIT_ENABLE_LIB_TIME AND CXXKIT_ENABLE_LIB_TOOLS AND CXXKIT_ENABLE_LIB_UNITS"
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_MEDIA "Enable this to build media lib" ON
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_THREAD AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_NETWORK "Enable this to build enable network lib" OFF
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TEXT AND CXXKIT_ENABLE_LIB_THREAD AND CXXKIT_ENABLE_LIB_MEMORY AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_TRACY "Enable this to build enable Tracy profiler lib" OFF
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_CRASH "Enable this to build enable crash lib" OFF
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_PATTERNS AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_IMGUI "Enable this to build enable imgui lib" OFF
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_TOOLS" OR_CONDITION CXXKIT_BUILD_ALL)
cxxkit_option(CXXKIT_ENABLE_LIB_UV "Enable this to build enable uv (libuv event loop) lib" OFF
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_KERNEL AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)
# Local machine (g++ 10.5) only: qt consumers link the conda libstdc++ explicitly so
# libQt6Core's GLIBCXX_3.4.30+ symbols resolve. CI (gcc>=11, system Qt) never sees this
# (EXISTS guard = no-op). Defined at top level so examples/tests scopes inherit it. PIT-39.
set(CXXKIT_QT_LOCAL_STDCPP "")
if(EXISTS "/tmp/opencode/qt-env/.pixi/envs/default/lib/libstdc++.so.6")
    set(CXXKIT_QT_LOCAL_STDCPP "/tmp/opencode/qt-env/.pixi/envs/default/lib/libstdc++.so.6" "/tmp/opencode/qt-env/.pixi/envs/default/lib/libgcc_s.so.1")
endif()
cxxkit_option(CXXKIT_ENABLE_LIB_QT "Enable this to build enable qt (Qt event loop bridge) lib" OFF
    DEPENDS "CXXKIT_ENABLE_LIB_BASE AND CXXKIT_ENABLE_LIB_KERNEL AND CXXKIT_ENABLE_LIB_TOOLS"
    OR_CONDITION CXXKIT_BUILD_ALL)

#-----------------------------------------------------------------------------------------------------------------------
# Sanitizers (ASAN/LSAN/UBSan) - applied at top-level scope so add_compile_options/add_link_options
# propagate to every subdirectory (all libraries + tests, built and installed here).
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_SANITIZERS)
    add_compile_options("-fsanitize=address,leak,undefined" "-fno-omit-frame-pointer")
    add_link_options("-fsanitize=address,leak,undefined")
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Coverage (gcov) - applied at top-level scope so add_compile_options/add_link_options
# propagate to every subdirectory (all libraries + tests). --coverage implies -fprofile-arcs
# -ftest-coverage and produces gcov-compatible .gcno/.gcda data.
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_COVERAGE)
    add_compile_options("--coverage")
    add_link_options("--coverage")
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Testing
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_TESTS)
    enable_testing()
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Coverage report target - runs ctest, then summarizes per-file line coverage from gcov
# output (no lcov/gcovr needed). Generates a plain textual report in <build>/coverage/.
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_COVERAGE)
    add_custom_target(BuildCoverage
        COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_CURRENT_BINARY_DIR} --output-on-failure || true
        COMMAND ${CMAKE_COMMAND} -E env bash "${PROJECT_SOURCE_DIR}/scripts/coverage.sh" "${CMAKE_CURRENT_BINARY_DIR}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Collecting gcov coverage report")
endif()

# Sublibraries (abseil-style: directory = sublibrary = CMake target).
# base/time/tools/profiling unconditional (R-M0-1 + M0 Rulings); 10 core libs opt-in per switch.
add_subdirectory(cxxkit/base)
cxxkit_add_subdirectory(cxxkit/containers CXXKIT_ENABLE_LIB_CONTAINERS)
cxxkit_add_subdirectory(cxxkit/functional CXXKIT_ENABLE_LIB_FUNCTIONAL)
cxxkit_add_subdirectory(cxxkit/memory CXXKIT_ENABLE_LIB_MEMORY)
cxxkit_add_subdirectory(cxxkit/numerics CXXKIT_ENABLE_LIB_NUMERICS)
cxxkit_add_subdirectory(cxxkit/patterns CXXKIT_ENABLE_LIB_PATTERNS)
cxxkit_add_subdirectory(cxxkit/units CXXKIT_ENABLE_LIB_UNITS)
cxxkit_add_subdirectory(cxxkit/kernel CXXKIT_ENABLE_LIB_KERNEL)
cxxkit_add_subdirectory(cxxkit/thread CXXKIT_ENABLE_LIB_THREAD)
add_subdirectory(cxxkit/time)  # unconditional (Ruling v2, M0): time↔tools symbol cycle
cxxkit_add_subdirectory(cxxkit/text CXXKIT_ENABLE_LIB_TEXT)
add_subdirectory(cxxkit/tools)  # unconditional (Ruling M0): CHECK/logging symbol hub
cxxkit_add_subdirectory(cxxkit/media CXXKIT_ENABLE_LIB_MEDIA)
add_subdirectory(cxxkit/profiling)

cxxkit_add_subdirectory(cxxkit/network CXXKIT_ENABLE_LIB_NETWORK)
cxxkit_add_subdirectory(cxxkit/crash CXXKIT_ENABLE_LIB_CRASH)
cxxkit_add_subdirectory(cxxkit/imgui CXXKIT_ENABLE_LIB_IMGUI)
cxxkit_add_subdirectory(cxxkit/uv CXXKIT_ENABLE_LIB_UV)
cxxkit_add_subdirectory(cxxkit/qt CXXKIT_ENABLE_LIB_QT)
cxxkit_add_subdirectory(tests CXXKIT_BUILD_TESTS)
cxxkit_add_subdirectory(examples CXXKIT_BUILD_EXAMPLES)


#-----------------------------------------------------------------------------------------------------------------------
# Doxygen documentation (CXXKIT_BUILD_DOCS)
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_DOCS)
    find_program(DOXYGEN_EXECUTABLE doxygen)
    if(DOXYGEN_EXECUTABLE)
        set(CXXKIT_PROJECT_NAME "${PROJECT_NAME}")
        set(CXXKIT_DOC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc")
        set(_doc_input "${PROJECT_SOURCE_DIR}/README.md")
        file(GLOB _doc_subs "${PROJECT_SOURCE_DIR}/cxxkit/*")
        foreach(_sub IN LISTS _doc_subs)
            if(IS_DIRECTORY "${_sub}")
                string(APPEND _doc_input " ${_sub}")
            endif()
        endforeach()
        set(CXXKIT_DOXYGEN_INPUT "${_doc_input}")
        configure_file(
            "${PROJECT_SOURCE_DIR}/docs/Doxyfile.in"
            "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            @ONLY)
        add_custom_target(Docs
            COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Generating API documentation with doxygen")
        message(STATUS "Doxygen documentation enabled (target: Docs)")
    else()
        message(WARNING "CXXKIT_BUILD_DOCS=ON but doxygen not found; skipping")
    endif()
endif()


#-----------------------------------------------------------------------------------------------------------------------
# Custom build targets (octk-style "top-level only"): BuildAll rebuilds, BuildInstall builds + installs
# Defined only when CxxKit is the top-level project (standalone build). When consumed via
# add_subdirectory (as a 3rdparty submodule of a parent project), these convenience targets are
# skipped so they can't collide with same-named targets owned by the parent. (octk convention)
#-----------------------------------------------------------------------------------------------------------------------
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_custom_target(BuildAll
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_custom_target(BuildInstall
        COMMAND ${CMAKE_COMMAND} --build ./ --parallel ${CXXKIT_NUMBER_OF_ASYNC_JOBS} --config ${CMAKE_BUILD_TYPE}
        COMMAND ${CMAKE_COMMAND} --install ./
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()


#-----------------------------------------------------------------------------------------------------------------------
# Install: export targets + config file
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_INSTALL)
    install(EXPORT cxxkitTargets
        FILE cxxkitTargets.cmake
        NAMESPACE cxxkit::
        DESTINATION lib/cmake/cxxkit)
    include(CMakePackageConfigHelpers)
    # Vendored deps with their own Config.cmake (vcpkg-style find_dependency chain).
    # Tracy is included only when the profiling lib is enabled.
    set(CXXKIT_VENDORED_FIND_DEPS "")
    if(CXXKIT_ENABLE_LIB_TEXT OR CXXKIT_ENABLE_LIB_TOOLS)
        list(APPEND CXXKIT_VENDORED_FIND_DEPS "fmt")  # vendored via text/tools
    endif()
    if(CXXKIT_ENABLE_LIB_TOOLS)
        list(APPEND CXXKIT_VENDORED_FIND_DEPS "spdlog")  # tools-only
    endif()
    if(CXXKIT_ENABLE_LIB_NETWORK)
        list(APPEND CXXKIT_VENDORED_FIND_DEPS "cpr" "CURL" "MbedTLS")  # network-only
    endif()
    if(CXXKIT_ENABLE_LIB_TRACY)
        list(APPEND CXXKIT_VENDORED_FIND_DEPS "Tracy")
    endif()
    if(CXXKIT_ENABLE_LIB_CRASH)
        list(APPEND CXXKIT_VENDORED_FIND_DEPS "unofficial-breakpad" "Backward")
    endif()
    if(CXXKIT_ENABLE_LIB_UV)
        list(APPEND CXXKIT_VENDORED_FIND_DEPS "libuv")
    endif()
    set(CXXKIT_QT_ENABLED ${CXXKIT_ENABLE_LIB_QT})
    configure_package_config_file(
        "${CXXKIT_CMAKE_DIR}/CxxKitConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cxxkitConfig.cmake"
        INSTALL_DESTINATION lib/cmake/cxxkit)
    configure_package_config_file(
        "${CXXKIT_CMAKE_DIR}/CxxKitConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cxxkitConfig.cmake"
        INSTALL_DESTINATION lib/cmake/cxxkit)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/cxxkitConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/cxxkitConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cxxkitConfigVersion.cmake"
        DESTINATION lib/cmake/cxxkit)
endif()
