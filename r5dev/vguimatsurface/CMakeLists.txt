cmake_minimum_required( VERSION 3.16 )

project( vguimatsurface )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "MatSystemSurface.cpp"
    "MatSystemSurface.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM vpc )
