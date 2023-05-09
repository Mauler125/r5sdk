cmake_minimum_required( VERSION 3.16 )

project( filesystem )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "basefilesystem.cpp"
    "basefilesystem.h"
    "filesystem.cpp"
    "filesystem.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/ifile.h"
    "${ENGINE_SOURCE_DIR}/public/ifilesystem.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" "${ENGINE_SOURCE_DIR}/tier1/" )
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM tier0 )
