cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitNumerics)
add_library(cxxkit_numerics INTERFACE)
add_library(cxxkit::numerics ALIAS cxxkit_numerics)
target_include_directories(cxxkit_numerics INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_numerics INTERFACE cxxkit::base)
target_compile_features(cxxkit_numerics INTERFACE cxx_std_11)
cxxkit_configure_library_end(CxxKitNumerics)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_numerics EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/numerics/
        DESTINATION include/cxxkit/numerics
        FILES_MATCHING PATTERN "*.hpp")
endif()
