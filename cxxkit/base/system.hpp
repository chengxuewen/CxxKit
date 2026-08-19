/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#ifndef _CXXKIT_SYSTEM_HPP
#define _CXXKIT_SYSTEM_HPP

/*
   The operating system, must be one of: (CXXKIT_OS_x)

     DARWIN   - Any Darwin system (macOS, iOS, watchOS, tvOS)
     MACOS    - macOS
     IOS      - iOS
     WATCHOS  - watchOS
     TVOS     - tvOS
     WIN32    - Win32 (Windows 2000/XP/Vista/7 and Windows Server 2003/2008)
     CYGWIN   - Cygwin
     SOLARIS  - Sun Solaris
     HPUX     - HP-UX
     LINUX    - Linux [has variants]
     FREEBSD  - FreeBSD [has variants]
     NETBSD   - NetBSD
     OPENBSD  - OpenBSD
     INTERIX  - Interix
     AIX      - AIX
     HURD     - GNU Hurd
     QNX      - QNX [has variants]
     QNX6     - QNX RTP 6.1
     LYNX     - LynxOS
     BSD4     - Any BSD 4.4 system
     UNIX     - Any UNIX BSD/SYSV system
     ANDROID  - Android platform

   The following operating systems have variants:
     LINUX    - both CXXKIT_OS_LINUX and CXXKIT_OS_ANDROID are defined when building for Android
              - only CXXKIT_OS_LINUX is defined if building for other Linux systems
     MACOS    - both CXXKIT_OS_BSD4 and CXXKIT_OS_IOS are defined when building for iOS
              - both CXXKIT_OS_BSD4 and CXXKIT_OS_MACOS are defined when building for macOS
     FREEBSD  - CXXKIT_OS_FREEBSD is defined only when building for FreeBSD with a BSD userland
              - CXXKIT_OS_FREEBSD_KERNEL is always defined on FreeBSD, even if the userland is from GNU
*/

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#    include <TargetConditionals.h>
#    if defined(TARGET_OS_MAC) && TARGET_OS_MAC
#        define CXXKIT_OS_DARWIN
#        define CXXKIT_OS_BSD4
#        ifdef __LP64__
#            define CXXKIT_OS_DARWIN64
#        else
#            define CXXKIT_OS_DARWIN32
#        endif
#        if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#            define CXXKIT_PLATFORM_UIKIT
#            if defined(TARGET_OS_WATCH) && TARGET_OS_WATCH
#                define CXXKIT_OS_WATCHOS
#            elif defined(TARGET_OS_TV) && TARGET_OS_TV
#                define CXXKIT_OS_TVOS
#            else
// TARGET_OS_IOS is only available in newer SDKs,
// so assume any other iOS-based platform is iOS for now
#                define CXXKIT_OS_IOS
#            endif
#        else
// TARGET_OS_OSX is only available in newer SDKs,
// so assume any non iOS-based platform is macOS for now
#            define CXXKIT_OS_MACOS
#        endif
#    else
#        error "cxxkit has not been ported to this Apple platform"
#    endif
#elif defined(__WEBOS__)
#    define CXXKIT_OS_WEBOS
#    define CXXKIT_OS_LINUX
#elif defined(__ANDROID__) || defined(ANDROID)
#    define CXXKIT_OS_ANDROID
#    define CXXKIT_OS_LINUX
#elif defined(__CYGWIN__)
#    define CXXKIT_OS_CYGWIN
#elif !defined(SAG_COM) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) &&                  \
    (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#    define CXXKIT_OS_WIN32
#    define CXXKIT_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#    define CXXKIT_OS_WIN32
#elif defined(__sun) || defined(sun)
#    define CXXKIT_OS_SOLARIS
#elif defined(hpux) || defined(__hpux)
#    define CXXKIT_OS_HPUX
#elif defined(__native_client__)
#    define CXXKIT_OS_NACL
#elif defined(__EMSCRIPTEN__)
#    define CXXKIT_OS_WASM
#elif defined(__linux__) || defined(__linux)
#    define CXXKIT_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#    ifndef __FreeBSD_kernel__
#        define CXXKIT_OS_FREEBSD
#    endif
#    define CXXKIT_OS_FREEBSD_KERNEL
#    define CXXKIT_OS_BSD4
#elif defined(__NetBSD__)
#    define CXXKIT_OS_NETBSD
#    define CXXKIT_OS_BSD4
#elif defined(__OpenBSD__)
#    define CXXKIT_OS_OPENBSD
#    define CXXKIT_OS_BSD4
#elif defined(__INTERIX)
#    define CXXKIT_OS_INTERIX
#    define CXXKIT_OS_BSD4
#elif defined(_AIX)
#    define CXXKIT_OS_AIX
#elif defined(__Lynx__)
#    define CXXKIT_OS_LYNX
#elif defined(__GNU__)
#    define CXXKIT_OS_HURD
#elif defined(__QNXNTO__)
#    define CXXKIT_OS_QNX
#elif defined(__INTEGRITY)
#    define CXXKIT_OS_INTEGRITY
#elif defined(__rtems__)
#    define CXXKIT_OS_RTEMS
#elif defined(VXWORKS) /* there is no "real" VxWorks define - this has to be set in the mkspec! */
#    define CXXKIT_OS_VXWORKS
#elif defined(__HAIKU__)
#    define CXXKIT_OS_HAIKU
#elif defined(__MACH__)
#    define CXXKIT_OS_MACH
#else
#    error "cxxkit has not been ported to this OS"
#endif

#if defined(CXXKIT_OS_WIN32) || defined(CXXKIT_OS_WIN64)
#    define CXXKIT_OS_WINDOWS
#    define CXXKIT_OS_WIN
// On Windows, pointers to dllimport'ed variables are not constant expressions,
// so to keep to certain initializations (like QMetaObject) constexpr, we need
// to use functions instead.
#    define CXXKIT_NO_DATA_RELOCATION
#endif

#if defined(CXXKIT_OS_WIN)
#    undef CXXKIT_OS_UNIX
#elif !defined(CXXKIT_OS_UNIX)
#    define CXXKIT_OS_UNIX
#endif

// Compatibility synonyms
#ifdef CXXKIT_OS_DARWIN
#    define CXXKIT_OS_MAC
#endif
#ifdef CXXKIT_OS_DARWIN32
#    define CXXKIT_OS_MAC32
#endif
#ifdef CXXKIT_OS_DARWIN64
#    define CXXKIT_OS_MAC64
#endif
#ifdef CXXKIT_OS_MACOS
#    define CXXKIT_OS_MACX
#    define CXXKIT_OS_OSX
#endif

#ifdef CXXKIT_OS_DARWIN
#    include <Availability.h>
#    include <AvailabilityMacros.h>
#    ifdef CXXKIT_OS_MACOS
#        if !defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED < __MAC_10_6
#            undef __MAC_OS_X_VERSION_MIN_REQUIRED
#            define __MAC_OS_X_VERSION_MIN_REQUIRED __MAC_10_6
#        endif
#        if !defined(MAC_OS_X_VERSION_MIN_REQUIRED) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
#            undef MAC_OS_X_VERSION_MIN_REQUIRED
#            define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_6
#        endif
#    endif
// Numerical checks are preferred to named checks, but to be safe
// we define the missing version names in case cxxkit uses them.
#    if !defined(__MAC_10_11)
#        define __MAC_10_11 101100
#    endif
#    if !defined(__MAC_10_12)
#        define __MAC_10_12 101200
#    endif
#    if !defined(__MAC_10_13)
#        define __MAC_10_13 101300
#    endif
#    if !defined(__MAC_10_14)
#        define __MAC_10_14 101400
#    endif
#    if !defined(__MAC_10_15)
#        define __MAC_10_15 101500
#    endif
#    if !defined(__MAC_10_16)
#        define __MAC_10_16 101600
#    endif
#    if !defined(MAC_OS_X_VERSION_10_11)
#        define MAC_OS_X_VERSION_10_11 __MAC_10_11
#    endif
#    if !defined(MAC_OS_X_VERSION_10_12)
#        define MAC_OS_X_VERSION_10_12 __MAC_10_12
#    endif
#    if !defined(MAC_OS_X_VERSION_10_13)
#        define MAC_OS_X_VERSION_10_13 __MAC_10_13
#    endif
#    if !defined(MAC_OS_X_VERSION_10_14)
#        define MAC_OS_X_VERSION_10_14 __MAC_10_14
#    endif
#    if !defined(MAC_OS_X_VERSION_10_15)
#        define MAC_OS_X_VERSION_10_15 __MAC_10_15
#    endif
#    if !defined(MAC_OS_X_VERSION_10_16)
#        define MAC_OS_X_VERSION_10_16 __MAC_10_16
#    endif
#    if !defined(__IPHONE_10_0)
#        define __IPHONE_10_0 100000
#    endif
#    if !defined(__IPHONE_10_1)
#        define __IPHONE_10_1 100100
#    endif
#    if !defined(__IPHONE_10_2)
#        define __IPHONE_10_2 100200
#    endif
#    if !defined(__IPHONE_10_3)
#        define __IPHONE_10_3 100300
#    endif
#    if !defined(__IPHONE_11_0)
#        define __IPHONE_11_0 110000
#    endif
#    if !defined(__IPHONE_12_0)
#        define __IPHONE_12_0 120000
#    endif
#endif

#ifdef __LSB_VERSION__
#    if __LSB_VERSION__ < 40
#        error "This version of the Linux Standard Base is unsupported"
#    endif
#    ifndef CXXKIT_LINUXBASE
#        define CXXKIT_LINUXBASE
#    endif
#endif

#endif // _CXXKIT_SYSTEM_HPP
