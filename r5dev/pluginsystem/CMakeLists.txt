cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "pluginsystem" "vpc" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Private"
    "modsystem.cpp"
    "modsystem.h"
    "pluginsystem.cpp"
    "pluginsystem.h"
)

add_sources( SOURCE_GROUP "Public"
    "ipluginsystem.h"
)

end_sources()
