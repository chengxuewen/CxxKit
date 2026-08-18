cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitKernel)

add_library(cxxkit_kernel
    ${PROJECT_SOURCE_DIR}/src/kernel/application.cpp
    ${PROJECT_SOURCE_DIR}/src/kernel/event.cpp
    ${PROJECT_SOURCE_DIR}/src/kernel/event_loop.cpp
    ${PROJECT_SOURCE_DIR}/src/kernel/object.cpp
    ${PROJECT_SOURCE_DIR}/src/kernel/signals.cpp)
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

cxxkit_configure_library_end(CxxKitKernel)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_kernel EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/kernel/
        DESTINATION include/cxxkit/kernel
        FILES_MATCHING PATTERN "*.hpp")
endif()
