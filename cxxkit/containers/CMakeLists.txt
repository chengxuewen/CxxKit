cmake_minimum_required(VERSION ${CXXKIT_MIN_NEW_POLICY_CMAKE_VERSION}...${CXXKIT_MAX_NEW_POLICY_CMAKE_VERSION})

cxxkit_configure_library_begin(CxxKitContainers)
add_library(cxxkit_containers INTERFACE)
add_library(cxxkit::containers ALIAS cxxkit_containers)
set_property(TARGET cxxkit_containers PROPERTY EXPORT_NAME containers)
target_include_directories(cxxkit_containers INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(cxxkit_containers INTERFACE cxxkit::base)
target_compile_features(cxxkit_containers INTERFACE cxx_std_11)
cxxkit_configure_library_end(CxxKitContainers)

if(CXXKIT_BUILD_INSTALL)
    install(TARGETS cxxkit_containers EXPORT cxxkitTargets)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/cxxkit/containers/
        DESTINATION include/cxxkit/containers
        FILES_MATCHING PATTERN "*.hpp")
endif()
