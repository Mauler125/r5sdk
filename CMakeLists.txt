cmake_minimum_required( VERSION 3.16 )
project( r5sdk )

include( "r5dev/cmake/Configure.cmake" )
include( "r5dev/cmake/Macros.cmake" )
include( "r5dev/cmake/Options.cmake" )

initial_setup()
setup_build_configurations()
define_compiler_variables()
apply_project_settings()

include_directories( "${ENGINE_SOURCE_DIR}" )
include_directories( "${ENGINE_SOURCE_DIR}/public" )
include_directories( "${ENGINE_SOURCE_DIR}/thirdparty" )

# Include the subdirectories that contain the individual projects
add_subdirectory( "${ENGINE_SOURCE_DIR}" )
