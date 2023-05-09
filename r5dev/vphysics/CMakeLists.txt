cmake_minimum_required( VERSION 3.16 )

project( vphysics )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "physics_collide.cpp"
    "QHull.cpp"
    "QHull.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/trace.h"
    "${ENGINE_SOURCE_DIR}/public/gametrace.h"
    "${ENGINE_SOURCE_DIR}/public/cmodel.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
