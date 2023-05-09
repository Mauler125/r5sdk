cmake_minimum_required( VERSION 3.16 )
project( r5sdk )

include( "r5dev/cmake/Configure.cmake" )
include( "r5dev/cmake/Macros.cmake" )
include( "r5dev/cmake/Options.cmake" )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

set( ENGINE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/r5dev )
set( GLOBAL_PCH ${ENGINE_SOURCE_DIR}/core/stdafx.h ) # Global precompiled header shared among all libraries

define_compiler_variables()
setup_build_configurations()
apply_project_settings()

include_directories( ${ENGINE_SOURCE_DIR} )
include_directories( ${ENGINE_SOURCE_DIR}/public )
include_directories( ${ENGINE_SOURCE_DIR}/thirdparty )

# Include the subdirectories that contain the individual projects
add_subdirectory( ${ENGINE_SOURCE_DIR} )
