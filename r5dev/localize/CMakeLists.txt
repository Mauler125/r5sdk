cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "localize" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Runtime"
    "localize.cpp"
    "localize.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/localize/ilocalize.h"
)

end_sources()
