# -----------------------------------------------------------------------------
# Initial setup for build system
# -----------------------------------------------------------------------------
macro( initial_setup )
    set( CMAKE_CXX_STANDARD 17 )
    set( CMAKE_CXX_STANDARD_REQUIRED True )

    set( THIRDPARTY_SOURCE_DIR "${ENGINE_SOURCE_DIR}/thirdparty" CACHE PATH "Thirdparty source directory" )
    set( BUILD_OUTPUT_DIR "game" CACHE PATH "Build output directory" )

    set( GLOBAL_PCH
        "${ENGINE_SOURCE_DIR}/core/stdafx.h"
    ) # Global precompiled header shared among all libraries

    set_property( GLOBAL PROPERTY USE_FOLDERS ON ) # Use filters
endmacro()

# -----------------------------------------------------------------------------
# Set global configuration types
# -----------------------------------------------------------------------------
macro( setup_build_configurations )
    set( CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release" CACHE STRING "" FORCE )
endmacro()
