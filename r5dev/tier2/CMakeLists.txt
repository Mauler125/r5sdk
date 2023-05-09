cmake_minimum_required( VERSION 3.16 )

project( tier2 )
add_library( ${PROJECT_NAME} )

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
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM tier0 )
