cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitFunctional)
add_library(cxxkit_functional INTERFACE)
add_library(cxxkit::functional ALIAS cxxkit_functional)
set_property(TARGET cxxkit_functional PROPERTY EXPORT_NAME functional)
target_include_directories(cxxkit_functional INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_functional INTERFACE cxxkit::base)
target_compile_features(cxxkit_functional INTERFACE cxx_std_11)
cxxkit_configure_library_end(CxxKitFunctional)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_functional EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/functional/
        DESTINATION include/cxxkit/functional
        FILES_MATCHING PATTERN "*.hpp")
endif()
