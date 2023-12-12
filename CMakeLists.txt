cmake_minimum_required( VERSION 3.16 )
project( r5sdk )

set( ENGINE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src" CACHE PATH "Engine source directory" )

include( "${ENGINE_SOURCE_DIR}/cmake/Configure.cmake" )
include( "${ENGINE_SOURCE_DIR}/cmake/Macros.cmake" )
include( "${ENGINE_SOURCE_DIR}/cmake/Options.cmake" )

initial_setup()
setup_build_configurations()
define_compiler_variables()
apply_project_settings()

include_directories( "${ENGINE_SOURCE_DIR}" )
include_directories( "${ENGINE_SOURCE_DIR}/public" )
include_directories( "${THIRDPARTY_SOURCE_DIR}" )

# Include the subdirectories that contain the individual projects
add_subdirectory( "${ENGINE_SOURCE_DIR}" )
add_subdirectory( "${ENGINE_SOURCE_DIR}/tiny-aes" )