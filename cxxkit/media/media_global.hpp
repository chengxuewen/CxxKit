#pragma once

#include <cxxkit/base/global.hpp>

/***********************************************************************************************************************
   cxxkit Compiler specific cmds for export and import code to DLL
***********************************************************************************************************************/
#ifdef CXXKIT_BUILD_SHARED_MEDIA      // compiled as a dynamic lib.
#    ifdef CXXKIT_BUILDING_MEDIA_LIB  // defined if we are building the lib
#        define CXXKIT_MEDIA_API CXXKIT_DECLARE_EXPORT
#    else
#        define CXXKIT_MEDIA_API CXXKIT_DECLARE_IMPORT
#    endif
#    define CXXKIT_MEDIA_HIDDEN CXXKIT_DECLARE_HIDDEN
#else // compiled as a static lib.
#    define CXXKIT_MEDIA_API
#    define CXXKIT_MEDIA_HIDDEN
#endif
