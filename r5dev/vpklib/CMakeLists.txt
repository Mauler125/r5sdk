cmake_minimum_required( VERSION 3.16 )

project( vpklib )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "packedstore.cpp"
    "packedstore.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/ipackedstore.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
