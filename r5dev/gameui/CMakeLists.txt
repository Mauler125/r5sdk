cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "gameui" "tier0" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "IBrowser.cpp"
    "IBrowser.h"
    "IConsole.cpp"
    "IConsole.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE "${ENGINE_SOURCE_DIR}/tier0/" "${ENGINE_SOURCE_DIR}/tier1/" )
