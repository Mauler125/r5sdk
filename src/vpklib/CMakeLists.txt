cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vpklib" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "packedstore.cpp"
    "packedstore.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/ipackedstore.h"
)

end_sources()
