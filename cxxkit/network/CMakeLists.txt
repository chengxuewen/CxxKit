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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
########################################################################################################################

cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitNetwork)

cxxkit_find_package(Libcpr PROVIDED_TARGETS CxxKitWrapLibcpr::WrapLibcpr)
# Backend selection (Task 4, D41): uv is the default; asio is the second (vendored,
# header-only standalone). Exactly ONE backend .cpp is compiled/linked per build —
# make_stream_backend() has one definition per backend and no #ifdef anywhere.
set(CXXKIT_NETWORK_BACKEND "uv" CACHE STRING "cxxkit network backend (uv|asio)")
set_property(CACHE CXXKIT_NETWORK_BACKEND PROPERTY STRINGS uv asio)
if(CXXKIT_NETWORK_BACKEND STREQUAL "asio")
    cxxkit_find_package(Asio PROVIDED_TARGETS CxxKitWrapAsio::WrapAsio)
    list(APPEND _backend_sources detail/stream_backend_asio.cpp)
    set(_backend_libs CxxKitWrapAsio::WrapAsio)
else()
    # libuv is the default backend (D38): TcpSocket/TcpServer drive uv_tcp_t directly.
    cxxkit_find_package(Libuv PROVIDED_TARGETS CxxKitWrapLibuv::WrapLibuv)
    list(APPEND _backend_sources detail/stream_backend_uv.cpp detail/dgram_backend_uv.cpp)
    set(_backend_libs CxxKitWrapLibuv::WrapLibuv)
endif()

cxxkit_add_library(cxxkit_network
    SOURCES http.cpp tcp_socket.cpp tcp_server.cpp socket_error.cpp tls_socket.cpp udp_socket.cpp ${_backend_sources}
    SOURCES http.cpp tcp_socket.cpp tcp_server.cpp socket_error.cpp tls_socket.cpp ${_backend_sources}
    PUBLIC_LIBRARIES cxxkit::base cxxkit::text cxxkit::thread cxxkit::memory cxxkit::tools cxxkit::kernel CxxKitWrapLibcpr::WrapLibcpr CxxKitWrapMbedTLS::WrapMbedTLS ${_backend_libs}
    EXPORT_NAME network
    FOLDER "CxxKit/libs")
# The backend choice must reach the preprocessor (PIT-48): backend-conditional include/branch
# gating keys off CXXKIT_NETWORK_BACKEND_ASIO; core_config carries no module switches.
if(CXXKIT_NETWORK_BACKEND STREQUAL "asio")
    target_compile_definitions(cxxkit_network PUBLIC CXXKIT_NETWORK_BACKEND_ASIO=1)
endif()
cxxkit_install_public_wrap_headers(cxxkit_network
    WRAPS
    CxxKitWrapLibcpr::WrapLibcpr|cpr
    CxxKitWrapLibcurl::WrapLibcurl|curl
    CxxKitWrapMbedTLS::WrapMbedTLS|mbedtls)

cxxkit_generate_pkg_config(cxxkit_network network)
cxxkit_configure_library_end(CxxKitNetwork)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_network EXPORT cxxkitTargets)
    # Backend-conditional header exclusion (Task 4.2b half): the non-selected backend's detail
    # header would drag its native include (uv.h / asio.hpp) into installed consumers. The
    # CxxKitConfig/.pc consumer half of this step is DEFERRED (later task).
    if(CXXKIT_NETWORK_BACKEND STREQUAL "asio")
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/network/
            DESTINATION include/cxxkit/network
            FILES_MATCHING PATTERN "*.hpp"
            PATTERN "detail/stream_backend_uv.hpp" EXCLUDE
            PATTERN "detail/dgram_backend_uv.hpp" EXCLUDE)
    else()
        install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/network/
            DESTINATION include/cxxkit/network
            FILES_MATCHING PATTERN "*.hpp"
            PATTERN "detail/stream_backend_asio.hpp" EXCLUDE
            PATTERN "detail/dgram_backend_asio.hpp" EXCLUDE)
    endif()
endif()
