########################################################################################################################
#
# Library: cxxkit
#
# Copyright (C) 2025~Present ChengXueWen.
#
# License: MIT License
#
# Install helper: cxxkit_install — thin wrapper over install() that skips install rules when
# CXXKIT_BUILD_INSTALL is OFF (dev build). Slimmed from OpenCTK OpenCTKInstallHelpers.cmake.
#
########################################################################################################################

function(cxxkit_install)
    if(CXXKIT_BUILD_INSTALL)
        install(${ARGV})
    endif()
endfunction()
