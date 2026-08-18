########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
########################################################################################################################

cmake_minimum_required(VERSION 3.15...3.31)
include(.cmake.conf)

project(cxxkit VERSION 0.1.0 LANGUAGES CXX C)

set(CXXKIT_VERSION ${PROJECT_VERSION})
set(CXXKIT_COPYRIGHT "Copyright (C) 2025~Present ChengXueWen.")
set(CXXKIT_LICENSE "MIT License")
set(CXXKIT_PRODUCT_NAME "cxxkit")

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
if(NOT DEFINED INPUT_CXXKIT_FEATURE_CXX_STANDARD)
    set(CXXKIT_FEATURE_CXX_STANDARD 11)
else()
    set(CXXKIT_FEATURE_CXX_STANDARD ${INPUT_CXXKIT_FEATURE_CXX_STANDARD})
endif()
message(STATUS "C++ standard: ${CXXKIT_FEATURE_CXX_STANDARD}")

#-----------------------------------------------------------------------------------------------------------------------
# Install prefix handling (octk-style WILL_INSTALL logic)
#-----------------------------------------------------------------------------------------------------------------------
if(NOT DEFINED INPUT_CXXKIT_FEATURE_INSTALL_PREFIX)
    if(CXXKIT_FEATURE_DEV_BUILD OR NOT CMAKE_INSTALL_PREFIX)
        set(CXXKIT_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
    else()
        set(CXXKIT_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    endif()
else()
    set(CXXKIT_INSTALL_PREFIX ${INPUT_CXXKIT_FEATURE_INSTALL_PREFIX})
endif()
message(STATUS "Project install prefix set as '${CXXKIT_INSTALL_PREFIX}'.")

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
include(CxxKitInstallHelpers)
include(CxxKitTestHelpers)
include(CxxKitPkgConfigHelpers)

#-----------------------------------------------------------------------------------------------------------------------
# Build options
#-----------------------------------------------------------------------------------------------------------------------
cxxkit_option(CXXKIT_BUILD_ALL "Enable this to build all artifacts" OFF)
cxxkit_option(CXXKIT_BUILD_SHARED_LIBS "Enable this to build as dynamically" OFF
    SET BUILD_SHARED_LIBS)
cxxkit_option(CXXKIT_BUILD_USE_PCH "Enable this to build use precompiled header files for compilation" ON)
cxxkit_option(CXXKIT_BUILD_COMPILER_WARNING "Enable this to build target with compiler warnings" OFF)
cxxkit_option(CXXKIT_BUILD_WARNINGS_ARE_ERRORS "Enable this to build with warnings as errors" ON)
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
cxxkit_option(CXXKIT_ENABLE_LIB_NETWORK "Enable this to build enable network lib" OFF
    OR_CONDITION CXXKIT_BUILD_ALL)

#-----------------------------------------------------------------------------------------------------------------------
# Testing
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_TESTS)
    enable_testing()
endif()

#-----------------------------------------------------------------------------------------------------------------------
# Sublibraries (Phase 1: base + header-only)
#-----------------------------------------------------------------------------------------------------------------------
add_subdirectory(cxxkit/base)
add_subdirectory(cxxkit/containers)
add_subdirectory(cxxkit/functional)
add_subdirectory(cxxkit/memory)
add_subdirectory(cxxkit/numerics)
add_subdirectory(cxxkit/patterns)
add_subdirectory(cxxkit/units)
add_subdirectory(cxxkit/kernel)
add_subdirectory(cxxkit/thread)
add_subdirectory(cxxkit/time)
add_subdirectory(cxxkit/text)
add_subdirectory(cxxkit/tools)

# Phase 2: add_subdirectory(src/network) when migrated
cxxkit_add_subdirectory(cxxkit/network CXXKIT_ENABLE_LIB_NETWORK)
cxxkit_add_subdirectory(tests CXXKIT_BUILD_TESTS)
cxxkit_add_subdirectory(examples CXXKIT_BUILD_EXAMPLES)

#-----------------------------------------------------------------------------------------------------------------------
# Install: export targets + config file
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_BUILD_INSTALL)
    install(EXPORT cxxkitTargets
        FILE cxxkitTargets.cmake
        NAMESPACE cxxkit::
        DESTINATION lib/cmake/cxxkit)
    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${CXXKIT_CMAKE_DIR}/cxxkitConfig.cmake.in"
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
