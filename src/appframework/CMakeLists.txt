cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "appframework" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "IAppSystemGroup.cpp"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/appframework/IAppSystem.h"
    "${ENGINE_SOURCE_DIR}/public/appframework/IAppSystemGroup.h"
)

end_sources()
