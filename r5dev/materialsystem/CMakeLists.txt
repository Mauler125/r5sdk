cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "materialsystem" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "cmaterialglue.cpp"
    "cmaterialglue.h"
    "cmaterialsystem.cpp"
    "cmaterialsystem.h"
    "cshaderglue.cpp"
    "cshaderglue.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/imaterial.h"
    "${ENGINE_SOURCE_DIR}/public/imaterialinternal.h"
    "${ENGINE_SOURCE_DIR}/public/imaterialsystem.h"
)

end_sources()
