cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "datacache" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "mdlcache.cpp"
    "mdlcache.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/datacache/idatacache.h"
    "${ENGINE_SOURCE_DIR}/public/datacache/imdlcache.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" "${ENGINE_SOURCE_DIR}/tier1/" )
