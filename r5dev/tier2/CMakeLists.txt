cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "tier2" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Utility"
    "curlutils.cpp"
    "fileutils.cpp"
    "meshutils.cpp"
    "renderutils.cpp"
    "socketcreator.cpp"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/tier2/curlutils.h"
    "${ENGINE_SOURCE_DIR}/public/tier2/fileutils.h"
    "${ENGINE_SOURCE_DIR}/public/tier2/meshutils.h"
    "${ENGINE_SOURCE_DIR}/public/tier2/renderutils.h"
    "${ENGINE_SOURCE_DIR}/public/tier2/socketcreator.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" "${ENGINE_SOURCE_DIR}/tier1/" )
