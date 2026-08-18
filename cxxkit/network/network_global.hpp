#ifndef _CXXKIT_NETWORK_GLOBAL_HPP
#define _CXXKIT_NETWORK_GLOBAL_HPP

#include <cxxkit/base/global.hpp>

/***********************************************************************************************************************
   cxxkit Compiler specific cmds for export and import code to DLL
***********************************************************************************************************************/
#ifdef CXXKIT_BUILD_SHARED_NETWORK // compiled as a dynamic lib.
#   ifdef CXXKIT_BUILDING_NETWORK_LIB // defined if we are building the lib
#       define CXXKIT_NETWORK_API CXXKIT_DECLARE_EXPORT
#   else
#       define CXXKIT_NETWORK_API CXXKIT_DECLARE_IMPORT
#   endif
#   define CXXKIT_NETWORK_HIDDEN CXXKIT_DECLARE_HIDDEN
#else // compiled as a static lib.
#   define CXXKIT_NETWORK_API
#   define CXXKIT_NETWORK_HIDDEN
#endif

#endif // _CXXKIT_NETWORK_GLOBAL_HPP
