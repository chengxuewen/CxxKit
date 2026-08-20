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

#-----------------------------------------------------------------------------------------------------------------------
# cxxkit_set01 finction
#-----------------------------------------------------------------------------------------------------------------------
function(cxxkit_set01 result)
    if(${ARGN})
        set("${result}" 1 PARENT_SCOPE)
    else()
        set("${result}" 0 PARENT_SCOPE)
    endif()
endfunction()


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit set system variable
#-----------------------------------------------------------------------------------------------------------------------
message(STATUS "Build in system: ${CMAKE_SYSTEM_NAME}")
set(CXXKIT_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(CXXKIT_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
set(CXXKIT_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
cxxkit_set01(CXXKIT_SYSTEM_LINUX
    CMAKE_SYSTEM_NAME STREQUAL "Linux")
cxxkit_set01(CXXKIT_SYSTEM_WINCE
    CMAKE_SYSTEM_NAME STREQUAL "WindowsCE")
cxxkit_set01(CXXKIT_SYSTEM_WIN
    CXXKIT_SYSTEM_WINCE OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
cxxkit_set01(CXXKIT_SYSTEM_HPUX
    CMAKE_SYSTEM_NAME STREQUAL "HPUX")
cxxkit_set01(CXXKIT_SYSTEM_ANDROID
    CMAKE_SYSTEM_NAME STREQUAL "Android")
cxxkit_set01(CXXKIT_SYSTEM_NACL
    CMAKE_SYSTEM_NAME STREQUAL "NaCl")
cxxkit_set01(CXXKIT_SYSTEM_INTEGRITY
    CMAKE_SYSTEM_NAME STREQUAL "Integrity")
cxxkit_set01(CXXKIT_SYSTEM_VXWORKS
    CMAKE_SYSTEM_NAME STREQUAL "VxWorks")
cxxkit_set01(CXXKIT_SYSTEM_QNX
    CMAKE_SYSTEM_NAME STREQUAL "QNX")
cxxkit_set01(CXXKIT_SYSTEM_OPENBSD
    CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
cxxkit_set01(CXXKIT_SYSTEM_FREEBSD
    CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
cxxkit_set01(CXXKIT_SYSTEM_NETBSD
    CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
cxxkit_set01(CXXKIT_SYSTEM_WASM
    CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR EMSCRIPTEN)
cxxkit_set01(CXXKIT_SYSTEM_SOLARIS
    CMAKE_SYSTEM_NAME STREQUAL "SunOS")
cxxkit_set01(CXXKIT_SYSTEM_HURD
    CMAKE_SYSTEM_NAME STREQUAL "GNU")
# This is the only reliable way we can determine the webOS platform as the yocto recipe adds this compile definition
# into its generated toolchain.cmake file
cxxkit_set01(CXXKIT_SYSTEM_WEBOS
    CMAKE_CXX_FLAGS MATCHES "-D__WEBOS__")
cxxkit_set01(CXXKIT_SYSTEM_BSD
    APPLE OR OPENBSD OR FREEBSD OR NETBSD)
cxxkit_set01(CXXKIT_SYSTEM_DARWIN
    APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
cxxkit_set01(CXXKIT_SYSTEM_IOS
    APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
cxxkit_set01(CXXKIT_SYSTEM_TVOS
    APPLE AND CMAKE_SYSTEM_NAME STREQUAL "tvOS")
cxxkit_set01(CXXKIT_SYSTEM_WATCHOS
    APPLE AND CMAKE_SYSTEM_NAME STREQUAL "watchOS")
cxxkit_set01(CXXKIT_SYSTEM_UIKIT
    APPLE AND (IOS OR TVOS OR WATCHOS))
cxxkit_set01(CXXKIT_SYSTEM_MACOS
    APPLE AND NOT UIKIT)
cxxkit_set01(CXXKIT_SYSTEM_UNIX UNIX)
cxxkit_set01(CXXKIT_SYSTEM_WIN32 WIN32)
cxxkit_set01(CXXKIT_SYSTEM_APPLE APPLE)
cxxkit_set01(CXXKIT_SYSTEM_MAC APPLE)


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit set processor variable
#-----------------------------------------------------------------------------------------------------------------------
message(STATUS "Build in processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Build in processor: ${CMAKE_SYSTEM_PROCESSOR}")
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CXXKIT_SYSTEM_PROCESSOR)
cxxkit_set01(CXXKIT_PROCESSOR_I386
    CXXKIT_SYSTEM_PROCESSOR STREQUAL "i386")
cxxkit_set01(CXXKIT_PROCESSOR_I686
    CXXKIT_SYSTEM_PROCESSOR MATCHES "i686")
cxxkit_set01(CXXKIT_PROCESSOR_X86_64
    CXXKIT_SYSTEM_PROCESSOR MATCHES "x86_64")
cxxkit_set01(CXXKIT_PROCESSOR_X86
    CXXKIT_SYSTEM_PROCESSOR MATCHES "x86")
cxxkit_set01(CXXKIT_PROCESSOR_AMD64
    CXXKIT_SYSTEM_PROCESSOR STREQUAL "amd64")
cxxkit_set01(CXXKIT_PROCESSOR_AARCH64
    CXXKIT_SYSTEM_PROCESSOR STREQUAL "aarch64")
cxxkit_set01(CXXKIT_PROCESSOR_ARM64
    CXXKIT_SYSTEM_PROCESSOR STREQUAL "arm64" OR CXXKIT_PROCESSOR_AARCH64)
cxxkit_set01(CXXKIT_PROCESSOR_ARM32
    CXXKIT_SYSTEM_PROCESSOR STREQUAL "arm32")
cxxkit_set01(CXXKIT_PROCESSOR_ARM
    CXXKIT_PROCESSOR_AARCH64 OR CXXKIT_PROCESSOR_ARM64 OR CXXKIT_PROCESSOR_ARM32)


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit set cxx compiler variable
#-----------------------------------------------------------------------------------------------------------------------
message(STATUS "Build in cxx compiler: ${CMAKE_CXX_COMPILER_ID}")
set(CXXKIT_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID})
set(CXXKIT_CXX_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
cxxkit_set01(CXXKIT_CXX_COMPILER_GNU
    CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
cxxkit_set01(CXXKIT_CXX_COMPILER_MSVC
    MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "Msvc")
cxxkit_set01(CXXKIT_CXX_COMPILER_MINGW
    MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "Mingw")
cxxkit_set01(CXXKIT_CXX_COMPILER_CLANG
    CMAKE_CXX_COMPILER_ID MATCHES "Clang|IntelLLVM")
cxxkit_set01(CXXKIT_CXX_COMPILER_APPLE_CLANG
    CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
cxxkit_set01(CXXKIT_CXX_COMPILER_INTEL_LLVM
    CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
cxxkit_set01(CXXKIT_CXX_COMPILER_QCC
    CMAKE_CXX_COMPILER_ID STREQUAL "QCC") # CMP0047


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit arch size variable
#-----------------------------------------------------------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CXXKIT_ARCH_BIT 64)
    set(CXXKIT_ARCH_NAME x64)
    set(CXXKIT_ARCH_64BIT TRUE)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CXXKIT_ARCH_BIT 32)
    set(CXXKIT_ARCH_NAME x86)
    set(CXXKIT_ARCH_32BIT TRUE)
endif()
message(STATUS "Build in bit: ${CXXKIT_ARCH_BIT}")


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit vcpkg triplets variable
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_PROCESSOR_X86_64 OR CXXKIT_PROCESSOR_AMD64)
    if (CXXKIT_ARCH_64BIT)
        set(CXXKIT_VCPKG_TRIPLET_ARCH x64)
    else()
        set(CXXKIT_VCPKG_TRIPLET_ARCH x86)
    endif()
    set(CXXKIT_VCPKG_TRIPLET_ARCH_ARM OFF)
elseif(CXXKIT_PROCESSOR_I686 OR CXXKIT_PROCESSOR_I386)
    set(CXXKIT_VCPKG_TRIPLET_ARCH x86)
    set(CXXKIT_VCPKG_TRIPLET_ARCH_ARM OFF)
elseif(CXXKIT_PROCESSOR_ARM64 OR CXXKIT_PROCESSOR_AARCH64)
    set(CXXKIT_VCPKG_TRIPLET_ARCH arm64)
    set(CXXKIT_VCPKG_TRIPLET_ARCH_ARM ON)
elseif(CXXKIT_PROCESSOR_ARM32)
    set(CXXKIT_VCPKG_TRIPLET_ARCH arm32)
    set(CXXKIT_VCPKG_TRIPLET_ARCH_ARM ON)
else()
    message(FATAL_ERROR "Unknown processor arch.")
endif()

if(CXXKIT_SYSTEM_WIN)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM windows)
elseif(CXXKIT_SYSTEM_IOS)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM ios)
elseif(CXXKIT_SYSTEM_TVOS)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM tvos)
elseif(CXXKIT_SYSTEM_DARWIN)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM osx)
elseif(CXXKIT_SYSTEM_LINUX)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM linux)
elseif(CXXKIT_SYSTEM_ANDROID)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM android)
elseif(CXXKIT_SYSTEM_FREEBSD)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM freebsd)
elseif(CXXKIT_CXX_COMPILER_MINGW)
    set(CXXKIT_VCPKG_TRIPLET_PLATFORM mingw)
else()
    message(FATAL_ERROR "Unknown system platform.")
endif()
set(CXXKIT_VCPKG_TRIPLET "${CXXKIT_VCPKG_TRIPLET_ARCH}-${CXXKIT_VCPKG_TRIPLET_PLATFORM}" CACHE INTERNAL "" FORCE)
message(STATUS "Vcpkg triplet name: ${CXXKIT_VCPKG_TRIPLET}")


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit platform compile arch variable
#-----------------------------------------------------------------------------------------------------------------------
string(TOUPPER "${CMAKE_BUILD_TYPE}" CXXKIT_UPPER_BUILD_TYPE)
string(TOLOWER "${CMAKE_BUILD_TYPE}" CXXKIT_LOWER_BUILD_TYPE)
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CXXKIT_LOWER_SYSTEM_NAME)
string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" CXXKIT_LOWER_CXX_COMPILER_ID)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CXXKIT_LOWER_SYSTEM_PROCESSOR)
string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" CXXKIT_LOWER_HOST_SYSTEM_NAME)
set(CXXKIT_X64_PROCESSORS "amd64" "x64" "x86_64")
set(CXXKIT_X86_PROCESSORS "i386" "i686" "x86")
set(CXXKIT_ARM32_PROCESSORS "arm32" "arm")
set(CXXKIT_AARCH64_PROCESSORS "aarch64")
set(CXXKIT_ARM64_PROCESSORS "arm64")
set(CXXKIT_ARMV7_PROCESSORS "armv7-a")
if(CXXKIT_LOWER_SYSTEM_PROCESSOR IN_LIST CXXKIT_X64_PROCESSORS)
    set(CXXKIT_PROCESSOR_MERGE_NAME x64)
elseif(CXXKIT_LOWER_SYSTEM_PROCESSOR IN_LIST CXXKIT_X86_PROCESSORS)
    set(CXXKIT_PROCESSOR_MERGE_NAME x86)
elseif(CXXKIT_LOWER_SYSTEM_PROCESSOR IN_LIST CXXKIT_ARM32_PROCESSORS)
    set(CXXKIT_PROCESSOR_MERGE_NAME arm32)
elseif(CXXKIT_LOWER_SYSTEM_PROCESSOR IN_LIST CXXKIT_ARM64_PROCESSORS)
    set(CXXKIT_PROCESSOR_MERGE_NAME arm64)
elseif(CXXKIT_LOWER_SYSTEM_PROCESSOR IN_LIST CXXKIT_ARMV7_PROCESSORS)
    set(CXXKIT_PROCESSOR_MERGE_NAME armv7)
elseif(CXXKIT_LOWER_SYSTEM_PROCESSOR IN_LIST CXXKIT_AARCH64_PROCESSORS)
    set(CXXKIT_PROCESSOR_MERGE_NAME aarch64)
else()
    message(FATAL_ERROR "Unknown system processor ${CMAKE_SYSTEM_PROCESSOR}.")
endif()
set(CXXKIT_PLATFORM_NAME "${CXXKIT_LOWER_SYSTEM_NAME}-${CXXKIT_PROCESSOR_MERGE_NAME}")
set(CXXKIT_PLATFORM_COMPILER_NAME "${CXXKIT_PLATFORM_NAME}-${CXXKIT_LOWER_CXX_COMPILER_ID}")
message(STATUS "Platform name: ${CXXKIT_PLATFORM_NAME}")
message(STATUS "Platform compiler name: ${CXXKIT_PLATFORM_COMPILER_NAME}")
set(CXXKIT_HOST_PLATFORM_NAME "${CXXKIT_LOWER_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "Host platform name: ${CXXKIT_HOST_PLATFORM_NAME}")


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit mkspecs version
#-----------------------------------------------------------------------------------------------------------------------
if(CXXKIT_SYSTEM_WIN32)
    set(CXXKIT_DEFAULT_PLATFORM_DEFINITIONS WIN32 _ENABLE_EXTENDED_ALIGNED_STORAGE)
    if(CXXKIT_ARCH_64BIT)
        list(APPEND CXXKIT_DEFAULT_PLATFORM_DEFINITIONS WIN64 _WIN64)
    endif()
    if(CXXKIT_CXX_COMPILER_MSVC)
        if(CXXKIT_CXX_COMPILER_CLANG)
            set(CXXKIT_DEFAULT_MKSPEC win32-clang-msvc)
        elseif(CXXKIT_PROCESSOR_ARM64)
            set(CXXKIT_DEFAULT_MKSPEC win32-arm64-msvc)
        else()
            set(CXXKIT_DEFAULT_MKSPEC win32-msvc)
        endif()
    elseif(CXXKIT_CXX_COMPILER_CLANG AND CXXKIT_CXX_COMPILER_MINGW)
        set(CXXKIT_DEFAULT_MKSPEC win32-clang-g++)
    elseif(CXXKIT_CXX_COMPILER_MINGW)
        set(CXXKIT_DEFAULT_MKSPEC win32-g++)
    endif()

    if(CXXKIT_CXX_COMPILER_MINGW)
        list(APPEND CXXKIT_DEFAULT_PLATFORM_DEFINITIONS MINGW_HAS_SECURE_API=1)
    endif()
elseif(CXXKIT_SYSTEM_LINUX)
    if(CXXKIT_CXX_COMPILER_GNU)
        set(CXXKIT_DEFAULT_MKSPEC linux-g++)
    elseif(CXXKIT_CXX_COMPILER_CLANG)
        set(CXXKIT_DEFAULT_MKSPEC linux-clang)
    endif()
elseif(CXXKIT_SYSTEM_ANDROID)
    if(CXXKIT_CXX_COMPILER_GNU)
        set(CXXKIT_DEFAULT_MKSPEC android-g++)
    elseif(CXXKIT_CXX_COMPILER_CLANG)
        set(CXXKIT_DEFAULT_MKSPEC android-clang)
    endif()
elseif(CXXKIT_SYSTEM_IOS)
    set(CXXKIT_DEFAULT_MKSPEC macx-ios-clang)
elseif(CXXKIT_SYSTEM_APPLE)
    set(CXXKIT_DEFAULT_MKSPEC macx-clang)
elseif(CXXKIT_SYSTEM_WASM)
    set(CXXKIT_DEFAULT_MKSPEC wasm-emscripten)
elseif(CXXKIT_SYSTEM_QNX)
    # Certain POSIX defines are not set if we don't compile with -std=gnuXX
    set(CXXKIT_ENABLE_CXX_EXTENSIONS ON)

    list(APPEND CXXKIT_DEFAULT_PLATFORM_DEFINITIONS _FORTIFY_SOURCE=2 _REENTRANT)

    set(compiler_aarch64le aarch64le)
    set(compiler_armle-v7 armv7le)
    set(compiler_x86-64 x86_64)
    set(compiler_x86 x86)
    foreach(arch aarch64le armle-v7 x86-64 x86)
        if(CMAKE_CXX_COMPILER_TARGET MATCHES "${compiler_${arch}}$")
            set(CXXKIT_DEFAULT_MKSPEC qnx-${arch}-qcc)
        endif()
    endforeach()
elseif(CXXKIT_SYSTEM_FREEBSD)
    if(CXXKIT_CXX_COMPILER_CLANG)
        set(CXXKIT_DEFAULT_MKSPEC freebsd-clang)
    elseif(CXXKIT_CXX_COMPILER_GNU)
        set(CXXKIT_DEFAULT_MKSPEC freebsd-g++)
    endif()
elseif(CXXKIT_SYSTEM_NETBSD)
    set(CXXKIT_DEFAULT_MKSPEC netbsd-g++)
elseif(CXXKIT_SYSTEM_OPENBSD)
    set(CXXKIT_DEFAULT_MKSPEC openbsd-g++)
elseif(CXXKIT_SYSTEM_SOLARIS)
    if(CXXKIT_CXX_COMPILER_GNU)
        if(CXXKIT_ARCH_64BIT)
            set(CXXKIT_DEFAULT_MKSPEC solaris-g++-64)
        else()
            set(CXXKIT_DEFAULT_MKSPEC solaris-g++)
        endif()
    else()
        if(CXXKIT_ARCH_64BIT)
            set(CXXKIT_DEFAULT_MKSPEC solaris-cc-64)
        else()
            set(CXXKIT_DEFAULT_MKSPEC solaris-cc)
        endif()
    endif()
elseif(CXXKIT_SYSTEM_HURD)
    set(CXXKIT_DEFAULT_MKSPEC hurd-g++)
endif()

if(NOT CXXKIT_DEFAULT_MKSPEC)
    message(FATAL_ERROR "mkspec not Detected!")
else()
    message(STATUS "Build in mkspec: ${CXXKIT_DEFAULT_MKSPEC}")
endif()

if(NOT DEFINED CXXKIT_DEFAULT_PLATFORM_DEFINITIONS)
    set(CXXKIT_DEFAULT_PLATFORM_DEFINITIONS "")
endif()

set(CXXKIT_PLATFORM_DEFINITIONS ${CXXKIT_DEFAULT_PLATFORM_DEFINITIONS} CACHE STRING "CxxKit platform specific pre-processor defines")


#-----------------------------------------------------------------------------------------------------------------------
# CxxKit parse version
#-----------------------------------------------------------------------------------------------------------------------
# Parses a version string like "xx.yy.zz" and sets the major, minor and patch variables.
function(cxxkit_parse_version_string version_string out_var_prefix)
    string(REPLACE "." ";" version_list ${version_string})
    list(LENGTH version_list length)

    set(out_var "${out_var_prefix}_MAJOR")
    set(value "")
    if(length GREATER 0)
        list(GET version_list 0 value)
        list(REMOVE_AT version_list 0)
        math(EXPR length "${length}-1")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)

    set(out_var "${out_var_prefix}_MINOR")
    set(value "")
    if(length GREATER 0)
        list(GET version_list 0 value)
        set(${out_var} "${value}" PARENT_SCOPE)
        list(REMOVE_AT version_list 0)
        math(EXPR length "${length}-1")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)

    set(out_var "${out_var_prefix}_PATCH")
    set(value "")
    if(length GREATER 0)
        list(GET version_list 0 value)
        set(${out_var} "${value}" PARENT_SCOPE)
        list(REMOVE_AT version_list 0)
        math(EXPR length "${length}-1")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# Set up the separate version components for the compiler version, to allow mapping of qmake
# conditions like 'equals(CXXKIT_GCC_MAJOR_VERSION,5)'.
if(CMAKE_CXX_COMPILER_VERSION)
    cxxkit_parse_version_string("${CMAKE_CXX_COMPILER_VERSION}" "CXXKIT_COMPILER_VERSION")
endif()
