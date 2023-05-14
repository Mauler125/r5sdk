cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "inputsystem" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "inputsystem.cpp"
    "inputsystem.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/inputsystem/ButtonCode.h"
)

end_sources()
