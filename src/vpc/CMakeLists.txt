cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "vpc" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "IAppSystem.cpp"
    "IAppSystem.h"
    "interfaces.cpp"
    "interfaces.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/ikeyvaluessystem.h"
)

end_sources()
target_precompile_headers( ${PROJECT_NAME} PRIVATE ${GLOBAL_PCH} )
