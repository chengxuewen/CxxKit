########################################################################################################################
#
# Library: CxxKit
#
# Copyright (C) 2025~Present ChengXueWen.
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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

# cxxkit::profiling — profiling sublibrary (Tracy backend, opt-in).
# Always available as a no-op header-only target; activates when CXXKIT_ENABLE_LIB_TRACY=ON.

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitProfiling)

cxxkit_add_library(cxxkit_profiling
    INTERFACE
    EXPORT_NAME profiling
    FOLDER "CxxKit/libs")
if(CXXKIT_ENABLE_LIB_TRACY)
    cxxkit_find_package(Tracy PROVIDED_TARGETS CxxKitWrapTracy::WrapTracy)
    # Link Tracy's own exported target directly: TracyConfig ships with Tracy (vcpkg-style),
    # so the installed link interface resolves via find_dependency(Tracy) in cxxkitConfig.cmake.
    target_link_libraries(cxxkit_profiling INTERFACE Tracy::TracyClient)
    # TRACY_ENABLE must be defined in consuming TUs for ZoneScoped to expand to real code.
    target_compile_definitions(cxxkit_profiling INTERFACE CXXKIT_PROFILING_ENABLED=1 TRACY_ENABLE)
    # Namespace Tracy headers as <cxxkit/3rdparty/tracy/...> (build + install trees).
    cxxkit_install_public_wrap_headers(cxxkit_profiling
        WRAPS
        CxxKitWrapTracy::WrapTracy|tracy)
    cxxkit_generate_pkg_config(cxxkit_profiling profiling
        DESCRIPTION "cxxkit profiling module (Tracy backend)")
endif()

cxxkit_configure_library_end(CxxKitProfiling)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_profiling EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/profiling/
        DESTINATION include/cxxkit/profiling
        FILES_MATCHING PATTERN "*.hpp")
endif()
