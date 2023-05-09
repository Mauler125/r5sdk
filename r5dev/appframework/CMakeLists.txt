cmake_minimum_required( VERSION 3.16 )

project( appframework )
add_library( ${PROJECT_NAME} )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "IAppSystemGroup.cpp"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/appframework/IAppSystem.h"
    "${ENGINE_SOURCE_DIR}/public/appframework/IAppSystemGroup.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} REUSE_FROM tier0 )
