cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "inputsystem" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "inputsystem.cpp"
    "inputsystem.h"
)

add_sources( SOURCE_GROUP "Public"
    "${ENGINE_SOURCE_DIR}/public/inputsystem/ButtonCode.h"
)

end_sources()
