cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitKernel)

file(GLOB _cxxkit_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/detail/*.hpp)
add_library(cxxkit_kernel
    ${_cxxkit_headers}
    application.cpp
    event.cpp
    event_loop.cpp
    object.cpp
    signals.cpp)
add_library(cxxkit::kernel ALIAS cxxkit_kernel)

target_include_directories(cxxkit_kernel PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_kernel PUBLIC cxxkit::base cxxkit::memory cxxkit::tools)
target_compile_features(cxxkit_kernel PUBLIC cxx_std_11)
set_target_properties(cxxkit_kernel PROPERTIES
    EXPORT_NAME kernel
    CXX_STANDARD ${CXXKIT_FEATURE_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

cxxkit_generate_pkg_config(cxxkit_kernel kernel)
cxxkit_configure_library_end(CxxKitKernel)

set_property(TARGET cxxkit_kernel PROPERTY FOLDER "CxxKit/libs")
if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_kernel EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/kernel/
        DESTINATION include/cxxkit/kernel
        FILES_MATCHING PATTERN "*.hpp")
endif()
