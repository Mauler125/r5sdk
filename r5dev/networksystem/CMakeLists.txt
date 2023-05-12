cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "networksystem" "vpc" ${FOLDER_CONTEXT} )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "bansystem.cpp"
    "bansystem.h"
    "listmanager.cpp"
    "listmanager.h"
    "pylon.cpp"
    "pylon.h"
    "serverlisting.h"
    "sm_protocol.h"
)

end_sources()
