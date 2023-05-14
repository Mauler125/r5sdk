cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "pluginsystem" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "ipluginsystem.h"
    "modsystem.cpp"
    "modsystem.h"
    "pluginsystem.cpp"
    "pluginsystem.h"
)

end_sources()
